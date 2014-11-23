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
 * NeuxMessageboxNew widget support routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2006-05-25 EY
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"


TIMER * NeuxTimerNew(WIDGET * owner, int interval, int xshot)
{
	NX_WIDGET * timer;

	timer = NxNewWidget(WIDGET_TYPE_TIMER, (NX_WIDGET *)owner);
	timer->spec.timer.interval = interval;
	timer->spec.timer.xshot = xshot;
	NxCreateWidget(timer);

	return timer;
}

void NeuxTimerSetCallback(TIMER * tmr, void * fptr)
{
	NX_TIMER *me = &((NX_WIDGET *)tmr)->spec.timer;

	NxRegisterCallBack(&me->cb_timer, OnTimer, fptr);
}

void NeuxTimerSetControl(TIMER *tmr, int interval, int xshot)
{
	NX_WIDGET *me = (NX_WIDGET *)tmr;
	NxSetTimerControl(me, interval, xshot);
}

void NeuxTimerSetEnabled(TIMER *tmr, int enable)
{
	NX_WIDGET *me = (NX_WIDGET *)tmr;
	NxSetTimerEnabled(me, enable);
}
