#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h> 
#include <wchar.h>
#include "tchar.h"
#include "convert.h"

#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#define OSD_DBG_MSG
#include "nc-err.h"

#include "Plextalk-i18n.h"
#include "File-helper.h"
#include "text_parse.h"
#include "text_mark.h"
#include "convert.h"
#include "typedef.h"
#include "dbmalloc.h"
#include "doc.h"
#include "docx.h"
#include "txt.h"

int txInfo_PagesInfo(TextParse *thiz, char *pagetstr, int maxlen)
{
	return 0;
}


int txInfo_BookmarkNumberInfo(TextParse *thiz, char *str, int maxlen)
{
	return_val_if_fail(thiz != NULL, -1);
	int num = mark_getMarkNum(thiz->mark);
	snprintf(str, maxlen, "%s:%d",_("Total number of bookmarks"), num);
	info_debug("%s\n",str);
	return 0;
}

int txInfo_TittleNameInfo(TextParse *thiz, char *str, int maxlen)
{
	return_val_if_fail(thiz != NULL, -1);

	char *file = PlextalkGetFilenameFromPath(thiz->fname);
	snprintf(str, maxlen, "%s:%s",_("Current selected title name"), file);
	return 0;
}


int txInfo_CreationDateInfo(TextParse *thiz, char *str, int maxlen)
{
	struct tm *tp;
	struct stat64 st;
	char timeinfo[20];

	return_val_if_fail(thiz != NULL, -1);
	
	if ( 0 != stat64(thiz->fname, &st) ){
		info_err("get file error\n");
		return -1;
	}
		
	tp = localtime (&st.st_mtime);
	strftime(timeinfo, sizeof(timeinfo), "%F %T", tp);
	snprintf(str, maxlen, "%s: %s",_("Updated date"), timeinfo);
	info_debug("%s\n",str);
	return 0;
}

extern int txMain_phrase2mark(MARK *mark);

int txInfo_PercentElapseInfo(TextParse *thiz, char *str, int maxlen, int isEnd)
{
	unsigned long ulTotalSize;
	unsigned long long ullCurOffset;
	char numstr[20];
	int percent;
	
	if(isEnd){
		DBGMSG("end\n");
		percent = 100;
	}else{
		MARK mark;
		
		ullCurOffset = 0;
		if(txMain_phrase2mark(&mark)>=0){
			ullCurOffset = mark.ulOffset;
		}
		DBGMSG("ullCurOffset:%lld\n",ullCurOffset);
		if(FILE_TYPE_TXT == thiz->filetype){
			ulTotalSize = txtGetTotalSize();
			DBGMSG("ulTotalSize:%ld\n",ulTotalSize);
			percent = (100*ullCurOffset)/ulTotalSize;
		}else if(FILE_TYPE_DOC == thiz->filetype){
			unsigned long ulPieceSize,ulPieceOffset;
			unsigned long ulCurPiece,ulTotalPiece;
			unsigned long ulTmep;
			ulPieceOffset = (unsigned long)ullCurOffset;
			ulCurPiece = (unsigned long)(ullCurOffset >> 32);
			ulTotalPiece = doc_get_total_piece(thiz->fp);
			ulPieceSize = doc_get_piece_size(thiz->fp, ullCurOffset);
			
			DBGMSG("ulPieceOffset:%ld ulPieceSize:%ld\n",ulPieceOffset, ulPieceSize);
			DBGMSG("ulCurPiece:%ld ulTotalPiece:%ld\n",ulCurPiece, ulTotalPiece);
			ulTmep = (ulPieceOffset*100)/ulPieceSize;
			percent = (100*ulCurPiece + ulTmep)/ulTotalPiece;
		}else if(FILE_TYPE_DOCX == thiz->filetype){
			ulTotalSize = docx_getTotalSize(thiz->fp);
			DBGMSG("ulTotalSize:%ld\n",ulTotalSize);
			percent = (100*ullCurOffset)/ulTotalSize;
		}else{
			percent = 0;
		}
	}
	snprintf(numstr, sizeof(numstr), " %d ", percent);
	strlcpy(str, _("Current playback position"), maxlen);
	strlcat(str, numstr, maxlen);
	strlcat(str, _("percent."), maxlen);
}

