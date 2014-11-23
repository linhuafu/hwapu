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
 * Widgets module source.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-04-03 MG
 * 1) Initial creation. ----------------------------------- 2006-05-19 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"

#define SAFETY_CHECK(w) do{if(!(w)){\
	  WARNLOG("!!-- NULL Widget Detected!");return;}}while(0)
#define SAFETY_CHECK_WITH_RETURN(w, ret) do{if(!(w)){\
	  WARNLOG("!!-- NULL Widget Detected!");return(ret);}}while(0)

extern NX_WIDGET *mainApp;

// private routines.

// this routine realizes the container widget and all its children widgets.
// layout management is applied here.
static void
realize_widget(NX_WIDGET * widget)
{
	int ii;
	NX_CONTAINER * ctn;
	NX_WIDGET * ww;

	//FIXME: shouldn't need this once code is finalized.
	if (widget->base.realized == GR_TRUE) return;

	DBGLOG("entering to realize widget\n");
	if (widget->base.type != WIDGET_TYPE_WINDOW)
	{
		DBGLOG("creating widget: %p type: %d\n", widget, widget->base.type);
		NxCreateWidget(widget);
	}

	//check to do container layout
	if ((widget->base.type == WIDGET_TYPE_WINDOW) ||
		(widget->base.type == WIDGET_TYPE_PANEL))
	{
		DBGLOG("entering to layout widget\n");
		NxLayout(widget);

		ctn = &widget->spec.container;

		for (ii = 0; ii < ctn->base.childNum; ii++)
		{
			ww = ctn->base.childTab[ii];
			realize_widget(ww);
		}
		widget->base.realized = GR_TRUE;
	}

	// by now, widget is realized, show it if it is visible.
	if (GR_TRUE == widget->base.visible)
		NxSetWidgetVisible(widget, GR_TRUE);
}

/*generic event call back function */

void
OnKeyDown(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	KEY__ tempkey;
	tempkey.ch = sender->base.event.keystroke.ch;
	tempkey.hotkey = sender->base.event.keystroke.hotkey;
	(*(KeydownCbfptr)ptr)(sender->base.wid, tempkey);
}

void
OnKeyUp(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	KEY__ tempkey;
	tempkey.ch = sender->base.event.keystroke.ch;
	tempkey.hotkey = sender->base.event.keystroke.hotkey;
	(*(KeyupCbfptr)ptr)(sender->base.wid, tempkey);
}

void
OnFocusIn(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(FocusinCbfptr)ptr)(sender->base.wid);
}

void
OnFocusOut(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(FocusoutCbfptr)ptr)(sender->base.wid);
}

void
OnExposure(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(ExposureCbfptr)ptr)(sender->base.wid);
}

void
OnTimer(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(TimerCbfptr)ptr)(sender->base.wid);
}

void
OnDestroy(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(DestroyCbfptr)ptr)(sender->base.wid);
}

void
OnChange(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(ChangeCbfptr)ptr)(sender->base.wid);
}

void
OnHotplug(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(HotplugCbfptr)ptr)(sender->base.wid,
			sender->base.event.hotplug.device,
			sender->base.event.hotplug.index,
			sender->base.event.hotplug.action);
}

void
OnSWOn(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(SWOnCbfptr)ptr)(sender->base.event.sw.sw);
}

void
OnSWOff(NX_WIDGET *sender, DATA_POINTER ptr)
{
    SAFETY_CHECK(sender);
	(*(SWOffCbfptr)ptr)(sender->base.event.sw.sw);
}

WIDGET *
NeuxGetWidgetFromWID(int wid)
{
	return NxGetFromRegistry(wid);
}

int
NeuxWidgetWID(WIDGET * widget)
{
    SAFETY_CHECK_WITH_RETURN(widget, 0);
	return ((NX_WIDGET *)widget)->base.wid;
}

int
NeuxWidgetGC(WIDGET * widget)
{
    SAFETY_CHECK_WITH_RETURN(widget, 0);
	return ((NX_WIDGET *)widget)->base.gc;
}

void*
NeuxWidgetGetData(WIDGET * widget)
{
	return NxGetWidgetData((NX_WIDGET *)widget);
}

int
NeuxWidgetIsVisible(WIDGET * widget)
{
    SAFETY_CHECK_WITH_RETURN(widget, 0);
	return ((NX_WIDGET *)widget)->base.visible;
}

int
NeuxWidgetIsEnable(WIDGET * widget)
{
    SAFETY_CHECK_WITH_RETURN(widget, 0);
	return ((NX_WIDGET *)widget)->base.enabled;
}

int
NeuxWidgetDestroy(WIDGET ** widget)
{
    SAFETY_CHECK_WITH_RETURN(widget, 0);
	return NxDestroyWidget((NX_WIDGET **)widget);
}

/** Show / hide given widget.
 *  when showing, realize widget if widget is not already realized and its parent
 *  is realized.
 *
 * @param widget
 *        target widget.
 * @param show
 *        GR_TRUE to show widget, GR_FALSE to hide.
 */
void
NeuxWidgetShow(WIDGET * widget, int show)
{
    SAFETY_CHECK(widget);

	//DBGLOG("SHOW: widget: %p %d\n", widget, show);

	// if widget is realized already, continue to set its visibility.
	NxSetWidgetVisible((NX_WIDGET *)widget, show);
	if (((NX_WIDGET *)widget)->base.realized != GR_TRUE)
	{
		NX_WIDGET * parent;
		int realize = 0;

		// if parent is realized, realize widget immediately
		parent = NxGetWidgetParent(widget);
		if (parent)
		{
			if (parent->base.realized == GR_TRUE) realize = 1;
		}

		// realize form immediately when showing.
		if (realize || ((((NX_WIDGET *)widget)->base.type == WIDGET_TYPE_WINDOW) &&
						(show == GR_TRUE)))
		{
			//DBGLOG("ENTER: realizing widget: %p\n", widget);
			realize_widget(widget);
		}
	}
}

void
NeuxWidgetEnable(WIDGET * widget, int enable)
{
    SAFETY_CHECK(widget);
	NxSetNeuxWidgetEnabled((NX_WIDGET *)widget, enable);
}


/** Manually set widget focus.
 *  It is recommended that application only call this API when switching
 *  focus between FORMs, or under certain situation where default
 *  focus changing buttons are remapped, for example, there is a listbox
 *  on the menu where UP / DOWN button is remapped to it by default.
 *
 * @param widget
 *        Target widget.
 */
void
NeuxWidgetFocus(WIDGET * widget)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
	SAFETY_CHECK(widget);

	NxSetWidgetFocus(me);
}

/**
 * Set widget focus index within current FORM.
 *
 * @param widget
 *        Target widget.
 * @param idx
 *        focus index of target widget in its root FORM, starting from 0.
 */
void
NeuxWidgetSetFocusIdx(WIDGET * widget, int idx)
{
	SAFETY_CHECK(widget);
	NxSetWidgetFocusIdx(widget, idx);
}


/** return current focus widget of given form / window.
 *
 * @param parent
 *        parent form / window.
 * @return
 *        widget that has the focus of given form / window.
 *
 */
WIDGET *
NeuxGetActiveWidget(WIDGET * parent)
{
	if (parent)
	{
		if (WIDGET_TYPE_WINDOW == ((NX_WIDGET *)parent)->base.type)
			return ((NX_WIDGET *)parent)->spec.container.spec.window.activewidget;
	}

#if 0
	if (parent)
	{
		if (WIDGET_TYPE_WINDOW == ((NX_WIDGET *)parent)->base.type)
		{
			NX_WINDOW * w = &((NX_WIDGET *)parent)->spec.container.spec.window;
			if (w->focusTabLen && (w->focusIdx < w->focusTabLen))
			{
				return(w->focus[w->focusIdx]);
			}
		}
	}
#endif

	return NULL;
}

void
NeuxWidgetMove(WIDGET * widget, int x, int y)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
    SAFETY_CHECK(widget);
	me->base.posx = x;
	me->base.posy = y;
	GrMoveWindow(me->base.wid, x, y);
}

void
NeuxWidgetResize(WIDGET * widget, int w, int h)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
    SAFETY_CHECK(widget);
	me->base.width = w;
	me->base.height = h;
	GrResizeWindow(me->base.wid, w, h);
}

void
NeuxWidgetGetSize(WIDGET * widget, int *w, int *h)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
    SAFETY_CHECK(widget);
    GR_WINDOW_INFO infoptr;
    GrGetWindowInfo(me->base.wid, &infoptr);
	*w = infoptr.width;
	*h = infoptr.height;
}

void
NeuxWidgetRaise(WIDGET * widget)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
    SAFETY_CHECK(widget);
	GrRaiseWindow(me->base.wid);
}


void
NeuxWidgetGetProp(WIDGET *widget, widget_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;
    SAFETY_CHECK(widget);
	prop->fgcolor = me->base.fgcolor;
	prop->bgcolor = me->base.bgcolor;

}

void
NeuxWidgetSetProp(WIDGET *widget, const widget_prop_t *prop)
{
	GR_WM_PROPERTIES props;
	NX_WIDGET *me = (NX_WIDGET *)widget;

    SAFETY_CHECK(widget);

	me->base.fgcolor = prop->fgcolor;
	me->base.bgcolor = prop->bgcolor;

	NxSetGCForeground(me->base.gc, me->base.fgcolor);
	NxSetGCBackground(me->base.gc, me->base.bgcolor);

	GrSetWindowBackgroundColor(me->base.wid, me->base.bgcolor);
}

void
NeuxWidgetRedraw(WIDGET *widget)
{
    SAFETY_CHECK(widget);
	NxRedrawWidget(widget);
}

void NeuxGetTextSize (WIDGET *widget, char *const text, int len, int *w, int *h, int *b)
{
	NxGetGCTextSize (((NX_WIDGET *)widget)->base.gc, text, len, MWTF_UTF8, w, h, b);
}

int
NeuxWidgetGetFontSize(WIDGET *widget)
{
	SAFETY_CHECK_WITH_RETURN(widget, 0);
	return ((NX_WIDGET *)widget)->base.fontsize;
}

void
NeuxWidgetSetFont(WIDGET *widget, const char *fontname, int fontsize)
{
	NX_WIDGET *me = (NX_WIDGET *)widget;

    SAFETY_CHECK(widget);

	if (me->base.fontid > 0)
		NxPutFontID(me->base.fontid);
	if (me->base.fontname)
		free(me->base.fontname);

	if (fontname == NULL || *fontname == '\0')
		me->base.fontname = NULL;
	else
		me->base.fontname = strdup(fontname);
	me->base.fontsize = fontsize;

	if (me->base.fontname != NULL) {
		me->base.fontid = NxGetFontID(me->base.fontname, me->base.fontsize);
		GrSetGCFont (me->base.gc, me->base.fontid);
	} else
		me->base.fontid = 0;
}


BITMAP *
NeuxFontToBitmap(const char *fontname, const char *fonticon, int fontsize, int *bpwidth, int *bpheight)
{
	GR_BITMAP *rBmp;
	GR_WINDOW_ID pixmap;
	GR_FONT_ID font;
	GR_SIZE		width;		/* width of character */
	GR_SIZE		height;		/* height of character */
	GR_SIZE		base;		/* height of baseline */
	GR_GC_ID gc;

	gc = GrNewGC();

	font = NxGetFontID(fontname, fontsize);
	GrSetFontAttr(font, GR_TFANTIALIAS | GR_TFKERNING, 0);

	GrSetGCFont(gc, font);
	GrGetGCTextSize(gc, (char *)fonticon, -1, GR_TFASCII | GR_TFTOP, &width, &height, &base);
	pixmap = GrNewPixmap(width, height, NULL);
	*bpwidth = width;
	*bpheight = height;
	GrText(pixmap, gc, 0, 0, (char *)fonticon, -1, GR_TFASCII | GR_TFTOP);
	rBmp = GrNewBitmapFromPixmap(pixmap, 0, 0, width, height);
	//GrCopyArea(listbox1->base.wid, listbox1->base.gc, 300, 10, 100, 100, pixmap, 0, 0, 0);

	NxPutFontID(font);
	GrDestroyWindow(pixmap);
	GrDestroyGC(gc);
	return rBmp;
}

void NeuxWidgetShadow(WIDGET *widget)
{
#ifndef FASTGUI
	NX_WIDGET *me = (NX_WIDGET *)widget;
	NX_WIDGET *pp;

    SAFETY_CHECK(widget);

	//pp = NxGetFromRegistry(me->base.pid);
	pp = me->parent;
	if (pp)
		if (pp->base.visible && me->base.visible)
			NxMakeShadow(pp->base.wid, pp->base.gc, me->base.wid, TRUE);
#endif
}

/**
 * Shamelessly stolen from Nano-X.h and modified a little bit. mgao@neuros
 * Please look into that file for very detailed documentation
 */
GR_BOOL NeuxWidgetGrabKey(WIDGET *widget, GR_KEY key, KEY_GRAB_TYPE grabtype)
{
	SAFETY_CHECK_WITH_RETURN(widget, 0);
	return GrGrabKey(NeuxWidgetWID(widget), key, grabtype);
}

GR_BOOL NeuxWidgetUngrabKey(WIDGET *widget, GR_KEY key)
{
	SAFETY_CHECK_WITH_RETURN(widget, 0);
	GrUngrabKey(NeuxWidgetWID(widget), key);
	return TRUE;
}

IMAGEID NeuxLoadImageFromFile(const char *filename)
{
	return GrLoadImageFromFile((char*)filename, 0);
}

void NeuxFreeImage(IMAGEID ID)
{
	if (ID > 0)
		GrFreeImage(ID);
}

GR_BOOL NeuxWidgetSetBgPixmap(WIDGET *widget, const char *filename)
{
	SAFETY_CHECK_WITH_RETURN(widget, 0);
	NX_WIDGET *me = (NX_WIDGET *)widget;
	GR_IMAGE_ID bg = GrLoadImageFromFile((char*)filename, 0);
	if (bg)
	{
		if (me->base.bgpixmap)
			GrDestroyWindow(me->base.bgpixmap);
		me->base.bgpixmap = GrNewPixmap(me->base.width, me->base.height, NULL);
		if (me->base.bgpixmap)
		{
			GrDrawImageToFit(me->base.bgpixmap, me->base.gc, 0, 0, me->base.width, me->base.height, bg);
			GrSetBackgroundPixmap(me->base.wid, me->base.bgpixmap, GR_BACKGROUND_CENTER);
			GrFreeImage(bg);
			return TRUE;
		}
		else
		{
			GrFreeImage(bg);
			return FALSE;
		}
	}
	else
		return FALSE;
}

GR_BOOL NeuxWidgetUseParentBgPixmap(WIDGET *widget)
{
	SAFETY_CHECK_WITH_RETURN(widget, 0);
	NX_WIDGET *me = (NX_WIDGET *)widget;

	if (me->parent->base.bgpixmap > 0)
	{
		if (me->base.bgpixmap)
			GrDestroyWindow(me->base.bgpixmap);
		me->base.bgpixmap = GrNewPixmap(me->base.width, me->base.height, NULL);
		if (me->base.bgpixmap)
		{
			GrCopyArea(me->base.bgpixmap, me->base.gc, 0, 0, me->base.width, me->base.height,
					   me->parent->base.bgpixmap, me->base.posx, me->base.posy, 0);
			GrSetBackgroundPixmap(me->base.wid, me->base.bgpixmap, GR_BACKGROUND_CENTER);
			return TRUE;
		}
	}

	return FALSE;
}
