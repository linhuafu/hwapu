
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h> 
#include <wchar.h>
#include <linebreak.h>
#include <microwin/nano-X.h>
#include "utf8_utils.h"
#include "text_show.h"
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#include "typedef.h"
#include "convert.h"
#include "text_parse.h"
#include "Convert.h"
#include "Plextalk-config.h"
#include "plextalk-theme.h"
#include "Plextalk-ui.h"
#include "Shared-mem.h"
#include "dbmalloc.h"


char* txShow_getCurSentence(TextShow *thiz){
	if(!thiz || !thiz->show_line){
		return NULL;
	}
	size_t ret_in, ret_out;

	ret_in = thiz->pSenEnd - thiz->pSenStart;
	ret_out = sizeof(thiz->curSenText);
	if(ret_in > ret_out){
		info_debug("!!!ERR:out of size\n");
		return NULL;
	}
	memset(thiz->curSenText, 0, sizeof(thiz->curSenText));
	if(MWTF_UTF8== thiz->codepage){
		strlcpy(thiz->curSenText, thiz->pSenStart, ret_in+1);
	}else if(MWTF_UC16 == thiz->codepage){
		utf16_to_utf8(thiz->pSenStart,&ret_in,thiz->curSenText,&ret_out);
		info_debug("ret_in:%d ret_out:%d\n", ret_in, ret_out);
	}
	return thiz->curSenText;
}

const uint16_t breaks_utf16[] ={
	0x002E, /*. 英文点号*/
	0x0021, /*!英文叹号*/
	0x003F, /*?*/
	0x000A, /*paragrah seperator 段分隔符*/
	0x3002, /*。中文句号*/
	0xFF01, /* ! 中文叹号 */
	0xFF1F, /* ？ */
	0
};

static int u16_isBreakChar(const utf16_t* p)
{
	uint16_t *accept = breaks_utf16;

	while (*accept) {
		if (*accept == *p)
			return 1;
		accept ++;
	}
	return 0;
}

const uint8_t breaks_utf8[] =
{
	0x2E, /*. 英文点号*/
	0x21, /*!英文叹号*/
	0x3F, /*?*/
	0x0A,	/*paragrah seperator 段分隔符*/
	0xe3,0x80,0x82, /*。中文句号*/
	0xef,0xbc,0x81, /* ! 中文叹号 */
	0xef,0xbc,0x9f, /* ？ */
	0
};

static int utf8_isBreakChar(const utf8_t* p)
{	
	int n = (utf8_t*)g_utf8_next_char(p) - p;
	utf8_t *accept = breaks_utf8;

	while (*accept) {
		if (0==u8_strncmp(p, accept, n)) {
			return 1;
		}
		accept = (utf8_t *)g_utf8_next_char(accept);
	}
	return 0;
}

static char *txShow_nextBreak(char *pStart, unsigned long codepage)
{
	if(MWTF_UC16 == codepage){
		uint16_t *pU16Str = (uint16_t *)pStart;
		pU16Str = u16_strpbrk((const uint16_t *)pU16Str, breaks_utf16);
		if(NULL == pU16Str){
			return NULL;
		}
		pU16Str ++;	//skip break symbol
		while(*pU16Str && u16_isBreakChar(pU16Str)){
			pU16Str ++;
		}
		return pU16Str;
	}else if(MWTF_UTF8 == codepage){
		char *ptr = pStart;
		ptr = (char*)u8_strpbrk((const uint8_t *)ptr, breaks_utf8);
		if(NULL == ptr){
			return NULL;
		}
		ptr = g_utf8_next_char(ptr);//skip break symbol
		while(*ptr && utf8_isBreakChar(ptr)){
			ptr = g_utf8_next_char(ptr);
		}
		return ptr;
	}
	return NULL;
}

int txShow_getSenEndByte(TextShow *thiz){
	if(!thiz || thiz->total_line <= 0){
		return 0;
	}
	return (thiz->pSenEnd - thiz->pText);
}

int txShow_getSenStartByte(TextShow *thiz){
	if(!thiz || thiz->total_line <= 0){
		return 0;
	}
	return (thiz->pSenStart - thiz->pText);
}

int txShow_getScrStartByte(TextShow *thiz){
	if(!thiz || thiz->total_line <= 0){
		return 0;
	}
	return (thiz->pScrStart - thiz->pText);
}

void txShow_showScreen(TextShow *thiz)
{
	int i,j,x,y;
	GR_SIZE gwidth, gheight, gbase;
	GR_TEXTFLAGS flags;

	if(!thiz || thiz->total_line <= 0){
		return;
	}

	if(MWTF_UC16 == thiz->codepage){
		flags = MWTF_UC16 | MWTF_TOP;
	}else{
		flags = MWTF_UTF8 | MWTF_TOP;
	}
	
	i = 0;
	y = 0;
	for(i = 0; i < thiz->show_line; i++){
		x = 0;
		for(j=0; j<3; j++){
			if (0 != thiz->showLen[i][j]) {
				if (1 == thiz->showFlag[i][j]) {
					NxSetGCForeground(thiz->show_gc, theme_cache.selected);
					NxSetGCBackground(thiz->show_gc, theme_cache.background);
				} else {
					NxSetGCForeground(thiz->show_gc, theme_cache.foreground);
					NxSetGCBackground(thiz->show_gc, theme_cache.background);
				}
				GrText(thiz->wid, thiz->show_gc, x, y, thiz->pShowLine[i][j], thiz->showLen[i][j], flags);
				GrGetGCTextSize(thiz->show_gc, thiz->pShowLine[i][j], thiz->showLen[i][j],flags,&gwidth, &gheight, &gbase);
				x += gwidth;
			}
		}
		y += thiz->line_height ;
	}	
}

static int filterinvalidChar(char *pstr, int len, unsigned long codepage)
{
	if(MWTF_UC16==codepage){
		uint16_t* pEnd = (uint16_t*)(pstr + len-2);
		while((char*)pEnd >= pstr && (0x0d == *pEnd || 0x0a == *pEnd)){
			pEnd --;
		}
		len = (char*)pEnd - pstr +2;
	}else if(MWTF_UTF8==codepage){
		char *pEnd = pstr + len-1;
		while(pEnd >= pstr && (0x0d == *pEnd || 0x0a == *pEnd)){
			pEnd --;
		}
		len = pEnd - pstr +1;
	}
	return len;
}

void txShow_parseScreen(TextShow *thiz)
{
	int left,right;
	int i,j;
	char *pLineStart, *pLineEnd;
	
	if(!thiz || thiz->total_line <= 0){
		return;
	}

	for(i = 0; i < thiz->show_line; i++){
		pLineStart = thiz->pText + thiz->lineStart[i+thiz->start_line];
		pLineEnd = pLineStart + thiz->lineLen[i+thiz->start_line];
		
		if ((thiz->pSenEnd <= pLineStart) || (thiz->pSenStart >= pLineEnd)) {
			thiz->pShowLine[i][0] = pLineStart;
			thiz->showLen[i][0] = pLineEnd-pLineStart;
			thiz->showFlag[i][0] = 0;
			
			thiz->pShowLine[i][1] = 0;
			thiz->showLen[i][1] = 0;
			thiz->showFlag[i][1] = 0;
			
			thiz->pShowLine[i][2] = 0;
			thiz->showLen[i][2] = 0;
			thiz->showFlag[i][2] = 0;
			continue;
		}

		left = thiz->pSenStart - pLineStart;
		right = thiz->pSenEnd - pLineEnd;

		if (left <= 0) {/*left part*/
			if(right<0){
				thiz->pShowLine[i][0] = pLineStart;
				thiz->showLen[i][0] = thiz->pSenEnd - pLineStart;
				thiz->showFlag[i][0] = 1;
				
				thiz->pShowLine[i][1] = thiz->pSenEnd;
				thiz->showLen[i][1] = pLineEnd - thiz->pSenEnd;
				thiz->showFlag[i][1] = 0;
			}
			else{//right>=0
				thiz->pShowLine[i][0] = pLineStart;
				thiz->showLen[i][0] = pLineEnd - pLineStart;
				thiz->showFlag[i][0] = 1;
				thiz->pShowLine[i][1] = 0;
				thiz->showLen[i][1] = 0;
				thiz->showFlag[i][1] = 0;
			}
			thiz->pShowLine[i][2] = 0;
			thiz->showLen[i][2] = 0;
			thiz->showFlag[i][2] = 0;
		} 
		else{//left>0
			thiz->pShowLine[i][0] = pLineStart;
			thiz->showLen[i][0] = thiz->pSenStart - pLineStart;
			thiz->showFlag[i][0] = 0;
			if(right < 0){
				thiz->pShowLine[i][1] = thiz->pSenStart;
				thiz->showLen[i][1] = thiz->pSenEnd - thiz->pSenStart;
				thiz->showFlag[i][1] = 1;
				
				thiz->pShowLine[i][2] = thiz->pSenEnd;
				thiz->showLen[i][2] = pLineEnd - thiz->pSenEnd;
				thiz->showFlag[i][2] = 0;
			}else{//right >=0
				thiz->pShowLine[i][1] = thiz->pSenStart;
				thiz->showLen[i][1] = pLineEnd - thiz->pSenStart;
				thiz->showFlag[i][1] = 1;
				
				thiz->pShowLine[i][2] = 0;
				thiz->showLen[i][2] = 0;
				thiz->showFlag[i][2] = 0;
			}
		} 
	}

	/*去掉行尾不可显示的字符*/
	for(i = 0; i < thiz->show_line; i++){
		for(j=0; j<3; j++){
			if (0 != thiz->showLen[i][j]) {
				thiz->showLen[i][j] = filterinvalidChar(thiz->pShowLine[i][j],thiz->showLen[i][j],thiz->codepage);
			}
		}
	}	
	return;
}


int txShow_nextScreen(TextShow *thiz)
{
	int i;
	if(!thiz || thiz->total_line <= 0){
		return -1;
	}

	DBGMSG("start_line:%d  total_line:%d\n", thiz->start_line, thiz->total_line);
	if(thiz->start_line + thiz->screen_line < thiz->total_line){
		thiz->start_line += thiz->screen_line;
		if(thiz->start_line + thiz->screen_line < thiz->total_line){
			thiz->show_line = thiz->screen_line;
		}
		else{
			thiz->show_line = thiz->total_line - thiz->start_line;
		}

		/*如果不满一屏显示，还有更多文本*/
		if(thiz->show_line < thiz->screen_line){
			if(thiz->bMoreText){
				info_debug("more text\n");
				return -1;
			}
		}
		
		thiz->pScrStart = thiz->pText + thiz->lineStart[thiz->start_line];
		thiz->pScrEnd = thiz->pScrStart ;
		for(i=0; i< thiz->show_line; i++){
			thiz->pScrEnd += thiz->lineLen[thiz->start_line+i];
		}
		thiz->pSenStart = thiz->pScrStart;
		thiz->pSenEnd = txShow_nextBreak(thiz->pSenStart, thiz->codepage);
		if(NULL == thiz->pSenEnd || thiz->pSenEnd > thiz->pScrEnd){
			thiz->pSenEnd = thiz->pScrEnd;
		}

		txShow_parseScreen(thiz);
	}
	else{
		/* next phase */
		info_debug("no normal screen\n");
		return -1;
	}
	return 0;
}

int txShow_prevScreen(TextShow *thiz)
{
	int i;
	info_debug("text prev screen\n");

	if(!thiz || thiz->total_line <= 0){
		return -1;
	}

	if(thiz->start_line - thiz->screen_line >= 0){
		thiz->start_line -= thiz->screen_line;
		/* fill screen */
		if(thiz->start_line + thiz->screen_line < thiz->total_line){
			thiz->show_line = thiz->screen_line;
		}
		else{
			thiz->show_line = thiz->total_line - thiz->start_line;
		}
		
		thiz->pScrStart = thiz->pText + thiz->lineStart[thiz->start_line];
		thiz->pScrEnd = thiz->pScrStart ;
		for(i=0; i< thiz->show_line; i++){
			thiz->pScrEnd += thiz->lineLen[thiz->start_line+i];
		}
		thiz->pSenStart = thiz->pScrStart;
		thiz->pSenEnd = txShow_nextBreak(thiz->pSenStart, thiz->codepage);
		if(NULL == thiz->pSenEnd || thiz->pSenEnd > thiz->pScrEnd){
			thiz->pSenEnd = thiz->pScrEnd;
		}
		txShow_parseScreen(thiz);
	}
	else{
		/* prev phase */
		info_debug("no prev screen\n");
		return -1;
	}

	return 0;
}


int txShow_nextSentence(TextShow *thiz)
{
	//info_debug("text next sentence\n");
	if(!thiz || thiz->total_line <= 0){
		return -1;
	}

	if(thiz->pSenEnd < thiz->pScrEnd){
		//also in current screen
		info_debug("pSenEnd:%d\n",thiz->pSenEnd);
		thiz->pSenStart = thiz->pSenEnd;
		thiz->pSenEnd = txShow_nextBreak(thiz->pSenStart, thiz->codepage);
		if(NULL == thiz->pSenEnd || thiz->pSenEnd > thiz->pScrEnd){
			thiz->pSenEnd = thiz->pScrEnd;
		}
		txShow_parseScreen(thiz);
		return 0;
	}else{
		return txShow_nextScreen(thiz);
	}
}

int txShow_parseText(TextShow *thiz, char *text, int size, unsigned long codepage, int bMoreText)
{
	int i;
	thiz->pText = text;
	thiz->textSize = size;
	thiz->codepage = codepage;
	thiz->bMoreText = bMoreText;
	
	DBGMSG("thiz->codepage=%ld\n",thiz->codepage);
	
	if(MWTF_UC16==codepage){
		thiz->textSize = u16_strlen(text)*2;
		DBGMSG("thiz->textSize=%d\n",thiz->textSize);
		thiz->total_line= utf16GetTextExWordBreakLines(thiz->show_gc, thiz->w, getenv("LANG"), text, thiz->textSize,
			MAX_SHOW_LINE, thiz->lineStart, thiz->lineLen, NULL);
	}else if(MWTF_UTF8==codepage){
		thiz->textSize = strlen(text);
		DBGMSG("thiz->textSize=%d\n",thiz->textSize);
		thiz->total_line= utf8GetTextExWordBreakLines(thiz->show_gc, thiz->w, getenv("LANG"), text, thiz->textSize,
			MAX_SHOW_LINE, thiz->lineStart, thiz->lineLen, NULL);	
	}else{
		DBGMSG("code err\n");
		thiz->total_line = 0;
	}

	if(thiz->total_line <= 0){
		return 0;
	}

	thiz->start_line = 0;
	if(thiz->screen_line < thiz->total_line){
		thiz->show_line = thiz->screen_line;
	}
	else{
		thiz->show_line = thiz->total_line;
	}

	
	thiz->pScrStart = thiz->pText + thiz->lineStart[thiz->start_line];
	thiz->pScrEnd = thiz->pScrStart ;
	for(i=0; i< thiz->show_line; i++){
		thiz->pScrEnd += thiz->lineLen[thiz->start_line+i];
	}
	thiz->pSenStart = thiz->pScrStart;
	thiz->pSenEnd = txShow_nextBreak(thiz->pSenStart, thiz->codepage);
	if(NULL == thiz->pSenEnd || thiz->pSenEnd > thiz->pScrEnd){
		thiz->pSenEnd = thiz->pScrEnd;
	}
	txShow_parseScreen(thiz);
	return 1;
}

int txShow_Init(TextShow *thiz,int fontsize)
{
	thiz->x = MAINWIN_LEFT;
	thiz->y = MAINWIN_TOP;
	thiz->w = MAINWIN_WIDTH;
	thiz->h = MAINWIN_HEIGHT;
	
	thiz->font_size = fontsize;
	thiz->line_height = fontsize + 1;
	thiz->screen_line = thiz->h/thiz->line_height;
		
	info_debug("font_size=%d\n", thiz->font_size);
	info_debug("screen_line=%d\n", thiz->screen_line);
	return 0;
}


