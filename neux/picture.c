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
 * Picture widget support routines.
 *
 * REVISION:
 *
 *
 * 2) Implemnet function. ----------------------------------- 2006-05-19 EY
 * 1) Initial creation.   ----------------------------------- 2006-05-04 MG
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"


/*
  x, y coordinates are container widget coordinates.
 */

/**
 * Function creates picture widget.
 *
 * @param owner
 *      Picture owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param type
 *      picture data type. currently the only available value is:
 *        PICTURE_TYPE_FILE: parameter pic is the path to the picture file (char *)
 * @param pic
 *      picture data pointer, see type definition.
 * @return
 *      Picture widget if successful, otherwise NULL.
 *
 */
PICTURE * NeuxPictureNew(WIDGET * owner, int x, int y, int w, int h,
					 PICTURE_TYPE type, void * buf)
{
	NX_WIDGET * picture;
	picture = NxNewWidget(WIDGET_TYPE_PICTURE, (NX_WIDGET *)owner);
	picture->base.posx = x;
	picture->base.posy = y;
	picture->base.width = w;
	picture->base.height = h;

	picture->spec.picture.type = type;
	if (type == PT_FILE)
	{
		if (buf)
			picture->spec.picture.buf = strdup(buf);//pic;
		else
			picture->spec.picture.buf = NULL;
		picture->spec.picture.size = 0;
	}
	else
	{
		picture_buffer_t* pic = (picture_buffer_t*)buf;
		picture->spec.picture.buf = pic->start;
		picture->spec.picture.size = pic->size;
	}

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(picture);
	}
	return picture;
}


/**
 * Reset picture data.
 *
 * @param picture
 *       Picture widget handle.
 * @param type
 *      picture data type. currently the only available value is:
 *        PICTURE_TYPE_FILE: parameter pic is the path to the picture file (char *)
 * @param pic
 *      picture data pointer, see type definition.
 *
 */
int NeuxPictureSetPic(PICTURE * picture, PICTURE_TYPE type, const void * buf)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	int ret;

	if (!buf)
		return -1;

	if (me->spec.picture.type == PT_FILE &&
		me->spec.picture.buf != 0)
		free(me->spec.picture.buf);

	me->spec.picture.type = type;
	if (type == PT_FILE)
	{
		me->spec.picture.size = 0;
		me->spec.picture.buf = strdup(buf);
	}
	else
	{
		picture_buffer_t* pic = (picture_buffer_t*)buf;
		me->spec.picture.buf = pic->start;
		me->spec.picture.size = pic->size;
	}

	ret = NxInitPicture(me);
	if (type == PT_BUFFER)
    {
	    if (me->spec.picture.buf != NULL)
	    	free(me->spec.picture.buf);
	    free((void *)buf);
	}
	if (ret == 1)
	{
		NxPictureDraw(me);
		return 0;
	}
	else
		return ret;
}


/**
 * Zoom in / out the picture.
 *
 * @param picture
 *       Picture widget handle.
 * @param zoom
 *       zoom factor.
 *          zoomx25%
 *
 */
void NeuxPictureZoom(PICTURE * picture, int zoom)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	if (zoom == -1)
		NxNeuxPictureZoomIn(me);
	else if (zoom == 1)
		NxNeuxPictureZoomOut(me);
}

/**
 * Revert from zoom state.
 *
 * @param picture
 *       Picture widget handle.
 *
 */
void NeuxPictureRevert(PICTURE * picture)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	NxNeuxPictureRevert(me);
}

/**
 * Pan display.
 *
 * @param picture
 *       Picture widget handle.
 * @param vertical
 *       y tab move
 * @param horizon
 *	   x tab move
 *
 */
void NeuxPicturePanview(PICTURE * picture, int vertical, int horizon)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	if (abs(vertical) > 1 || abs(horizon) > 1)
		return;
	NxNeuxPicturePanview(me, vertical, horizon);
}

/**
 * picture is in zoom statue.
 *
 * @param picture
 *       Picture widget handle.
 * @return
 *       zoom statue.
 *
 *
 */
int NeuxPictureIsZoomed(PICTURE * picture)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	return me->spec.picture.zoom;
}

void NeuxPictureGetProp(PICTURE * picture, picture_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	prop->transparent = me->spec.picture.transparent;
	prop->zoomable = me->spec.picture.zoomable;
}

void NeuxPictureSetProp(PICTURE * picture, const picture_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)picture;
	GR_WM_PROPERTIES props;

	GrGetWMProperties (me->base.wid, &props);
	props.flags = GR_WM_FLAGS_PROPS;
	if (prop->transparent)
		props.props |= GR_WM_PROPS_NOBACKGROUND;
	else
		props.props = GR_WM_PROPS_NODECORATE;
	GrSetWMProperties (me->base.wid, &props);

	me->spec.picture.transparent = prop->transparent;
	me->spec.picture.zoomable = prop->zoomable;
}
