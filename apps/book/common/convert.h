#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <iconv.h>

int gbk_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int gbk_to_utf32(char *input, size_t *insize, char *output, size_t *outsize);
int gbk_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);
int utf32_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int utf32_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);

int utf32be_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int utf16_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int utf16_to_utf32(char *input, size_t *insize, char *output, size_t *outsize);
int utf16_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);
int utf8_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);
int utf8_to_utf32(char *input, size_t *insize, char *output, size_t *outsize);

int big5_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);
int utf16be_to_utf16(char *input, size_t *insize, char *output, size_t *outsize);

int big5_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int utf16le_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);
int utf16be_to_utf8 (char *input, size_t *insize, char *output, size_t *outsize);


int wchar_to_utf8(char *input, size_t *insize, char *output, size_t *outsize);


#endif
