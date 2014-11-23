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
 * Widget Picture routines.
 *
 * REVISION:
 *
 * 3) Pull in WM/Desktop support. ------------------------- 2007-04-12 MG
 * 2) add zoom function------------------------------------ 2006-07-24 EY
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

#define ICON_WIDTH  15
#define ICON_HEIGHT 15
#define SCREEN_EDGE 12
#define SCALEUP(A) (A * 5 / 4)
#define SCALEDOWN(A) (A * 4 / 5)
#define LINEWIDE 3
#define MOVESTEP 5
#define THUM_MARGIN 30;

static int
CurrToPic(NX_PICTURE * pic, int x)
{
	return x * pic->pix.width / pic->curr.width;
}

static int
PicToThumb(NX_PICTURE * pic, int x)
{
	return x * pic->thumb.width / pic->pix.width;
}

static void
InitPicture(NX_WIDGET * picture)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	GR_RECT wd, ws;
	int width, height;

	DBGLOG("InitOriginalSize\n");

	pic->zoom = 0;
	pic->thumb.width = base->width / 4;
	pic->thumb.height = base->height / 4;
	pic->visible.x1 = 0;
	pic->visible.y1 = 0;
	pic->visible.x2 = pic->pix.width - 1;
	pic->visible.y2 = pic->pix.height - 1;

	ws.x = 0;
	ws.y = 0;
	ws.width = base->width;
	ws.height = base->height;
	width = pic->visible.x2 - pic->visible.x1;
	height = pic->visible.y2 - pic->visible.y1;
	if (width < base->width && height < base->height)
	{
		wd.width = width;
		wd.height = height;
	}
	else
	{
		NxResize(&wd, &ws, width, height);
	}
	pic->orig.width = wd.width;
	pic->orig.height = wd.height;
	pic->curr.width = wd.width;
	pic->curr.height = wd.height;
}


static void
DrawThumb(NX_WIDGET *picture)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	GR_RECT ws, wd;
	ws.width  = base->width / 4;
	ws.height = base->height / 4;
	ws.x      = base->width - ws.width - THUM_MARGIN;
	ws.y      = base->height - ws.height - THUM_MARGIN;

	NxSetGCForeground (base->gc, DKGRAY);

    GrFillRect(base->wid, base->gc, ws.x, ws.y,
			ws.width, ws.height);

	NxSetGCForeground(base->gc, LTGRAY);
	NxEnlargeRect(&wd, &ws, 1);
	GrRect(base->wid, base->gc, wd.x, wd.y,
			wd.width, wd.height);
	NxResize(&wd, &ws, pic->pix.width, pic->pix.height);
	pic->thumb.width = wd.width;
	pic->thumb.height = wd.height;


	GrDrawImageToFit(base->wid, base->gc, wd.x, wd.y,
					wd.width, wd.height, pic->imageid);

	wd.x = wd.x + PicToThumb(pic, pic->visible.x1);
	wd.y = wd.y + PicToThumb(pic, pic->visible.y1);
	wd.width = PicToThumb(pic, pic->visible.x2 - pic->visible.x1);
	wd.height = PicToThumb(pic, pic->visible.y2 - pic->visible.y1);
	NxSetGCForeground(base->gc, RED);
	NxEnlargeRect(&ws, &wd, -1 * LINEWIDE);
	NxDrawRect(picture, &ws, LINEWIDE);
	GrRect(base->wid, base->gc, wd.x, wd.y,
			wd.width, wd.height);
}

static void
GetDisplayPos(NX_WIDGET * picture, GR_RECT *rect)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;

	if (pic->curr.width <= base->width)
		rect->width = pic->curr.width;
	else
		rect->width = base->width;

	if (pic->curr.height <= base->height)
		rect->height = pic->curr.height;
	else
		rect->height = base->height;

	rect->x = (base->width - rect->width) / 2;
	rect->y = (base->height - rect->height) / 2;
}


static void
GetVisibleRect(NX_WIDGET * picture, NX_REGION* rect)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	int center;
	if (pic->curr.width < base->width)
	{
		pic->visible.x1 = 0;
		pic->visible.x2 = pic->pix.width - 1;
	}
	else
	{
		int screenwidth = CurrToPic(pic, base->width);
		if (screenwidth >= pic->pix.width)
			screenwidth = pic->pix.width - 1;
		center = (pic->visible.x1 + pic->visible.x2) / 2;
		pic->visible.x1 = center - screenwidth / 2;
		pic->visible.x2 = center + screenwidth / 2;
		if (pic->visible.x1 < 0)
		{
			pic->visible.x2 -= pic->visible.x1;
			pic->visible.x1 = 0;
		}
		if (pic->visible.x2 > pic->pix.width - 1)
		{
			pic->visible.x1 -= (pic->visible.x2 - pic->pix.width + 1);
			pic->visible.x2 = pic->pix.width - 1;
		}
	}

	if (pic->curr.height < base->height)
	{
		pic->visible.y1 = 0;
		pic->visible.y2 = pic->pix.height - 1;
	}
	else
	{
		int screenheight = CurrToPic(pic, base->height);
		if (screenheight >= pic->pix.height)
			screenheight = pic->pix.height - 1;
		center = (pic->visible.y1 + pic->visible.y2) / 2;
		pic->visible.y1 = center - screenheight / 2;
		pic->visible.y2 = center + screenheight / 2;
		if (pic->visible.y1 < 0)
		{
			pic->visible.y2 -= pic->visible.y1;
			pic->visible.y1 = 0;
		}
		if (pic->visible.y2 > pic->pix.height - 1)
		{
			pic->visible.y1 -= (pic->visible.y2 - pic->pix.height + 1);
			pic->visible.y2 = pic->pix.height - 1;
		}
	}
}

void
NxPictureCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	GrSetGCUseBackground(widget->base.gc, GR_FALSE);

	if (widget->base.height == 0)
		widget->base.height = PICTURE_HEIGHT;
	if (widget->base.width == 0)
		widget->base.width = PICTURE_WIDTH;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Picture%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	NxInitPicture(widget);

	widget->spec.picture.cb_exposure.fp			= NULL;
	widget->spec.picture.cb_exposure.inherit	= GR_TRUE;

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOFOCUS;
	props.props |= GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties (widget->base.wid, &props);

	GrSelectEvents (widget->base.wid, GR_EVENT_MASK_EXPOSURE);
}


void
NxPictureExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("pexprosureevent\n");
	if (widget->spec.picture.cb_exposure.fp != NULL)
		(*(widget->spec.picture.cb_exposure.fp))(widget, widget->spec.picture.cb_exposure.dptr);
	if (widget->spec.picture.cb_exposure.inherit)
		NxPictureDraw(widget);
	DBGLOG("pexprosureevent returned\n");
}

void
NxPictureDirectDraw (NX_WIDGET * picture)
{
//cancel zoom in and zoom out, support transparently show image.
	if (picture->spec.picture.bredrawflag)
	{
		GrUnmapWindow(picture->base.wid);
		GrMapWindow(picture->base.wid);
		GrDrawImageToFit(picture->base.wid, picture->base.gc, 0, 0,
						picture->base.width, picture->base.height,
						picture->spec.picture.imageid);
	}
	picture->spec.picture.bredrawflag = !picture->spec.picture.bredrawflag;
}


void
NxPictureZoomDraw (NX_WIDGET * picture)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	GR_RECT wd;

	GetVisibleRect(picture, &pic->visible);
	GetDisplayPos(picture, &wd);

	GrClearWindow(base->wid, GR_FALSE);
	DBGLOG("pic width = %d, height = %d\n", pic->pix.width, pic->pix.height);
	DBGLOG("x = %d, y = %d, width = %d, height = %d\n", pic->visible.x1,
		pic->visible.y1, pic->visible.x2 - pic->visible.x1,
		pic->visible.y2 - pic->visible.y1);
	GrDrawImagePartToFit(base->wid, base->gc, wd.x, wd.y, wd.width, wd.height,
					pic->visible.x1, pic->visible.y1,
					pic->visible.x2 - pic->visible.x1,
					pic->visible.y2 - pic->visible.y1, pic->imageid);

	if (pic->zoom > 0)
		DrawThumb(picture);
}

void
NxPictureDraw (NX_WIDGET * picture)
{
	if (picture->spec.picture.imageid <= 0)
		return;
	if (!NxGetWidgetAbsVisible(picture))
		return;

	if (picture->spec.picture.zoomable)
		NxPictureZoomDraw(picture);
	else
		NxPictureDirectDraw(picture);
}


void
NxNeuxPictureZoomOut (NX_WIDGET * picture)
{
	NX_PICTURE * pic = &picture->spec.picture;

	pic->zoom -= 1;
	if (pic->zoom < -4 || pic->zoom == 0)
	{
		InitPicture(picture);
	}
	else
	{
		pic->curr.width = SCALEDOWN(pic->curr.width);
		pic->curr.height = SCALEDOWN(pic->curr.height);
	}

	NxPictureDraw(picture);
}

void
NxNeuxPictureZoomIn(NX_WIDGET * picture)
{
	NX_PICTURE * pic = &picture->spec.picture;

	pic->zoom+= 1;
	if (pic->zoom > 4 || pic->zoom == 0)
	{
		InitPicture(picture);
	}
	else
	{
		pic->curr.width = SCALEUP(pic->curr.width);
		pic->curr.height = SCALEUP(pic->curr.height);
	}

	NxPictureDraw(picture);
}

void
NxNeuxPictureRevert(NX_WIDGET * picture)
{
	InitPicture(picture);
	picture->spec.picture.zoom = 0;
	NxPictureDraw(picture);
}

void
NxNeuxPicturePanview(NX_WIDGET *picture, int vertical, int horizon)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	int x, y;
	int width, height, offsetx, offsety;
	height = pic->visible.y2 - pic->visible.y1;
	width = pic->visible.x2 - pic->visible.x1;

	offsetx = 0;
	offsety = 0;
	if (picture->spec.picture.zoom == 0)
		return;
	if (horizon == 1)
	{
		x = pic->visible.x2 + width / MOVESTEP;
		if (x > pic->pix.width)
			offsetx = pic->pix.width - pic->visible.x2;
		else
			offsetx = width / MOVESTEP;
	}
	else if (horizon == -1)
	{
		x = pic->visible.x1 - width / MOVESTEP;
		if (x < 0)
			offsetx = -1 * pic->visible.x1;
		else
			offsetx = -1 * height / MOVESTEP;
	}
	if (vertical == 1)
	{
		y = pic->visible.y2 + height / MOVESTEP;
		if (y > pic->pix.height)
			offsety = pic->pix.height - pic->visible.y2;
		else
			offsety = height / MOVESTEP;
	}
	else if (vertical == -1)
	{
		y = pic->visible.y1 - height / MOVESTEP;
		if (y < 0)
			offsety = -1 * pic->visible.y1;
		else
			offsety = -1 * height / MOVESTEP;
	}
	if (abs(offsetx) <= 1 && abs(offsety) <= 1)
		return;
	pic->visible.x1 += offsetx;
	pic->visible.x2 += offsetx;
	pic->visible.y1 += offsety;
	pic->visible.y2 += offsety;
	GrClearWindow(base->wid, GR_FALSE);
	NxPictureDraw(picture);
}

void
NxPictureDestroy(NX_WIDGET *widget)
{
	if (widget->spec.picture.imageid > 0)
		GrFreeImage(widget->spec.picture.imageid);
	if (widget->spec.picture.buf != NULL&&
		widget->spec.picture.type == PT_FILE)
		free(widget->spec.picture.buf);
	NxDeleteFromRegistry(widget);
	return;
}

int
NxInitPicture(NX_WIDGET *picture)
{
	struct widgetbase * base = &picture->base;
	NX_PICTURE * pic = &picture->spec.picture;
	GR_IMAGE_INFO iinfo;

	if (base->type != WIDGET_TYPE_PICTURE)
		return -1;
	if (pic->buf == NULL)
		return -1;

	if (pic->imageid > 0)
		GrFreeImage(pic->imageid);

	if (pic->type == PT_BUFFER)
		pic->imageid = GrLoadImageFromBuffer(pic->buf, pic->size, 0);
	else
		pic->imageid = GrLoadImageFromFile(pic->buf, 0);
	if (pic->imageid == 0)
	{
		return -2;
	}

	GrGetImageInfo(pic->imageid, &iinfo);
	pic->pix.width = iinfo.width;
	pic->pix.height = iinfo.height;
	pic->bredrawflag = TRUE;

	InitPicture(picture);

	return 1;
}

void
NxEnlargeRect(GR_RECT* dest,GR_RECT* source,int range)
{
	dest->x = source->x-range;
	dest->y = source->y-range;
	dest->width = source->width+range*2;
	dest->height = source->height+range*2;
}

void
NxDrawRect(NX_WIDGET *widget,GR_RECT* rect,int wide)
{
	struct widgetbase * base = &widget->base;
	int i;
	GR_RECT dest;

	for(i=0;i<wide;i++)
	{
		NxEnlargeRect(&dest,rect,i+1);
		GrRect(base->wid, base->gc, dest.x, dest.y, dest.width,dest.height);
	}
}

void
NxResize(GR_RECT* dest,GR_RECT* source,int width,int height)
{
	int pb=width*source->height;
	int db=height*source->width;

	if(pb < db)
	{
		dest->width = width*source->height/height;
		dest->height = source->height;
		dest->x = source->x + (source->width - dest->width)/2;
		dest->y = source->y;
	}
	else
	{
		dest->height = height*source->width/width;
		dest->width = source->width;
		dest->y = source->y + (source->height - dest->height)/2;
		dest->x = source->x;
	}
}
