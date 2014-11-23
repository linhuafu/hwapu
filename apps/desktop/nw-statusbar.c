/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Form StatusBar routines.
 *
 * REVISION:
 *
 * 2) new struct. ----------------------------------------- 2006-06-01 EY
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nxutils.h"
#include "plextalk-config.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"
#include "nw-statusbar.h"
#include "plextalk-statusbar.h"
#include "theme-defines.h"
#include "nw-main.h"
#define MWINCLUDECOLORS
#include <nano-X.h>
#include "mwtypes.h"
#include "device.h"


static const char* category_icons[] = {
	THEME_ICON_CATEGORY_BOOK,
	THEME_ICON_CATEGORY_BOOK_SELECT,
	THEME_ICON_CATEGORY_BOOK_DAISY,
	THEME_ICON_CATEGORY_BOOK_TEXT,
	THEME_ICON_CATEGORY_MUSIC,
	THEME_ICON_CATEGORY_RADIO,
	THEME_ICON_CATEGORY_MENU,
	THEME_ICON_CATEGORY_INFO,
	THEME_ICON_CATEGORY_CALC,
	THEME_ICON_CATEGORY_TIMER,
	THEME_ICON_CATEGORY_RECODER,
	THEME_ICON_CATEGORY_BACKUP,
};

static const char* condition_icons[] = {
	THEME_ICON_CONDITION_PLAY,
	THEME_ICON_CONDITION_PAUSE,
	THEME_ICON_CONDITION_STOP,
	THEME_ICON_CONDITION_SELECT,
	THEME_ICON_CONDITION_SEARCH,
	THEME_ICON_CONDITION_RECORD,
	THEME_ICON_CONDITION_RECORD,//THEME_ICON_CONDITION_RECORD_BLINK,
};

static const char* media_icons[] = {
	THEME_ICON_MEDIA_TYPE_INTERNAL,
	THEME_ICON_MEDIA_TYPE_SD,
	THEME_ICON_MEDIA_TYPE_USB,
};

static const char* volume_icons[] = {
	THEME_ICON_VOL0,
	THEME_ICON_VOL1,
	THEME_ICON_VOL2,
	THEME_ICON_VOL3,
	THEME_ICON_VOL4,
	THEME_ICON_VOL5,
	THEME_ICON_VOL6,
	THEME_ICON_VOL7,
};

static const char* battery_icons[] = {
	THEME_ICON_BATTERY_LEVEL0,
	THEME_ICON_BATTERY_LEVEL1,
	THEME_ICON_BATTERY_LEVEL2,
	THEME_ICON_BATTERY_LEVEL3,
	THEME_ICON_BATTERY_LEVEL4,
	THEME_ICON_BATTERY_CHARGING,
	THEME_ICON_BATTERY_ACIN,
};

static const char* timer_icons[] = {
	THEME_ICON_TIMER1,
	THEME_ICON_TIMER2,
	THEME_ICON_TIMER12,
};

static const char* lock_icons[] = {
	THEME_ICON_KEY_LOCK,
};

static const char* ampm_icons[] = {//lhf start
	THEME_ICON_AM,
	THEME_ICON_PM,
};//lhf end

#define STATUSBAR_ICON_WIDTH	16//app must put before
#define STATUSBAR_FONT_HEIGHT	16

#define IMAGE_FILE_PATH_LEN	512


static struct {
	PICTURE *picture;
	int icon_id;
	const char* const * icons;
	int icon_nr;
	int icon_w;
} pictures[8] = {//modify by lhf
	{ .icons = category_icons,  .icon_nr = ARRAY_SIZE(category_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = condition_icons, .icon_nr = ARRAY_SIZE(condition_icons),.icon_w=STATUSBAR_ICON_WIDTH },
	{ .icons = media_icons,     .icon_nr = ARRAY_SIZE(media_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = volume_icons,    .icon_nr = ARRAY_SIZE(volume_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = battery_icons,   .icon_nr = ARRAY_SIZE(battery_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = timer_icons,     .icon_nr = ARRAY_SIZE(timer_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = lock_icons,      .icon_nr = ARRAY_SIZE(lock_icons), .icon_w=STATUSBAR_ICON_WIDTH},
	{ .icons = ampm_icons,		.icon_nr = ARRAY_SIZE(ampm_icons), .icon_w=8},//add lhf
};


static TIMER *condition_blink_timer;
static int condition_blink_stat;

//added by ztz
static GR_WINDOW_ID clock_pix;
static LABEL *clock_label;


#define STATU_BAR_COUNT	8
static GR_WINDOW_ID status_bar_pixwid[STATU_BAR_COUNT] ;
static GR_IMAGE_ID	status_img_id[STATU_BAR_COUNT];
static int clock_label_posx = 0;


static void
CreatePixStatusBar (void)
{
	int i;
	for (i = 0; i < STATU_BAR_COUNT; i++)
		status_bar_pixwid[i] = GrNewPixmap(pictures[i].icon_w, STATUSBAR_HEIGHT, NULL);
}

// note for Destroy status Bar
static void
DestroyPixStatusBar (void)
{
	int i;

	for (i = 0; i < STATU_BAR_COUNT; i++)
		GrDestroyWindow(status_bar_pixwid[i]);
}


static void
LoadStatusBarPictureToPix (void)
{
	int i;
	char file[PATH_MAX];

	for (i = 0; i < STATU_BAR_COUNT; i++) {
		GrSetGCForeground(NeuxWidgetGC(pictures[i].picture), theme_cache.background);
		GrSetGCBackground(NeuxWidgetGC(pictures[i].picture), theme_cache.background);
		GrFillRect(status_bar_pixwid[i], NeuxWidgetGC(pictures[i].picture), 
			0, 0, pictures[i].icon_w, STATUSBAR_HEIGHT);
		if (pictures[i].icon_id > 0) {
			NeuxThemeGetFile((pictures[i].icons[pictures[i].icon_id - 1]), file, PATH_MAX);
			status_img_id[i] = GrLoadImageFromFile(file, 0);
			GrSetGCForeground(NeuxWidgetGC(pictures[i].picture), theme_cache.foreground);
			GrDrawImageToFit(status_bar_pixwid[i], NeuxWidgetGC(pictures[i].picture), 
				0, 0, pictures[i].icon_w, STATUSBAR_HEIGHT, status_img_id[i]);
		}
	}
}


static void
CopyPixStatusBar (void) 
{
	int i;

	for (i = 0; i < STATU_BAR_COUNT; i++) {
		if (pictures[i].icon_id <= 0)
			GrMapWindow(NeuxWidgetWID(pictures[i].picture));

		GrSetGCForeground(NeuxWidgetGC(pictures[i].picture), theme_cache.background);
		GrSetGCBackground(NeuxWidgetGC(pictures[i].picture), theme_cache.foreground);

		GrCopyArea(NeuxWidgetWID(pictures[i].picture), NeuxWidgetGC(pictures[i].picture), 
			0, 0, pictures[i].icon_w, STATUSBAR_HEIGHT, status_bar_pixwid[i], 0, 0, 0);			
	} 
}


//add by lhf
static int hour24To12(int hour,int *ampm)
{
	 *ampm = (hour>=12);
	 if (hour == 0){
	 	hour = 12;
	 } else if(hour > 12) {
		 hour -= 12;
	 }
	 return hour;
}
//add by lhf end


#define TIME_STR_LEN	10
static const char*
GetSysTimeStr (sys_time_t *stm, int hour_system, int* ampm)
{	
	static char timestr[TIME_STR_LEN];
	
	if (hour_system)		//24hour {
		snprintf(timestr, TIME_STR_LEN, "%d:%02d", stm->hour, stm->min);
	else {				
		int hour;
		hour = hour24To12(stm->hour, ampm);
		
		snprintf(timestr, TIME_STR_LEN, "%d:%02d", hour, stm->min);
	}
	
	return timestr;
}


//check if time (unit - minute) has changed
static char time_text_old[TIME_STR_LEN];

/* refresh clock label with Grtext */
int
StatusBarRereshTime (sys_time_t *stm)
{
	DBGMSG("Refresh clock label called!\n");

	widget_prop_t wprop;
	int retwidth, retheight, retbase;
	int captionx;
	int hour_system ;
	int ampm;
	int ret = 0;
	const char* time_text_new;
	GR_GC_ID gc;
	GR_WINDOW_ID wid;

	/* Set property (reserved) */
	NeuxWidgetGetProp(clock_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(clock_label, &wprop);
	
	NeuxThemeInit(desktop_theme_index);	

	gc = NeuxWidgetGC(clock_label);
	wid = NeuxWidgetWID(clock_label);

	/* Get hour system & time string */
	CoolShmReadLock(g_config_lock);
	hour_system = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);

	time_text_new = GetSysTimeStr(stm, hour_system, &ampm);
	
	/* strlen to check if time_text_old has been inited */
	if (strlen(time_text_old) && strncmp(time_text_old, time_text_new, TIME_STR_LEN))
		ret = 1;
	
	snprintf(time_text_old, TIME_STR_LEN, "%s", time_text_new);

	/* Fill widow with background */
	GrSetGCForeground(gc, theme_cache.background);
	GrSetGCBackground(gc, theme_cache.background);
	GrFillRect(wid, gc, 0, 0, STATUSBAR_WIDTH - clock_label_posx, STATUSBAR_FONT_HEIGHT);

	/* Recover gc foreground */
	GrSetGCForeground(gc, theme_cache.foreground);

	/* Get text size */
	GrGetGCTextSize(gc, time_text_new, strlen(time_text_new), MWTF_UTF8, &retwidth, &retheight, &retbase);
	captionx = STATUSBAR_WIDTH - clock_label_posx - retwidth;

	GrText(wid, gc, captionx, 0, time_text_new, strlen(time_text_new), MWTF_UTF8 | GR_TFTOP);

	/* Set ampm icon */
	if (hour_system)
		StatusBarSetIcon(STATUSBAR_AMPM_INDEX, SBAR_AMPM_ICON_NO);
	else
		StatusBarSetIcon(STATUSBAR_AMPM_INDEX, SBAR_AMPM_ICON_AM + ampm);

	return ret;
}


static void
OnTimerBlinkTimer(WID__ wid)
{
	DBGMSG("Timer Blink...\n");

	int icon_id = pictures[STATUSBAR_CONDITION_INDEX].icon_id;
	GR_IMAGE_ID img;
	GR_GC_ID gc ;
	GR_WINDOW_ID pwid;

	if (icon_id != SBAR_CONDITION_ICON_RECORD_BLINK) {
		DBGMSG("icon_id != SBAR_CONDITION_ICON_RECORD_BLINK, returned!\n");
		return;
	}
	condition_blink_stat ^= 1;

	gc = NeuxWidgetGC(pictures[STATUSBAR_CONDITION_INDEX].picture);
	pwid = NeuxWidgetWID(pictures[STATUSBAR_CONDITION_INDEX].picture);

	/* Fill rect with backgound */
	GrSetGCForeground(gc, theme_cache.background);
	GrSetGCBackground(gc, theme_cache.background);
	GrFillRect(pwid, gc, 0, 0, pictures[STATUSBAR_CONDITION_INDEX].icon_w, STATUSBAR_HEIGHT);

	if (!condition_blink_stat) {
		char file[IMAGE_FILE_PATH_LEN];

		GrSetGCForeground(gc, theme_cache.foreground);
		NeuxThemeGetFile((pictures[STATUSBAR_CONDITION_INDEX].icons[icon_id - 1]), file, IMAGE_FILE_PATH_LEN);
		DBGMSG("Get Image File: %s\n", file);
		img = GrLoadImageFromFile(file, 0);
		GrDrawImageToFit(pwid, gc, 0, 0, pictures[STATUSBAR_CONDITION_INDEX].icon_w, STATUSBAR_HEIGHT, img);
	}		
}


#define CLOCK_TEXT_FONT	"times-bold.ttf"

static void
CreateClockLabel (FORM *widget)
{
	widget_prop_t wprop;
	label_prop_t lprop;
	sys_time_t stm;

	/* Create clock label and pixmap */
	clock_label = NeuxLabelNew(widget,
						clock_label_posx,
						STATUSBAR_TOP + (STATUSBAR_HEIGHT - STATUSBAR_FONT_HEIGHT) / 2,
						STATUSBAR_WIDTH - clock_label_posx,
						STATUSBAR_FONT_HEIGHT, NULL);
	clock_pix = GrNewPixmap(STATUSBAR_WIDTH - clock_label_posx,
						STATUSBAR_FONT_HEIGHT, NULL);

	/* Set gc color */
	NeuxWidgetGetProp(clock_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(clock_label, &wprop);

	/* Set text location & font */
	NeuxLabelGetProp(clock_label, &lprop);
	lprop.align = LA_RIGHT;			
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(clock_label, &lprop);
	NeuxWidgetSetFont(clock_label, CLOCK_TEXT_FONT, STATUSBAR_FONT_HEIGHT);

	/* Show (map window!) */
	NeuxWidgetShow(clock_label, TRUE);

	/* Show Time immediately */
	sys_get_time(&stm);
	StatusBarRereshTime(&stm);
}


void 
CreateStatusBar (FORM *widget)
{
	DBGMSG("Desktop create status bar called!\n");
	
	widget_prop_t wprop;
	label_prop_t lprop;
	sys_time_t stm;
	int i,x,y;

	/* Create Picture widget */
	x = STATUSBAR_LEFT;
	for (i = 0; i < ARRAY_SIZE(pictures); i++) {
		pictures[i].picture = NeuxPictureNew(widget, x, STATUSBAR_TOP,
									pictures[i].icon_w, STATUSBAR_HEIGHT, 0, NULL);
		x += pictures[i].icon_w;
		NeuxWidgetGetProp(pictures[i].picture, &wprop);
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(pictures[i].picture, &wprop);

		NeuxWidgetShow(pictures[i].picture, TRUE);
	}
	CreatePixStatusBar();

	/* Create Clock widget */
	clock_label_posx = x ;
	CreateClockLabel(widget);

	/* Set timer for condition blink */
	condition_blink_stat = 0;
	condition_blink_timer = NeuxTimerNew(widget, 500, -1);
	NeuxTimerSetEnabled(condition_blink_timer, GR_FALSE);
	NeuxTimerSetCallback(condition_blink_timer, OnTimerBlinkTimer);
}


/* Set Icon now with pix_map and copyerea */
void
StatusBarSetIcon(int index, int icon_id)
{
	DBGMSG("StatusBarSetIcon called!\n");
	DBGMSG("desktop_them_index = %d\n", desktop_theme_index);
	DBGMSG("[Attention]: this call, index = %d, icon_id = %d\n", index, icon_id);
	
	widget_prop_t wprop;
	NeuxThemeInit(desktop_theme_index);		
	
	if (index >= ARRAY_SIZE(pictures)|| (icon_id > 0 && icon_id - 1 >= pictures[index].icon_nr)) {//modify by lhf 
		DBGMSG("Condition not fit, returned!\n");
		return;
	}

	if (icon_id <= 0) {		

		/* Set property (reserved) */
		NeuxWidgetGetProp(pictures[index].picture, &wprop);
		wprop.fgcolor = theme_cache.background;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(pictures[index].picture, &wprop);

		/* Fill window with background */
		GrClearWindow(NeuxWidgetWID(pictures[index].picture), GR_FALSE);
		GrFillRect(NeuxWidgetWID(pictures[index].picture), NeuxWidgetGC(pictures[index].picture), 
					0, 0, pictures[index].icon_w, STATUSBAR_HEIGHT);

		if (index == STATUSBAR_CONDITION_INDEX)
			NeuxTimerSetEnabled(condition_blink_timer, GR_FALSE);

	} else {
		GR_GC_ID gc = GrNewGC();		
		char file[IMAGE_FILE_PATH_LEN];

		/* Set property (reserved) */
		NeuxWidgetGetProp(pictures[index].picture, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(pictures[index].picture, &wprop);

		/* If condition index , control timer */
		if (index == STATUSBAR_CONDITION_INDEX) {
			if (icon_id == SBAR_CONDITION_ICON_RECORD_BLINK) {
				condition_blink_stat = 0;
				NeuxTimerSetEnabled(condition_blink_timer, GR_TRUE);
			} else if (pictures[index].icon_id == SBAR_CONDITION_ICON_RECORD_BLINK)
				NeuxTimerSetEnabled(condition_blink_timer, GR_FALSE);
		}

		/* Fill old with background */
		GrSetGCForeground(gc, theme_cache.background);
		GrSetGCBackground(gc, theme_cache.background);
		GrFillRect(status_bar_pixwid[index], gc, 0, 0, pictures[index].icon_w, STATUSBAR_HEIGHT);

		/* Recover foreground after */
		GrSetGCForeground(gc, theme_cache.foreground);

		/* Get Image file path & LoadImageFile */
		NeuxThemeGetFile((pictures[index].icons[icon_id - 1]), file, 512);
		DBGMSG("After Get Image File, file = %s\n", file);
		status_img_id[index] = GrLoadImageFromFile(file, 0);		

		/* Draw Image and fit to window */
		GrDrawImageToFit(status_bar_pixwid[index], gc, 0, 0, pictures[index].icon_w, 
			STATUSBAR_HEIGHT, status_img_id[index]);
		GrCopyArea(NeuxWidgetWID(pictures[index].picture), gc, 
			0, 0, pictures[index].icon_w, STATUSBAR_HEIGHT, status_bar_pixwid[index], 0, 0, 0);
		GrDestroyGC(gc);

		if (index == STATUSBAR_CATEGORY_INDEX) {	//zzx added
			CoolShmWriteLock(g_config_lock);
			g_config->statusbar_category_icon = icon_id;
			CoolShmWriteUnlock(g_config_lock);
		}											//zzx
	}
	
	pictures[index].icon_id = icon_id;
}


void
RefreshStatusBar(int confirm)
{
	DBGMSG("Refresh status Bar called, confirm = %d !\n", confirm);
	
	widget_prop_t wprop;
	label_prop_t lprop;
	sys_time_t stm;
	int i;

	if (confirm) {
		for (i = 0; i < ARRAY_SIZE(pictures); i++) {

			/* Set property (reserved)*/
			NeuxWidgetGetProp(pictures[i].picture, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(pictures[i].picture, &wprop);

			/* Just reset the icon */
			StatusBarSetIcon(i, pictures[i].icon_id );
		}
	} else {
		LoadStatusBarPictureToPix();
		CopyPixStatusBar();
	}	
	
	sys_get_time(&stm);
	StatusBarRereshTime(&stm);
}
