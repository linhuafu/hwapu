/*
 *  Copyright(C) 2006-2007 Neuros Technology International LLC.
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
 * Widget Application routines.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"

#include "nx-widgets.h"
#include "widgets.h"
#include "wm.h"
#include "shared-mem.h"
#include "plextalk-keys.h"

#define DEFAULT_SAVER_TIME 300 /* 5minutes */
static GR_BOOL  bScrSaverEnabled;
static GR_BOOL  bScrSaverPrevEnabled;
static NX_WIDGET * AppWidget;

#define FONT_NAME_LEN		32
#define APP_FONT_TAB_MAX	8

typedef struct
{
	GR_FONT_ID fontid;
	int        fontheight;

	// fontname and fontsize dictate font ID.
	char       fontname[FONT_NAME_LEN];
	GR_SIZE    fontsize;

	int refcount;
} app_font_t;

static app_font_t app_font_tab[APP_FONT_TAB_MAX];
static int app_font_count;

static int key_lock;
static int key_lock_inited;

/** Intializes application widget.
 *
 * @param wm
 *        1 to indicate this is an WM widget, 0 otherwise.
 * @return
 *        widget pointer if successfully created new application widget,
 *        otherwise NULL.
 */
NX_WIDGET *
NxAppInitialize (int is_wm)
{
	if (GrOpen() < 0)
	{
        ERRLOG("Can not open graphics!\n");
		return NULL;
	}

	AppWidget = (NX_WIDGET *)malloc(sizeof(NX_WIDGET));
	memset(AppWidget, 0, sizeof(NX_WIDGET));
	AppWidget->base.realized = GR_TRUE;

	DBGLOG("root window id = %d\n", GR_ROOT_WINDOW_ID);
	AppWidget->base.wid = GR_ROOT_WINDOW_ID;
	AppWidget->base.type = WIDGET_TYPE_APPLICATION;
	AppWidget->base.gc = GrNewGC();

	AppWidget->spec.application.cb_screensaver.fp		= NULL;
	AppWidget->spec.application.cb_screensaver.inherit	= GR_TRUE;
	AppWidget->spec.application.cb_idle.fp				= NULL;
	AppWidget->spec.application.cb_idle.inherit			= GR_TRUE;
	AppWidget->spec.application.cb_msg.fp				= NULL;
	AppWidget->spec.application.cb_msg.inherit			= GR_TRUE;
	AppWidget->spec.application.cb_destroy.fp			= NULL;
	AppWidget->spec.application.cb_destroy.inherit		= GR_TRUE;
	AppWidget->spec.application.cb_sw_on.fp				= NULL;
	AppWidget->spec.application.cb_sw_on.inherit		= GR_TRUE;
	AppWidget->spec.application.cb_sw_off.fp			= NULL;
	AppWidget->spec.application.cb_sw_off.inherit		= GR_TRUE;

	GrGetScreenInfo(&AppWidget->spec.application.screen);
	AppWidget->spec.application.iswm = is_wm;

	AppWidget->spec.application.ewid = GrNewInputWindow(GR_ROOT_WINDOW_ID, 1, 1, 1, 1);
	GrSelectEvents(AppWidget->spec.application.ewid,
						GR_EVENT_MASK_MSG | GR_EVENT_MASK_FD_ACTIVITY |
						GR_EVENT_MASK_SW_ON | GR_EVENT_MASK_SW_OFF);

	DBGLOG("appp event window id = %d\n", AppWidget->spec.application.ewid);
	if (is_wm)
	{
		AppWidget->spec.application.isssaveractive = GR_FALSE;
		AppWidget->spec.application.iScrSaverTimeout = DEFAULT_SAVER_TIME;
		GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_SCREENSAVER);
	}

	//GrSetScreenSaverTimeout(2);
	NxAddToRegistry (AppWidget);
	DBGLOG("root window id = %d\n", AppWidget->base.wid);

	return AppWidget;
}

void
NxAppDestroy(NX_WIDGET *widget)
{
	NX_APPLICATION *app = &widget->spec.application;
	if (app->cb_destroy.fp != NULL)
		(*(app->cb_destroy.fp))(widget, app->cb_destroy.dptr);
	NxDeleteFromRegistry(widget);
}

/* This could be a hashtable instead of a linked list;
   but then again we are usually only having very few entries */
struct source {
	struct source *next;
	int fd;
	void *data;
	void (*cb_read)(void *);
	void (*cb_write)(void *);
	int want_read;
	int want_write;
};

static struct source *sources = NULL;

void
NxAppSourceRegister (int fd, void *data, void (*cb_read)(void *), void (*cb_write)(void *))
{
	struct source *n;

	/* sanity check */
	for (n = sources; n; n = n->next) {
		assert (fd != n->fd);
	}

	n = malloc(sizeof(struct source));
	n->fd = fd;
	n->data = data;
	n->cb_read = cb_read;
	n->cb_write = cb_write;
	n->want_read = 0;
	n->want_write = 0;

	n->next = sources;
	sources = n;
}

void
NxAppSourceUnregister(int fd)
{
	struct source *n, **n_p;

	n_p = &sources;
	for (n = sources; n; n = n->next) {
		if (fd == n->fd) {
			*n_p = n->next;
			free(n);
			return;
		}
		n_p = &n->next;
	}
	assert(1);
}

void
NxAppSourceActivate(int fd, int want_read, int want_write)
{
	struct source *n;

	for (n = sources; n; n = n->next) {
		if (fd == n->fd) {
			if (!n->want_read ^ !want_read) {
				n->want_read = want_read;
				if (want_read)
					GrRegisterInput(fd);
				else
					GrUnregisterInput(fd);
			}
			if (!n->want_write ^ !want_write) {
				n->want_write = want_write;
				if (want_write)
					GrRegisterOutput(fd);
				else
					GrUnregisterOutput(fd);
			}
			return;
		}
	}
}

void
nxAppSourceEventHandler(int fd, int can_read, int can_write)
{
	struct source *n;

	for (n = sources; n; n = n->next) {
		if (fd == n->fd) {
			if (can_read)
				n->cb_read(n->data);
			if (can_write)
				n->cb_write(n->data);
			return;
		}
	}
}

void
NxAppLoop (void)
{
	GrMainLoop(NxEventResolveRoutine);
}

void
NxAppTerminate(void)
{
	GrDestroyWindow(AppWidget->spec.application.ewid);
    DBGLOG("Neux destroying appwidget\n");
	NxDestroyWidget(&AppWidget);
    DBGLOG("Neux client closing\n");
}

void
NxAppUnregister(void)
{
	NxAppTerminate();
	WmUnregisterApp();
	WmNotifyTopAppChange(FALSE, FALSE);
}

void
NxAppExit(void)
{
	GrClose();
	exit(0);
}

void
NxAppStop(void)
{
	NxAppUnregister();
	NxAppExit();
}

int
NxAppGetKeyLock(int read_hw_always)
{
	if (!key_lock_inited || read_hw_always) {
		char buf[16];
		int res;

		res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches",
						buf, sizeof(buf));
		if (res < 0)
			return -1;

		if (res > 0)
			key_lock = !strcmp(buf, "0");
		else
			key_lock = 0;
		key_lock_inited = 1;
	}

	return key_lock;
}

void
NxAppScreenSaverHandler (GR_EVENT * event, NX_WIDGET *widget)
{
    // override widget here, only main-app handles this.
    if (AppWidget)
	{
		widget = AppWidget;
		widget->base.event = *event;
	}

	if (widget->spec.application.cb_screensaver.fp != NULL)
		(*(widget->spec.application.cb_screensaver.fp))
		  (widget, widget->spec.application.cb_screensaver.dptr);
}

void
NxAppIdleHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    // override widget here, only main-app handles this.
    if (AppWidget)
	{
		widget = AppWidget;
		widget->base.event = *event;
	}

	if (widget->spec.application.cb_idle.fp != NULL)
		(*(widget->spec.application.cb_idle.fp))
		  (widget, widget->spec.application.cb_idle.dptr);
}

void
NxAppMsgHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    // override widget here, only main-app handles this.
    if (AppWidget)
	{
        int * pm = (int*)&event->msg.msg[0];
		switch (*pm)
		{
		case SYS_MSG_RUNAPP:
			if (AppWidget->spec.application.iswm)
			{
				pm++;
				DBGLOG("run application: %s\n", (char*)pm);
				WmRunAppImmediately((const char *)pm);
			}
			return;
		case SYS_MSG_STOPAPP:
			NxAppStop();
			return;
		case SYS_MSG_SET_SSAVER_TIMEOUT:
			if (AppWidget->spec.application.iswm)
			{
				pm++;
				NxSetAppScreenSaverTimeout(*pm);
			}
			return;
		case SYS_MSG_SSAVER_PAUSE_RESUME:
			if (AppWidget->spec.application.iswm)
			{
				pm++;
				switch (*pm)
				{
				case spsPause:
					bScrSaverPrevEnabled = bScrSaverEnabled;
					if (bScrSaverEnabled)
						NxSetAppScreenSaverEnabled(GR_FALSE);
					break;
				case spsResume:
					if (bScrSaverPrevEnabled)
						NxSetAppScreenSaverEnabled(GR_TRUE);
					break;
				}
			}
			break;
		}

		// done with system messages, continue to handle user level messages.
		widget = AppWidget;
		widget->base.event = *event;
	}

	if (widget->spec.application.cb_msg.fp != NULL)
		(*(widget->spec.application.cb_msg.fp))
		  (widget, widget->spec.application.cb_msg.dptr);
}

void
NxAppSWOnHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	if (event->sw.sw == SW_KBD_LOCK) {
		key_lock = 1;
		key_lock_inited = 1;
	}

    // override widget here, only main-app handles this.
    if (AppWidget)
	{
		widget = AppWidget;
		widget->base.event = *event;
	}

	if (widget->spec.application.cb_sw_on.fp != NULL)
		(*(widget->spec.application.cb_sw_on.fp))
		  (widget, widget->spec.application.cb_sw_on.dptr);
}

void
NxAppSWOffHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	if (event->sw.sw == SW_KBD_LOCK) {
		key_lock = 0;
		key_lock_inited = 1;
	}

    // override widget here, only main-app handles this.
    if (AppWidget)
	{
		widget = AppWidget;
		widget->base.event = *event;
	}

	if (widget->spec.application.cb_sw_off.fp != NULL)
		(*(widget->spec.application.cb_sw_off.fp))
		  (widget, widget->spec.application.cb_sw_off.dptr);
}

/**
 * Function sets the screen saver timeout value in seconds.
 *
 * @param iTimeOut
 *        saver timeout value in seconds.
 */
void
NxSetAppScreenSaverTimeout(int iTimeout)
{
	AppWidget->spec.application.iScrSaverTimeout = iTimeout;
	bScrSaverEnabled = (AppWidget->spec.application.iScrSaverTimeout ? GR_TRUE : GR_FALSE);
	GrSetScreenSaverTimeout(AppWidget->spec.application.iScrSaverTimeout);
}

void
NxGetAppScreenSaverTimeout(int *iTimeout)
{
	*iTimeout = AppWidget->spec.application.iScrSaverTimeout;
}

void
NxSetAppScreenSaverEnabled(GR_BOOL enabled)
{
	bScrSaverEnabled = enabled;
	if (bScrSaverEnabled)
	{
		if (0 >= AppWidget->spec.application.iScrSaverTimeout)
			AppWidget->spec.application.iScrSaverTimeout = DEFAULT_SAVER_TIME;
		GrSetScreenSaverTimeout(AppWidget->spec.application.iScrSaverTimeout);
	}
	else
		GrSetScreenSaverTimeout(0);
}

GR_BOOL
NxGetAppScreenSaverEnabled(void)
{
	return bScrSaverEnabled;
}

/**
 * Function returns screen height.
 *
 * @return
 *       Screen height in pixel.
 *
 */
int NxAppGetNeuxScreenHeight(void)
{
    return AppWidget->spec.application.screen.rows;
}

/**
 * Function returns screen width.
 *
 * @return
 *       Screen width in pixel.
 *
 */
int NxAppGetNeuxScreenWidth(void)
{
	return AppWidget->spec.application.screen.cols;
}

GR_FONT_ID
NxGetFontID(const char *fontname, GR_SIZE fontsize)
{
	int i;
	GR_FONT_ID fid;

	for (i = 0; i < app_font_count; i++)
	{
		if (app_font_tab[i].fontsize == fontsize &&
			!strncasecmp(app_font_tab[i].fontname, fontname, FONT_NAME_LEN - 1))
		{
			app_font_tab[i].refcount++;
			return app_font_tab[i].fontid;
		}
	}

	fid = GrCreateFont ((char *)fontname, fontsize, NULL);
	if (i < APP_FONT_TAB_MAX)
	{
		GR_FONT_INFO finfo;
		GrGetFontInfo(fid, &finfo);
		app_font_tab[i].fontheight = finfo.height;
		app_font_tab[i].fontid = fid;
		strlcpy(app_font_tab[i].fontname, fontname, FONT_NAME_LEN);
		app_font_tab[i].fontsize = fontsize;
		app_font_tab[i].refcount = 1;
		app_font_count = i;
	}

	return fid;
}

void
NxPutFontID(GR_FONT_ID fid)
{
	int i, j;

	for (i = 0; i < app_font_count; i++)
	{
		if (app_font_tab[i].fontid != fid)
			continue;

		app_font_tab[i].refcount--;
		if (app_font_tab[i].refcount == 0)
		{
			GrDestroyFont(fid);
			app_font_count--;
			if (i < app_font_count)
				memmove(&app_font_tab[i], &app_font_tab[i + 1], (app_font_count - i) * sizeof(app_font_t));
		}
		return;
	}

	GrDestroyFont(fid);
}

int
NxGetFontHeight(GR_FONT_ID fid)
{
	int i;
	GR_FONT_INFO finfo;

	for (i = 0; i < app_font_count; i++)
	{
		if (app_font_tab[i].fontid == fid)
		{
			return app_font_tab[i].fontheight;
		}
	}

	GrGetFontInfo(fid, &finfo);
	return finfo.height;
}
