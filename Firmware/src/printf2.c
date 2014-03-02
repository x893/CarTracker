#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

int max_output_len = -1 ;
int curr_output_len = 0 ;

//****************************************************************************
void printchar (char **str, int c)
{
	if (max_output_len >= 0  &&  curr_output_len >= max_output_len)
		return ;
	if (str)
	{
		**str = (char) c;
		++(*str);
		curr_output_len++ ;
	}
}

//****************************************************************************
//  This version returns the length of the output string.
//  It is more useful when implementing a walking-string function.
//****************************************************************************
const double round_nums[8] = {
   0.5,
   0.05,
   0.005,
   0.0005,
   0.00005,
   0.000005,
   0.0000005,
   0.00000005
} ;

unsigned dbl2stri(char *buffer, double dbl, uint8_t dec_digits)
{
	uint8_t idx;
	long dbl_int, dbl_frac;
	long mult = 1;
	char *output = buffer;
	char tbfr[40] ;
	// double fp;

	if (dbl < 0.0)
	{
		*output++ = '-';
		dbl *= -1.0;
	}

	//  handling rounding by adding .5LSB to the floating-point data
	if (dec_digits < 8)
		dbl += round_nums[dec_digits] ;

	//**************************************************************************
	//  construct fractional multiplier for specified number of digits.
	//**************************************************************************
	for (idx = 0; idx < dec_digits; idx++)
		mult *= 10;

	//
	// dbl_frac = lround(modf(dbl, &fp) * (double)mult);
	// dbl_int = lround(fp);
	//
	dbl_int = (long) dbl;
	dbl_frac = (long) ((dbl - (double)dbl_int) * (double)mult);
	//

	//*******************************************
	//  convert integer portion
	//*******************************************
	idx = 0 ;
	while (dbl_int != 0)
	{
		tbfr[idx++] = '0' + (dbl_int % 10) ;
		dbl_int /= 10 ;
	}

	if (idx == 0)
	{
		*output++ = '0' ;
	}
	else
	{
		while (idx > 0)
		{
			*output++ = tbfr[idx-1];
			idx-- ;
		}
	}

	if (dec_digits > 0)
	{
		*output++ = '.' ;

		//*******************************************
		//  convert fractional portion
		//*******************************************
		idx = 0 ;
		while (dbl_frac != 0)
		{
			tbfr[idx++] = '0' + (dbl_frac % 10) ;
			dbl_frac /= 10 ;
		}
		//  pad the decimal portion with 0s as necessary;
		//  We wouldn't want to report 3.093 as 3.93, would we??
		while (idx < dec_digits)
			tbfr[idx++] = '0';

		if (idx == 0)
			*output++ = '0' ;
		else
			while (idx > 0)
			{
				*output++ = tbfr[idx-1] ;
				idx-- ;
			}
	}
	*output = 0 ;

	return strlen(buffer);
}

//****************************************************************************
#define  PAD_RIGHT   1
#define  PAD_ZERO    2

int prints (char **out, const char *string, int width, int pad)
{
	register int pc = 0, padchar = ' ';
	if (width > 0)
	{
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr)
			++len;
		if (len >= width)
			width = 0;
		else
			width -= len;
		if (pad & PAD_ZERO)
			padchar = '0';
	}
	if (!(pad & PAD_RIGHT))
	{
		for (; width > 0; --width)
		{
			printchar (out, padchar);
			++pc;
		}
	}
	for (; *string; ++string)
	{
		printchar (out, *string);
		++pc;
	}
	for (; width > 0; --width)
	{
		printchar (out, padchar);
		++pc;
	}
	return pc;
}

//****************************************************************************
/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12
int printi (char **out, int i, unsigned int base, int sign, int width, int pad, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	char *s;
	int t, neg = 0, pc = 0;
	unsigned int u = (unsigned int) i;
	if (i == 0)
	{
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (out, print_buf, width, pad);
	}
	if (sign && base == 10 && i < 0)
	{
		neg = 1;
		u = (unsigned int) -i;
	}
	//  make sure print_buf is NULL-term
	s = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	while (u)
	{
		t = u % base;  //lint !e573 !e737 !e713 Warning 573: Signed-unsigned mix with divide
		if (t >= 10)
			t += letbase - '0' - 10;
		*--s = (char) t + '0';
		u /= base;  //lint !e573 !e737  Signed-unsigned mix with divide
	}
	if (neg)
	{
		if (width && (pad & PAD_ZERO))
		{
			printchar (out, '-');
			++pc;
			--width;
		}
		else
		{
			*--s = '-';
		}
	}
	return pc + prints (out, s, width, pad);
}

//****************************************************************************
int print (char **out, int *varg)
{
	int post_decimal ;
	int width, pad ;
	unsigned int dec_width = 6 ;
	int pc = 0;
	char *format = (char *) (*varg++);
	char scr[2];
	char bfr[81] ;

	for (; *format != 0; ++format)
	{
		if (*format == '%')
		{
			dec_width = 6 ;
			++format;
			width = pad = 0;
			if (*format == '\0')
				break;
			if (*format == '%')
				goto out_lbl;
			if (*format == '-')
			{
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0')
			{
				++format;
				pad |= PAD_ZERO;
			}
			post_decimal = 0 ;
			if (*format == '.' || (*format >= '0' &&  *format <= '9'))
			{
				while (1)
				{
					if (*format == '.')
					{
						post_decimal = 1 ;
						dec_width = 0 ;
						format++ ;
					}
					else if ((*format >= '0' &&  *format <= '9'))
					{
						if (post_decimal)
						{
							dec_width *= 10;
							dec_width += (unsigned int)(*format - '0');
						}
						else
						{
							width *= 10;
							width += *format - '0';
						}
						format++ ;
					}
					else
					{
						break;
					}
				}
			}
			if (*format == 'l')
				++format;
			switch (*format)
			{
				case 's':
					{
						char *s = (char *) *varg++ ;  //lint !e740 !e826  
						pc += prints (out, s ? s : "(null)", width, pad);
					}
					break;
				case 'd':
					pc += printi (out, *varg++, 10, 1, width, pad, 'a');
					break;
				case 'x':
					pc += printi (out, *varg++, 16, 0, width, pad, 'a');
					break;
				case 'X':
					pc += printi (out, *varg++, 16, 0, width, pad, 'A');
					break;
				case 'p':
					case 'u':
					pc += printi (out, *varg++, 10, 0, width, pad, 'a');
					break;
				case 'c':
					/* char are converted to int then pushed on the stack */
					scr[0] = (char) *varg++;
					scr[1] = '\0';
					pc += prints (out, scr, width, pad);
					break;
				case 'f':
					{
						// http://wiki.debian.org/ArmEabiPort#Structpackingandalignment
						// Stack alignment
						// 
						// The ARM EABI requires 8-byte stack alignment at public function entry points, 
						// compared to the previous 4-byte alignment.
					#ifdef USE_NEWLIB               
						char *cptr = (char *) varg ;  //lint !e740 !e826  convert to double pointer
						uint caddr = (uint) cptr ;
						if ((caddr & 0xF) != 0) {
						cptr += 4 ;
						}
						double *dblptr = (double *) cptr ;  //lint !e740 !e826  convert to double pointer
					#else
						double *dblptr = (double *) varg ;  //lint !e740 !e826  convert to double pointer
					#endif
						double dbl = *dblptr++ ;   //  increment double pointer
						varg = (int *) dblptr ;    //lint !e740  copy updated pointer back to base pointer
						// unsigned slen =
						dbl2stri(bfr, dbl, dec_width) ;
						// stuff_talkf("[%s], width=%u, dec_width=%u\n", bfr, width, dec_width) ;
						pc += prints (out, bfr, width, pad);
					}
					break;
				default:
					printchar (out, '%');
					printchar (out, *format);
					break;
			}
		}
		else 
		{
out_lbl:
			printchar (out, *format);
			++pc;
		}
	}
	if (out)
		**out = '\0';
	return pc;
}
