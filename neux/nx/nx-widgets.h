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
 * Widget header.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXWIDGET_H_
#define _NXWIDGET_H_

#include <unistd.h>
#include <signal.h>

#ifndef MWPIXEL_FORMAT
# define MWPIXEL_FORMAT	MWPF_TRUECOLOR565
#endif
#define MWINCLUDECOLORS
#include "nano-X.h"

typedef enum {
	WIDGET_TYPE_APPLICATION = 1,
	WIDGET_TYPE_WINDOW,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_LISTBOX,
	WIDGET_TYPE_TIMER,
	WIDGET_TYPE_LABEL,
	WIDGET_TYPE_PICTURE,
	WIDGET_TYPE_PANEL,
	WIDGET_TYPE_LINE,
	WIDGET_TYPE_PROGRESSBAR,
	WIDGET_TYPE_SCROLLBAR,
} NX_WIDGET_TYPE;

struct scrollbar
{
	GR_SIZE	 width;
	GR_COLOR fgcolor;
	GR_COLOR bgcolor;
	GR_COLOR bordercolor;
};

struct nxwidget;
#include "nx-base.h"
#include "nx-application.h"
#include "nx-window.h"
#include "nx-button.h"
#include "nx-listbox.h"
#include "nx-timer.h"
#include "nx-label.h"
#include "nx-picture.h"
#include "nx-panel.h"
#include "nx-line.h"
#include "nx-progressbar.h"
#include "nx-scrollbar.h"

#define FASTGUI

#define WIDGET_NAME_LENGTH			20

enum EM_ORIENTATION_MODE
{
	omHorizontal,
	omVertical
};

union containerspecial
{
	NX_WINDOW        window;
	NX_PANEL         panel;
};

typedef struct
{
	struct containerbase    base;
	union  containerspecial spec;
} NX_CONTAINER;

union widgetspecial
{
	NX_APPLICATION   application;
	NX_CONTAINER     container;
	//NX_WINDOW        window;
	NX_BUTTON        button;
	NX_LISTBOX       listbox;
	NX_TIMER         timer;
	NX_LABEL         label;
	NX_PICTURE       picture;
	//	NX_PANEL         panel;
	NX_LINE          line;
	NX_PROGRESSBAR   progressbar;
	NX_SCROLLBAR     scrollbar;
};

struct widgetcheck
{
  short runningId;
  short checkId;
};

struct nxwidget
{
	struct widgetbase     base;
	union  widgetspecial  spec;
	struct nxwidget *     parent;
	struct nxwidget *     child;
	struct nxwidget *     prev;
	struct nxwidget *     next;
    struct widgetcheck    check;
};


/*public  functions*/

//int 
//NxRegisterCallBack (CallBackStruct*, CallBackFuncPtr, DATA_POINTER);
#define NxRegisterCallBack(cbs, func, data) do {	\
		(cbs)->fp = func; (cbs)->dptr = data; } while (0)

NX_WIDGET *
NxNewWidget (int, NX_WIDGET *);
int
NxCreateWidget (NX_WIDGET *);
void
NxRedrawWidget (NX_WIDGET *);
int
NxDestroyWidget(NX_WIDGET**);
void
NxSetWidgetData(NX_WIDGET *, DATA_POINTER);
DATA_POINTER
NxGetWidgetData(NX_WIDGET *);

#define NxGetWidgetParent(w) (((NX_WIDGET *)w)->parent)
//NX_WIDGET *
//NxGetWidgetParent(NX_WIDGET *);

void
NxSetWidgetVisible(NX_WIDGET *, GR_BOOL);
void
NxSetNeuxWidgetEnabled(NX_WIDGET *, GR_BOOL);
void
NxSetWidgetFocus(NX_WIDGET *);
void
NxSetWidgetFocusIdx(WIDGET * widget, int idx);
//NX_WIDGET *
//NxGetWidgetFocus(void);
void
NxRefreshWidgetGC(NX_WIDGET *);
void
NxRefreshWidget(NX_WIDGET *);
int
NxSetWidgetBackGroundPixmap(NX_WIDGET *, GR_WINDOW_ID, GR_COORD, GR_COORD, unsigned long, int);
void
NxGetWidgetAbsXY(NX_WIDGET *, int *, int *);
GR_BOOL
NxGetWidgetAbsVisible(NX_WIDGET *);


void
NxEventResolveRoutine (GR_EVENT *);
void
NxAddToRegistry(NX_WIDGET *);
NX_WIDGET *
NxGetFromRegistry(GR_WINDOW_ID);
NX_WIDGET *
GetParent(NX_WIDGET *);
void
NxDeleteFromRegistry(NX_WIDGET *);

void
NxAlphaBmp(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_COORD dstx, GR_COORD dsty,
	GR_SIZE width, GR_SIZE height, GR_WINDOW_ID srcid,
	GR_COORD srcx, GR_COORD srcy, unsigned char alpha, long transcolor);

void
NxMakeShadow(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_WINDOW_ID srcid, GR_BOOL usealpha);

void
NxDecoratePopup(NX_WIDGET * pw, NX_WIDGET * w);

void
NxPixmapMaskColor(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_COORD dstx, GR_COORD dsty,
	GR_SIZE width, GR_SIZE height, GR_COLOR color);


int
NxCheckWidget(NX_WIDGET * w);

void
NxLayout(NX_WIDGET * widget);

//void 
//NxGetGCTextSize(GR_GC_ID gc, void *str, int count, GR_TEXTFLAGS flags,
//				GR_SIZE *retwidth, GR_SIZE *retheight, GR_SIZE *retbase);
#define NxGetGCTextSize GrGetGCTextSize

void
NxSetGCForeground(GR_GC_ID gc, GR_COLOR color);
void
NxSetGCBackground(GR_GC_ID gc, GR_COLOR color);

void
NxSbarDraw(NX_WIDGET * widget, struct scrollbar *sbar,
           int v, int rows, int range,
           int x, int y, int h);

#endif //_NXWIDGET_H_
