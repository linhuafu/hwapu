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
 * Widget Timer header.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXTIMER_H_
#define _NXTIMER_H_
#include "nx-base.h"


#define TIMER_WIDTH 		10
#define TIMER_HEIGHT		10

typedef struct
{
	GR_TIMER_ID timerid;
	int interval;
	int xshot;  //-1:no limited
	int xcount;  //only counter
	CallBackStruct cb_timer;
}NX_TIMER;

/*member  function */
void
NxTimerCreate (NX_WIDGET *);
void
NxTimerDestroy(NX_WIDGET *);


/*event handler */
void
NxTimerTimerHandler (GR_EVENT *, NX_WIDGET *);


/*public function*/
int
NxSetTimerControl(NX_WIDGET *, int, int);
void
NxSetTimerEnabled(NX_WIDGET *, GR_BOOL);


#endif //_NXTIMER_H_
