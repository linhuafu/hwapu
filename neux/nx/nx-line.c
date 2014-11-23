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
 * Widget Line routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-03-02 EY
 *
 */

#include <string.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

void
NxLineCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if (widget->base.width == 0)
		widget->base.width = LINE_WIDTH;
	if (widget->base.height == 0)
		widget->base.height = LINE_HEIGHT;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Line%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	props.flags = GR_WM_FLAGS_PROPS;// | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOFOCUS;
	GrSetWMProperties (widget->base.wid, &props);/*Set the WM props*/

	widget->spec.line.cb_exposure.fp		= NULL;
	widget->spec.line.cb_exposure.inherit	= GR_TRUE;

	GrSelectEvents (widget->base.wid,
		  GR_EVENT_MASK_EXPOSURE);

	DBGLOG("Create Line %d\n", widget->base.wid);
}

void
NxLineDraw(NX_WIDGET * Line)
{
	if (!Line->base.visible)
		return;
	GrClearWindow(Line->base.wid, GR_FALSE);

	NxSetGCForeground(Line->base.gc, Line->base.bgcolor);
	GrFillRect(Line->base.wid, Line->base.gc, 0, 0, Line->base.width, Line->base.height);
}


void
NxLineExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("lineexprosureevent\n");
	if (widget->spec.line.cb_exposure.inherit)
		NxLineDraw(widget);
	if (widget->spec.line.cb_exposure.fp != NULL)
		(*(widget->spec.line.cb_exposure.fp))(widget, widget->spec.line.cb_exposure.dptr);
}

void
NxLineDestroy(NX_WIDGET *widget)
{
	NxDeleteFromRegistry(widget);
}
