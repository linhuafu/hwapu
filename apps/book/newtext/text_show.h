#ifndef __TEXT_SHOW_H__
#define __TEXT_SHOW_H__

#include <stdint.h>
//#include "efont.h"
//nclude "system.h"
#include <microwin/nano-X.h>
#include "widgets.h"
#include <linebreak.h>
#include "breaklines.h"

#define MAX_SHOW_LEN 158

typedef enum{
	FONT_SIZE_12PT =12,
	FONT_SIZE_16PT =16,
	FONT_SIZE_24PT =24,
}FONT_SIZE;

#define MAX_SHOW_LINE 200
#define MAX_SCREEN_BYTE 1024
#define MAX_SCREEN_LINE 8

typedef struct{
	FORM				*form;
	GR_WINDOW_ID		wid;
	GR_GC_ID        	show_gc;

	int					x;
	int					y;
	int					w;
	int					h;
	
	FONT_SIZE 			font_size;
	int 				line_height;
	int 				screen_line;	//one screen can display line

	char				*pText;			//show text
	int					textSize;		//show line
	unsigned long		codepage;
	int					bMoreText;
	
	int 				lineStart[MAX_SHOW_LINE];	//line offset to ptext
	int 				lineLen[MAX_SHOW_LINE];		//line len
	
	int 				total_line;		//total line
	int 				start_line;		//show start line
	int 				show_line;		//can show line

	char				*pScrStart;		//cur screen start
	char				*pScrEnd;		//cur screen end

	char 				*pSenStart;		//cur sentence start
	char				*pSenEnd;		//cur sentence end

	char				*pShowLine[MAX_SCREEN_LINE][3];	//for screen display
	int					showLen[MAX_SCREEN_LINE][3];
	int					showFlag[MAX_SCREEN_LINE][3];
	
	char 				curSenText[MAX_SCREEN_BYTE];
	
}TextShow;


char* txShow_getCurSentence(TextShow *thiz);

void txShow_showScreen(TextShow *thiz);

int txShow_nextScreen(TextShow *thiz);

int txShow_prevScreen(TextShow *thiz);

int txShow_nextSentence(TextShow *thiz);

int txShow_getSenEndByte(TextShow *thiz);

int txShow_getSenStartByte(TextShow *thiz);

int txShow_getScrStartByte(TextShow *thiz);

int txShow_parseText(TextShow *thiz, char *text, int size, unsigned long codepage, int bMoreText);

int txShow_Init(TextShow *thiz,int fontsize);


#endif
