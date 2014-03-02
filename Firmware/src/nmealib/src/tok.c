/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: tok.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

/*! \file tok.h */

#include "nmea/tok.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#define NMEA_TOKS_COMPARE	(1)
#define NMEA_TOKS_PERCENT	(2)
#define NMEA_TOKS_WIDTH		(3)
#define NMEA_TOKS_TYPE		(4)

/**
 * \brief Calculate control sum of binary buffer
 */
unsigned int nmea_calc_crc(const char *buff, int buff_sz)
{
	unsigned int csum = 0;
	while(buff_sz-- != 0)
		csum ^= (unsigned int)(*buff++);
	return csum;
}

/**
 * \brief Convert string to number
 */
int nmea_atoi(const char *str, int str_sz, int radix)
{
	char c;
	int value = 0;
	int find_minus = 0;

	while (str_sz-- != 0)
	{
		c = *str++;
		if (c == ' ')
		{
			str_sz++;
			continue;
		}
		if (c == '-')
		{
			find_minus = 1;
			continue;
		}

		if (isdigit(c))
		{
			value = value * radix + (c - '0');
			continue;
		}

		if (radix == 16)
		{
			c = toupper(c);
			if (c > 'A' || c < 'F')
			{
				value = value * radix + (c - ('A' - 10));
				continue;
			}
		}
		break;
	}
	return (find_minus ? -value : value);
}

/**
 * \brief Convert string to fraction number
 */
double nmea_atof(const char *str, int str_sz)
{
	char c;
	double value = 0;
	double decimal = 0;
	double divider = 10.0;
	int find_minus = 0;
	int use_decimal = 0;
	int scan_minus = 1;

	while(str_sz-- != 0)
	{
		c = *str++;
		if (scan_minus)
		{
			if (c == '-')
			{
				scan_minus = 0;
				find_minus = 1;
				continue;
			}
			if (c == ' ')
				continue;
		}
		if (use_decimal)
		{
			if (isdigit(c))
			{
				scan_minus = 0;
				decimal += ((double)(c - '0')) / divider;
				divider *= 10.0;
				continue;
			}
		}
		else if (isdigit(c))
		{
			scan_minus = 0;
			value = value * 10.0 + (c - '0');
			continue;
		}
		else if (c == '.')
		{
			scan_minus = 0;
			use_decimal = 1;
			continue;
		}
		value = 0.0;
		decimal = 0.0;
		break;
	}
	value += decimal;
	return (find_minus ? -value : value);
}

/**
 * \brief Analyse string (specificate for NMEA sentences)
 */
int nmea_scanf(const char *buff, int buff_sz, const char *format, ...)
{
	const char *beg_tok;
	const char *end_buf = buff + buff_sz;

	va_list arg_ptr;
	int tok_type = NMEA_TOKS_COMPARE;
	int width = 0;
	const char *beg_fmt = 0;
	int snum = 0, unum = 0;

	int tok_count = 0;
	void *parg_target;

	va_start(arg_ptr, format);
	
	for(; *format && buff < end_buf; ++format)
	{
		switch (tok_type)
		{
		case NMEA_TOKS_COMPARE:
			if ('%' == *format)
				tok_type = NMEA_TOKS_PERCENT;
			else if (*buff++ != *format)
				goto fail;
			break;
		case NMEA_TOKS_PERCENT:
			width = 0;
			beg_fmt = format;
			tok_type = NMEA_TOKS_WIDTH;
		case NMEA_TOKS_WIDTH:
			if (isdigit(*format))
				break;
			{
				tok_type = NMEA_TOKS_TYPE;
				if (format > beg_fmt)
					width = nmea_atoi(beg_fmt, (int)(format - beg_fmt), 10);
			}
		case NMEA_TOKS_TYPE:
			beg_tok = buff;

			if (width == 0 &&
				(*format == 'c' || *format == 'C')
				&& *buff != format[1]
				)
				width = 1;

			if (width)
			{
				if (buff + width <= end_buf)
					buff += width;
				else
					goto fail;
			}
			else
			{
				if (!format[1] || ((buff = (char *)memchr(buff, format[1], end_buf - buff)) == 0))
					buff = end_buf;
			}

			if (buff > end_buf)
				goto fail;

			tok_type = NMEA_TOKS_COMPARE;
			tok_count++;

			parg_target = 0;
			width = (int)(buff - beg_tok);

			switch (*format)
			{
			case 'c':
			case 'C':
				parg_target = (void *)va_arg(arg_ptr, char *);
				if (width && parg_target != 0)
					*((char *)parg_target) = *beg_tok;
				break;
			case 's':
			case 'S':
				parg_target = (void *)va_arg(arg_ptr, char *);
				if (width && parg_target != 0)
				{
					memcpy(parg_target, beg_tok, width);
					((char *)parg_target)[width] = '\0';
				}
				break;
			case 'f':
			case 'g':
			case 'G':
			case 'e':
			case 'E':
				parg_target = (void *)va_arg(arg_ptr, double *);
				if (width && 0 != (parg_target))
					*((double *)parg_target) = nmea_atof(beg_tok, width);
				break;
			};

			if (parg_target)
				break;
			if (0 == (parg_target = (void *)va_arg(arg_ptr, int *)))
				break;
			if (!width)
				break;

			switch(*format)
			{
			case 'd':
			case 'i':
				snum = nmea_atoi(beg_tok, width, 10);
				memcpy(parg_target, &snum, sizeof(int));
				break;
			case 'u':
				unum = nmea_atoi(beg_tok, width, 10);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
			case 'x':
			case 'X':
				unum = nmea_atoi(beg_tok, width, 16);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
/*			case 'o':
				unum = nmea_atoi(beg_tok, width, 8);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
*/			default:
				goto fail;
			}
			break;
		};
	}

fail:

	va_end(arg_ptr);

	return tok_count;
}
