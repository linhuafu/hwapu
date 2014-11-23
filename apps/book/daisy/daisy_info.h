#ifndef __DAISY_INFO_H_
#define __DAISY_INFO_H_

#include "daisy_parse.h"
//#define _(S)    gettext(S)
int dsInfo_PagesInfo(DaisyParse *thiz, char *pagetstr, int maxlen);

int dsInfo_BookmarkNumberInfo(DaisyParse *thiz, char *str, int maxlen);

int dsInfo_TittleNameInfo(DaisyParse *thiz, char *str, int maxlen);

int dsInfo_CreationDateInfo(DaisyParse *thiz, char *str, int maxlen);

int dsInfo_ElapsedTimeInfo(DaisyParse *thiz, char *str, int maxlen);

int dsInfo_PercentElapseInfo(DaisyParse *thiz, char *str, int maxlen);

int dsInfo_HeadingInfo(DaisyParse *thiz, char *str, int strLen,char *path, int pathLen,
	struct vocinfo* voc_info, int *voc_num);

#endif
