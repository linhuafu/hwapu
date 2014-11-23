#ifndef __DAISY_SHOW_H__
#define __DAISY_SHOW_H__

#include <stdint.h>
//#include "efont.h"
//nclude "system.h"
#include <microwin/nano-X.h>

#include "widgets.h"
//#include "dlist.h"
#include <linebreak.h>

#define ACHAR_U32 0

#if ACHAR_U32
typedef uint32_t  achar_t;
#define gbk_to_achar  gbk_to_utf32
#define achar_strpbrk u32_strpbrk
#define achar_strrpbrk u32_strrpbrk
#define achar_strlen u32_strlen
#define achar_strchr u32_strchr
#define set_linebreaks_achar set_linebreaks_utf32
#define breaks_char utf32_t
#define ACHAR_FONT_FALG MWTF_UC32
#define CAL_BY_CHAR  0
#define achar_to_utf8 utf32_to_utf8
#define utf16_to_achar utf16_to_utf32
#define utf8_to_achar utf8_to_utf32

#else
typedef uint16_t  achar_t;
#define gbk_to_achar  gbk_to_utf16
#define achar_strpbrk u16_strpbrk
#define achar_strrpbrk u16_strrpbrk
#define achar_strlen u16_strlen
#define achar_strchr u16_strchr
#define set_linebreaks_achar set_linebreaks_utf16
#define breaks_char utf16_t
#define ACHAR_FONT_FALG MWTF_UC16
#define CAL_BY_CHAR  0
#define achar_to_utf8 utf16_to_utf8
#define utf16_to_achar utf16_to_utf16
#define utf8_to_achar utf8_to_utf16

#endif

#define MAX_SHOW_LEN 158

typedef enum{
	FONT_SIZE_12PT =12,
	FONT_SIZE_16PT =16,
	FONT_SIZE_24PT =24,
}FONT_SIZE;

#define MAX_SHOW_LINE 200
#define MAX_SCREEN_BYTE 2048

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

	char				*pText;
	int					iTextSize;
	
	int 				lineStart[MAX_SHOW_LINE];
	int 				lineLen[MAX_SHOW_LINE];
	
	int 				total_line;		//total line
	int 				start_line;		// show start line
	int 				show_line;		

	char 				pScrText[MAX_SCREEN_BYTE];
}DaisyShow;


char* dsShow_getCurScrText(DaisyShow *thiz);

int dsShow_nextScreen(DaisyShow *thiz);

int dsShow_prevScreen(DaisyShow *thiz);

int dsShow_showScreen(DaisyShow *thiz);

int dsShow_parseText(DaisyShow *thiz, char *text, int size);

int dsShow_Init(DaisyShow *thiz,int fontsize);


#endif
