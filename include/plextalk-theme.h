#ifndef __PLEXTALK_THEME_H__
#define __PLEXTALK_THEME_H__

#include <limits.h>
#include "nano-X.h"
#include "widgets.h"

enum PLEXTALK_THEME {
	PLEXTALK_THEME_WHITE_BLACK,
	PLEXTALK_THEME_BLACK_WHITE,
	PLEXTALK_THEME_BLACK_YELLOW,
	PLEXTALK_THEME_YELLOW_BLACK,
	PLEXTALK_THEME_YELLOW_BLUE,
	PLEXTALK_THEME_BLUE_YELLOW,
	PLEXTALK_THEME_BLUE_WHITE,
	PLEXTALK_THEME_WHITE_BLUE,
	PLEXTALK_THEME_ORANGE_BLACK,
	PLEXTALK_THEME_BLACK_ORANGE,
	PLEXTALK_THEME_RED_WHITE,
	PLEXTALK_THEME_WHITE_RED,
	PLEXTALK_THEME_GREEN_BLACK,
	PLEXTALK_THEME_BLACK_GREEN,
	PLEXTALK_THEME_PURPLE_WHITE,
	PLEXTALK_THEME_WHITE_PURPLE,
	/* */
	PLEXTALK_THEME_COUNT,
};

struct plextalk_theme {
	char path[PATH_MAX];
	GR_COLOR background;
	GR_COLOR foreground;
	GR_COLOR selected;
};

extern struct plextalk_theme theme_cache;
extern int theme_index;

void NeuxThemeInit(int index);

void NeuxThemeGetFile(const char *themeProperty,
                     char *pValue, int iLen);

GR_BOOL NeuxThemeSetBgPixmap(WIDGET *owner, const char *themeElement);

PICTURE *NeuxThemeIconNew(WIDGET *owner,
			  int x, int y, int w, int h,
			  const char *themeElement);

int NeuxThemeIconSetPic(PICTURE *picture, const char *themeElement);

IMAGEID NeuxThemeLoadImage(const char *themeElement);

#endif
