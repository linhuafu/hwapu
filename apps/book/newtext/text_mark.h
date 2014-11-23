#ifndef __TEXT_MARK_H_
#define __TEXT_MARK_H_

#include <limits.h>

//struct TextMark;

#define MAX_MARK_COUNT 1024

typedef struct Mark{
	unsigned long long ulOffset;			//in file offset
}MARK;

struct TextHead{
	uint32_t magic;
	uint32_t count;
	uint32_t flag;
	uint32_t mark_count;
	int32_t last;
	uint32_t font_type; /*0, 12, 16, 24*/
	/*parse offset*/
	uint32_t lang;
	uint32_t resume_mrk;/*have resume mark*/
};

typedef struct TextMark
{
	struct TextHead header;
	FILE 	*fp;
	int 	head_size;
	int flag;
	long	r_offset; 	
	long	w_offset;
	int cur;
	char mrk_file[PATH_MAX];
}TEXTMARK;

TEXTMARK *mark_create(const char*fname);

int mark_getResumeMark(TEXTMARK *thiz, MARK *mark);

int mark_saveResumeMark(TEXTMARK *thiz, MARK *mark);

int mark_checkMark(TEXTMARK *thiz, MARK *mark);

int mark_addMark(TEXTMARK *thiz, MARK *mark);

int mark_delCurMark(TEXTMARK *thiz);

int mark_delAllMark(TEXTMARK *thiz);

char *mark_getPath(TEXTMARK *thiz);

int mark_getCurMark(TEXTMARK *thiz, MARK *mark);

int mark_getNextMark(TEXTMARK *thiz, MARK *mark);

int mark_getPrevMark(TEXTMARK *thiz, MARK *mark);

int mark_getMarkNum(TEXTMARK *thiz);

void mark_destroy(TEXTMARK *thiz);


#endif
