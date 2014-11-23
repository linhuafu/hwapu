#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linebreak.h>
#include <unistr.h>
#define MWINCLUDECOLORS
#include <nano-X.h>
#include <stdint.h>
#include "microwin/mwtypes.h"
#include "file-helper.h"
#include "plextalk-config.h"
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "convert.h"
#include "dbmalloc.h"


#define achar_strrpbrk u16_strrpbrk

#define true 1
#define false 0


#define TXT_BUF_LEN  4096

utf16_t para_breaks[] = {
	0xA, /*paragrah seperator 段分隔符*/
	0
};

/*结合linebreak分析判断句子*/
utf16_t sen_breaks[] ={
	0x002E, /*. 英文点号*/
	0x0021, /*!英文叹号*/
	0x003F, /*?*/
	0x000A,	/*paragrah seperator 段分隔符*/
	0x3002, /*。中文句号*/
	0xFF01, /* ! 中文叹号 */
	0xFF1F, /* ？ */
	0
};

typedef enum {
	TXT_ASCII = 0,
	TXT_UTF16_LE,
	TXT_UTF16_BE,
	TXT_UTF8,
	TXT_UNKONWN,
	
} TxtCode;

struct TxtBook{
	GR_TEXTFLAGS code;
	unsigned char *buf;
	FILE *fp;
	long size;
	long buf_size;
};

struct TxtBook g_txtbook;


/* strrbrk function defination */
#include "config.h"

#define FUNC u16_strrpbrk
#define UNIT uint16_t
#define U_STRMBTOUC u16_strmbtouc
#define U_STRCHR u16_strchr
#define U_STRRCHR u16_strrchr

UNIT *
FUNC (const UNIT *str, const UNIT *accept)
{
  /* Optimize two cases.  */
  if (accept[0] == 0)
    return NULL;
  {
    ucs4_t uc;
    int count = U_STRMBTOUC (&uc, accept);
    if (count >= 0 && accept[count] == 0)
      return U_STRRCHR (str, uc);
  }
  /* General case.  */
  {
    const UNIT *ptr = str;
	const UNIT *last = NULL;
    for (;;)
      {
        ucs4_t uc;
        int count = U_STRMBTOUC (&uc, ptr);
        if (count <= 0)
          break;
        if (U_STRCHR (accept, uc)) {
          last =  (UNIT *) ptr;
        }
        ptr += count;
      }
    return (UNIT *)last;
  }
}

static int utf8_char_start(const char *str)
{
	 int i = 0;
	 while ((str[i] & 0xc0) == 0x80){
		i += 1;
	 }
	return i;
}

static int gbk_char_start(const unsigned char *str)
{
	 int i = 0;
	 while (str[i]>0x7f){
		i ++;
	 }
	return  i + 1;
}

static int getCharStart(const char *str)
{
	int offset = 0;
	switch (g_txtbook.code) 
	{					
	case MWTF_UTF8:
		offset  = utf8_char_start(str);	
		break;
	case MWTF_ASCII:
	case MWTF_DBCS_GBK:
	case MWTF_DBCS_BIG5:		
		offset  = gbk_char_start(str);
		break;
	default:
		break;
	}
	DBGMSG("*******skip:%d\n",offset );
	return offset;
}

/*根据不同的编码，将当前的buf转换为achar型，即uc16型*/
static int code_to_achar(char* buf_in, size_t* inleft, char* buf_out, size_t* outleft, GR_TEXTFLAGS code)
{
	int ret = -1;
	switch (code) {
		case MWTF_ASCII:
		case MWTF_DBCS_GBK:
			ret = gbk_to_utf16(buf_in, inleft, buf_out, outleft);
			printf("GBK, ASCII\n");
			break;
		case MWTF_DBCS_BIG5:
			ret = big5_to_utf16(buf_in, inleft, buf_out, outleft);
			printf("Big5!\n");
			break;		
		case MWTF_UC16LE:
			ret = utf16_to_utf16(buf_in, inleft, buf_out, outleft);
			printf("uc16 le!\n");
			break;		
		case MWTF_UC16BE:
			ret = utf16be_to_utf16(buf_in, inleft, buf_out, outleft);
			printf("uc16 be!\n");
			break;		
		case MWTF_UTF8:
			ret = utf8_to_utf16(buf_in, inleft, buf_out, outleft);
			printf("UTF8\n");
			break;
	}
	return ret;
}


static TxtCode txt_type (const char* fpath)
{
//	int ret;
	TxtCode type;
	char t_buf[2];
	char s_buf;

	char utf16_le[2]  = {0xFF, 0xFE};
	char utf16_be[2] = {0xFE, 0xFF};
	char utf8[3] 	 = {0xEF, 0xBB, 0xBF};
	
	int fd = open(fpath, O_RDONLY);
	if (fd < 0) {
		printf("open file failure!\n");
		type = TXT_UNKONWN;
	}

	if (read(fd, t_buf, 2) < 2) 
		type = TXT_ASCII;
	else {
		if ((t_buf[0] == utf16_le[0]) && (t_buf[1] == utf16_le[1]))
			type = TXT_UTF16_LE;
		else if ((t_buf[0] == utf16_be[0]) && (t_buf[1] == utf16_be[1]))
			type = TXT_UTF16_BE;
		else {
			if (read(fd, &s_buf, 1) < 1)
				type = TXT_ASCII;
			else {
				 if ((t_buf[0] == utf8[0]) && (t_buf[1] == utf8[1]) && 
					(s_buf == utf8[2]))
					type = TXT_UTF8;
				else
					type = TXT_ASCII;
			}
		}
	}
	close(fd);

	return type;
 }

static unsigned long skipTxtLable(unsigned long offset)
{
	switch (g_txtbook.code) 
	{					
	case MWTF_UTF8:
		offset += 3;	//skip three bytes			
		break;
	case MWTF_UC16LE:
	case MWTF_UC16BE:			
		offset += 2;	//skip two bytes
		break;
	default:
		break;
	}
	return offset;
}

/*
功能：打开文件
输入：file文件路径
返回：bool
*/
int txt_openfile(char *filename)
{
	if(filename)
	{
		if(PlextalkIsFileExist(filename))
		{
			int type = 0;
			type = txt_type(filename);

			DBGMSG("type = %d\n", type);
			if (type != TXT_UNKONWN) {
				
				g_txtbook.buf = (unsigned char*)app_malloc(TXT_BUF_LEN);
				memset(g_txtbook.buf, 0x00, TXT_BUF_LEN);
				g_txtbook.buf_size = TXT_BUF_LEN - 2;
				g_txtbook.fp = fopen(filename, "rb");
				fseek(g_txtbook.fp, 0, SEEK_END);
				g_txtbook.size = ftell(g_txtbook.fp);
				fseek(g_txtbook.fp, 0, SEEK_SET);

				switch (type)
				{					
				case TXT_UTF8:
					fseek(g_txtbook.fp, 3, SEEK_SET);
					g_txtbook.size -= 3;				//skip three bytes
					g_txtbook.code = MWTF_UTF8;
					break;					
				case TXT_UTF16_BE:
					fseek(g_txtbook.fp, 2, SEEK_SET);
					g_txtbook.size -= 2;				//skip two bytes
					g_txtbook.code = MWTF_UC16BE;
					break;					
				case TXT_UTF16_LE:
					fseek(g_txtbook.fp, 2, SEEK_SET);
					g_txtbook.size -= 2;				//skip two bytes
					g_txtbook.code = MWTF_UC16LE;
					break;					
				case TXT_ASCII:
					CoolShmReadLock(g_config_lock);
					if (!strncmp(g_config->setting.lang.lang, "zh_CN", 5)) { //中文简体
						DBGMSG("ASCII: with zh-CN, use GBK!\n");
						g_txtbook.code = MWTF_DBCS_GBK;
					} else if (!strncmp(g_config->setting.lang.lang, "hi_IN", 5)) {
						DBGMSG("ASCII: with hi-IN, use ASCII!\n");   //印度语，非unicode的时候...
						g_txtbook.code = MWTF_ASCII;
					} else if (!strncmp(g_config->setting.lang.lang, "en_US", 5)) {
						DBGMSG("ASCII: with en-US, use ASCII");
						g_txtbook.code = MWTF_ASCII;
					} else if (!strncmp(g_config->setting.lang.lang, "zh_TW", 5)) {
						DBGMSG("ASCII: with zh-TW, use BIG5!\n");   //中文繁体，big5
						g_txtbook.code = MWTF_DBCS_BIG5;
					}
					else
					{
						g_txtbook.code = MWTF_ASCII;
					}
					CoolShmReadUnlock(g_config_lock);
					break;
				}

				return true;
			}
		}
	}
	return false;
}
 
 
/*
功能：关闭
输入: 无
返回：无
*/
void txt_closefile()
{
	if(g_txtbook.fp)
	{
		fclose(g_txtbook.fp);
	}
	app_free(g_txtbook.buf);
}
 
/*
功能：从文件的偏移offset处获取长度为len的文本

输入：offset 文件偏移

      buf  存放字符

      len 欲获取的长度

输出：*len 实际获取的长度

返回：-
		 1 到文件结尾
		-1 获取失败
         0  正常
*/
int getText(unsigned long offset, char *buf,  int *len)
{
	int ret = 0;
	size_t ret_size;
	size_t inleft, outleft;
	size_t size;
	char *contentbuf = NULL;
	unsigned long ulSeekOffset;
		
	if(-1 == offset)
		return 0;

	contentbuf = (char *)app_malloc(*len + 2);
	memset(contentbuf, 0x00, *len + 2);

	ulSeekOffset = skipTxtLable(offset);
	fseek(g_txtbook.fp, ulSeekOffset, SEEK_SET);
	ret_size = fread((void*)contentbuf, 1, *len, g_txtbook.fp);
	if(ret_size <= 0){
		*len = 0;
		buf[0] = '\0';
		ret = -1;
		goto fail;
	}
	
	inleft = ret_size;
	size = (*len);               //* sizeof(utf16_t);
	outleft = size;
	DBGMSG("inleft=%d\n", inleft);
	DBGMSG("outleft=%d\n", outleft);

	if(g_txtbook.code != MWTF_UC16LE)
	{
		code_to_achar(contentbuf, &inleft, buf, &outleft, g_txtbook.code);
		(*len) = size - outleft; ///sizeof(utf16_t);
	}
	else
	{
		if(*len > ret_size)
			*len = ret_size;
		memcpy(buf, contentbuf, *len);
	}

	DBGMSG("*len = %d\n", *len);
//	size -= outleft;

	if(offset + ret_size >= g_txtbook.size){
		DBGMSG("get last text\n");
		ret = 1;
	}
fail:
	app_free(contentbuf);
	contentbuf = NULL;
	return ret;
}
 

/*
功能：根据字符偏移获取字节偏移

输入：offset偏移，nChar字节个数，为UNICODE

返回：字节偏移
*/
unsigned long getByteOffset(unsigned long offset, int nChar)
{
//	utf16_t *pUf16;
	size_t ret_size;
	size_t len = g_txtbook.buf_size;
	unsigned long ret = 0;
	unsigned long ulSeekOffset = 0;
	
	DBGMSG("offset = %ld\n", offset);
	if(nChar == 0)
		return offset;

	if(g_txtbook.code != MWTF_UC16LE)
	{
		ulSeekOffset = skipTxtLable(offset);
		fseek(g_txtbook.fp, ulSeekOffset, SEEK_SET);
		ret_size = fread((void*)g_txtbook.buf, 1, len, g_txtbook.fp);
		if(ret_size <= 0){
			return -1;
		}
	}

	switch(g_txtbook.code)
	{
	case MWTF_UTF8:
		{
			gchar *p, *q;
			int count = 0;
			p = (gchar*)g_txtbook.buf;

			while(p){
				q = g_utf8_next_char(p);
				p = q;
				count++;
				if(count >= nChar/2)
					break;
			}
			ret = offset + p - (gchar *)g_txtbook.buf;
		}
		break;
	case MWTF_UC16LE:
	case MWTF_UC16BE:
		ret = offset + nChar;
		break;
	case MWTF_ASCII:
	case MWTF_DBCS_GBK:
	case MWTF_DBCS_BIG5:
		{
			unsigned char *p;
			int count = 0;
			p = g_txtbook.buf;
			while(p){
				if(*p > 0x80)
				{
					p++;
				}
				p++;
				count++;
				if(count >= nChar/2)
					break;
			}
			DBGMSG("count = %d", count);
			ret = offset + p - g_txtbook.buf;
		}
		break;
	}
	
	if(ret >= g_txtbook.size){
		DBGMSG("get last text\n");
		ret = g_txtbook.size;
	}

	DBGMSG("ret = %ld", ret);
	return ret;
}


static int sentence_symbol_utf16 (const utf16_t* p)
{
	int n = sizeof(sen_breaks)/sizeof(sen_breaks[0]);
	int i;

	for (i = 0; i < n; i++) {
		if (*p == sen_breaks[i])
			return 1;
	}

	return 0;
}


/*
功能：从offset处查找往前/后查找1个段落

输入：offset 为开始位置，n为个数（0或负数：往前找，正数：往后找）

返回：新句子的偏移， 为0xffffffff表示未找到
*/
unsigned long skipPara(unsigned long offset, int n)
{
	utf16_t *ptr;
	unsigned long len = g_txtbook.buf_size;
	unsigned long ret = 0xffffffff;
	unsigned long ulSeekOffset;
	utf16_t *pUf16 = NULL;
	size_t ret_size;
	size_t inleft, outleft;
	size_t outsize;
//	utf16_t save;
	int pos;

	if(offset==0 && n<=0)
		return ret;
	if(offset >= g_txtbook.size && n > 0)
		return ret;	
	
	DBGMSG("offset=%ld\n", offset);
	
	memset(g_txtbook.buf, 0x00, TXT_BUF_LEN);	

	outsize = 2*g_txtbook.buf_size+2;
	pUf16 = (utf16_t *)app_malloc(outsize);
	memset(pUf16, 0x00, outsize);

	if(n <= 0)
	{
		if(offset <= len){
			len = offset;
			offset = 0;
		}else{
			offset -= len;			
		}
	}

	ulSeekOffset = skipTxtLable(offset);
	fseek(g_txtbook.fp, ulSeekOffset, SEEK_SET);
	ret_size = fread((void*)g_txtbook.buf, 1, len, g_txtbook.fp);
	if(ret_size <= 0){
		goto fail;
	}

	inleft = ret_size;
	outleft = outsize;

	if(g_txtbook.code != MWTF_UC16LE){
		int skip = 0;
		if( n < 0  &&  0 != offset){
			skip = getCharStart(g_txtbook.buf);
			offset += skip;
			inleft -= skip;
		}
		code_to_achar((char*)g_txtbook.buf+skip, &inleft, (char*)pUf16, &outleft, g_txtbook.code);
		if( 0 != inleft){
			DBGMSG("inleft:%d outleft=%d\n", inleft, outleft);
		}
		outleft = outsize - outleft;
		DBGMSG("outleft=%d, u16_strlen(pUf16) = %d\n", outleft, u16_strlen(pUf16));
	}
	else{
		memcpy(pUf16, g_txtbook.buf, ret_size);
	}	
	
	if(n > 0)
	{
		ptr = u16_strpbrk(pUf16, para_breaks);
		if (ptr) {
			if(ptr - pUf16 < u16_strlen(pUf16))
			{
				ptr++;
				if(*(ptr)==0x000D)
					ptr++;
				//连续的断句符算是一个
				while(*ptr && (para_breaks[0] == *ptr))
				{
					ptr++;
					if(*(ptr)==0x000D)
						ptr++;
					//DBGMSG("continue pre\n");
				}
			}
			pos = ptr - pUf16+1;
			//DBGMSG("pos=%d\n", pos);
			ret = getByteOffset(offset, pos*2);
		}
	}
	else{

		ptr = pUf16 + u16_strlen(pUf16);

		while (ptr > pUf16) {
			if (para_breaks[0] == *ptr)
			{
				break;
			}
			ptr --;				
		}

		do {
			ptr --;
			if(*(ptr)==0x000D)
				ptr--;

		} while ((ptr>pUf16) && (para_breaks[0] == *ptr));
		
		while (ptr>pUf16) {
	
			ptr --;
			if (para_breaks[0] == *ptr)
			{
				break;
			}			
		}
		
		do {
			ptr --;
			if(*(ptr)==0x000D)
				ptr--;
		} while ((ptr>pUf16) && (para_breaks[0] == *ptr));

		if(ptr <= pUf16)
		{
			pos = 0;
		}
		else{
			pos = ptr - pUf16+2;
		}

		//DBGMSG("pos=%d\n", pos);
		
		ret = getByteOffset(offset, pos*2);
	}

	DBGMSG("ret=%ld\n", ret);
	if(ret >= g_txtbook.size){
		ret = -1;
	}
fail:
	app_free(pUf16);
	pUf16 = NULL;

	return ret;
}

/*
功能：从offset处查找往前/后查找n个句子

输入：offset 为开始位置，n为个数（负数：往前找，正数：往后找）

返回：新句子的偏移， 为0xffffffff表示未找到
*/
unsigned long skipSentences(unsigned long offset, int n)
{
	unsigned long len = g_txtbook.buf_size;
	unsigned long ret = 0xffffffff;
	unsigned long ulSeekOffset;
	DBGMSG("offset=%ld\n", offset);
	
	if(0 == n)
		return offset;
	if(0 == offset && n < 0)
		return ret;
	if(offset >= g_txtbook.size && n > 0)
		return ret;
	
	utf16_t *pUf16 = NULL;
	size_t ret_size;
	size_t inleft, outleft;
	size_t outsize;
	int pos;

	
	memset(g_txtbook.buf, 0x00, TXT_BUF_LEN);

	outsize = 2*g_txtbook.buf_size+2;
	pUf16 = (utf16_t *)app_malloc(outsize);
	memset(pUf16, 0x00, outsize);
	if(n < 0){
		if(offset <= len){
			len = offset;
			offset = 0;
		}else{
			offset -= len;			
		}
	}

	ulSeekOffset = skipTxtLable(offset);
	fseek(g_txtbook.fp, ulSeekOffset, SEEK_SET);
	
	ret_size = fread((void*)g_txtbook.buf, 1, len, g_txtbook.fp);
	if(ret_size <= 0){
		goto fail;
	}

	inleft = ret_size;
	outleft = outsize;
	if(g_txtbook.code != MWTF_UC16LE)
	{
		int skip = 0;
		if( n < 0  && 0 != offset){
			skip = getCharStart(g_txtbook.buf);
			offset += skip;
			inleft -= skip;
		}
		code_to_achar((char*)g_txtbook.buf + skip, &inleft, (char*)pUf16, &outleft, g_txtbook.code);
		if( 0 != inleft){
			DBGMSG("inleft:%d outleft=%d\n", inleft, outleft);
		}
		outleft = outsize - outleft;
		DBGMSG("outleft=%d, u16_strlen(pUf16) = %d\n", outleft, u16_strlen(pUf16));
	}
	else
	{
		memcpy(pUf16, g_txtbook.buf, ret_size);
	}

	int i = 0;
	utf16_t *ptr,*sptr;

	if(n < 0)
	{
		ptr = pUf16 + u16_strlen(pUf16);

		int i;

		while (ptr > pUf16) {
			if (sentence_symbol_utf16(ptr))
			{
				break;
			}
			ptr --;
		}
		
		do {
			ptr --;
			if((*ptr==0x000D && *(ptr - 1)==0x000A)
				||(*ptr==0x000D && *(ptr + 1)==0x000A))
				ptr--;
				
		} while ((ptr > pUf16) && sentence_symbol_utf16(ptr));
		//..
		for (i =0;i < abs(n); i++) {

			while (ptr > pUf16) {
				if (sentence_symbol_utf16(ptr)) {					
					break;
				}
				ptr --;
			}			
			do {
				ptr --;
				if( (*ptr==0x000D && *(ptr - 1)==0x000A)
					||(*ptr==0x000D && *(ptr + 1)==0x000A))
					ptr--;					
			} while ((ptr > pUf16) && sentence_symbol_utf16(ptr));
			
		}
		if(ptr <= pUf16)
		{
			pos = 0;
		}
		else{
			pos = ptr - pUf16 + 2;
		}
		
		ret = getByteOffset(offset, pos*2);
		
	}
	else{
		ptr = u16_strpbrk(pUf16, sen_breaks);
		
		while (ptr) {

			if(ptr - pUf16 < u16_strlen(pUf16))
			{
				ptr++;//指向下一句子的开头
				
				if((*(ptr)==0x000D && *(ptr - 1)==0x000A)
					||(*(ptr)==0x000D && *(ptr + 1)==0x000A))
						ptr++;
				//连续的断句符算是一个
				while(*(ptr) && sentence_symbol_utf16((const utf16_t*)ptr))
				{
					ptr++;
					if((*(ptr)==0x000D && *(ptr - 1)==0x000A)
						||(*(ptr)==0x000D && *(ptr + 1)==0x000A))
						ptr++;
					//DBGMSG("continue pre\n");
				}
			}
			pos = ptr - pUf16;
			//DBGMSG("pos=%d\n", pos);
			i++;
			if(i >= n)
			{
				ret = getByteOffset(offset, (pos)*2);  //跳过句号
				break;
			}
#if 0			
			if (brks[pos] == LINEBREAK_ALLOWBREAK
					|| brks[pos] == LINEBREAK_MUSTBREAK) {
				DBGMSG("pos=%d\n", pos);
				i++;
				if(i >= n)
				{
					ret = getByteOffset(offset, pos);
					break;
				}
			}
#endif
			
			sptr = ptr + 1;
			ptr = u16_strpbrk(sptr, sen_breaks);
		}
		
	}

	DBGMSG("ret=%ld\n", ret);
	if(ret >= g_txtbook.size){
		ret = -1;
	}

fail:
	app_free(pUf16);
	pUf16 = NULL;
	return ret;
} 

unsigned long txtGetTotalSize(void)
{
	if(g_txtbook.fp){
		return g_txtbook.size; 
	}else{
		return 0;
	}
}

