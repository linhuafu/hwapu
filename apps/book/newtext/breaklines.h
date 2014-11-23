#ifndef __BREAKLINES_H__
#define __BREAKLINES_H__


#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#define  DEBUG_PRINT 1
#include "linebreak.h"

#include "nx-widgets.h"
#include "widgets.h"
//#include "events.h"
#include "application.h"
//#include "wm.h"


#define DEBUG_PRINT 1
#define OSD_DBG_MSG
#include "nc-err.h"



int
utf8GetTextExWordBreakLines(GR_GC_ID gc, GR_SIZE width,const char* lang,
		const char *text, size_t len, int max_line,int line_from[],int line_len[],int line_width[]);


int
utf16GetTextExWordBreakLines(GR_GC_ID gc, GR_SIZE width,const char* lang,
		const char *text, size_t len, int max_line,int line_from[],int line_len[],int line_width[]);

int
utf32GetTextExWordBreakLines(GR_GC_ID gc, GR_SIZE width,const char* lang,
		const char *text, size_t len, int max_line,int line_from[],int line_len[],int line_width[]);



#endif

