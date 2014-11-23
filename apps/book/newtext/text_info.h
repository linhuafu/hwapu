#ifndef __TEXT_INFO_H_
#define __TEXT_INFO_H_

#include "text_parse.h"
//#define _(S)    gettext(S)
int txInfo_PagesInfo(TextParse *thiz, char *pagetstr, int maxlen);

int txInfo_BookmarkNumberInfo(TextParse *thiz, char *str, int maxlen);

int txInfo_TittleNameInfo(TextParse *thiz, char *str, int maxlen);

int txInfo_CreationDateInfo(TextParse *thiz, char *str, int maxlen);

int txInfo_PercentElapseInfo(TextParse *thiz, char *str, int maxlen, int isEnd);


#endif
