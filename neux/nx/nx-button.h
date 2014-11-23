/*
 *  Copyright(C) 2006-2007 Neuros Technology International LLC.
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
 * Widget Button header.
 *
 * REVISION:
 *
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXBUTTON_H_
#define _NXBUTTON_H_

#include "nx-base.h"

#define BUTTON_WIDTH 		60
#define BUTTON_HEIGHT		20

typedef enum
{
	btNormal,
    btGeneral,
	btFlat,
	btCircle,
	btImage
}EM_BUTTON_TYPE;

typedef enum
{
	btLeft,
	btCenter,
	btRight
}EM_BUTTON_ALIGN;


typedef struct
{
	char *caption;
	GR_BOOL transparent;
	GR_IMAGE_ID imageid;
	GR_BOOL st_down;
	EM_BUTTON_ALIGN align;
	EM_BUTTON_TYPE type;
	GR_BOOL bredrawflag;
	GR_COLOR selectedcolor;
	GR_COLOR shadowdcolor;

	CallBackStruct cb_keyup;
	CallBackStruct cb_keydown;
	CallBackStruct cb_exposure;
	CallBackStruct cb_focusin;
	CallBackStruct cb_focusout;
	GetImageFuncPtr cb_getimage;  //if image type, use this callback get image to display
}NX_BUTTON;

/*member  function */
void
NxButtonCreate (NX_WIDGET *);
void
NxButtonDestroy(NX_WIDGET *);
void
NxButtonDraw (NX_WIDGET *);

/*event handler */
void
NxButtonKeyDownHandler (GR_EVENT *, NX_WIDGET *);
void
NxButtonKeyUpHandler (GR_EVENT *, NX_WIDGET *);
void
NxButtonExposureHandler (GR_EVENT *, NX_WIDGET *);
void
NxButtonFocusInHandler (GR_EVENT *, NX_WIDGET *);
void
NxButtonFocusOutHandler (GR_EVENT *, NX_WIDGET *);

/*public function*/
int
NxSetButtonPixmap(NX_WIDGET *, char *);
int
NxRemoveButtonPixmap(NX_WIDGET *);

int
NxSetButtonCaption(NX_WIDGET *, const char *);
int
NxGetButtonCaption(NX_WIDGET *, char **);
void
NxSetButtonProp(NX_WIDGET *widget);


#endif //_NXBUTTON_H_

