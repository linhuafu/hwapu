#include <string.h>
#define MWINCLUDECOLORS
#include <nano-X.h>
#include "plextalk-config.h"
#include "plextalk-theme.h"
#include "widgets.h"

struct plextalk_theme theme_cache;
int theme_index;

static const char * const plextalk_theme_dirs[PLEXTALK_THEME_COUNT] = {
	"white_black/",
	"black_white/",
	"black_yellow/",
	"yellow_black/",
	"yellow_blue/",
	"blue_yellow/",
	"blue_white/",
	"white_blue/",
	"orange_black/",
	"black_orange/",
	"red_white/",
	"white_red/",
	"green_black/",
	"black_green/",
	"purple_white/",
	"white_purple/",
};

static const struct {
	GR_COLOR foreground;
	GR_COLOR background;
	GR_COLOR selected;
} plextalk_theme_colors[PLEXTALK_THEME_COUNT] = {
	{WHITE, BLACK, GRAY},
	{BLACK, WHITE, GRAY},
	{BLACK, YELLOW, /*BLUE*/GRAY},
	{YELLOW, BLACK, /*WHITE*/GRAY},
	{YELLOW, BLUE, /*ORANGE*/GRAY},
	{/*BLUE*/MWRGB( 0, 0, 255 ), YELLOW, /*BLACK*/GRAY},
	{/*BLUE*/MWRGB( 0, 0, 255 ), WHITE, /*BLACK*/GRAY},
	{WHITE, /*BLUE*/MWRGB( 0, 0, 255 ), /*YELLOW*/GRAY},
	{/*ORANGE*/MWRGB( 255, 153, 0 ), BLACK, /*YELLOW*/GRAY},
	{BLACK, /*ORANGE*/MWRGB( 255, 153, 0 ), /*BLUE*/GRAY},
	{/*RED*/MWRGB( 255, 0, 0 ), WHITE, /*BLUE*/GRAY},
	{WHITE, /*RED*/MWRGB( 255, 0, 0 ), /*YELLOW*/GRAY},
	{/*GREEN*/MWRGB( 0, 255, 0 ), BLACK, /*BLUE*/GRAY},
	{BLACK, /*GREEN*/MWRGB( 0, 255, 0 ), /*BLUE*/GRAY},
	{PURPLE, WHITE, /*BLUE*/GRAY},
	{WHITE, PURPLE, /*YELLOW*/GRAY},
};

void NeuxThemeInit(int index)
{
	if (index >= PLEXTALK_THEME_COUNT)
		index = 0;
	theme_index = index;

	strlcpy(theme_cache.path, PLEXTALK_THEMES_DIR, sizeof(theme_cache.path));
	strlcat(theme_cache.path, plextalk_theme_dirs[index], sizeof(theme_cache.path));

	theme_cache.background = plextalk_theme_colors[index].background;
	theme_cache.foreground = plextalk_theme_colors[index].foreground;
	theme_cache.selected   = plextalk_theme_colors[index].selected;
}

void NeuxThemeGetFile(const char *themeProperty,
                     char *pValue, int iLen)
{
	strlcpy(pValue, theme_cache.path, iLen);
	strlcat(pValue, themeProperty, iLen);
}

GR_BOOL NeuxThemeSetBgPixmap(WIDGET *owner, const char *themeElement)
{
	char file[PATH_MAX];

	NeuxThemeGetFile(themeElement, file, sizeof(file));
	return NeuxWidgetSetBgPixmap(owner, file);
}

PICTURE *NeuxThemeIconNew(WIDGET *owner,
			  int x, int y, int w, int h,
			  const char *themeElement)
{
	char file[PATH_MAX];

	if (themeElement == NULL)
		return NeuxPictureNew(owner, x, y, w, h, PT_FILE, NULL);

	NeuxThemeGetFile(themeElement, file, sizeof(file));
	return NeuxPictureNew(owner, x, y, w, h, PT_FILE, file);
}

int NeuxThemeIconSetPic(PICTURE *picture, const char *themeElement)
{
	char file[PATH_MAX];

	if (!themeElement)
		return -1;

	NeuxThemeGetFile(themeElement, file, sizeof(file));
	return NeuxPictureSetPic(picture, PT_FILE, file);
}

IMAGEID NeuxThemeLoadImage(const char *themeElement)
{
	char file[PATH_MAX];

	if (!themeElement)
		return 0;

	NeuxThemeGetFile(themeElement, file, sizeof(file));
	return NeuxLoadImageFromFile(file);
}
