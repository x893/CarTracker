#ifndef __UTILITY_H__
#define __UTILITY_H__

char* NextToken(char *src);
char* FindTokenStartWith(char *src, const char *pattern);
char* FindToken(char *src, const char *pattern);
char* TokenBefore(char *src, const char *pattern);
char* CharReplace(char *src, char from, char to);

int TokenSizeComma(char* src);
int TokenSizeQuote(char* src);
char* TokenNextComma(char *src);

// int Base64Decode(u8* dst, int dst_size, char* src);
uint32_t GetCRC32(uint32_t * address, int u32_count, uint32_t * exclude);

char * itoa(char * result, int32_t value, uint8_t width);
char * itoa16(char * result, int32_t value, uint8_t width);
char * strcpyEx(char *dst, const char *src);
int NmeaAddChecksum(char *dst, char *src);

#endif
