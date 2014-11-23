#ifndef __DAISY_PARSE_H_
#define __DAISY_PARSE_H_

#include "tchar.h"
#include "PlexDaisyAnalyzeAPI.h"
#include "daisy_mark.h"


typedef enum{
	BMODE_INVALID = 0,
	BMODE_AUDIO_ONLY= 0x1,
	BMODE_TEXT_ONLY= 0x2,
	BMODE_AUDIO_TEXT= 0x3,
}DAISY_PLAY_MODE;

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
}DAISY_FUNC;

#define JU_MAX_FUNC 10

typedef struct {
	unsigned long curtime;	/*current time in audio file*/
	unsigned long begtime;	/*begin time in audio file*/
	unsigned long endtime;	/*end time in audio file*/
	unsigned long pasttime; /*past time in Total*/
	unsigned long totaltime;/*Total time*/
}DTimeInfo;

#define STR_TEXT_LEN 4096

struct daisy_parse;
//typedef struct daisy_parse DaisyParse;

typedef int (*FuncPhraseExit)(struct daisy_parse *thiz);

typedef struct daisy_parse{
	char		daisy_file[PATH_MAX];	//daisy main file
	
	DAISY_PLAY_MODE mode;
	FuncPhraseExit isPhraseExist;
	
	int 		funcs;			//DAISY_FUNC
	
	char 		strText[STR_TEXT_LEN];
	//int			strTextSize;
	char 		strPath[PATH_MAX];
	
	DTimeInfo	time;
		
	stFileAccessCallback fileAccess;
	stPhraseInfoElement* pphraseInfo;
	stPhraseInfoElement* pphraseTitle;
	stPhraseInfoElement* pphraseBookmark;
	stPhraseString*		 pphraseLabel;
	stTitleInfoElement	 titleInfo;

	struct DaisyMark *mark;
	struct DMark dMark;
	int	backup;
}DaisyParse;


int daisy_getPhaseText(DaisyParse *thiz);

int daisy_getPhaseTime(DaisyParse *thiz);


int daisy_getPageInfo(DaisyParse *thiz,
	unsigned long *curPage, unsigned long *totalPage);

int daisy_nextMark(DaisyParse *thiz);


int daisy_prevMark(DaisyParse *thiz);


int daisy_resumeMark(DaisyParse *thiz);


int daisy_jumpHead(DaisyParse *thiz);


int daisy_jumpTail(DaisyParse *thiz);


int daisy_nextPhrase(DaisyParse *thiz);

int daisy_prevPhrase(DaisyParse *thiz);


int daisy_prevScreen( DaisyParse *thiz);

int daisy_nextScreen( DaisyParse *thiz);


int daisy_prevSentence( DaisyParse *thiz);

int daisy_nextSentence( DaisyParse *thiz);


int daisy_prevParagraph( DaisyParse *thiz);

int daisy_nextParagraph( DaisyParse *thiz);


int daisy_setHeadingLevel(DaisyParse *thiz, int level);

int daisy_getHeadingLevel(DaisyParse *thiz, 
	int *cur, int *maxlevel);


int daisy_prevHeading( DaisyParse *thiz);

int daisy_nextHeading( DaisyParse *thiz);


int daisy_nextGroup( DaisyParse *thiz);

int daisy_prevGroup( DaisyParse *thiz);


int daisy_JumpPage( DaisyParse *thiz, unsigned long ulPageNo);

int daisy_nextPage( DaisyParse *thiz);

int daisy_prevPage( DaisyParse *thiz);


int daisy_next10min( DaisyParse *thiz, unsigned long *curtime);

int daisy_prev10min( DaisyParse *thiz, unsigned long *curtime);

int daisy_next30sec( DaisyParse *thiz, unsigned long *curtime);

int daisy_prev30sec( DaisyParse *thiz, unsigned long *curtime);

int daisy_next5sec( DaisyParse *thiz, unsigned long *curtime);

int daisy_prev5sec( DaisyParse *thiz, unsigned long *curtime);

int daisy_getHeadingPhrase( DaisyParse *thiz, unsigned long ulHeadingNo,
	char *strText, size_t textLen, char *strPath, size_t pathLen, unsigned long *begtime, unsigned long *endtime);

int daisy_phrase2mark(DaisyParse *thiz,struct DMark *mark);

DaisyParse *daisy_open(const char *fname);
void daisy_close(DaisyParse *thiz);

PlexResult ConvertHtmlEpubToText(const char *filepath, char *outfile, const int bufsize);

#endif
