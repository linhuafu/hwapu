#include "convert.h"
#include <stdio.h>
#include <string.h> /*strerror*/
#include <iconv.h>
#include <errno.h>
#include "dbmalloc.h"


size_t encoding_convert(const char *tocode, const char *fromcode,char *input,
	size_t *inleft, char *output, size_t *outleft)
{
	iconv_t cd;
	size_t  size;

	if((!tocode) ||(!fromcode)){
		printf("invalid encoding name\n");
		return -1;
	}
		
	cd = iconv_open(tocode, fromcode);


	size = iconv(cd, &input, inleft,  &output, outleft);
	if(size == -1){
		printf("convert error: input left=0x%x, outputleft=0x%x\n", 
			(int)*inleft, (int)*outleft);
		switch(*inleft){
		case EILSEQ:
			printf("convert error:%s\n", strerror(EILSEQ));
			break;
		case E2BIG:
			printf("convert error:%s\n", strerror(E2BIG));
			break;
		case EINVAL:
			printf("convert error:%s\n", strerror(EINVAL));
			break;
		case EBADF:
			printf("convert error:%s\n", strerror(EBADF));
			break;
		}
	}

	iconv_close(cd);
	
	return *inleft;
}

/*
*inbytesleft: EILSEQ, E2BIG, EINVAL, EBADF
* //TRANSLIT is appended to tocode
* //IGNORE is appended to tocode
*/
int gbk_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "GBK", input, insize, output, outsize);
}

int gbk_to_utf32(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-32LE", "GBK", input, insize, output, outsize);
}

int gbk_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-16LE", "GBK", input, insize, output, outsize);
}

int utf32_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "UTF-32LE", input, insize, output, outsize);
}

int utf32_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-16LE", "UTF-32LE", input, insize, output, outsize);
}


int utf32be_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "UTF-32LE", input, insize, output, outsize);
}


int utf16_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "UTF-16LE", input, insize, output, outsize);
}

int utf16_to_utf32(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-32LE", "UTF-16LE", input, insize, output, outsize);
}

int utf16_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{ 
	return encoding_convert("UTF-16LE", "UTF-16LE", input, insize, output, outsize);
}

int utf8_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-16LE", "UTF-8", input, insize, output, outsize);
}

int utf8_to_utf32(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-32LE", "UTF-8", input, insize, output, outsize);
}

int big5_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-16LE", "BIG5", input, insize, output, outsize);
}

int utf16be_to_utf16(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-16LE", "UTF-16BE", input, insize, output, outsize);
}

int big5_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "BIG5", input, insize, output, outsize);
}

int utf16le_to_utf8(char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "UTF-16LE", input, insize, output, outsize);
}

int utf16be_to_utf8 (char *input, size_t *insize, char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "UTF-16BE", input, insize, output, outsize);
}

int wchar_to_utf8(char *input, size_t *insize, 
	char *output, size_t *outsize)
{
	return encoding_convert("UTF-8", "WCHAR_T", 
		input, insize, output, outsize);
}
