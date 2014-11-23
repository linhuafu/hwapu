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
 * Widget Window routines.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-04-28 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#include <string.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "application.h"
#include "wm.h"
#include "widgets.h"

void
NxWindowCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;
	NX_WINDOW * window;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if (widget->base.width == 0)
		widget->base.width = WINDOW_WIDTH;
	if (widget->base.height == 0)
		widget->base.height = WINDOW_HEIGHT;

	DBGLOG("form pid = %d\n", widget->parent->base.wid);

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	DBGLOG("form wid = %d\n", widget->base.wid);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Window%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	window = &widget->spec.container.spec.window;
	if (window->caption == NULL)
		window->caption = strdup(widget->base.name);
	else
		window->caption = strdup(window->caption);

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOAUTOMOVE;//GR_WM_PROPS_APPWINDOW|
	if (window->transparent)
		props.props |= GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(widget->base.wid, &props);

	if (!window->transparent && widget->base.bgpixmap > 0)
		GrSetBackgroundPixmap(widget->base.wid, widget->base.bgpixmap, GR_BACKGROUND_CENTER);

	window->cb_closereq.fp		= NULL;
	window->cb_closereq.inherit	= GR_TRUE;
	window->cb_exposure.fp		= NULL;
	window->cb_exposure.inherit	= GR_TRUE;
	window->cb_keydown.fp		= NULL;
	window->cb_keydown.inherit	= GR_TRUE;
	window->cb_keyup.fp			= NULL;
	window->cb_keyup.inherit	= GR_TRUE;
	window->cb_focusin.fp		= NULL;
	window->cb_focusin.inherit	= GR_TRUE;
	window->cb_focusout.fp		= NULL;
	window->cb_focusout.inherit	= GR_TRUE;
	window->cb_destroy.fp		= NULL;
	window->cb_destroy.inherit	= GR_TRUE;
	window->cb_hotplug.fp		= NULL;
	window->cb_hotplug.inherit	= GR_TRUE;

	GrSelectEvents (widget->base.wid,
					GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ  |
					GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
					GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT |
					GR_EVENT_MASK_HOTPLUG);

	DBGLOG("Create window %d\n", widget->base.wid);
}

void
NxWindowDraw(NX_WIDGET * window)
{
	if (window->base.visible == GR_FALSE)
		return;

	if (window->base.bgpixmap < 1)
	{
		NxSetGCForeground(window->base.gc, window->base.bgcolor);
		GrFillRect(window->base.wid, window->base.gc, 0, 0, window->base.width, window->base.height);
		NxSetGCForeground(window->base.gc, window->base.fgcolor);
	}

	//DBGLOG("wdraw%d    %d \n", window->base.wid, window->base.pid);
}

void
NxWindowKeyDownHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("wkeydownevent\n");
	DBGLOG("cbfp addr = %p  wid = %d\n", window->cb_keydown.fp, widget->base.wid);

	if (window->cb_keydown.fp != NULL)
		(*(window->cb_keydown.fp))(widget, window->cb_keydown.dptr);
}

void
NxWindowKeyUpHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	if (window->cb_keyup.fp != NULL)
		(*(window->cb_keyup.fp))(widget, window->cb_keyup.dptr);
}

void
NxWindowExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("wexprosureevent%d\n", widget->base.wid);

	if (window->cb_exposure.inherit)
		NxWindowDraw(widget);

	if (window->cb_exposure.fp != NULL)
		(*(window->cb_exposure.fp))(widget, window->cb_exposure.dptr);
}

void
NxWindowCloseReqHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("wcloseevent\n");

	if (window->cb_closereq.fp != NULL)
		(*(window->cb_closereq.fp))(widget, window->cb_closereq.dptr);
}

void
NxWindowFocusInHandler(GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("wfocusinevent %d\n", widget->base.wid);

	if (window->cb_focusin.fp != NULL)
		(*(window->cb_focusin.fp))(widget, window->cb_focusin.dptr);
	if (window->cb_focusin.inherit)
		;
}

void
NxWindowFocusOutHandler(GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("wfocusoutevent %d\n", widget->base.wid);

	if (window->cb_focusout.fp != NULL)
		(*(window->cb_focusout.fp))(widget, window->cb_focusout.dptr);
	if (window->cb_focusout.inherit)
		;
}

void
NxWindowDestroy(NX_WIDGET *widget)
{
	NX_WIDGET widget_copy = *widget;
	NX_WINDOW * window = &widget_copy.spec.container.spec.window;

	if (window->caption != NULL)
		free(window->caption);

	NxDeleteFromRegistry(widget);

	if (window->cb_destroy.fp != NULL)
		(*(window->cb_destroy.fp))(widget, window->cb_destroy.dptr);
}

void
NxWindowHotplugHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WINDOW * window = &widget->spec.container.spec.window;

	DBGLOG("whotplugevent %d\n", widget->base.wid);

	if (window->cb_hotplug.fp != NULL)
		(*(window->cb_hotplug.fp))(widget, window->cb_hotplug.dptr);
	if (window->cb_hotplug.inherit)
		;
}

int
NxGetWindowTitle(NX_WIDGET *widget, char **caption)
{
	GR_WM_PROPERTIES props;
	if (widget->base.type != WIDGET_TYPE_WINDOW)
		return -1;

	GrGetWMProperties(widget->base.wid, &props);
	*caption = props.title;
	return 1;
}

int
NxSetWindowTitle(NX_WIDGET *widget, char *caption)
{
	GR_WM_PROPERTIES props;
	if (widget->base.type != WIDGET_TYPE_WINDOW)
		return -1;

	GrSetWindowTitle(widget->base.wid, caption);
	return 1;
}
