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
 * Widget Label header.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXLABEL_H_
#define _NXLABEL_H_

#include "nx-base.h"

#define LABEL_WIDTH 		60
#define LABEL_HEIGHT		20

typedef struct
{
	char *caption;
	GR_BOOL transparent;
	GR_BOOL transb;
	GR_BOOL autosize;
	GR_BOOL automshow;
	LABEL_ALIGN align;
	GR_IMAGE_ID imageid;

	GR_TIMER_ID 	timerid;  //for auto move show
	GR_WINDOW_ID    mshowpixmap;
	int 			interval;
	int 			mshowx;  //x offset
	int 			mshowt;  //sleep count
    GR_SIZE     captionwidth;
    GR_SIZE     captionheight;

	CallBackStruct cb_exposure;

}NX_LABEL;

/*member  function */
void
NxLabelCreate (NX_WIDGET *);
void
NxLabelDestroy(NX_WIDGET *);
void
NxLabelDraw (NX_WIDGET *);

/*event handler */
void
NxLabelExposureHandler (GR_EVENT *, NX_WIDGET *);
void
NxLabelTimerHandler (GR_EVENT *event, NX_WIDGET *widget);

/*public function*/
int
NxSetLabelCaption(NX_WIDGET *, const char *);
int
NxGetLabelCaption(NX_WIDGET *, char **);
int
NxSetLabelMShow(NX_WIDGET *, int);


#endif //_NXLABEL_H_

