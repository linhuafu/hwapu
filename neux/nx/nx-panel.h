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
 * Widget Panel header.
 *
 * REVISION:
 *
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-04-08 MG
 * 1) Initial creation. ----------------------------------- 2006-03-01 EY
 *
 */

#ifndef _NXPANEL_H_
#define _NXPANEL_H_

#include "nx-base.h"

#define PANEL_WIDTH 			100
#define PANEL_HEIGHT 			60

typedef struct
{
	GR_IMAGE_ID imageid; //background
	GR_BOOL transparent;
	GR_BOOL canfocus;
	//NX_CONTAINER container;
	CallBackStruct cb_exposure;
	CallBackStruct cb_destroy;
	CallBackStruct cb_keydown;
}NX_PANEL;

/*member  function */
void
NxPanelCreate (NX_WIDGET *);
void
NxPanelDestroy(NX_WIDGET *);
void
NxPanelDraw(NX_WIDGET *);

/*event handler */
void
NxPanelExposureHandler (GR_EVENT *, NX_WIDGET *);
void
NxPanelKeyDownHandler(GR_EVENT *event, NX_WIDGET *widget);


/*public function*/

#endif //_NXPANEL_H_

