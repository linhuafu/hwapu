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
 * Widget ProgressBar routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-03-02 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include <string.h>
#include "nx-widgets.h"


void
NxProgressBarCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if (widget->spec.progressbar.orientation == omHorizontal)
	{
		if (widget->base.width == 0)
			widget->base.width = PROGRESSBAR_WIDTH;
		if (widget->base.height == 0)
			widget->base.height = PROGRESSBAR_HEIGHT;
	}
	else
	{
		if (widget->base.width == 0)
			widget->base.width = PROGRESSBAR_HEIGHT;
		if (widget->base.height == 0)
			widget->base.height = PROGRESSBAR_WIDTH;
	}

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "ProgressBar%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	props.flags = GR_WM_FLAGS_PROPS;// | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOFOCUS;
	GrSetWMProperties (widget->base.wid, &props);/*Set the WM props*/

	widget->spec.progressbar.cb_exposure.fp			= NULL;
	widget->spec.progressbar.cb_exposure.inherit	= GR_TRUE;

	GrSelectEvents (widget->base.wid,
		  GR_EVENT_MASK_EXPOSURE);

	DBGLOG("Create ProgressBar %d\n", widget->base.wid);
}

void
NxProgressBarDraw(NX_WIDGET * ProgressBar)
{
	int w, h, w1, w2, h1, h2, b;
	int hw;  //head width

	if (!ProgressBar->base.visible)
		return;

	b = ProgressBar->spec.progressbar.borderwidth;
	w = ProgressBar->base.width;
	h = ProgressBar->base.height;

	if (ProgressBar->spec.progressbar.type == ptNormal)
	{
		if (ProgressBar->spec.progressbar.orientation == omHorizontal)
		{
			hw = h - 2 * b;
			w2 = w - 2 * b - 3 * hw;
			h2 = h - 2 * b;

			if (ProgressBar->spec.progressbar.max > ProgressBar->spec.progressbar.min)
				w1 = (w2 * (ProgressBar->spec.progressbar.value - ProgressBar->spec.progressbar.min) / (ProgressBar->spec.progressbar.max - ProgressBar->spec.progressbar.min));
			else
				w1 = w2 / 2;
			h1 = h2;

			NxSetGCForeground (ProgressBar->base.gc, ProgressBar->spec.progressbar.blockcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b, hw, hw);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b + w2 + 2 * hw, b, hw, hw);

			NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.bgcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b + hw * 3 / 2, b, w2, h2);

			NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.fgcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b + hw * 3 / 2, b, w1, h1);
		}
		else
		{
			hw = w - 2 * b;
			w2 = w - 2 * b;
			h2 = h - 2 * b - 3 * hw;

			if (ProgressBar->spec.progressbar.max > ProgressBar->spec.progressbar.min)
				h1 = h2 - (h2 * (ProgressBar->spec.progressbar.value - ProgressBar->spec.progressbar.min) / (ProgressBar->spec.progressbar.max - ProgressBar->spec.progressbar.min));
			else
				h1 = h2 / 2;
			w1 = w2;

			NxSetGCForeground (ProgressBar->base.gc, ProgressBar->spec.progressbar.blockcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b, hw, hw);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b + w2 + 2 * hw, hw, hw);

			NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.bgcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b + hw * 3 / 2, w2, h2);

			NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.fgcolor);
			GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b + hw * 3 / 2, w1, h1);
		}
	}
	else
	{
		w2 = w - 2 * b;
		h2 = h - 2 * b;
		if (ProgressBar->spec.progressbar.orientation == omHorizontal)
		{
			if (ProgressBar->spec.progressbar.max > ProgressBar->spec.progressbar.min)
				w1 = (w2 * (ProgressBar->spec.progressbar.value - ProgressBar->spec.progressbar.min) / (ProgressBar->spec.progressbar.max - ProgressBar->spec.progressbar.min));
			else
				w1 = w2 / 2;
			h1 = h2;
		}
		else
		{
			if (ProgressBar->spec.progressbar.max > ProgressBar->spec.progressbar.min)
				h1 = h2 - (h2 * (ProgressBar->spec.progressbar.value - ProgressBar->spec.progressbar.min) / (ProgressBar->spec.progressbar.max - ProgressBar->spec.progressbar.min));
			else
				h1 = h2 / 2;
			w1 = w2;
		}

		DBGLOG("\n\n%d %d %d %d %d %d %d\n", w, w2, h2, w1, ProgressBar->spec.progressbar.value, ProgressBar->spec.progressbar.max, ProgressBar->spec.progressbar.min);

		NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.bgcolor);
		GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b, w2, h2);

		NxSetGCForeground(ProgressBar->base.gc, ProgressBar->base.fgcolor);
		GrFillRect(ProgressBar->base.wid, ProgressBar->base.gc, b, b, w1, h1);
	}
}


void
NxProgressBarExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("pbexprosureevent\n");
	if (widget->spec.progressbar.cb_exposure.inherit)
		NxProgressBarDraw(widget);
	if (widget->spec.progressbar.cb_exposure.fp != NULL)
		(*(widget->spec.progressbar.cb_exposure.fp))(widget, widget->spec.progressbar.cb_exposure.dptr);
}

void
NxProgressBarDestroy(NX_WIDGET *widget)
{
	NxDeleteFromRegistry(widget);
}

int
NxSetProgressBarValue(NX_WIDGET *widget, int value)
{
	if (widget->base.type != WIDGET_TYPE_PROGRESSBAR)
		return -1;
	if (value > widget->spec.progressbar.max)
		return -2;
	if (value < widget->spec.progressbar.min)
		return -3;
	widget->spec.progressbar.value = value;
    GrClearWindow(widget->base.wid, GR_FALSE);
	NxProgressBarDraw(widget);
	return 1;
}


int
NxGetProgressBarValue(NX_WIDGET *widget, int *value)
{
	if (widget->base.type != WIDGET_TYPE_PROGRESSBAR)
		return -1;
	*value = widget->spec.progressbar.value;
	return 1;
}

