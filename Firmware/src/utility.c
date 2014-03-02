#include <string.h>

#include "hardware.h"
#include "utility.h"
#include <nmea/tok.h>

char* NextToken(char *src)
{
	if (src != NULL)
	{
		while (*src++ != 0);
		if (*src != 0)
			return src;
	}
	return NULL;
}

int TokenSizeQuote(char* src)
{
	char* pos = src;
	if (src != NULL && *src != 0)
	{
		while(*src != 0 && *src != '"')
			src++;
		return (src - pos);
	}
	return 0;
}

int TokenSizeComma(char* src)
{
	char* pos = src;
	if (src != NULL && *src != 0)
	{
		while (*src != 0 && *src != ',')
			src++;
		return (src - pos);
	}
	return 0;
}

char* TokenNextComma(char *src)
{
	if (src != NULL)
	{
		while (*src != 0)
			if (*src++ == ',')
				break;
		if (*src != 0)
			return src;
	}
	return NULL;
}

char* FindTokenStartWith(char *src, const char *pattern)
{
	int patt_len;
	patt_len = strlen(pattern);
	while (src != NULL && *src != 0)
	{
		if (strncmp(src, pattern, patt_len) == 0)
			return src;
		src = NextToken(src);
	}
	return NULL;
}

char* FindToken(char *src, const char *pattern)
{
	while (src != NULL && *src != 0)
	{
		if (strcmp(src, pattern) == 0)
			return src;
		src = NextToken(src);
	}
	return NULL;
}

char* TokenBefore(char *src, const char *pattern)
{
	char* before = NULL;
	while (src != NULL && *src != 0)
	{
		if (strcmp(src, pattern) == 0)
			break;
		before = src;
		if ((src = NextToken(src)) == NULL)
			before = NULL;
	}
	return before;
}

char* CharReplace(char* src, char from, char to)
{
	char* dst = src;
	if (src == NULL || *src == 0)
		return NULL;

	while(*src != 0)
		if (*src == from)
			*src++ = to;
		else
			src++;
	return dst;
}
#if 0
const signed char map64[] = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
		-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
		-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static signed char alphabet64[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/',
};

/*******************************************************************************
* Function Name	:	Base64Decode
* Description	:	Decode a buffer from "string" and into "outbuf"
*******************************************************************************/
int Base64Decode(u8* dst, int dst_size, char* src)
{
	unsigned long shiftbuf;
	int c, i, j, shift, len = 0;
	if (dst_size == 0)
		dst_size = 0x7FFF;

	while ((*src != 0) && (*src != '='))
	{
		// Map 4 (6bit) input bytes and store in a single long (shiftbuf)
 		shiftbuf = 0;
		shift = 18;
		for (i = 0; (i < 4) && (*src != 0) && (*src != '='); i++, src++)
		{
			c = map64[*src & 0xff];
			if (c == -1)
				return 0;
			shiftbuf = shiftbuf | (c << shift);
			shift -= 6;
		}
		// Interpret as 3 normal 8 bit bytes (fill in reverse order).
 		// Check for potential buffer overflow before filling.
 		--i;
		for (j = 0; j < i; j++)
		{
			len++;
			if (dst_size-- != 0)
				*dst++ = (u8)((shiftbuf >> (8 * (2 - j))) & 0xff);
			else
				return 0;
		}
	}
	return len;
}

/*******************************************************************************
* Function Name	:	Base64Encode
* Description	:	Encode a buffer from "string" into "outbuf"
*******************************************************************************/
u8 Base64Encode(char* outbuf, int outlen, char* string)
{
	unsigned long shiftbuf;
	char *cp, *op;
	int i, j, shift;

	op = outbuf;
	*op = '\0';
	cp = string;
	while (*cp)
	{
		// Take three characters and create a 24 bit number in shiftbuf
 		shiftbuf = 0;
		for (j = 2; j >= 0 && *cp; j--, cp++)
			shiftbuf |= ((*cp & 0xff) << (j * 8));
		// Now convert shiftbuf to 4 base64 letters.  The i,j magic calculates
 		// how many letters need to be output.
 		shift = 18;
		for (i = ++j; i < 4 && op < &outbuf[outlen] ; i++)
		{
			*op++ = alphabet64[(shiftbuf >> shift) & 0x3f];
			shift -= 6;
		}
		// Pad at the end with '='
 		while (j-- > 0)
			*op++ = '=';
		*op = '\0';
	}
	return E_OK;
}
#endif

/*******************************************************************************
 *
 * Function Name:	GetCRC32
 * Description	:	Calculate CRC32
 *
 *******************************************************************************/
uint32_t GetCRC32(uint32_t * address, int u32_count, uint32_t * exclude)
{
	CRC_ResetDR();
	while(u32_count-- != 0)
	{
		if (address == exclude)
			CRC->DR = 0;
		else
			CRC->DR = *address;
		address++;
	}
	return CRC->DR;
}

#ifndef BL_VERSION
/*******************************************************************************
 *
 * Function Name:	itoa
 * Description	:
 *
 *******************************************************************************/
const char ITOA_CONST[] = "fedcba9876543210123456789abcdef";
char * itoa(char * result, int32_t value, uint8_t width)
{
	char *ptr = result, *eos, tmp_char;
	int32_t tmp_value;
	uint8_t len = 0;

	do
	{
		tmp_value = value;
		value /= 10;
		*ptr++ = ITOA_CONST[15 + (tmp_value - value * 10)];
		len++;
	} while (value != 0);

	while (width > 0 && len < width)
	{
		*ptr++ = '0';
		len++;
	}

	if (tmp_value < 0)
	{
		*ptr++ = '-';
		len++;
	}
	eos = ptr;
	*ptr-- = '\0';

	while (result < ptr)
	{
		tmp_char = *ptr;
		*ptr--= *result;
		*result++ = tmp_char;
	}
	return eos;
}

char * itoa16(char * result, int32_t value, uint8_t width)
{
	char *ptr = result, *eos, tmp_char;
	int32_t tmp_value;
	uint8_t len = 0;

	do
	{
		tmp_value = value;
		value /= 16;
		*ptr++ = "fedcba9876543210123456789abcdef" [15 + (tmp_value - value * 16)];
		len++;
	} while (value != 0);

	while (width > 0 && len < width)
	{
		*ptr++ = '0';
		len++;
	}

	if (tmp_value < 0)
	{
		*ptr++ = '-';
		len++;
	}
	eos = ptr;
	*ptr-- = '\0';

	while (result < ptr)
	{
		tmp_char = *ptr;
		*ptr--= *result;
		*result++ = tmp_char;
	}
	return eos;
}

char * strcpyEx(char *dst, const char *src)
{
	if (src != NULL)
		while (*src != '\0')
			*dst++ = *src++;
	*dst = '\0';
	return dst;
}

int NmeaAddChecksum(char *dst, char *src)
{
	*dst++ = '*';
	dst = strcpyEx(itoa16(dst, nmea_calc_crc(src + 1, dst - src- 2), 2), "\r\n");
	return (dst - src);
}
#endif
