/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
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
 * Window manager event handler routines.
 *
 * REVISION:
 *
 * 1) Initial creation.--- ----------------------------------- 2007-02-06 MG
 *
 */

#define OSD_DBG_MSG
#include "nc-err.h"
#include "nc-type.h"
#include "wm.h"
#include "nx-widgets.h"
#include "shared-mem.h"

#if 0
/** WM screen saver even handler.
 *
 * @param wmapp
 *        main application widget.
 * @param ptr
 *        widget specific data pointer if any.
 *
 */
void WmOnScreenSaver(NX_WIDGET *wmapp, DATA_POINTER ptr)
{
#if 0
	if (GR_TRUE == app->base.event.screensaver.activate)
	{
		if (curSSaver.ShowScrSaver != NULL)
		{
			GR_WINDOW_ID pid;
			pid = GrNewPixmap(SCREEN_WIDTH, SCREEN_HEIGHT, NULL);
			GrCopyArea(pid, mainApp->base.gc,
					   0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, mainApp->base.wid, 0, 0, 0);

			(*(curSSaver.ShowScrSaver))();
			GrDestroyWindow(pid);
		}
	}
	else
	{
		if (curSSaver.HideScrSaver != NULL)
			(*(curSSaver.HideScrSaver))();
	}
#endif
}

/**
 * Set screen saver time.
 *
 * @param t
 *      saver activation time in seconds. 0 to disable saver.
 *
 */
void AppSetScreenSaverTime(int t)
{
    if (0 >= t) NxSetAppScreenSaverEnabled(GR_FALSE);
	else	    NxSetAppScreenSaverTimeout(t);
}

/**
 * Enable / disable screen saver time.
 *
 * @param enable
 *      1 to enable, 0 to disable.
 *
 */
void AppEnableScreenSaver(int enable)
{
    (enable)?
	  NxSetAppScreenSaverEnabled(GR_TRUE):
	  NxSetAppScreenSaverEnabled(GR_FALSE);
}

#endif

//----------------------------------------------------------------------
// public APIs
//----------------------------------------------------------------------

/** WM child update even handler.
 *
 * @param event
 *        Nano-X event.
 *
 */
void WmOnChildUpdate(GR_EVENT_UPDATE * event)
{
#if 0
	win *window;

	DBGLOG("do_update: wid %d, subwid %d, x %d, y %d, width %d, height %d, "
		   "utype %d\n", event->wid, event->subwid, event->x, event->y, event->width,
		   event->height, event->utype);

	if (!(window = find_window(event->subwid))) {
		if (event->utype == GR_UPDATE_MAP) WmNewClientWindow(event->subwid);
		return;
	}

	if (window->type == WINDOW_TYPE_CONTAINER) {
		if (event->utype == GR_UPDATE_ACTIVATE)
			redraw_ncarea(window);
		return;
	}

	if (window->type == WINDOW_TYPE_CLIENT) {
		if (event->utype == GR_UPDATE_MAP) client_window_remap(window);
		if (event->utype == GR_UPDATE_DESTROY) client_window_destroy(window);
		if (event->utype == GR_UPDATE_UNMAP) client_window_unmap(window);
		if (event->utype == GR_UPDATE_SIZE) client_window_resize(window);
	}
#endif
}
