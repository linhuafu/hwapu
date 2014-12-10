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
#include "PlexDaisyAnalyzeAPI.h"
#include "convert.h"

#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#define BUSYBOX "/bin/busybox"


#define OSD_DBG_MSG
#include "nc-err.h"
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"

#include "daisy_parse.h"
#include "daisy_mark.h"
#include "convert.h"
#include "typedef.h"
#include "dbmalloc.h"


/*Daisy SDK spec*/
#define CODE_PAGE_UTF8  65001
#define CODE_PAGE_GBK  936
#define CODE_PAGE_BIG5  950
#define CODE_PAGE_ISCII 57002 //India

#define WORK_DIR_PATH "/tmp/Work/"
//#define WORK_DIR_PATH  "/media/mmcblk0p2/"

//#define WORK_DIR_PATH  "/media/mmcblk0p2/Work"


#define WORK_DIR_SIZE (5*1024*1024L)

/* sequential playback */
static int daisy_prevPhraseImpl(DaisyParse *thiz);

/* skip, jump */

static void OutputTimeInfo(unsigned long ulTime)
{
#if 0
	unsigned long	ulSec, ulMin;
	ulSec = ulTime/1000;
	ulMin = ulSec/60;
	//_tprintf(_T("%.2d[h]%.2d[m]%.2d.%.3d[s]\n"), ulMin/60, ulMin%60, ulSec%60, ulTime%1000);

	info_normal("%.2ld[h]%.2ld[m]%.2ld.%.3ld[s]\n", ulMin/60, ulMin%60, ulSec%60, ulTime%1000);
#endif
}

static void DebugPhraseInfo(const stPhraseInfoElement* pInfo, int isaudio)
{
#if 0
	info_normal("Phase Attribute:");
	if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_HEAD){
		info_normal("Book Head ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_TAIL){
		info_normal("Book Tail ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_SECTION){
		info_normal("Section ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_GROUP){
		info_normal("Group ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_MASKPAGE){
		info_normal("Page ");
		/* normal , front , special */
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_PHRASE){
		info_normal("Phrase ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_NONE){
		info_normal("None ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_AUDIOTITLE){
		info_normal("AudioTitle ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_TEXTTITLE){
		info_normal("TextTitle ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_NOAUDIO){
		info_normal("NoAudio ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_PAGE_NORMAL){
		info_normal("Page Normal ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_PAGE_FRONT){
		info_normal("Page Front ");
	}else if(pInfo->ulAttribute & PLEX_DAISY_ATTRIB_PAGE_SPECIAL){
		info_normal("Page Special ");
	}
	else{
		info_normal("Unknow");
	}
	info_normal("\n");
	
	if(isaudio){
		info_normal("Audio Info:\n");

		if(pInfo->strPath.usStringSize <=0){
			info_normal("    no audio\n");
		}
		else{
			if(pInfo->ulMillsBegin != 0xffffffff){
				info_normal("Play begin time: ");
				OutputTimeInfo(pInfo->ulMillsBegin);
			}
			else{
				info_normal("Invalid Play begin time\n");
			}

			if(pInfo->ulMillsEnd != 0xffffffff){
				info_normal("Play end time: ");
				OutputTimeInfo(pInfo->ulMillsEnd);
			}
			else{
				info_normal("Invalid Play end time\n");
			}
		}
	}
	else{
		info_normal("Text Info:\n");
		if(pInfo->strText.usStringSize <=0){
			info_normal("  empty text\n");
		}
		else{
		}
	}
#endif
}

static int daisy_setPlayMode(DaisyParse *thiz, unsigned short mode)
{
	PlexResult ret;
	unsigned short gmode;
	
	return_val_if_fail(thiz != NULL, -1);
	
	ret = PlexDaisyGetPlayModeType(&gmode);
	if(ret != PLEX_OK){
		info_err("get mode err\n");
		return -1;
	}

	if(mode == gmode){
		//info_debug("equal not need to change\n");
		return 0;
	}

	if(thiz->mode != BMODE_AUDIO_TEXT){
		//info_err("audio only, or text only\n");
		return -1;
	}

	/*change mode*/
	ret = PlexDaisySetPlayModeType(mode);
	if(ret != PLEX_OK){
		info_err("set play mode =%d err\n", mode);
		return -1;
	}
	return 0;
}

static int daisy_filterText(TCHAR *out, size_t outlen,TCHAR *in, size_t inlen){
	size_t i_in = 0, i_out = 0;
	for(i_in=0; i_in<inlen; i_in++){
		if(0x0 == in[i_in])
		{
			break;
		}

		if(0x11 == in[i_in])
		{	//filtre
			continue;
		}
		out[i_out] = in[i_in];
		i_out ++;
	}
	out[i_out] = 0;
	return 0;
}

int daisy_getPhaseText(DaisyParse *thiz)
{
	PlexResult result;
	int bsize;
	
	return_val_if_fail(thiz != NULL, -1);
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_TEXT_MODE);
	
	if(PlexDaisyGetCurPhrase(thiz->pphraseInfo) == PLEX_OK){
		DebugPhraseInfo(thiz->pphraseInfo, 0);
		if(thiz->pphraseInfo->strText.usStringSize <= 0 
			|| thiz->pphraseInfo->strText.ptcString[0] == 0){
			info_debug("empty text\n");
			thiz->strText[0] = 0;
			return 0;
		}

		memset(thiz->strText, 0, STR_TEXT_LEN);
		bsize = wcslen(thiz->pphraseInfo->strText.ptcString);
		if(bsize > thiz->pphraseInfo->strText.usStringSize)
		{
			bsize = thiz->pphraseInfo->strText.usStringSize;
		}

		if(bsize > STR_TEXT_LEN/4){//over array size
			info_err("over size=%d\n", bsize);
			bsize = STR_TEXT_LEN/4 -1;
		}
		
		daisy_filterText((TCHAR*)thiz->strText,STR_TEXT_LEN/4 -1,
			thiz->pphraseInfo->strText.ptcString,bsize);

		return 0;
	}
	else{
		info_err("PlexDaisyGetCurPhrase err\n");
		thiz->strText[0] = 0;
		return -1;
	}

}

int daisy_getPhaseTime(DaisyParse *thiz)
{
	unsigned long PastTime,TotalTime;
	size_t in_ret, out_ret;

	return_val_if_fail(thiz != NULL, -1);

	if(thiz->mode == BMODE_TEXT_ONLY){
		info_debug("text only\n");
		thiz->time.curtime = -1;
		thiz->time.begtime = -1;
		thiz->time.endtime = -1;
		thiz->time.pasttime = -1;
		thiz->time.totaltime = -1;
		thiz->strPath[0] = 0;
		return -1;
	}

	PlexDaisySetPlayModeType(PLEX_DAISY_PLAY_AUDIO_MODE);
	
	if(PlexDaisyGetCurPhrase(thiz->pphraseInfo) == PLEX_OK){
		DebugPhraseInfo(thiz->pphraseInfo, 1);
		thiz->time.begtime = thiz->pphraseInfo->ulMillsBegin;
		thiz->time.endtime = thiz->pphraseInfo->ulMillsEnd;

		info_debug("begtime=%ld, endtime=%ld\n",
			thiz->time.begtime, thiz->time.endtime);
		PlexDaisyGetTimeInfo(&PastTime, &TotalTime);
		info_debug("Time, Past=%ld,Total=%ld\n",PastTime, TotalTime);
		thiz->time.pasttime = PastTime;
		thiz->time.totaltime = TotalTime;
		if(thiz->pphraseInfo->strPath.usStringSize <= 0){
			thiz->strPath[0] = 0;
			info_debug("empty audio\n");
			return 0;
		}
		
		in_ret = thiz->pphraseInfo->strPath.usStringSize*4;
		out_ret = PATH_MAX;
		memset(thiz->strPath, 0, PATH_MAX);
		utf32_to_utf8((char*)thiz->pphraseInfo->strPath.ptcString,&in_ret,
			(char*)thiz->strPath, &out_ret);
	}
	else{
		
		info_err("PlexDaisyGetCurPhrase err\n");
		thiz->time.curtime = -1;
		thiz->time.begtime = -1;
		thiz->time.endtime = -1;
		thiz->time.pasttime = -1;
		thiz->time.totaltime= -1;
		thiz->strPath[0] = 0;
		return -1;
	}
	
	return 0;
}


int daisy_getPageInfo(DaisyParse *thiz,
	unsigned long *curPage, unsigned long *totalPage)
{
	unsigned long pulCurPage,pulMaxPage,pulLastPage;
	unsigned long pulFrPage,pulSpPage,pulTotalPage;
	PlexResult ret;

	return_val_if_fail(thiz != NULL, -1);
	
	ret = PlexDaisyGetPageInfo(&pulCurPage, &pulMaxPage,&pulLastPage,
		&pulFrPage,&pulSpPage,&pulTotalPage);
	if(ret != PLEX_OK){
		info_err("get page error\n");
		*curPage = PLEX_DAISY_PAGE_NONE;
		*totalPage = 0;
		return -1;
	}

//	info_normal("pulCurPage=%ld\n",pulCurPage);
//	info_normal("pulMaxPage=%ld\n",pulMaxPage);
//	info_normal("pulLastPage=%ld\n",pulLastPage);
//	info_normal("pulFrPage=%ld\n",pulFrPage);
//	info_normal("pulSpPage=%ld\n",pulSpPage);
//	info_normal("pulTotalPage=%ld\n",pulTotalPage);

	*curPage = pulCurPage;
	*totalPage = pulMaxPage;
	
	return 0;
}


static int daisy_isAudioExist(DaisyParse *thiz)
{
	size_t in_ret, out_ret;
	return_val_if_fail(thiz != NULL, 0);
	if(thiz->pphraseInfo->strPath.usStringSize){
		char file[PATH_MAX];
		memset(file, 0, PATH_MAX);
		in_ret = thiz->pphraseInfo->strPath.usStringSize*4;
		out_ret = PATH_MAX;
		utf32_to_utf8((char*)thiz->pphraseInfo->strPath.ptcString,
			&in_ret,
			(char*)file, &out_ret);
	
		if(!access(file,F_OK)){
			return 1;
		}
		
		return 0;
	}
	else{
		return 0;
	}
}


static int daisy_isTextExist(DaisyParse *thiz)
{
	return_val_if_fail(thiz != NULL, 0);
	if(thiz->pphraseInfo->strText.usStringSize>0 
		&& thiz->pphraseInfo->strText.ptcString[0]!=0){
		return 1;
	}
	else{
		return 0;
	}
}


static int daisy_isPhrase(DaisyParse *thiz)
{
	return_val_if_fail(thiz != NULL, 0);

	if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_PHRASE){
			//info_debug("phrase phrase\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_MASKPAGE){
			//info_debug("phrase page\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_GROUP){
			//info_debug("phrase group\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_SECTION){
			//info_debug("phrase section\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_TAIL){
			//info_debug("phrase tail\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_HEAD){
			//info_debug("phrase head\n");
			return 1;
	}
	else{
		return 0;
	}
}

static int daisy_isPage(DaisyParse *thiz)
{
	return_val_if_fail(thiz != NULL, 0);
	
	if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_MASKPAGE){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_GROUP){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_SECTION){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_TAIL){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_HEAD){
			return 1;
	}
	else{
		return 0;
	}
}

static int daisy_isGroup(DaisyParse *thiz)
{
	return_val_if_fail(thiz != NULL, 0);
	
	if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_GROUP){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_SECTION){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_TAIL){
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_HEAD){
			return 1;
	}
	else{
		return 0;
	}
}

static int daisy_isSection(DaisyParse *thiz)
{
	return_val_if_fail(thiz != NULL, 0);
	
	if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_SECTION){
			info_debug("section\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_TAIL){
			info_debug("tail\n");
			return 1;
	}
	else if(thiz->pphraseInfo->ulAttribute 
			& PLEX_DAISY_ATTRIB_HEAD){
			info_debug("head\n");
			return 1;
	}
	else{
		return 0;
	}
}

int daisy_mark2phrase(DaisyParse *thiz, 
	struct DMark *mark)
{
	PlexResult ret;
//	info_debug("backup mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	thiz->pphraseInfo->ulMillsBegin = mark->ulMillsBegin;
	thiz->pphraseInfo->ulMillsPlayed = mark->ulMillsPlayed;
	thiz->pphraseInfo->ulMillsEnd = mark->ulMillsEnd;
	thiz->pphraseInfo->ulElapsedMills = mark->ulElapsedMills;

	thiz->pphraseInfo->ulAttribute = mark->ulAttribute;
	thiz->pphraseInfo->usSectionLevel = mark->usSectionLevel;
	thiz->pphraseInfo->ulNormalPageNum = mark->ulNormalPageNum;
	thiz->pphraseInfo->ulPhraseNumInSec = mark->ulPhraseNumInSec;
	thiz->pphraseInfo->ulTocSeqNo = mark->ulTocSeqNo;

	thiz->pphraseInfo->ulSecOffset = mark->ulSecOffset;
	thiz->pphraseInfo->ulPhrOffset = mark->ulPhrOffset;
	thiz->pphraseInfo->ulTxtOffset = mark->ulTxtOffset;
	thiz->pphraseInfo->ulBasOffset = mark->ulBasOffset;
	thiz->pphraseInfo->ulUser = mark->ulUser;
	
//	info_debug("ulSecOffset=0x%lx\n", thiz->pphraseInfo->ulSecOffset);
//	info_debug("ulPhrOffset=0x%lx\n", thiz->pphraseInfo->ulPhrOffset);
//	info_debug("ulTxtOffset=0x%lx\n", thiz->pphraseInfo->ulTxtOffset);
//	info_debug("ulBasOffset=0x%lx\n", thiz->pphraseInfo->ulBasOffset);
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisySetCurPhrase(thiz->pphraseInfo, 1);
	if(PLEX_OK != ret){
		info_err("!!set error\n");
		return -1;
	}
	
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	if(PLEX_OK != ret){
		info_err("!!get error\n");
		return -1;
	}
	if(!thiz->isPhraseExist(thiz))
	{
		info_err("err\n");
		return -1;
	}

	//next err need seek
	if(BMODE_TEXT_ONLY != thiz->mode)
	{
		unsigned long PastTime, TotalTime;
		unsigned long newPastTm,newTotalTm;
		
		PlexDaisyGetTimeInfo(&PastTime, &TotalTime);
		info_debug("PastTime:%ld\n", PastTime);
		
		ret = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
		while(PLEX_OK == ret){
			if( daisy_isPhrase(thiz) && thiz->isPhraseExist(thiz)){
				PlexDaisyGetTimeInfo(&newPastTm, &newTotalTm);
				//info_debug("newPastTm:%ld\n", newPastTm);
				if(newPastTm >= PastTime ){
					break;
				}
			}
			//info_err(" next phrase\n");
			ret = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
		}

		if(PLEX_OK != ret){
			info_err("find err\n");
			return -1;
		}

		if(newPastTm == PastTime ){
			return 0;
		}
		int iret = daisy_prevPhraseImpl(thiz);
		if(iret >= 0){
			PlexDaisyGetTimeInfo(&newPastTm, &newTotalTm);
			//info_debug("newPastTm:%ld\n", newPastTm);
			if(newPastTm != PastTime ){
				info_err("#############err\n");
			}
		}
		//info_err("iret:%d\n",iret);
		return iret;
	}

	return 0;
}

int daisy_phrase2mark(DaisyParse *thiz,struct DMark *mark)
{
//	info_debug("set mark\n");
	
	return_val_if_fail(thiz != NULL, -1);
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	if(PLEX_OK != PlexDaisyGetCurPhrase(thiz->pphraseInfo))
	{
		info_err("get error\n");
		return -1;
	}
	
	if(!daisy_isPhrase(thiz)){
		info_err("non phrase\n");
		return -1;
	}
	
	if(!thiz->isPhraseExist(thiz)){
		info_err("non exist\n");
		return -1;
	}

	mark->ulMillsBegin = thiz->pphraseInfo->ulMillsBegin;
	mark->ulMillsPlayed = thiz->pphraseInfo->ulMillsPlayed;
	mark->ulMillsEnd = thiz->pphraseInfo->ulMillsEnd;
	mark->ulElapsedMills = thiz->pphraseInfo->ulElapsedMills;
	mark->ulAttribute = thiz->pphraseInfo->ulAttribute;
	mark->usSectionLevel = thiz->pphraseInfo->usSectionLevel;
	mark->ulNormalPageNum = thiz->pphraseInfo->ulNormalPageNum;
	mark->ulPhraseNumInSec = thiz->pphraseInfo->ulPhraseNumInSec;
	mark->ulTocSeqNo = thiz->pphraseInfo->ulTocSeqNo;
	mark->ulSecOffset = thiz->pphraseInfo->ulSecOffset;
	mark->ulPhrOffset = thiz->pphraseInfo->ulPhrOffset;
	mark->ulTxtOffset = thiz->pphraseInfo->ulTxtOffset;
	mark->ulBasOffset = thiz->pphraseInfo->ulBasOffset;
	mark->ulUser = thiz->pphraseInfo->ulUser;
	
//	info_debug("ulSecOffset=0x%lx\n", thiz->pphraseInfo->ulSecOffset);
//	info_debug("ulPhrOffset=0x%lx\n", thiz->pphraseInfo->ulPhrOffset);
//	info_debug("ulTxtOffset=0x%lx\n", thiz->pphraseInfo->ulTxtOffset);
//	info_debug("ulBasOffset=0x%lx\n", thiz->pphraseInfo->ulBasOffset);
	return 0;
}


int daisy_nextMark(DaisyParse *thiz)
{
	struct DMark mark;
	int ret;	
	info_debug("next mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = dsMark_nextMark(thiz->mark, 
				&mark);
	if(ret < 0){
		info_err("dsMark_nextMark err\n");
		return -1;
	}
	info_debug("backup\n");
	ret = daisy_mark2phrase(thiz, &mark);
	return ret;
}

int daisy_prevMark(DaisyParse *thiz)
{
	struct DMark mark;
	int ret;

	info_debug("prev mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = dsMark_prevMark(thiz->mark, 
				&mark);
	if(ret < 0){
		return -1;
	}
	ret = daisy_mark2phrase(thiz, &mark);
	return ret;
}

int daisy_resumeMark(DaisyParse *thiz)
{
	struct DMark mark;
	int ret;

	info_debug("resume mark\n");
	return_val_if_fail(thiz != NULL, -1);
	
	ret = dsMark_getResumeMark(thiz->mark, 
				&mark);
	if(ret < 0){
		return -1;
	}
	ret = daisy_mark2phrase(thiz, &mark);
	return ret;
}


int daisy_jumpHead(DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	ret = PlexDaisyJumpBookHead();
	if(ret == PLEX_OK){
		return 0;
	}
	return -1;
}

int daisy_jumpTail(DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	ret = PlexDaisyJumpBookTail();
	if(ret == PLEX_OK){
		return 0;
	}
	return -1;
}


static void daisy_backupPhrese(DaisyParse *thiz)
{
	int ret = daisy_phrase2mark(thiz,&thiz->dMark);
	if(-1 != ret){
		thiz->backup ++;
		//DBGMSG("thiz->backup:%d\n",thiz->backup);
	}else{
		DBGMSG("backup err\n");
	}
}

static void daisy_restorePhrase(DaisyParse *thiz)
{
	if(thiz->backup){
		daisy_mark2phrase(thiz,&thiz->dMark);
		thiz->backup --;
		//DBGMSG("thiz->backup:%d\n",thiz->backup);
	}
}

static int daisy_nextPhraseImpl(DaisyParse *thiz)
{
	PlexResult result;
	return_val_if_fail(thiz != NULL, -1);
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	result = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
	while(result == PLEX_OK){
		if(daisy_isPhrase(thiz) && thiz->isPhraseExist(thiz)){
			break;
		}
		info_err(" next phrase\n");
		result = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
	}

	if(result != PLEX_OK){
		info_err("no phrase\n");
		return -1;
	}
	return 0;
}

static int daisy_prevPhraseImpl(DaisyParse *thiz)
{
	PlexResult result;
	return_val_if_fail(thiz != NULL, -1);
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	result = PlexDaisyGetPrevPhrase(thiz->pphraseInfo);
	while(result == PLEX_OK){
		if(daisy_isPhrase(thiz)&& thiz->isPhraseExist(thiz))
		{
			break;
		}
		info_err("prev phase\n");
		result = PlexDaisyGetPrevPhrase(thiz->pphraseInfo);
	}

	if(result != PLEX_OK){
		info_err("err\n");
		return -1;
	}
	return 0;
}


int daisy_nextPhrase(DaisyParse *thiz)
{
	PlexResult result;
	return_val_if_fail(thiz != NULL, -1);
	//backup current phrese
	DBGMSG("\n");
	daisy_backupPhrese(thiz);
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	result = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
	while(result == PLEX_OK){
		if(daisy_isPhrase(thiz) 
			&& thiz->isPhraseExist(thiz)){
			break;
		}
		info_err(" next phrase\n");
		result = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
	}

	if(result != PLEX_OK){
		info_err("no next\n");
		daisy_restorePhrase(thiz);
		return -1;
	}
	return 0;
}



int daisy_prevPhrase(DaisyParse *thiz)
{
	PlexResult result;
	return_val_if_fail(thiz != NULL, -1);

	daisy_backupPhrese(thiz);

	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	result = PlexDaisyGetPrevPhrase(thiz->pphraseInfo);
	
	while(result == PLEX_OK){
		if(daisy_isPhrase(thiz)&& thiz->isPhraseExist(thiz))
		{
			break;
		}
		info_err("prev phase\n");
		result = PlexDaisyGetPrevPhrase(thiz->pphraseInfo);
	}

	if(result != PLEX_OK){
		info_err("err\n");
		daisy_restorePhrase(thiz);
		return -1;
	}

	return 0;
}


static int daisy_skipSentence(DaisyParse *thiz,long lSkipCnt)
{
	PlexResult ret;
	info_debug("skip sentence\n");
	return_val_if_fail(thiz != NULL, -1);

	daisy_backupPhrese(thiz);
	
	ret = PlexDaisySkipSentence(lSkipCnt);
	if(ret != PLEX_OK){
		info_debug("error\n");
		goto lfail;
	}

	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	if(!daisy_isPhrase(thiz)){
		info_err("skip\n");
		goto lfail;
	}
	
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;
}

int daisy_prevScreen( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,-20);
}

int daisy_nextScreen( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,20);
}

int daisy_prevSentence( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,-1);
}

int daisy_nextSentence( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,1);
}

int daisy_prevParagraph( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,-5);
}

int daisy_nextParagraph( DaisyParse *thiz)
{
	return daisy_skipSentence(thiz,5);
}

int daisy_setHeadingLevel(DaisyParse *thiz, int level)
{
	//unsigned short curLevel, MaxDepth;
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	ret = PlexDaisySetHeadingLevel((unsigned short)level);
	if(ret != PLEX_OK){
		info_err("set heading level\n");
		return -1;
	}
	
	return 0;
}


int daisy_getHeadingLevel(DaisyParse *thiz, 
	int *cur, int *maxlevel)
{
	unsigned short curLevel, MaxDepth;
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	ret = PlexDaisyGetHeadingLevel(&curLevel, &MaxDepth);
	if(ret != PLEX_OK){
		info_err("get heading level\n");
		*cur = 0;
		*maxlevel = 0;
		return -1;
	}
	*cur = curLevel;
	*maxlevel = MaxDepth;
	
	return 0;
}


int daisy_prevHeading( DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisySkipHeading(-1);
	if(ret != PLEX_OK){
		info_debug("error\n");
		goto lfail;
	}

	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	if(!daisy_isSection(thiz)){
		info_err("perv head\n");
		goto lfail;
	}
	
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;
}


int daisy_nextHeading( DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisySkipHeading(1);
	if(ret != PLEX_OK){
		info_debug("error\n");
		goto lfail;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	info_debug("result=%d\n", ret);
	if(!daisy_isSection(thiz)){
		info_err("next head\n");
		goto lfail;
	}
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;
}

int daisy_nextGroup( DaisyParse *thiz)
{
	unsigned long total;
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisyGetGroupInfo(&total);
	if(ret < 0){
		goto lfail;
	}

	ret = PlexDaisySkipGroup(1);
	if(ret != PLEX_OK){
		info_err("skip next group\n");
		goto lfail;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	info_debug("result=%d\n", ret);
	if(!daisy_isGroup(thiz)){
		info_err("no prev/n");
		goto lfail;
	}

	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;
}

int daisy_prevGroup( DaisyParse *thiz)
{
	unsigned long total;
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisyGetGroupInfo(&total);
	if(ret < 0){
		goto lfail;
	}

	ret = PlexDaisySkipGroup(-1);
	if(ret != PLEX_OK){
		info_err("skip prev group\n");
		goto lfail;
	}

	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	info_debug("result=%d\n", ret);
	if(!daisy_isGroup(thiz)){
		info_err("no prev/n");
		goto lfail;
	}

	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;	
}

int daisy_JumpPage( DaisyParse *thiz, unsigned long ulPageNo)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisyJumpPage(ulPageNo); /*next page*/
	if(ret != PLEX_OK){
		info_err("jump page end\n");
		goto lfail;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	
	if(!daisy_isPage(thiz)){
		info_err("no next/n");
		ret = -1;
		goto lfail;
	}
	
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return ret;		
}


int daisy_nextPage( DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisySkipPage(1); /*next page*/
	if(ret != PLEX_OK){
		info_err("jump page end\n");
		goto lfail;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	
	if(!daisy_isPage(thiz)){
		info_err("no next/n");
		goto lfail;
	}
	
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;		
}


int daisy_prevPage( DaisyParse *thiz)
{
	PlexResult ret;
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	ret = PlexDaisySkipPage(-1);
	if(ret != PLEX_OK){
		info_err("prev page head\n");
		goto lfail;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	if(!daisy_isPage(thiz)){
		info_err("no prev\n");
		goto lfail;
	}
	
	if(thiz->isPhraseExist(thiz)){
		return 0;
	}
	if(0 == daisy_nextPhraseImpl(thiz)){
		return 0;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;	
}


static int daisy_timeCmp(DTimeInfo *t1, DTimeInfo *t2)
{
	unsigned long g_curtime;
	unsigned long limit;
	/*convert to uni*/
	//g_curtime = t1->curtime + t1->pasttime;
	g_curtime = t1->curtime;
	limit = t2->pasttime + t2->endtime - t2->begtime;
	
	if(g_curtime < t2->pasttime){
		return -1;
	}
	else if(g_curtime < limit)
	{
		/*match*/
		return 0;
	}
	else{
		return 1;
	}
}

static int daisy_skipSec( DaisyParse *thiz, 
	long skip_msec,unsigned long *curtime)
{
	PlexResult ret;
	unsigned long PastTime;
	unsigned long TotalTime;
	long save_cur;
	DTimeInfo newtime;
	DTimeInfo checktime;
	
	int cmp;
	int loop;
	int ret_val;
	int ret_test;

	return_val_if_fail(thiz != NULL, -1);

	if(0==skip_msec){
		return 0;
	}
	daisy_backupPhrese(thiz);
	
	save_cur = *curtime;
	info_debug("curtime=%ld\n", save_cur);

	ret = PlexDaisyGetTimeInfo(&PastTime, &TotalTime);
	if(ret != PLEX_OK){
		info_err("get total time err\n");
		goto lfail;
	}

	info_debug("Past=%ld, Total=%ld\n",
		PastTime, TotalTime);

	if(skip_msec < 0 ){
		//prev time
		info_debug("prev skip\n");
		if(save_cur <= 0){
			info_err("is head here\n");
			goto lfail;
		}
		if((save_cur + skip_msec) <= 0){
			info_debug("goto head\n");
			goto lfail;
		}
	}else{
		info_debug("next skip\n");
		if(save_cur >= TotalTime){
			info_err("is end here\n");
			goto lfail;
		}
	
		if(save_cur + skip_msec >= TotalTime){
			info_debug("goto end\n");
			goto lfail;
		}
	}
	
	/*Search from current phase.*/
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	
	newtime.curtime = save_cur + skip_msec;
	newtime.begtime = thiz->time.begtime;
	newtime.endtime = thiz->time.endtime;
	newtime.pasttime = thiz->time.pasttime;

	checktime.curtime = 0;
	checktime.begtime = thiz->pphraseInfo->ulMillsBegin;
	checktime.endtime = thiz->pphraseInfo->ulMillsEnd;
	checktime.pasttime = PastTime;
	
	ret_val = -1;
	loop = 1;
	do{
		cmp = daisy_timeCmp(&newtime, &checktime);
		if(cmp < 0){
			/*search prev*/
			info_debug("prev search\n");
			if(daisy_prevPhraseImpl(thiz)<0){
				ret_val = -1;
				info_err("no prev phase match\n");
				break;
			}
			ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
			ret = PlexDaisyGetTimeInfo(&PastTime, &TotalTime);
			checktime.curtime = 0;
			checktime.begtime = thiz->pphraseInfo->ulMillsBegin;
			checktime.endtime = thiz->pphraseInfo->ulMillsEnd;
			checktime.pasttime = PastTime;

		}
		else if(cmp == 0){
			/*match*/
			//time->curtime = new_time;
			ret_val = 0;
			info_debug("match break\n");
			ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
			*curtime = newtime.curtime;
			info_debug("cur time=%ld\n",*curtime);
			break;
		}
		else{
			info_debug("next search\n");
			if(daisy_nextPhraseImpl(thiz)<0){
				ret_val = -1;
				info_err("no next phase match\n");
				break;
			}
			ret = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
			ret = PlexDaisyGetTimeInfo(&PastTime, &TotalTime);
			checktime.curtime = 0;
			checktime.begtime = thiz->pphraseInfo->ulMillsBegin;
			checktime.endtime = thiz->pphraseInfo->ulMillsEnd;
			checktime.pasttime = PastTime;

			/*continue cmp*/
		}
	}while(loop);
	if(0 == ret_val){
		return ret_val;
	}
lfail:
	daisy_restorePhrase(thiz);
	return -1;
}


int daisy_next10min( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, 600*1000, curtime);
}

int daisy_prev10min( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, -(600*1000), curtime);
}

int daisy_next30sec( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, 30*1000, curtime);
}

int daisy_prev30sec( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, -30*1000, curtime);
}

int daisy_next5sec( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, 5*1000, curtime);
}

int daisy_prev5sec( DaisyParse *thiz, unsigned long *curtime)
{
	return daisy_skipSec(thiz, -5*1000, curtime);
}

static int getPhraseTextUtf8(DaisyParse *thiz,char *strText, size_t len)
{
	int size;
	size_t in_ret,out_ret;

	strText[0] = 0;
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_TEXT_MODE);
	if(PlexDaisyGetCurPhrase(thiz->pphraseInfo) == PLEX_OK){
		if(thiz->pphraseInfo->strText.usStringSize <= 0 
			|| thiz->pphraseInfo->strText.ptcString[0] == 0){
			info_debug("empty text\n");
			return -1;
		}
		size = wcslen(thiz->pphraseInfo->strText.ptcString);
		DBGMSG("size:%d\n",size);
		daisy_filterText((TCHAR*)thiz->pphraseInfo->strText.ptcString,size,
			thiz->pphraseInfo->strText.ptcString,size);

		in_ret = wcslen(thiz->pphraseInfo->strText.ptcString)*4;
		out_ret = len;
		memset(strText, 0, len);
		utf32_to_utf8(thiz->pphraseInfo->strText.ptcString, &in_ret, strText, &out_ret);
		return 0;
	}
	return -1;
}

static int getPhrasePathUtf8(DaisyParse *thiz,char *strPath, size_t len,
	unsigned long *begtime, unsigned long *endtime)
{
	int size;
	size_t in_ret,out_ret;

	strPath[0] = 0;
	if(thiz->mode == BMODE_TEXT_ONLY){
		info_debug("text only\n");
		return -1;
	}
	
	PlexDaisySetPlayModeType(PLEX_DAISY_PLAY_AUDIO_MODE);
	if(PlexDaisyGetCurPhrase(thiz->pphraseInfo) == PLEX_OK){
		*begtime = thiz->pphraseInfo->ulMillsBegin;
		*endtime = thiz->pphraseInfo->ulMillsEnd;
		info_debug("begtime=%ld, endtime=%ld\n", *begtime, *endtime);
		if(thiz->pphraseInfo->strPath.usStringSize <= 0){
			info_debug("empty audio\n");
			return -1;
		}
		
		in_ret = thiz->pphraseInfo->strPath.usStringSize*4;
		out_ret = len;
		memset(strPath, 0, len);
		utf32_to_utf8((char*)thiz->pphraseInfo->strPath.ptcString,&in_ret,
			(char*)strPath, &out_ret);
		return 0;
	}

	return -1;
}

int daisy_getHeadingPhrase( DaisyParse *thiz, unsigned long ulHeadingNo,
	char *strText, size_t textLen, char *strPath, size_t pathLen, unsigned long *begtime, unsigned long *endtime)
{
	PlexResult result;
	int ret;
	
	return_val_if_fail(thiz != NULL, -1);
	daisy_backupPhrese(thiz);
	result = PlexDaisyJumpHeading(ulHeadingNo);
	if(result != PLEX_OK){
		info_debug("error\n");
		ret = -1;
		goto lexit;
	}
	
	daisy_setPlayMode(thiz, PLEX_DAISY_PLAY_AUDIO_MODE);
	result = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
	if(!daisy_isSection(thiz)){
		info_err("next head\n");
		ret = -1;
		goto lexit;
	}
	if(thiz->isPhraseExist(thiz)){
		getPhraseTextUtf8(thiz, strText, textLen);
		getPhrasePathUtf8(thiz, strPath, pathLen, begtime, endtime);
		ret = 0;
		goto lexit;
	}

	if(0 == daisy_nextPhraseImpl(thiz)){
		getPhraseTextUtf8(thiz, strText, textLen);
		getPhrasePathUtf8(thiz, strPath, pathLen, begtime, endtime);
		ret = 0;
		goto lexit;
	}
	ret = -1;
lexit:
	daisy_restorePhrase(thiz);
	return ret;
}


//==========================================================================================
//  declaration of file access function
//==========================================================================================

int dir_exist(const char* dir) 
{
	DIR *pdir = opendir(dir);

	if (pdir)
		closedir(pdir);

	return !!pdir;
}

#if 1
static void* SdkFileOpen( const TCHAR* ptcFileName, const TCHAR* ptcMode )
{
	char pFile[PATH_MAX];
	char pMode[PATH_MAX];
	size_t	iLen,outleft;
	FILE *fp;
	
	memset(pFile, 0, sizeof(pFile));
	memset(pMode, 0, sizeof(pMode));
	if(sizeof(TCHAR) > 1){
		iLen = _tcslen(ptcFileName)*sizeof(TCHAR);
		outleft = PATH_MAX;
		utf32_to_utf8(ptcFileName, &iLen, pFile, &outleft);
		iLen = _tcslen(ptcMode)*sizeof(TCHAR);
		outleft = PATH_MAX;
		utf32_to_utf8(ptcMode, &iLen, pMode, &outleft);
	}else{
		strlcpy(pFile,ptcFileName,PATH_MAX);
		strlcpy(pMode,ptcMode,PATH_MAX);
	}
	info_debug("SdkFileOpen:%s pMode:%s\n",pFile,pMode);
	fp = fopen(pFile, pMode);
	info_debug("SdkFileOpen: fp=%d\n",fp);
	return (void*)fp;
}
#else

void* SdkFileOpen( const TCHAR* ptcFileName, const TCHAR* ptcMode )
{
	FILE*			fp;
	char*			pFilename;
	char*			pMode;
	size_t			iLen, outleft;

	iLen = _tcslen(ptcFileName);
	pFilename = (char*)app_malloc((iLen+1)*sizeof(TCHAR));
	memset(pFilename, 0, (iLen+1)*sizeof(TCHAR));
	if(sizeof(TCHAR) > 1){
		//setlocale( LC_ALL, "Japanese" );   
		//wcstombs(pFilename, (const wchar_t*)ptcFileName, (iLen+1)*sizeof(TCHAR));
		iLen *= sizeof(TCHAR);
		utf32_to_utf8(ptcFileName, &iLen, pFilename, &outleft);
	}else{
		memcpy(pFilename, ptcFileName, (iLen+1)*sizeof(TCHAR));
	}

	iLen = _tcslen(ptcMode);
	pMode = (char*)app_malloc((iLen+1)*sizeof(TCHAR)); 
	memset(pMode, 0, (iLen+1)*sizeof(TCHAR));
	if(sizeof(TCHAR) > 1){
		//setlocale( LC_ALL, "Japanese" );
		//wcstombs(pMode, (const wchar_t*)ptcMode, (iLen+1)*sizeof(TCHAR));
		iLen *= sizeof(TCHAR);
		utf32_to_utf8(ptcMode, &iLen, pMode, &outleft);
	}else{
		memcpy(pMode, ptcMode, (iLen+1)*sizeof(TCHAR));
	}

	fp = fopen(pFilename, pMode);

	if(pFilename != NULL){
		app_free(pFilename);
		pFilename = NULL;
	}

	if(pMode != NULL){
		app_free(pMode);
		pMode = NULL;
	}

	return fp;
}
#endif
static bool SdkFileRead( void* pFile, void* pBuffer, unsigned long ulSize, unsigned long* pulReadSize)
{
//	info_debug("SdkFileRead\n");
	*pulReadSize = fread(pBuffer, 1, ulSize, (FILE *)pFile);
	if(*pulReadSize == ulSize){
		return true;
	}else{
		if(feof((FILE *)pFile) != 0){
			return true;
		}else{
			return false;
		}
	}
}

static bool SdkFileWrite( void* pFile, void* pBuffer, unsigned long ulSize, unsigned long* pulWriteSize)
{
	//info_debug("SdkFileWrite fp=%d\n",pFile);

	//info_debug("SdkFileWrite size=%d\n",ulSize);
	
	//info_debug("SdkFileWrite buf=%s\n",pBuffer);
	
	*pulWriteSize = fwrite(pBuffer, 1, ulSize, (FILE *)pFile);

	//info_debug("SdkFileWrite pulWriteSize=%d\n",*pulWriteSize);
	
	
	if(*pulWriteSize == ulSize)
	{


	//	info_debug("SdkFileWrite  equal\n");
		return true;
	}
	else
	{
	//	info_debug("SdkFileWrite  not equal\n");
		
		return false;
	}
	
}

static unsigned long SdkFileSeek(void* pFile, unsigned long ulPos, int iStartPoint)
{
	info_debug("SdkFileSeek ulpos=%uld \n", ulPos);
	fseek((FILE *)pFile, ulPos, iStartPoint);
	return ftell((FILE *)pFile);
}

static unsigned long SdkFileSize(void* pFile)
{
	unsigned long ulRet = 0;

	long lPos = ftell((FILE *)pFile);

	fseek((FILE *)pFile, 0, SEEK_END);
	ulRet = ftell((FILE *)pFile);

	fseek((FILE *)pFile, lPos, SEEK_SET);

	return ulRet;
}

static bool SdkFileClose(void* pFile)
{
	return fclose((FILE *)pFile);
	//return true;
}

static int SdkFileDelete(const TCHAR* ptcFileName)
{
	int iRet;
	char			pFilename[PATH_MAX];
	size_t			iLen,outleft;
 
	memset(pFilename, 0, sizeof(pFilename));
	if(sizeof(TCHAR) > 1){
		//setlocale( LC_ALL, "Japanese" );   
		//wcstombs(pFilename, (const wchar_t*)ptcFileName, (iLen+1)*sizeof(TCHAR));
		iLen = _tcslen(ptcFileName)*sizeof(TCHAR);
		outleft = PATH_MAX;
		utf32_to_utf8(ptcFileName, &iLen, pFilename, &outleft);
	}else{
		strlcpy(pFilename, ptcFileName, PATH_MAX);
	}
	info_debug("SdkFileDelete %s \n", pFilename);
	iRet = remove(pFilename);
	return iRet;
}

int unzip_file_onefile (const char* file, const char* unzipfile ,  const char* dir) 
{

	if (!file || !dir)
	{
		info_debug("unzip_file_onefile  parameter error.\n");
		return -1;
	}
	

	if (access(file, F_OK) != 0)
	{
		info_debug("unzip_file_onefile: %s, %d: %s not exist!!!\n", __func__, __LINE__, file);
		return -1;
	}


	if (!dir_exist(dir)) 
	{
		info_debug("unzip_file_onefile: %s, %d: %s dir not exist!!!\n", __func__, __LINE__, dir);
		return -1;
	}

	pid_t pid;
	int status;

	pid = vfork();
	
	if (pid == 0) 
	{
		if (execl(BUSYBOX, "unzip", file,unzipfile, "-d", dir, NULL) < 0) 
		{
			perror("execl error:");
		}
	}
	else
	{
		waitpid(pid, &status, 0);
	}
	
	return 0;
}





int unzip_file_all (const char* file, const char* dir) 
{

	if (!file || !dir)
	{
		info_debug("unzip_file_all  parameter error.\n");
		return -1;
	}
	

	if (access(file, F_OK) != 0)
	{
		info_debug("unzip_file_all: %s, %d: %s not exist!!!\n", __func__, __LINE__, file);
		return -1;
	}


	if (!dir_exist(dir)) 
	{
		info_debug("unzip_file_all: %s, %d: %s dir not exist!!!\n", __func__, __LINE__, dir);
		return -1;
	}

	pid_t pid;
	int status;

	pid = vfork();
	
	if (pid == 0) 
	{
		if (execl(BUSYBOX, "unzip", file, "-d", dir, NULL) < 0) 
		{
			perror("execl error:");
		}
	}
	else
	{
		waitpid(pid, &status, 0);
	}
	
	return 0;
}




/****************************************
 ptcZipFile=/media/mmcblk0p2/book/EPUB2/Frankenstein_or_The_Modern_Prometheus.epub
 
ptcGetFilename=META-INF/container.xml

ptcExpandPath=/tmp/Work/Temporary/

******************************************/
static bool SdkFileUnzip(const TCHAR* ptcZipFile, const TCHAR* ptcGetFilename, const TCHAR* ptcExpandPath)
{
	bool blRet = false;
	
	char pMode[PATH_MAX];
	
	size_t	iLen,outleft;
	
	char filename[PATH_MAX];

	char savepath[PATH_MAX];
	
	char tempname[PATH_MAX];
	int iret=0;
	
	
	FILE *fp=0;
	 unsigned long   filesize=0;
	 
	info_debug("Enter SdkFileUnzip fun\n");
	
	memset(filename, 0, sizeof(filename));
	iLen = _tcslen(ptcZipFile)*sizeof(TCHAR);
	outleft = PATH_MAX;
	utf32_to_utf8(ptcZipFile, &iLen, filename, &outleft);
		
	info_debug("ptcZipFile=%s\n",filename);
	
//================================

	memset(tempname, 0, sizeof(tempname));
	iLen = _tcslen(ptcGetFilename)*sizeof(TCHAR);
	outleft = PATH_MAX;
	utf32_to_utf8(ptcGetFilename, &iLen, tempname, &outleft);
	
	info_debug("ptcGetFilename=%s\n",tempname);

//================================

	memset(savepath, 0, sizeof(savepath));
	iLen = _tcslen(ptcExpandPath)*sizeof(TCHAR);
	outleft = PATH_MAX;
	utf32_to_utf8(ptcExpandPath, &iLen, savepath, &outleft);
	
	info_debug("ptcExpandPath=%s\n",savepath);

   //iret =unzip_file_all(filename,savepath);

     iret =unzip_file_onefile(filename,tempname,savepath);
   
    info_debug("unzip_file return =%d\n",iret);

	if(iret==0)
	{
		blRet = true;
	
	}
	else
	{
		blRet = false;
	}

	return blRet;
	
}

static int SdkDirCreate(const TCHAR* ptcDirName)
{
	int		iRet;
	char	pDirname[PATH_MAX];
	size_t	iLen,outleft;

	memset(pDirname, 0, sizeof(pDirname));
	if(sizeof(TCHAR) > 1){
		//setlocale( LC_ALL, "Japanese" );   
		//wcstombs(pDirname, (const wchar_t*)ptcDirName, (iLen+1)*sizeof(TCHAR));
		iLen = _tcslen(ptcDirName)*sizeof(TCHAR);
		outleft = PATH_MAX;
		utf32_to_utf8(ptcDirName, &iLen, pDirname, &outleft);
	}else{
		strlcpy(pDirname, ptcDirName, PATH_MAX);
	}

	if(dir_exist(pDirname)){
		return 0;
	}

	info_debug("SdkDirCreate=%s\n",pDirname);
	iRet = mkdir(pDirname, 0x0FFF);
	info_debug("iRet=%d\n",iRet);
	return iRet;
}

static int SdkDirDelete(const TCHAR* ptcDirName)
{
	int		iRet;
	char	pDirname[PATH_MAX];
	size_t	iLen,outleft;

	memset(pDirname, 0, sizeof(pDirname));
	if(sizeof(TCHAR) > 1){
		//setlocale( LC_ALL, "Japanese" );   
		//wcstombs(pDirname, (const wchar_t*)ptcDirName, (iLen+1)*sizeof(TCHAR));
		iLen = _tcslen(ptcDirName)*sizeof(TCHAR);
		outleft = PATH_MAX;
		utf32_to_utf8(ptcDirName, &iLen, pDirname, &outleft);
	}else{
		strlcpy(pDirname, ptcDirName, PATH_MAX);
	}
	info_debug("SdkDirDelete=%s\n",pDirname);
	iRet = rmdir(pDirname);
	info_debug("iRet=%d\n",iRet);
	return iRet;
}

//==========================================================================================
//==========================================================================================
static int firstPhase( DaisyParse *thiz)
{
//	PlexResult result;
	
	return_val_if_fail(thiz != NULL, -1);
//	if(thiz->mode == BMODE_AUDIO_TEXT){
//		result = PlexDaisySetPlayModeType(PLEX_DAISY_PLAY_AUDIO_MODE);
//		if(result != PLEX_OK){
//			info_err("set play mode err\n");
//		}
//	}
//	result = PlexDaisyGetCurPhrase(thiz->pphraseInfo);
//	info_debug("result=%d\n", result);
//	while(result == PLEX_OK){
//		if(daisy_is_phrase_up(thiz) && thiz->isPhraseExist(thiz))
//		{
//			break;
//		}
//		result = PlexDaisyGetNextPhrase(thiz->pphraseInfo);
//	}
//
//	if(result != PLEX_OK){
//		info_err("no phase\n");
//		return -1;
//	}

//	info_debug("first phase end\n");
	PlexDaisySetHeadingLevel(1);
	
	return 0;
}


static int testDaisyMode( DaisyParse *thiz)
{
	PlexResult result;
	int mode;
	unsigned short get_mode;
	
	mode = 0;
	return_val_if_fail(thiz != NULL, -1);
	
	result = PlexDaisyGetPlayModeType(&get_mode);
	info_debug("get_mode=0x%x\n", get_mode);
	if(get_mode & PLEX_DAISY_PLAY_AUDIO_MODE){
		mode |= 0x01;
		info_debug("set mode 0x01\n");
	}
	else if(get_mode & PLEX_DAISY_PLAY_TEXT_MODE){
		mode |= 0x02;
		info_debug("set mode 0x02\n");
	}


	if(mode == 0x01){
		result = PlexDaisySetPlayModeType(PLEX_DAISY_PLAY_TEXT_MODE);
		if(result == PLEX_OK){
			mode |= 0x2;
		}
	}
	
	if(mode == 0x02){
		result = PlexDaisySetPlayModeType(PLEX_DAISY_PLAY_AUDIO_MODE);
		if(result == PLEX_OK){
			mode |= 0x1;
		}
	}
	
	if(mode == 0x1){
		thiz->mode = BMODE_AUDIO_ONLY;
		thiz->isPhraseExist = daisy_isAudioExist;
	}
	else if(mode == 0x2){
		thiz->mode = BMODE_TEXT_ONLY;
		thiz->isPhraseExist = daisy_isTextExist;
	}
	else if(mode == 0x3){
		thiz->mode = BMODE_AUDIO_TEXT;
		thiz->isPhraseExist = daisy_isAudioExist;
	}
	else{
		info_err("err mode\n");
		thiz->mode = BMODE_INVALID;
		return -1;
	}
	info_debug("mode=0x%x\n", thiz->mode);

	return 0;
}

/*
#define JU_HEADING_FUNC (1<<0)
#define JU_GROUP_FUNC (1<<1)
#define JU_PAGE_FUNC (1<<2)
#define JU_PHRASE_FUNC (1<<3)
#define JU_SCREEN_FUNC (1<<4)
#define JU_PARA_FUNC (1<<5)
#define JU_SEN_FUNC (1<<6)
#define JU_BOOKMARK_FUNC (1<<7)
*/

static int testDaisyFunc( DaisyParse *thiz)
{
	PlexResult result;
	unsigned long cur;
	unsigned long max;
	unsigned long last;
	unsigned long sp;
	unsigned long fr;
	unsigned long Tol;
	
	info_debug("test daisy function\n");
	
	return_val_if_fail(thiz != NULL, -1);
	
	thiz->funcs = 0;
	/*test heading*/
	result = PlexDaisyGetHeadingInfo(&cur, 
				&Tol);
	if(result == PLEX_OK){
		if(Tol > 0){
			thiz->funcs |= JU_HEADING_FUNC;
			info_debug("heading func\n");
		}
	}
	/*test group*/
	result = PlexDaisyGetGroupInfo(&Tol);
	if(result == PLEX_OK){
		if(Tol > 0){
			thiz->funcs |= JU_GROUP_FUNC;
			info_debug("group func\n");
		}
	}
	/*test page*/
	result = PlexDaisyGetPageInfo(&cur,
		&max,&last, &fr, &sp,&Tol);
	if(result == PLEX_OK){
		if(Tol > 0){
			thiz->funcs |= JU_PAGE_FUNC;
			info_debug("page func\n");
		}
	}
	
	/*test phrase*/
	thiz->funcs |= JU_PHRASE_FUNC;
	
	if(thiz->mode == BMODE_TEXT_ONLY){
		/*test screen*/
		thiz->funcs |= JU_SCREEN_FUNC;
		info_debug("screen func\n");
		/*test para*/
		thiz->funcs |= JU_PARA_FUNC;
		info_debug("para func\n");
		/*test sen*/
		thiz->funcs |= JU_SEN_FUNC;
		info_debug("sentence func\n");
	}
	else{
		thiz->funcs |= JU_30SEC_FUNC;
		thiz->funcs |= JU_10MIN_FUNC;
	}

	/*test bookmark*/
	thiz->funcs |= JU_BOOKMARK_FUNC;
	info_debug("bookmark func\n");
	
	return 0;
}

DaisyParse *daisy_open(const char *fname)
{
	PlexResult result;
	TCHAR tchPath[PATH_MAX];
	size_t out_ret;
	size_t in_ret;
	int ret;
	
	info_debug("daisy open\n");
	DaisyParse *thiz = (DaisyParse*)app_malloc(sizeof(DaisyParse));
	if(!thiz){
		goto faile;
	}
	memset(thiz, 0, sizeof(DaisyParse));

	strlcpy(thiz->daisy_file, fname, sizeof(thiz->daisy_file));
	// Initialize Daisy SDK
	thiz->fileAccess.FileOpen   = SdkFileOpen;
	thiz->fileAccess.FileRead   = SdkFileRead;
	thiz->fileAccess.FileWrite  = SdkFileWrite;
	thiz->fileAccess.FileSeek   = SdkFileSeek;
	thiz->fileAccess.FileSize   = SdkFileSize;
	thiz->fileAccess.FileClose  = SdkFileClose;
	thiz->fileAccess.FileDelete = SdkFileDelete;
	thiz->fileAccess.FileUnzip  = SdkFileUnzip;
	thiz->fileAccess.DirCreate  = SdkDirCreate;
	thiz->fileAccess.DirDelete  = SdkDirDelete;

	thiz->pphraseInfo = PlexDaisyCreatePhraseInfo();
	thiz->pphraseTitle = PlexDaisyCreatePhraseInfo();
	thiz->pphraseBookmark = PlexDaisyCreatePhraseInfo();
	thiz->pphraseLabel = PlexDaisyCreatePhraseString();

	info_debug("file=%s\n", thiz->daisy_file);
	memset(tchPath, 0, sizeof(tchPath));
	
	in_ret = strlen(WORK_DIR_PATH);
	out_ret = sizeof(tchPath);
	utf8_to_utf32((char*)WORK_DIR_PATH, &in_ret,
		(char*)tchPath, &out_ret);

	info_debug("init\n");
	result = PlexDaisyInitialize(&thiz->fileAccess, CODE_PAGE_UTF8, CODE_PAGE_GBK, tchPath, WORK_DIR_SIZE);
	if(result != PLEX_OK){
		goto faile;
	}
	{
		TCHAR ptcVersion[20];
		char version[20];
		PlexDaisyVersion(&ptcVersion);
		size_t iLen = _tcslen(ptcVersion)*sizeof(TCHAR);
		size_t outleft = sizeof(version);
		utf32_to_utf8(ptcVersion, &iLen, version, &outleft);
		info_debug("daisy version:%s\n",version);
	}
	memset(tchPath, 0, sizeof(tchPath));
	in_ret = strlen(thiz->daisy_file);
	out_ret = sizeof(tchPath);
	utf8_to_utf32((char*)thiz->daisy_file, &in_ret,
		(char*)tchPath, &out_ret);
	info_debug("first\n");
	result = PlexDaisyParseFirst(tchPath);
	info_debug("PlexDaisyParseFirst ret=%d\n", result);
	if(result != PLEX_OK){
		goto faile;
	}
	
	result = PlexDaisyParseSecond();
	info_debug("PlexDaisyParseSecond ret=%d\n", result);
	if(result != PLEX_OK){
		goto faile;
	}

	/*Test Play Mode*/
	ret = testDaisyMode(thiz);
	if(ret < 0){
		goto faile;
	}

	testDaisyFunc(thiz);
	
	info_debug("first phase\n");
	ret = firstPhase(thiz);
	if(ret < 0){
		info_err("no first phase\n");
		goto faile;
	}
	
	result = PlexDaisyGetTitleInfo(&thiz->titleInfo);
	if(result != PLEX_OK){
		info_err("get title info\n");
	}
	
	info_debug("ulDepth:%ld\n", thiz->titleInfo.ulDepth);
	info_debug("ulMaxPage:%ld\n", thiz->titleInfo.ulMaxPage);
	info_debug("ulPageNormal:%ld\n", thiz->titleInfo.ulPageNormal);
	info_debug("ulPageFront:%ld\n", thiz->titleInfo.ulPageFront);
	info_debug("ulPageSpecial:%ld\n", thiz->titleInfo.ulPageSpecial);
	info_debug("ulFileSize:%ld\n", thiz->titleInfo.ulFileSize);
	info_debug("ulTocItems:%ld\n", thiz->titleInfo.ulTocItems);
	info_debug("ulTotalSection:%ld\n", thiz->titleInfo.ulTotalSection);
	info_debug("ulTotalGroup:%ld\n", thiz->titleInfo.ulTotalGroup);
	info_debug("ulTotalTime:%ld\n", thiz->titleInfo.ulTotalTime);
	info_debug("ulScriptCnt:%ld\n", thiz->titleInfo.ulScriptCnt);
	info_debug("blPermitMove:%d\n", thiz->titleInfo.blPermitMove);
	info_debug("blMultimedia:%d\n", thiz->titleInfo.blMultimedia);
	info_debug("ulDepth:%ld\n", thiz->titleInfo.ulDepth);
	info_debug("createTime:%ld\n", thiz->titleInfo.createTime);

	thiz->mark = dsMark_create(thiz->daisy_file);	
	if(NULL == thiz->mark){
		info_err("create mark err\n");
	}
	return thiz;
faile:
	daisy_close(thiz);
	return NULL;
}

void daisy_close(DaisyParse *thiz)
{
	if(thiz){
		if(thiz->mark){
			struct DMark mark;
			memset(&mark, 0, sizeof(struct DMark));
			daisy_phrase2mark(thiz, &mark);
			dsMark_saveResumeMark(thiz->mark, &mark);
			dsMark_destroy(thiz->mark);
			thiz->mark = NULL;
		}
		
		PlexDaisyDeletePhraseInfo(thiz->pphraseTitle);
		PlexDaisyDeletePhraseInfo(thiz->pphraseInfo);
		PlexDaisyDeletePhraseInfo(thiz->pphraseBookmark);
		PlexDaisyDeletePhraseString(thiz->pphraseLabel);

		PlexDaisyParseEnd();
		/*engine*/
		PlexDaisyUninitialize();
		
		app_free(thiz);
		thiz = NULL;
		info_debug("daisy_close end\n");
	}
}


static int delete_Temporary(void)
{
	char cmd[PATH_MAX];
	snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", "/tmp/Work/Temporary/");
	info_debug("cmd=%s\n", cmd);
	system(cmd); 
	return 0;
}

PlexResult ConvertHtmlEpubToText(const char *filepath, char *outfile, const int bufsize)
{
	TCHAR tchPath[PATH_MAX];
	TCHAR workdir[PATH_MAX];
	size_t out_ret;
	size_t in_ret;
	stFileAccessCallback fileAccess;
	PlexResult result = PLEX_ERR;
	
	fileAccess.FileOpen   = SdkFileOpen;
	fileAccess.FileRead   = SdkFileRead;
	fileAccess.FileWrite  = SdkFileWrite;
	fileAccess.FileSeek   = SdkFileSeek;
	fileAccess.FileSize   = SdkFileSize;
	fileAccess.FileClose  = SdkFileClose;
	fileAccess.FileDelete = SdkFileDelete;
	fileAccess.FileUnzip  = SdkFileUnzip;
	fileAccess.DirCreate  = SdkDirCreate;
	fileAccess.DirDelete  = SdkDirDelete;

	info_debug("Enter ConvertHtmlEpubToText\n");

	if (!dir_exist(WORK_DIR_PATH)) 
	{//ц╩сп/tmp/Work/
		info_debug("tmp/Work   no exist %s, %d: work dir not exist!!!\n", __func__, __LINE__);

		if (mkdir(WORK_DIR_PATH, 0x0FFF) == -1) 
		{
			info_debug("creat tem.Work  error!\n");
		} 
		else
		{
			info_debug("creat tem.Work  sucess!\n");
		}
	}
	
	memset(workdir, 0, sizeof(workdir));
	
	in_ret = strlen(WORK_DIR_PATH);
	
	out_ret = sizeof(workdir);
	
	utf8_to_utf32((char*)WORK_DIR_PATH, &in_ret,
		(char*)workdir, &out_ret);

	info_debug("init\n");
	result = PlexDaisyInitialize(&fileAccess, CODE_PAGE_UTF8, CODE_PAGE_GBK, workdir, 5*1024*1024);
	if(result != PLEX_OK){
		goto faile;
	}
	
	info_debug("file=%s\n", filepath);
	memset(tchPath, 0, sizeof(tchPath));
	in_ret = strlen(filepath);
	out_ret = sizeof(tchPath);
	utf8_to_utf32((char*)filepath, &in_ret,
		(char*)tchPath, &out_ret);
	info_debug("first\n");
	
	// Parse Html/EPUB Content
	result = PlexDaisyConvertToText(tchPath, workdir);
	
	if(result == PLEX_OK)
	{
		info_debug("Convert Text from Html/EPUB : success...\n");
		delete_Temporary();
		
		char* filename = PlextalkGetFilenameFromPath(filepath);
		
		strlcpy(outfile, WORK_DIR_PATH, bufsize);
		
		PlextalkGetRealnameOfFilename(filename, tchPath, sizeof(tchPath));
		
		strlcat(outfile, tchPath, bufsize);
		
		strlcat(outfile, ".txt", bufsize);
		
	}
	else if(result == PLEX_ERR_UNZIP)
	{
		info_debug("Convert Text from Html/EPUB : unzip error...\n");
	}else if(result == PLEX_ERR_NO_WORK_AREA){
		info_debug("Convert Text from Html/EPUB : no work area error...\n");
	}else if(result == PLEX_ERR_NOT_ENOUGH_WORK_AREA){
		info_debug("Convert Text from Html/EPUB : work size error...\n");
	}else if(result == PLEX_ERR_CONVERT_CHARSET){
		info_debug("Convert Text from Html/EPUB : convert error...\n");
	}else{
		info_debug("Convert Text from Html/EPUB : error...\n");
	}
	PlexDaisyUninitialize();
faile:
	return result;
}

/*
#endif
*/

