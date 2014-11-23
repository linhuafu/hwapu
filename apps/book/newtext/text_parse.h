#ifndef __TEXT_PARSE_H_
#define __TEXT_PARSE_H_

#include "text_mark.h"

typedef enum{
	FILE_TYPE_UNKOWN=0,
	FILE_TYPE_TXT,
	FILE_TYPE_DOC,
	FILE_TYPE_DOCX,
}FILE_TYPE;

typedef enum{
	JU_HEADING_FUNC =(1<<0),
	JU_GROUP_FUNC =(1<<1),
	JU_PAGE_FUNC =(1<<2),
	JU_PHRASE_FUNC =(1<<3),
	JU_SCREEN_FUNC =(1<<4),
	JU_PARA_FUNC =(1<<5),
	JU_SEN_FUNC =(1<<6),
	JU_BOOKMARK_FUNC =(1<<7),
	JU_30SEC_FUNC =(1<<8),
	JU_10MIN_FUNC =(1<<9),
}TEXT_FUNC;

#define JU_MAX_FUNC 10

#define STR_TEXT_LEN 1024

typedef struct text_parse{
	char		fname[PATH_MAX];
	FILE_TYPE	filetype;
	void		*fp;
	unsigned long	codepage;
	int 		funcs;			//text_FUNC

	unsigned long long ulOffset;	//offset
	
	char 		strText[STR_TEXT_LEN];
	int			strSize;

	int			isTextEnd;
	TEXTMARK 	*mark;
}TextParse;

int text_mark2phrase(TextParse *thiz, MARK *mark);

//int text_phrase2mark(TextParse *thiz,MARK *mark);

int text_nextMark(TextParse *thiz);


int text_prevMark(TextParse *thiz);


int text_resumeMark(TextParse *thiz);


int text_jumpHead(TextParse *thiz);

int text_jumpTail(TextParse *thiz);

unsigned long long text_nextOffset(TextParse *thiz, int byteOff);

int text_prevScreen( TextParse *thiz, int byteoff);

int text_nextScreen( TextParse *thiz, int byteoff);


int text_prevSentence( TextParse *thiz, int byteoff);

int text_nextSentence( TextParse *thiz, int byteoff);


int text_prev5Sentence( TextParse *thiz, int byteoff);

int text_next5Sentence( TextParse *thiz, int byteoff);


int text_prevParagraph( TextParse *thiz, int byteoff);

int text_nextParagraph( TextParse *thiz, int byteoff);


int text_prevHeading( TextParse *thiz, int byteoff);

int text_nextHeading( TextParse *thiz, int byteoff);

int text_nextText(TextParse *thiz, int byteOff);

int text_getText( TextParse *thiz);

TextParse *text_open(const char *origfile, const char *genfile);
void text_close(TextParse *thiz);

#endif
