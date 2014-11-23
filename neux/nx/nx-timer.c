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
 * Widget Timer routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

void
NxTimerCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	if (widget->base.height == 0)
		widget->base.height = TIMER_HEIGHT;
	if (widget->base.width == 0)
		widget->base.width = TIMER_WIDTH;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	widget->spec.timer.xcount = widget->spec.timer.xshot;
	if ((widget->spec.timer.interval <= 0) || (widget->spec.timer.xshot == 0))
		widget->base.enabled = GR_FALSE;
	else
		widget->spec.timer.timerid = GrCreateTimer(widget->base.wid, widget->spec.timer.interval);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Timer%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	props.flags = GR_WM_FLAGS_PROPS;// | GR_WM_FLAGS_TITLE;
	props.props = GR_WM_PROPS_NOFOCUS;
	GrSetWMProperties (widget->base.wid, &props);/*Set the WM props*/

	//GrText (widget->base.wid, widget->base.gc, 0, 0, widget->base.name, strlen(widget->base.name), MWTF_UTF8 | GR_TFTOP);

	widget->spec.timer.cb_timer.fp		= NULL;
	widget->spec.timer.cb_timer.inherit	= GR_TRUE;

	GrSelectEvents (widget->base.wid, GR_EVENT_MASK_TIMER);

	DBGLOG("Create Timer %d\n", widget->base.wid);
}

void
NxTimerTimerHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	if (widget->spec.timer.cb_timer.fp != NULL)
	{
	    if (widget->base.enabled)
		    (*(widget->spec.timer.cb_timer.fp))(widget, widget->spec.timer.cb_timer.dptr);

		if (widget->spec.timer.xcount != -1) {
			widget->spec.timer.xcount--;
			if (widget->spec.timer.xcount == 0)
				NxSetTimerEnabled(widget, GR_FALSE);
		}
	}

	if (widget->spec.timer.cb_timer.inherit)
		;
}


void
NxTimerDestroy(NX_WIDGET *widget)
{
	if (widget->spec.timer.timerid > 0)
		GrDestroyTimer(widget->spec.timer.timerid);
	NxDeleteFromRegistry(widget);
}

int
NxSetTimerControl(NX_WIDGET *widget, int interval, int xshot)
{
	widget->spec.timer.interval = interval;
	widget->spec.timer.xshot = xshot;
	widget->spec.timer.xcount = widget->spec.timer.xshot;

	if ((widget->spec.timer.interval <= 0) || (widget->spec.timer.xshot == 0))
		NxSetTimerEnabled(widget, GR_FALSE);
	else
		NxSetTimerEnabled(widget, GR_TRUE);

	return 0;
}

void
NxSetTimerEnabled(NX_WIDGET *widget, GR_BOOL enabled)
{
	if (widget->spec.timer.timerid > 0)
	{
		GrDestroyTimer(widget->spec.timer.timerid);
		widget->spec.timer.timerid = 0;
	}

	if (enabled && ((widget->spec.timer.interval <= 0) || (widget->spec.timer.xshot == 0)))
		return;

	widget->base.enabled = enabled;
	if (widget->base.enabled)
	{
		widget->spec.timer.xcount = widget->spec.timer.xshot;
		widget->spec.timer.timerid = GrCreateTimer(widget->base.wid, widget->spec.timer.interval);
	}
}
