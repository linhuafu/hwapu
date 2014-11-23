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
 * Widget Window header.
 *
 * REVISION:
 *
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-04-08 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXWINDOW_H_
#define _NXWINDOW_H_

#include "nx-base.h"


#define WINDOW_WIDTH 			100
#define WINDOW_HEIGHT 			60

#define FORM_MAX_FOCUS_IDX      32 /* maximum child focus index per form. */

typedef struct
{
	char *         caption;
	GR_IMAGE_ID    imageid;                   //background id
	GR_BOOL        transparent;
	NX_WIDGET *    activewidget;
	NX_WIDGET *    focus[FORM_MAX_FOCUS_IDX]; //FORM manages focus for all its children.
	int            focusTabLen;               // focus table length.
	int            focusIdx;                  // active focus index.
	CallBackStruct cb_keyup;
	CallBackStruct cb_keydown;
	CallBackStruct cb_exposure;
	CallBackStruct cb_closereq;
	CallBackStruct cb_focusin;
	CallBackStruct cb_focusout;
	CallBackStruct cb_destroy;
	CallBackStruct cb_hotplug;
}NX_WINDOW;

/*member  function */
void
NxWindowCreate (NX_WIDGET *);
void
NxWindowDestroy(NX_WIDGET *);
void
NxWindowDraw(NX_WIDGET *);

/*event handler */
void
NxWindowKeyDownHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowKeyUpHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowExposureHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowCloseReqHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowFocusInHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowFocusOutHandler (GR_EVENT *, NX_WIDGET *);
void
NxWindowHotplugHandler (GR_EVENT *, NX_WIDGET *);

/*public function*/
int
NxGetWindowTitle(NX_WIDGET *, char **);
int
NxSetWindowTitle(NX_WIDGET *, char *);

#endif //_NXWINDOW_H_

