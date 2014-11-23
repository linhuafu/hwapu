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
 * Widget Picture header.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXPICTURE_H_
#define _NXPICTURE_H_
#include "nx-base.h"

#define PICTURE_COLOR 		GR_RGB(64, 64, 200)
#define PICTURE_WIDTH 		100
#define PICTURE_HEIGHT		100

typedef struct
{
	int width;
	int height;
}NX_SIZE;

typedef struct
{
	int x1;
	int y1;
	int x2;
	int y2;
}NX_REGION;

typedef struct
{
	void *buf; // the point to buf
	size_t size;  //  the size of buf
	PICTURE_TYPE type;
	GR_BOOL transparent;
	GR_BOOL bredrawflag;
	GR_IMAGE_ID imageid;
	GR_BOOL zoomable;
	int zoom;

	NX_REGION visible;
	NX_SIZE thumb;   //thumb size
	NX_SIZE orig;		/** image display size when first displayed. */
	NX_SIZE pix;    //image size
	NX_SIZE curr;    //current size

	CallBackStruct cb_exposure;

}NX_PICTURE;

/*member  function */
void
NxPictureCreate (NX_WIDGET *);
void
NxPictureDestroy(NX_WIDGET *);
void
NxPictureDraw (NX_WIDGET *);
void
NxNeuxPictureZoomIn (NX_WIDGET * picture);
void
NxNeuxPictureZoomOut (NX_WIDGET * picture);
void
NxNeuxPictureRevert(NX_WIDGET * picture);
void
NxNeuxPicturePanview(NX_WIDGET *picture, int vertical, int horizon);
/*event handler */
void
NxPictureExposureHandler (GR_EVENT *, NX_WIDGET *);

/*public function*/
int
NxInitPicture(NX_WIDGET *);

void
NxResize(GR_RECT* dest,GR_RECT* source,int width,int height);
void
NxDrawRect(NX_WIDGET *widget,GR_RECT* rect,int wide);
void
NxEnlargeRect(GR_RECT* dest,GR_RECT* source,int range);

#endif //_NXPICTURE_H_

