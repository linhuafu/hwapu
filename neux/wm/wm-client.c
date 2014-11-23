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
 * Window manager client support routines.
 *
 * REVISION:
 *
 * 1) Initial creation.--- ----------------------------------- 2007-03-31 MG
 *
 */

#define OSD_DBG_MSG
#include "nc-err.h"
#include "nc-type.h"
#include "wm.h"
#include "nx-widgets.h"
#include "shared-mem.h"

/** create new client window .
 * A new client window has been mapped, so we need to reparent and decorate it.
 *
 * @param wid
 *        client window ID.
 * @return
 *        -1 on failure or 0 on success.
 */
int WmNewClientWindow(GR_WINDOW_ID wid)
{
#if 0
	win window;
	GR_WINDOW_ID pid;
	GR_WINDOW_INFO winfo;
	GR_COORD x, y, width, height, xoffset, yoffset;
	GR_WM_PROPS style;
	GR_WM_PROPERTIES props;

	/* get client window information*/
	GrGetWindowInfo(wid, &winfo);
	style = winfo.props;

	/* if not redecorating or not child of root window, return*/
	if (winfo.parent != GR_ROOT_WINDOW_ID ||
	    (style & GR_WM_PROPS_NODECORATE))
		return 0;

	/* deal with replacing borders with window decorations*/
	if (winfo.bordersize) {
		/*
		 * For complex reasons, it's easier to unmap,
		 * remove the borders, and then map again,
		 * rather than try to recalculate the window
		 * position in the server w / o borders.  By
		 * the time we get this event, the window has
		 * already been painted with borders...
		 * This currently causes a screen flicker as
		 * the window is painted twice.  The workaround
		 * is to create the window without borders in
		 * the first place.
		 */
		GrUnmapWindow(wid);

		/* remove client borders, if any*/
		GrSetWindowBorderSize(wid, 0);

		/* remap the window without borders, call this routine again*/
		GrMapWindow(wid);
		return 0;
	}

	/* if default decoration style asked for, set real draw bits*/
	if ((style & GR_WM_PROPS_APPMASK) == GR_WM_PROPS_APPWINDOW) {
		GR_WM_PROPERTIES pr;

		style = (style & ~GR_WM_PROPS_APPMASK)|DEFAULT_WINDOW_STYLE;
		pr.flags = GR_WM_FLAGS_PROPS;
		pr.props = style;
		GrSetWMProperties(wid, &pr);
	}

	/* determine container widths and client child window offsets*/
	if (style & GR_WM_PROPS_APPFRAME) {
		width = winfo.width + CXFRAME;
		height = winfo.height + CYFRAME;
		xoffset = CXBORDER;
		yoffset = CYBORDER;
	} else if (style & GR_WM_PROPS_BORDER) {
		width = winfo.width + 2;
		height = winfo.height + 2;
		xoffset = 1;
		yoffset = 1;
	} else {
		width = winfo.width;
		height = winfo.height;
		xoffset = 0;
		yoffset = 0;
	}
	if (style & GR_WM_PROPS_CAPTION) {
		height += CYCAPTION;
		yoffset += CYCAPTION;
		if (style & GR_WM_PROPS_APPFRAME) {
			/* extra line under caption with appframe*/
			++height;
			++yoffset;
		}
	}

	/* determine x, y window location*/
	if (style & GR_WM_PROPS_NOAUTOMOVE) {
		x = winfo.x;
		y = winfo.y;
	} else {
		/* We could proably use a more intelligent algorithm here */
		x = lastx + WINDOW_STEP;
		if ((x + width) > si.cols)
			x = FIRST_WINDOW_LOCATION;
		lastx = x;
		y = lasty + WINDOW_STEP;
		if ((y + height) > si.rows)
			y = FIRST_WINDOW_LOCATION;
		lasty = y;
	}

	/* create container window*/
	pid = GrNewWindow(GR_ROOT_WINDOW_ID, x, y, width, height,
		0, LTGRAY, BLACK);
	window.wid = pid;
	window.pid = GR_ROOT_WINDOW_ID;
	window.type = WINDOW_TYPE_CONTAINER;
	window.sizing = GR_FALSE;
	window.active = 0;
	window.data = NULL;
	window.clientid = wid;
	add_window(&window);

	/* don't erase background of container window*/
	props.flags = GR_WM_FLAGS_PROPS;
	props.props = style | GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(pid, &props);

	Dprintf("New client window %d container %d\n", wid, pid);

	GrSelectEvents(pid, GR_EVENT_MASK_CHLD_UPDATE
		| GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN
		| GR_EVENT_MASK_MOUSE_POSITION | GR_EVENT_MASK_EXPOSURE);

	/* reparent client to container window (child is already mapped)*/
	GrReparentWindow(wid, pid, xoffset, yoffset);

	/* map container window*/
	GrMapWindow(pid);

	GrSetFocus(wid);	/* force fixed focus*/

	/* add client window*/
	window.wid = wid;
	window.pid = pid;
	window.type = WINDOW_TYPE_CLIENT;
	window.sizing = GR_FALSE;
	window.active = 0;
	window.clientid = 0;
	window.data = NULL;
	add_window(&window);

#endif
	return 0;
}
