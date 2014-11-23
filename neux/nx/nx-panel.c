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
 * Widget Panel routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-03-01 EY
 *
 */

#include <string.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

void
NxPanelCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;
	NX_PANEL * panel;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if (widget->base.width == 0)
		widget->base.width = PANEL_WIDTH;
	if (widget->base.height == 0)
		widget->base.height = PANEL_HEIGHT;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Panel%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	panel = &widget->spec.container.spec.panel;

	props.flags = GR_WM_FLAGS_PROPS;// | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOFOCUS;
	if (panel->transparent)
		props.props |= GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(widget->base.wid, &props);

	if (!panel->transparent && widget->base.bgpixmap > 0)
		GrSetBackgroundPixmap(widget->base.wid, widget->base.bgpixmap, GR_BACKGROUND_CENTER);

	panel->cb_exposure.fp		= NULL;
	panel->cb_exposure.inherit	= GR_TRUE;
	panel->cb_destroy.fp		= NULL;
	panel->cb_destroy.inherit	= GR_TRUE;
	panel->cb_keydown.fp		= NULL;
	panel->cb_keydown.inherit	= GR_TRUE;

	if (panel->canfocus)
	{
		GrSelectEvents (widget->base.wid,
		  	GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN);
	}
	else
	{
		GrSelectEvents (widget->base.wid,
			  GR_EVENT_MASK_EXPOSURE);
	}

	DBGLOG("Create Panel %d\n", widget->base.wid);
}

void
NxPanelDraw(NX_WIDGET * Panel)
{
	if (!Panel->base.visible)
		return;

	if (Panel->base.bgpixmap > 0)
	{
		//GrCopyArea(Panel->base.wid, Panel->base.gc, 0, 0, Panel->base.width, Panel->base.height, Panel->base.bgpixmap, 0, 0, 0);
	}
	else
	{
		NxSetGCForeground(Panel->base.gc, Panel->base.bgcolor);
		GrFillRect(Panel->base.wid, Panel->base.gc, 0, 0, Panel->base.width, Panel->base.height);
		NxSetGCForeground(Panel->base.gc, Panel->base.fgcolor);
	}
}


void
NxPanelExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NX_PANEL * panel = &widget->spec.container.spec.panel;
	DBGLOG("paexprosureevent\n");
	if (panel->cb_exposure.inherit)
		NxPanelDraw(widget);
	if (panel->cb_exposure.fp != NULL)
		(*(panel->cb_exposure.fp))(widget, panel->cb_exposure.dptr);
}

void
NxPanelDestroy(NX_WIDGET *widget)
{
	NX_PANEL * panel = &widget->spec.container.spec.panel;
	if (panel->cb_destroy.fp != NULL)
		(*(panel->cb_destroy.fp))(widget, panel->cb_destroy.dptr);
	NxDeleteFromRegistry(widget);
}

void
NxPanelKeyDownHandler(GR_EVENT *event, NX_WIDGET *widget)
{
	NX_PANEL * panel = &widget->spec.container.spec.panel;
	if (panel->cb_keydown.fp != NULL)
		(*(panel->cb_keydown.fp))(widget, panel->cb_keydown.dptr);
}
