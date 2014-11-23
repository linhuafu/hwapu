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
#include "PlexDaisyAnalyzeAPI.h"
#include "convert.h"

#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#define OSD_DBG_MSG
#include "nc-err.h"

#include "Plextalk-i18n.h"
#include "daisy_parse.h"
#include "daisy_mark.h"
#include "convert.h"
#include "typedef.h"
#include "dbmalloc.h"
#include "libinfo.h"

int dsInfo_PagesInfo(DaisyParse *thiz, char *pagetstr, int maxlen)
{
	unsigned long pulCurPage,pulMaxPage,pulLastPage;
	unsigned long pulFrPage,pulSpPage,pulTotalPage;
	PlexResult ret;
	char *pageinfo = _("Page information");
	char *curpage = _("Current page");
	char numstr[10];
	
	return_val_if_fail(thiz != NULL, -1);

	strlcpy(pagetstr, pageinfo, maxlen);
	strlcat(pagetstr, "\n", maxlen);
	ret = PlexDaisyGetPageInfo(&pulCurPage, &pulMaxPage,&pulLastPage,
		&pulFrPage,&pulSpPage,&pulTotalPage);
	if(ret != PLEX_OK){
		info_err("get page error\n");
		strlcat(pagetstr, _("There is no page."), maxlen);
		return -1;
	}

	info_normal("pulCurPage=%lu\n",pulCurPage);
	info_normal("pulMaxPage=%lu\n",pulMaxPage);
	info_normal("pulLastPage=%lu\n",pulLastPage);
	info_normal("pulFrPage=%lu\n",pulFrPage);
	info_normal("pulSpPage=%lu\n",pulSpPage);
	info_normal("pulTotalPage=%lu\n",pulTotalPage);

	if(PLEX_DAISY_PAGE_NONE == pulCurPage){
		info_debug("none page\n");
		strlcat(pagetstr, _("Beginning of title."), maxlen);
	}else if(PLEX_DAISY_PAGE_FRONT == pulCurPage){
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, _("Front Page"), maxlen);
	}else if(PLEX_DAISY_PAGE_SPECIAL == pulCurPage){
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, _("Special Page"), maxlen);
	}else{
		snprintf(numstr, sizeof(numstr), "%s %lu", _("page"), pulCurPage);
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, numstr, maxlen);
	}

	strlcat(pagetstr, "\n", maxlen);
	snprintf(numstr, sizeof(numstr), "%lu", pulMaxPage);
	strlcat(pagetstr, _("Maximum number of pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	strlcat(pagetstr, "\n", maxlen);
	
	snprintf(numstr, sizeof(numstr), "%lu", pulFrPage);
	strlcat(pagetstr, _("Total number of front pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	strlcat(pagetstr, "\n", maxlen);
	
	snprintf(numstr, sizeof(numstr), "%lu", pulSpPage);
	strlcat(pagetstr, _("Total number of special pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	info_debug("page:%s\n",pagetstr);
	return 0;
}


int dsInfo_BookmarkNumberInfo(DaisyParse *thiz, char *str, int maxlen)
{
	return_val_if_fail(thiz != NULL, -1);
	int num = dsMark_getMarkNum(thiz->mark);
	snprintf(str, maxlen, "%s:%d",_("Total number of bookmarks"), num);
	info_debug("%s\n",str);
	return 0;
}

int dsInfo_TittleNameInfo(DaisyParse *thiz, char *str, int maxlen)
{
	size_t in_ret,out_ret;
	char *out_str;
	
	return_val_if_fail(thiz != NULL, -1);
	
	PlexResult ret = PlexDaisyGetTitleInfo(&thiz->titleInfo);
	if(ret != PLEX_OK){
		info_err("get title error\n");
		return -1;
	}

	in_ret = wcslen(thiz->titleInfo.tchTitle)*4;
	info_debug("in_ret:%d\n",in_ret);
	out_ret = in_ret;
	out_str = (char*)app_malloc(out_ret);
	if(NULL == out_str){
		str[0] = 0;
		return -1;
	}
	memset(out_str, 0, out_ret);
	utf32_to_utf8((char*)thiz->titleInfo.tchTitle,&in_ret,
		(char*)out_str, &out_ret);
	//info_debug("title:%s\n",out_str);
	snprintf(str, maxlen, "%s:%s",_("Current selected title name"), out_str);
	app_free(out_str);
	info_debug("%s\n",str);
	return 0;
}


int dsInfo_CreationDateInfo(DaisyParse *thiz, char *str, int maxlen)
{
	struct tm *tp;
	struct stat64 st;
	char timeinfo[20];

	return_val_if_fail(thiz != NULL, -1);
	
	if ( 0 != stat64(thiz->daisy_file, &st) ){
		info_err("get file error\n");
		return -1;
	}
		
	tp = localtime (&st.st_mtime);
	strftime(timeinfo, sizeof(timeinfo), "%F %T", tp);
	snprintf(str, maxlen, "%s: %s",_("Updated date"), timeinfo);
	info_debug("%s\n",str);
	return 0;
}

int dsInfo_ElapsedTimeInfo(DaisyParse *thiz, char *str, int maxlen)
{
	char elapseStr[50];
	char totalStr[50];
	char remainstr[50];
	char *elapse = _("Elapsed");
	char *total = _("Total");
	char *remaining = _("Remaining");
	
	unsigned long	ulSec, ulMin;
	unsigned long	remaintime = thiz->time.totaltime - thiz->time.curtime;
	
	return_val_if_fail(thiz != NULL, -1);
	ulSec = thiz->time.curtime/1000;
	ulMin = ulSec/60;
	snprintf(elapseStr, sizeof(elapseStr), "%s:\n%.2ld:%.2ld:%.2ld", elapse, ulMin/60, ulMin%60, ulSec%60);
	ulSec = thiz->time.totaltime/1000;
	ulMin = ulSec/60;
	snprintf(totalStr, sizeof(totalStr), "%s:\n%.2ld:%.2ld:%.2ld", total, ulMin/60, ulMin%60, ulSec%60);
	
	ulSec = remaintime/1000;
	ulMin = ulSec/60;
	snprintf(remainstr, sizeof(remainstr), "%s:\n%.2ld:%.2ld:%.2ld", remaining, ulMin/60, ulMin%60, ulSec%60);

	snprintf(str, maxlen, "%s\n%s\n%s",elapseStr, remainstr, totalStr);
	info_debug("%s\n",str);
	return 0;
}

int dsInfo_PercentElapseInfo(DaisyParse *thiz, char *str, int maxlen)
{
	unsigned long pulCurHeadingNo,pulTotalHeadingCnt;
	PlexResult ret;
	char numstr[20];
	
	ret = PlexDaisyGetHeadingInfo(&pulCurHeadingNo, &pulTotalHeadingCnt);
	if(ret != PLEX_OK){
		info_err("get heading error\n");
		return -1;
	}	
	snprintf(numstr, sizeof(numstr), " %ld ", (pulCurHeadingNo*100)/pulTotalHeadingCnt);
	strlcpy(str, _("Current playback position"), maxlen);
	strlcat(str, numstr, maxlen);
	strlcat(str, _("percent."), maxlen);
	return 1;
}

int dsInfo_HeadingInfo(DaisyParse *thiz, char *str, int strLen,char *path, int pathLen,
	struct vocinfo* voc_info, int *voc_num)
{
	unsigned long ulCurHeadingNo,ulTotalHeadingCnt;
	unsigned long begtime,endtime;
	PlexResult result;
	char numstr[20];
	char strText[256];
	int ret;

	*voc_num = 0;
	result = PlexDaisyGetHeadingInfo(&ulCurHeadingNo, &ulTotalHeadingCnt);
	if(result != PLEX_OK){
		info_err("get heading error\n");
		return -1;
	}
	snprintf(numstr, sizeof(numstr), "%ld", ulCurHeadingNo);
	strlcpy(str, _("Current heading"), strLen);
	strlcat(str, " : ", strLen);
	strlcat(str, numstr, strLen);
	strlcat(str, "\n", strLen);

	snprintf(numstr, sizeof(numstr), "%ld", ulTotalHeadingCnt);
	strlcat(str, _("Total number of headings"), strLen);
	strlcat(str, " : ", strLen);
	strlcat(str, numstr, strLen);
	strlcat(str, "\n", strLen);

	ret = daisy_getHeadingPhrase(thiz,ulCurHeadingNo,strText,sizeof(strText),
		path, pathLen ,&begtime,&endtime);
	DBGMSG("ret:%d\n",ret);
	if( -1 == ret ){
		return 0;
	}
	DBGMSG("ret:%d\n",ret);
	if(BMODE_TEXT_ONLY == thiz->mode){
		strlcat(str, strText, strLen);
		return;
	}
	DBGMSG("path:%s\n",path);
	DBGMSG("begtime:%d endtime:%d\n",begtime,endtime);
	if(0 != strlen(path)){
		voc_info->btext = 1;
		voc_info->vocfile = path;
		voc_info->start = begtime;
		voc_info->len = endtime - begtime;
		if((voc_info->len) > 3000){
			voc_info->len = 3000;
		}
		*voc_num = 1;
	}
	return 1;
}

