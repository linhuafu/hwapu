#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h> 
#include <wchar.h>
#include <microwin/nano-X.h>
#include "tchar.h"
#include "convert.h"
#define OSD_DBG_MSG
#include "nc-err.h"
#include "text_parse.h"
#include "text_mark.h"
#include "text_show.h"
#include "File-helper.h"
#include "convert.h"
#include "typedef.h"
#include "txt.h"
#include "docx.h"
#include "doc.h"
#include "dbmalloc.h"

int txMain_phrase2mark(MARK *mark);

int text_mark2phrase(TextParse *thiz, MARK *mark)
{
	return_val_if_fail(thiz != NULL, -1);
	thiz->ulOffset = mark->ulOffset;
	DBGMSG("offset:%lld\n", thiz->ulOffset);
	return text_getText(thiz);
}

#if 0
int text_phrase2mark(TextParse *thiz, MARK *mark)
{
	return_val_if_fail(thiz != NULL, -1);
	
	mark->ulOffset = thiz->ulOffset;
	DBGMSG("ulOffset:%lld\n",mark->ulOffset);
	return 0;
}
#endif

int text_nextMark(TextParse *thiz)
{
	MARK mark;
	int ret;	
	DBGMSG("next mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = mark_getNextMark(thiz->mark, 
				&mark);
	if(ret < 0){
		DBGMSG("mark_getNextMark err\n");
		return -1;
	}
	DBGMSG("backup\n");
	ret = text_mark2phrase(thiz, &mark);
	return ret;
}

int text_prevMark(TextParse *thiz)
{
	MARK mark;
	int ret;

	DBGMSG("prev mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = mark_getPrevMark(thiz->mark, 
				&mark);
	if(ret < 0){
		return -1;
	}
	ret = text_mark2phrase(thiz, &mark);
	return ret;
}

int text_resumeMark(TextParse *thiz)
{
	MARK mark;
	int ret;

	DBGMSG("prev mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = mark_getResumeMark(thiz->mark, 
				&mark);
	if(ret < 0){
		return -1;
	}
	ret = text_mark2phrase(thiz, &mark);
	return ret;
}


int text_jumpHead(TextParse *thiz)
{
	thiz->ulOffset = 0;
	return 0;
}

int text_jumpTail(TextParse *thiz)
{
	unsigned long long ulTotalSize;
	if(FILE_TYPE_TXT == thiz->filetype){
		ulTotalSize = txtGetTotalSize();
	}else if(FILE_TYPE_DOC == thiz->filetype){
		unsigned long ulPieceSize;
		ulTotalSize = doc_get_total_piece(thiz->fp) -1;
		ulTotalSize <<= 32;
		ulPieceSize = doc_get_piece_size(thiz->fp, ulTotalSize);
		ulTotalSize |= ulPieceSize;
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulTotalSize = docx_getTotalSize(thiz->fp);
	}
	DBGMSG("ulTotalSize:%lld\n",ulTotalSize);
	thiz->ulOffset = ulTotalSize;
	return 0;
}

unsigned long long text_nextOffset(TextParse *thiz, int byteOff)
{
	unsigned long long offset;
	DBGMSG("ulOffset:%lld byteOff=%d\n",thiz->ulOffset, byteOff);
	if(FILE_TYPE_TXT == thiz->filetype){
		offset = getByteOffset(thiz->ulOffset, byteOff);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		offset = doc_get_byte_offset(thiz->fp, thiz->ulOffset,byteOff);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		offset = docx_get_byte_offset(thiz->fp, thiz->ulOffset, byteOff);
	}else{
		offset = 0;
	}
	DBGMSG("ulOffset:%lld\n",offset);
	return offset;
}

int text_prevScreen( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);
	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, -20);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,-20);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,-20);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_nextScreen( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);
	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, 20);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,20);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,20);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_prevSentence( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, -1);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,-1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,-1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_nextSentence( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, 1);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_prev5Sentence( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, -5);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,-5);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,-5);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;
}

int text_next5Sentence( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipSentences(ulOffset, 5);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_sentences(thiz->fp, ulOffset,5);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_sentences(thiz->fp, ulOffset,5);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_prevParagraph( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipPara(ulOffset, -1);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_paragraph(thiz->fp, ulOffset,-1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_paragraph(thiz->fp, ulOffset,-1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;

}

int text_nextParagraph( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_TXT == thiz->filetype){
		ulOffset = skipPara(ulOffset, 1);
	}else if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_paragraph(thiz->fp, ulOffset,1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_paragraph(thiz->fp, ulOffset,1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	DBGMSG("offset:-1\n");
	return -1;

}


int text_prevHeading( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_heading(thiz->fp, ulOffset,-1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_heading(thiz->fp, ulOffset, -1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;
}


int text_nextHeading( TextParse *thiz, int byteoff)
{
	unsigned long long ulOffset;
	 
	ulOffset = text_nextOffset(thiz, byteoff);
	DBGMSG("ulOffset:%lld\n",ulOffset);

	if(FILE_TYPE_DOC == thiz->filetype){
		ulOffset = doc_skip_heading(thiz->fp, ulOffset,1);
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		ulOffset = docx_skip_heading(thiz->fp, ulOffset,1);
	}else{
		return -1;
	}
	DBGMSG("offset:%lld\n",ulOffset);
	if(-1 != (int)ulOffset){
		thiz->ulOffset = ulOffset;
		return 0;
	}
	return -1;
}

int text_nextText(TextParse *thiz, int byteOff)
{
	if(thiz->isTextEnd){
		return -1;	//end
	}
	thiz->ulOffset = text_nextOffset(thiz, byteOff);
	return 0;
}

int text_getText(TextParse *thiz)
{
	int ret;
	thiz->strSize = sizeof(thiz->strText)-2;
	
	memset(thiz->strText, 0, sizeof(thiz->strText));
	
	DBGMSG("ulOffset:%lld\n",thiz->ulOffset);
	
	if(FILE_TYPE_TXT == thiz->filetype)
	{
		ret = getText(thiz->ulOffset,thiz->strText, &thiz->strSize);
		
	}
	else if(FILE_TYPE_DOC == thiz->filetype)
	{
		ret = doc_get_text(thiz->fp, thiz->ulOffset,thiz->strText, &thiz->strSize);
		
	}
	else if(FILE_TYPE_DOCX == thiz->filetype)
	{
		ret = docx_get_text(thiz->fp, thiz->ulOffset,thiz->strText, &thiz->strSize);
	}
	else
	{
		ret = -1;
	}
	DBGMSG("ret:%d strSize:%d\n",ret, thiz->strSize);
	
	if(-1 == ret){//-1 获取失败
		thiz->isTextEnd = 1;
	}else if(1 == ret){//1到文件结尾
		thiz->isTextEnd = 1;
	}else if(0 == ret){//0  正常
		if(0==thiz->strSize){
			ret = -1;
		}
		thiz->isTextEnd = 0;
	}
	return ret;
}

static int test_text_func( TextParse *thiz)
{
	thiz->funcs = 0;
	if(FILE_TYPE_TXT == thiz->filetype){
		thiz->funcs = JU_SCREEN_FUNC | JU_PARA_FUNC | JU_SEN_FUNC | JU_BOOKMARK_FUNC;
	}else if(FILE_TYPE_DOC == thiz->filetype){
		thiz->funcs = JU_HEADING_FUNC| JU_SCREEN_FUNC | JU_PARA_FUNC | JU_SEN_FUNC | JU_BOOKMARK_FUNC;
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		thiz->funcs = JU_HEADING_FUNC| JU_SCREEN_FUNC | JU_PARA_FUNC | JU_SEN_FUNC | JU_BOOKMARK_FUNC;
	}
	return 0;
}

FILE_TYPE getFileType(const char *file)
{
	char* ext = PlextalkGetFileExtension(file);
	if (NULL == ext){
		return FILE_TYPE_UNKOWN;
	}
	
	if(strcasecmp(ext, "txt") == 0){
		return FILE_TYPE_TXT;
	}
	else if(strcasecmp(ext, "doc") == 0){
		return FILE_TYPE_DOC;
	}
	else if(strcasecmp(ext, "docx") == 0){
		return FILE_TYPE_DOCX;
	}
	return FILE_TYPE_UNKOWN;
}

TextParse *text_open(const char *origfile, const char *genfile)
{
//	int ret;
	
	DBGMSG("text open:%s\n", origfile);
	TextParse *thiz = (TextParse*)app_malloc(sizeof(TextParse));
	if(!thiz){
		goto faile;
	}
	memset(thiz, 0, sizeof(TextParse));
	strlcpy(thiz->fname,origfile,PATH_MAX);
	
	thiz->filetype = getFileType(genfile);
	if(FILE_TYPE_UNKOWN == thiz->filetype){
		goto faile;
	}

	DBGMSG("thiz->filetype=%d\n",thiz->filetype);
	if(FILE_TYPE_TXT == thiz->filetype){
		thiz->fp = (void *)txt_openfile(genfile);
		thiz->codepage = MWTF_UC16;
	}else if(FILE_TYPE_DOC == thiz->filetype){
		thiz->fp = doc_open(genfile);
		thiz->codepage = MWTF_UC16;
	}else if(FILE_TYPE_DOCX == thiz->filetype){
		thiz->fp = docx_open(genfile);
		thiz->codepage = MWTF_UTF8;
	}
	DBGMSG("thiz->codepage=%ld\n",thiz->codepage);
	DBGMSG("thiz->fp=%x\n",(int)thiz->fp);
	if(NULL == thiz->fp){
		goto faile;
	}

	test_text_func(thiz);

	thiz->mark = mark_create(thiz->fname);	
	if(NULL == thiz->mark){
		DBGMSG("create mark err\n");
	}
	return thiz;
faile:
	text_close(thiz);
	return NULL;
}

void text_close(TextParse *thiz)
{
	if(thiz){
		if(thiz->mark){
			MARK mark;
			if(0 == txMain_phrase2mark(&mark)){
				mark_saveResumeMark(thiz->mark, &mark);
			}
			mark_destroy(thiz->mark);
			thiz->mark = NULL;
		}
		if(NULL != thiz->fp){
			if(FILE_TYPE_TXT == thiz->filetype){
				txt_closefile();
			}else if(FILE_TYPE_DOC == thiz->filetype){
				doc_close(thiz->fp);
			}else if(FILE_TYPE_DOCX == thiz->filetype){
				docx_close(thiz->fp);
			}
		}
		app_free(thiz);
		thiz = NULL;
		DBGMSG("text_close end\n");
	}
}

/*
#endif
*/

