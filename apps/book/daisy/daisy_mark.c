#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#include "daisy_mark.h"
#include "book.h"
#include "typedef.h"
#include "dbmalloc.h"


/*dasy*/
struct DaisyHead daisy_head = {
	.magic = 0x79736164,
	.count = 0,
	.flag = 0,
	.mark_count = 0,
	.last = 0,
	.font_type = 0,
	.lang = 0,
	.resume_mrk = 0
};

#if 0
int copy_mark_files(struct LstnBook *thiz )
{
	/*copy to tmp*/
	char tmppath[PATH_MAX];
	char targetpath[PATH_MAX];
	char cmd[1024];
	char *p;
	
	snprintf(tmppath, PATH_MAX,"%s.mrk", thiz->daisy_dir);
	info_debug("tmppath=%s\n", tmppath);
	if(!access(tmppath, F_OK)){
	
		p = strrchr(tmppath, '/');
		snprintf(targetpath, 1024,"/tmp%s", p);
		info_debug("targetpath=%s\n", targetpath);
		snprintf(cmd, 1024, "cp %s %s", tmppath, targetpath);
		system(cmd);
	}
	return 0;
}

int save_mark_files(struct LstnBook *thiz)
{
	char *tmp;
	char tmppath[1024];
	char cmd[1024];

	strcpy(tmppath, thiz->daisy_dir);
	info_debug("targetpath=%s\n", tmppath);
	/*save tmp to path*/
	tmp = daisy_mark_get_path(thiz->daisy_parse->mark);
	info_debug("daisy mark=%s\n",tmp );
	snprintf(cmd, 1024, "cp %s %s", tmp, tmppath);
	system(cmd);		
	
	return 0;
}
#endif
static FILE *daisy_open_file(struct DaisyMark *thiz, const char *fname)
{
	FILE *file;
	info_debug("daisy_open_file\n");
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

struct DaisyMark *dsMark_create(const char *fname)
{
	struct DaisyMark *thiz = (struct DaisyMark*)app_malloc(sizeof(struct DaisyMark));
	if(NULL == thiz){
		goto faile;
	}
	memset(thiz, 0, sizeof(struct DaisyMark));

	char *p = strrchr(fname, '/');
	if(NULL == p){
		goto faile;
	}
	
	memset(thiz->mrk_file, 0, sizeof(thiz->mrk_file));
	strncpy(thiz->mrk_file, fname, (p-fname));
	strcat(thiz->mrk_file, ".bmk");

	info_debug("mrk_file:%s\n",thiz->mrk_file);
	
	thiz->fp = daisy_open_file(thiz, thiz->mrk_file);
	if(NULL == thiz->fp){
		goto faile;
	}
	
	thiz->head_size = sizeof(struct DaisyHead)+ sizeof(struct DMark);
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
		fread((void*)(&thiz->header), sizeof(struct DaisyHead), 1,thiz->fp);
		
		if(thiz->header.magic != daisy_head.magic){
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
		struct DMark mark;
		info_debug("write\n");
		memcpy(&thiz->header, &daisy_head, sizeof(thiz->header));
		memset(&mark, 0, sizeof(struct DMark));
		fwrite((const void*)(&daisy_head), 
			sizeof(struct DaisyHead), 
			1,thiz->fp);
		fwrite((const void*)(&mark), //resume mark
			sizeof(struct DMark), 
			1,thiz->fp);
	}

	info_debug("header, count=%d\n",thiz->header.count);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return thiz;
faile:
	dsMark_destroy(thiz);
	return NULL;
}

int dsMark_saveResumeMark(struct DaisyMark *thiz, 
	struct DMark *mark)
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
	fwrite((const void*)mark, sizeof(struct DMark), 
		1,thiz->fp);
	fflush(thiz->fp);
	return 0;
}

int dsMark_getResumeMark(struct DaisyMark *thiz, 
	struct DMark *mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz,-1);
	
	if(!thiz->header.resume_mrk){
		info_err("no resume mark\n");
		return -1;
	}

	offset = sizeof(thiz->header);
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, sizeof(struct DMark), 1,thiz->fp);
	return 0;
}


int dsMark_checkMark(struct DaisyMark *thiz, 
	struct DMark *mark)
{
	uint32_t cur;
	long offset;
	struct DMark curMark;

	return_val_if_fail(NULL!=thiz,-1);
	
	if(thiz->header.count >= MAX_MARK_COUNT){
		info_debug("mark full\n");
		return MAX_MARK_COUNT;
	}

	for(cur=0; cur< thiz->header.count; cur++){
		offset = thiz->head_size + cur*sizeof(struct DMark);
		fseek(thiz->fp, offset, SEEK_SET);
		fread((void*)&curMark, sizeof(struct DMark), 1,thiz->fp);
		if(0 ==memcmp((void*)&curMark, mark, sizeof(struct DMark))){
			info_debug("mark already exist\n");
			return cur;
		}
	}
	return -1;
}

int dsMark_addMark(struct DaisyMark *thiz, 
	struct DMark *mark)
{
	return_val_if_fail(NULL!=thiz, -1 );
	if(thiz->header.count >= MAX_MARK_COUNT){
		info_debug("mark full\n");
		return -1;
	}

	thiz->header.count++;
	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header,
		sizeof(thiz->header), 
		1,thiz->fp);
	thiz->w_offset = thiz->head_size + 
		(thiz->header.count-1)*sizeof(struct DMark);
	info_debug("w_offset=0x%lx\n", thiz->w_offset);
	fseek(thiz->fp, thiz->w_offset, SEEK_SET);
	fwrite((const void*)mark, sizeof(struct DMark), 
		1,thiz->fp);
	thiz->w_offset += sizeof(struct DMark);
	info_debug("mark count=%d\n", thiz->header.count);
	fflush(thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return thiz->header.count;
}

int dsMark_delCurMark(struct DaisyMark *thiz)
{
	long offset;
	int num;
	char buf[MAX_MARK_COUNT*sizeof(struct DMark)];

	return_val_if_fail(NULL!=thiz,-1);
	/*last, rear*/
	if(thiz->cur +1 >= thiz->header.count){
		thiz->header.count--;
		thiz->cur--;
		fseek(thiz->fp, 0, SEEK_SET);
		fwrite((const void*)&thiz->header,
			sizeof(thiz->header), 
			1,thiz->fp);
		book_set_bookmark(thiz->cur ,thiz->header.count);
		return 0;
	}

	/*move marks*/
	offset = thiz->head_size+
		(thiz->cur +1)*sizeof(struct DMark);

	num = (thiz->header.count - thiz->cur-1);
	
	fseek(thiz->fp, offset, SEEK_SET);
	
	fread((void*)buf, 
		sizeof(struct DMark), 
		num,thiz->fp);

	fseek(thiz->fp, offset-sizeof(struct DMark), 
		SEEK_SET);
	fwrite((const void*)buf, sizeof(struct DMark), 
		num,thiz->fp);

	thiz->header.count--;
	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header,
		sizeof(thiz->header), 
		1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

int dsMark_delAllMark(struct DaisyMark *thiz)
{
	return_val_if_fail(NULL!=thiz,-1);
	
	thiz->header.count = 0;
	thiz->cur = 0;

	fseek(thiz->fp, 0, SEEK_SET);
	fwrite((const void*)&thiz->header,
		sizeof(thiz->header), 
		1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

char *dsMark_getPath(struct DaisyMark *thiz)
{
	return_val_if_fail(NULL!=thiz, NULL);
	
	return thiz->mrk_file;
}

int dsMark_getCurMark(struct DaisyMark *thiz, 
	struct DMark*mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz, -1);
	
	if((thiz->cur < 0)||
		(thiz->cur +1 >= thiz->header.count)){
		return -1;
	}

	offset = thiz->head_size +
		thiz->cur*sizeof(struct DMark);
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, 
		sizeof(struct DMark), 
		1,thiz->fp);
	return 0;
}

int dsMark_nextMark(struct DaisyMark *thiz, 
	struct DMark*mark)
{
	long offset;
	
	return_val_if_fail(NULL!=thiz, -1);
	
	if(thiz->header.count == 0 || thiz->header.count > MAX_MARK_COUNT){
		return -1;
	}
	thiz->cur++;
	if(thiz->cur >= thiz->header.count){
		thiz->cur = 0;
	}
	
	info_debug("next mark cur=%d\n", thiz->cur);
	
	offset = thiz->head_size +
		thiz->cur*sizeof(struct DMark);
	fseek(thiz->fp, offset, SEEK_SET);
	fread(( void*)mark, 
		sizeof(struct DMark), 
		1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}


int dsMark_prevMark(struct DaisyMark *thiz, 
	struct DMark*mark)
{
	long offset;

	return_val_if_fail(NULL!=thiz, -1);
	if(thiz->header.count == 0 || thiz->header.count > MAX_MARK_COUNT){
		return -1;
	}

	thiz->cur --;
	if(thiz->cur < 0){
		thiz->cur = thiz->header.count -1;
	}
	info_debug("prev mark cur=%d\n", thiz->cur);
	offset = thiz->head_size +
		thiz->cur*sizeof(struct DMark);
	
	fseek(thiz->fp, offset, SEEK_SET);
	fread((void*)mark, 
		sizeof(struct DMark), 
		1,thiz->fp);
	book_set_bookmark(thiz->cur ,thiz->header.count);
	return 0;
}

int dsMark_getMarkNum(struct DaisyMark *thiz)
{
	return_val_if_fail(NULL!=thiz, 0);
	return thiz->header.count;
}


void dsMark_destroy(struct DaisyMark *thiz)
{
	info_debug("dsMark_destroy\n");
	if(thiz){
		if(thiz->fp){
			fclose(thiz->fp);
			thiz->fp = NULL;
		}
		app_free(thiz);
		thiz = NULL;
	}
}

