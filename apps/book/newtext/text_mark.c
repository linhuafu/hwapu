#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#include "text_mark.h"
#include "book.h"
#include "typedef.h"
#include "dbmalloc.h"


/*dasy*/
struct TextHead text_head = {
	.magic = 0x35467892,
	.count = 0,
	.flag = 0,
	.mark_count = 0,
	.last = 0,
	.font_type = 0,
	.lang = 0,
	.resume_mrk = 0
};

static FILE *mark_openFile(TEXTMARK *thiz, const char *fname)
{
	FILE *file;
	info_debug("text_open_file\n");
	file = fopen(fname, "r+");
	if(!file){/*not exist*/
		info_debug("not exist\n");
		file = fopen(fname, "w");
		if(!file)
		{
			info_debug("create faile!\n");
			return NULL;	
		}
		fclose(file);
		
		thiz->flag = 0;
		file = fopen(fname, "r+");
		if(!file){
			info_debug("reopen faile!\n");
			return NULL;
		}
	}
	else{
		thiz->flag = 1;
	}
	return file;
}

TEXTMARK *mark_create(const char *fname)
{
	TEXTMARK *thiz = (TEXTMARK*)app_malloc(sizeof(TEXTMARK));
	if(NULL == thiz){
		goto faile;
	}
	memset(thiz, 0, sizeof(TEXTMARK));

//	char *p = strrchr(fname, '.');
//	if(NULL == p){
//		goto faile;
//	}
//	memset(thiz->mrk_file, 0, sizeof(thiz->mrk_file));
//	strncpy(thiz->mrk_file, fname, (p-fname));
//	strlcat(thiz->mrk_file, "_", sizeof(thiz->mrk_file));
//	strlcat(thiz->mrk_file, p+1, sizeof(thiz->mrk_file));
//	strlcat(thiz->mrk_file, ".mrk", sizeof(thiz->mrk_file));

	memset(thiz->mrk_file, 0, sizeof(thiz->mrk_file));
	strlcpy(thiz->mrk_file, fname, sizeof(thiz->mrk_file));
	strlcat(thiz->mrk_file, ".bmk", sizeof(thiz->mrk_file));
	info_debug("mrk_file:%s\n",thiz->mrk_file);
	
	thiz->fp = mark_openFile(thiz, thiz->mrk_file);
	if(NULL == thiz->fp){
		goto faile;
	}
	
	thiz->head_size = sizeof(struct TextHead)+ sizeof(MARK);

	do{
		if(!thiz->flag){
			break;
		}
		int fileSize= PlextalkGetFileSize(thiz->mrk_file);
		info_debug("filesize:%d\n", fileSize);
		if(fileSize < thiz->head_size){
			info_debug("size err:%d\n", thiz->head_size);
			thiz->flag = 0;
			break;
		}
		fread((void*)(&thiz->header), sizeof(struct TextHead), 1,thiz->fp);
		
		if(thiz->header.magic != text_head.magic){
			info_debug("magic err\n");
			thiz->flag = 0;
			break;
		}
		
		if(thiz->header.count > MAX_MARK_COUNT){
			info_debug("num err\n");
			thiz->flag = 0;
			break;
		}
	}while(0);

	if(!thiz->flag){
		/*write header*/
		MARK mark;
		info_debug("write\n");
		memcpy(&thiz->header , &text_head, sizeof(thiz->header));
		memset(&mark, 0, sizeof(MARK));
		fwrite((const void*)(&text_head), 
			sizeof(struct TextHead), 
			1,thiz->fp);
		fwrite((const void*)(&mark), //resume mark
			sizeof(MARK), 
			1,thiz->fp);
	}

	info_debug("header, count=%d\n",thiz->header.count);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return thiz;
faile:
	mark_destroy(thiz);
	return NULL;
}


int mark_saveResumeMark(TEXTMARK *thiz, MARK *mark)
{
	long offset;
	info_debug("save resume mark\n");
	return_val_if_fail(NULL!=thiz,-1);
	
	thiz->header.resume_mrk = 1;
	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header,
		sizeof(thiz->header), 
		1,thiz->fp);
	offset = sizeof(thiz->header);
	fseek(thiz->fp, offset, SEEK_SET);
	fwrite((const void*)mark, sizeof(MARK), 
		1,thiz->fp);
	fflush(thiz->fp);
	return 0;
}

int mark_getResumeMark(TEXTMARK *thiz, MARK *mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz,-1);
	
	if(!thiz->header.resume_mrk){
		info_err("no resume mark\n");
		return -1;
	}

	offset = sizeof(thiz->header);
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, sizeof(MARK), 1,thiz->fp);
	return 0;
}


int mark_checkMark(TEXTMARK *thiz, MARK *mark)
{
	uint32_t cur;
	long offset;
	MARK curMark;

	return_val_if_fail(NULL!=thiz,-1);
	
	if(thiz->header.count >= MAX_MARK_COUNT){
		info_debug("mark full\n");
		return MAX_MARK_COUNT;
	}

	for(cur=0; cur< thiz->header.count; cur++){
		offset = thiz->head_size + cur*sizeof(MARK);
		fseek(thiz->fp, offset, SEEK_SET);
		fread((void*)&curMark, sizeof(MARK), 1,thiz->fp);
		if(0 ==memcmp((void*)&curMark, mark, sizeof(MARK))){
			info_debug("mark already exist\n");
			return cur;
		}
	}
	return -1;
}

int mark_addMark(TEXTMARK *thiz, MARK *mark)
{
	return_val_if_fail(NULL!=thiz, -1 );
	if(thiz->header.count >= MAX_MARK_COUNT){
		info_debug("mark full\n");
		return -1;
	}

	thiz->header.count++;
	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header, sizeof(thiz->header), 1,thiz->fp);
	thiz->w_offset = thiz->head_size + 
		(thiz->header.count-1)*sizeof(MARK);
	info_debug("w_offset=0x%lx\n", thiz->w_offset);
	fseek(thiz->fp, thiz->w_offset, SEEK_SET);
	fwrite((const void*)mark, sizeof(MARK), 1,thiz->fp);
	
	thiz->w_offset += sizeof(MARK);
	info_debug("mark count=%d\n", thiz->header.count);
	fflush(thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return thiz->header.count;
}

int mark_delCurMark(TEXTMARK *thiz)
{
	long offset;
	int num;
	char buf[MAX_MARK_COUNT*sizeof(MARK)];

	return_val_if_fail(NULL!=thiz,-1);
	/*last, rear*/
	if(thiz->cur +1 >= thiz->header.count){
		thiz->header.count--;
		thiz->cur--;
		fseek(thiz->fp, 0, SEEK_SET);
		fwrite((const void*)&thiz->header,sizeof(thiz->header), 1,thiz->fp);
		fflush(thiz->fp);
		book_set_bookmark(thiz->cur ,thiz->header.count);
		return 0;
	}

	/*move marks*/
	offset = thiz->head_size+ (thiz->cur +1)*sizeof(MARK);

	num = (thiz->header.count - thiz->cur-1);
	
	fseek(thiz->fp, offset, SEEK_SET);
	
	fread((void*)buf, sizeof(MARK), num,thiz->fp);

	fseek(thiz->fp, offset-sizeof(MARK), SEEK_SET);
	
	fwrite((const void*)buf, sizeof(MARK), num,thiz->fp);

	thiz->header.count--;
	
	fseek(thiz->fp, 0, SEEK_SET);
	
	fwrite((const void*)&thiz->header, sizeof(thiz->header), 1,thiz->fp);
	
	fflush(thiz->fp);
	
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

int mark_delAllMark(TEXTMARK *thiz)
{
	return_val_if_fail(NULL!=thiz,-1);
	
	thiz->header.count = 0;
	thiz->cur = 0;

	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header,sizeof(thiz->header), 1,thiz->fp);
	
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

char *mark_getPath(TEXTMARK *thiz)
{
	return_val_if_fail(NULL!=thiz, NULL);
	
	return thiz->mrk_file;
}

int mark_getCurMark(TEXTMARK *thiz, 
	MARK*mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz, -1);
	
	if((thiz->cur < 0)||
		(thiz->cur +1 >= thiz->header.count)){
		return -1;
	}

	offset = thiz->head_size +thiz->cur*sizeof(MARK);
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, sizeof(MARK), 1,thiz->fp);
	return 0;
}

int mark_getNextMark(TEXTMARK *thiz, 
	MARK*mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz, -1);
	
	if(thiz->header.count < 1 || thiz->header.count > MAX_MARK_COUNT){
		return -1;
	}
	thiz->cur++;
	if(thiz->cur >= thiz->header.count){
		thiz->cur = 0;
	}
	
	info_debug("next mark cur=%d\n", thiz->cur);
	
	offset = thiz->head_size +thiz->cur*sizeof(MARK);
	fseek(thiz->fp, offset, SEEK_SET);
	fread(( void*)mark, sizeof(MARK), 1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}


int mark_getPrevMark(TEXTMARK *thiz, 
	MARK*mark)
{
	long offset;

	return_val_if_fail(NULL!=thiz, -1);
	if(thiz->header.count < 1 || thiz->header.count > MAX_MARK_COUNT){
		return -1;
	}

	thiz->cur --;
	if(thiz->cur < 0){
		thiz->cur = thiz->header.count -1;
	}
	info_debug("prev mark cur=%d\n", thiz->cur);
	offset = thiz->head_size + thiz->cur*sizeof(MARK);
	
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, sizeof(MARK), 1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

int mark_getMarkNum(TEXTMARK *thiz)
{
	return_val_if_fail(NULL!=thiz, 0);
	return thiz->header.count;
}


void mark_destroy(TEXTMARK *thiz)
{
	info_debug("mark_destroy\n");
	if(thiz){
		if(thiz->fp){
			fclose(thiz->fp);
			thiz->fp = NULL;
		}
		app_free(thiz);
		thiz = NULL;
	}
}

