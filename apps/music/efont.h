#ifndef __EFONT_H__
#define __EFONT_H__

#define MWINCLUDECOLORS
#include <microwin/nano-X.h>

#define EFONT_COLOR_GRAY		0x000f0f0f
#define EFONT_COLOR_BLACK		0x00000000

#define GBK_HZK_FONT   "HZKFONT.GBK"
//#define TTF_STSONG "stsong.ttf"
#define TTF_STSONG "ARIALUNI.TTF"//"unifont.ttf"//"ARIALUNI.TTF"//"stsong.ttf"
#define TTF_SIMFANG "simfang.ttf"

struct EFont
{
	GR_GC_ID		gc;
	GR_FONT_ID 		id;
	GR_SIZE			height;
	GR_TEXTFLAGS 	flag;
	unsigned long		color;
};

struct EFont  *efont_create(char *name, GR_SIZE height);

/*
GR_SIZE efont_get_height(struct EFont *thiz);

*/

GR_GC_ID efont_get_gc(struct EFont *thiz);

GR_TEXTFLAGS efont_get_flags(struct EFont *thiz);

void efont_destroy(struct EFont *thiz);

#endif