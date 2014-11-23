#ifndef __DAISY_MARK_H_
#define __DAISY_MARK_H_

#include <limits.h>

struct DaisyMark;

#define MAX_MARK_COUNT 1024

struct DMark{
	unsigned long	ulMillsBegin;			// start time of audio file (msec)
	unsigned long	ulMillsPlayed;			// played time in phrase (msec)
	unsigned long	ulMillsEnd;				// end time of audio file (msec)
	unsigned long	ulElapsedMills;			// played time in contents (msec)
	unsigned long	ulAttribute;			// attribute
	unsigned short	usSectionLevel;			// section level
	unsigned long	ulNormalPageNum;		// page number 
	unsigned long	ulPhraseNumInSec;		// phrase number in section
	unsigned long	ulTocSeqNo;				// play order number
	unsigned long	ulSecOffset;			// offset value in ncc/ncx file
	unsigned long	ulPhrOffset;			// offset value in smile file
	unsigned long	ulTxtOffset;			// offset value in text file (current position)
	unsigned long	ulBasOffset;			// offset value in text file (base position)
	unsigned long	ulUser;					// additional information
};

struct DaisyHead{
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

struct DaisyMark
{
	struct DaisyHead header;
	FILE 	*fp;
	int 	head_size;
	int flag;
	long	r_offset; 	
	long	w_offset;
	int cur;
	char mrk_file[PATH_MAX];
};

struct DaisyMark *dsMark_create(const char*fname);

int dsMark_saveResumeMark(struct DaisyMark *thiz, 
	struct DMark*mark);

int dsMark_getResumeMark(struct DaisyMark *thiz, 
	struct DMark*mark);

int dsMark_checkMark(struct DaisyMark *thiz, 
	struct DMark *mark);

int dsMark_addMark(struct DaisyMark *thiz, 
	struct DMark*mark);

int dsMark_delCurMark(struct DaisyMark *thiz);

int dsMark_delAllMark(struct DaisyMark *thiz);

char *dsMark_getPath(struct DaisyMark *thiz);

int dsMark_getCurMark(struct DaisyMark *thiz, 
	struct DMark*mark);


int dsMark_nextMark(struct DaisyMark *thiz, 
	struct DMark*mark);

int dsMark_prevMark(struct DaisyMark *thiz, 
	struct DMark*mark);

int dsMark_getMarkNum(struct DaisyMark *thiz);

void dsMark_destroy(struct DaisyMark *thiz);


#endif
