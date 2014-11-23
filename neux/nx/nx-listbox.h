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
 * Widget Listbox header.
 *
 * REVISION:
 *
 * 2) struct modify  -------------------------------------- 2006-05-26 EY
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXLISTBOX_H_
#define _NXLISTBOX_H_

#include "nx-base.h"
#include "nx-widgets.h"
#include "nxlist.h"

#define LISTBOX_WIDTH 		80
#define LISTBOX_HEIGHT		100

struct multiselect
{
	slist_t list;
	int index;
};

struct margin
{
	int left;
	int top;
	int right;
	int bottom;
};

struct region
{
	int x;
	int y;
	int w;
	int h;
};

typedef enum
{
	ltIcon,
	ltImage
} EM_LISTBOX_GRAPHICTYPE;

typedef enum
{
	SCROLL_IDLE,
	SCROLL_START,
	SCROLL_RUNNING,
	SCROLL_ENDING,
	SCROLL_REVRUNNING,
	SCROLL_REVENDING
} scroll_stat_e;

typedef struct
{
	GR_TIMER_ID scrolltimer;
	const char* text;
	int offset;
	int showlen;
	int scrollcnt;
	struct region textregion;
    scroll_stat_e nextstatus;
} scroll_info_t;

typedef struct
{
	int count;
	GR_BOOL transparent;
	GR_IMAGE_ID imageid;
	int indexitem;		// curr index of page
	int topitem;		// top item index of all items
	int pitems;			// one page items
	int numitems;		// all items
	int itemheight; 	// every item height
	struct margin textmargin;
	struct margin itemmargin;

	GR_COLOR selectedcolor;
	GR_COLOR selectedfgcolor;

	GR_BOOL ismultiselec;
	GR_COLOR selectmarkcolor;
	slist_t multiseleclist;
	int selectedcount;

	GR_BOOL useicon;
	EM_LISTBOX_GRAPHICTYPE graphictype;
	struct region iconregion;

	struct scrollbar sbar;

    GR_BOOL bScroll;   //scroll when the length of string is too long
    GR_BOOL bScrollBidi;
	int scroll_interval;
	int scroll_endingscrollcnt;
    scroll_info_t scroll_info;

	GR_BOOL bWrapAround;

	CallBackStruct cb_keydown;
	CallBackStruct cb_keyup;
	CallBackStruct cb_exposure;
	CallBackStruct cb_focusin;
	CallBackStruct cb_focusout;
	CallBackStruct cb_destroy;
	GetIconFuncPtr cb_geticon;
	GetItemFuncPtr cb_getitem;
	GetItemFgcolorFuncPtr cb_getitemfgcolor;
} NX_LISTBOX;

/*member  function */
void
NxListboxCreate (NX_WIDGET *);
void
NxListboxDestroy(NX_WIDGET *);
void
NxListboxDraw (NX_WIDGET *);

void
NxListboxRecalcPitems (NX_WIDGET * listbox);
void
NxNeuxListboxSetIndex(NX_WIDGET * listbox, int index);
void
NxNeuxListboxSetItem(NX_WIDGET * listbox, int index, int top);
int
NxListboxGetMultiSelectedCount(NX_WIDGET * listbox);
int
NxListboxGetMultiSelectedData(NX_WIDGET * listbox, int* selectedIndexarray, int arraysize);
void
NxListboxClearMultiSelected(NX_WIDGET * listbox);

/*event handler */
void
NxListboxKeyDownHandler (GR_EVENT *, NX_WIDGET *);
void
NxListboxKeyUpHandler (GR_EVENT *, NX_WIDGET *);
void
NxListboxExposureHandler (GR_EVENT *, NX_WIDGET *);
void
NxListboxFocusInHandler (GR_EVENT *, NX_WIDGET *);
void
NxListboxFocusOutHandler (GR_EVENT *, NX_WIDGET *);
void
NxListboxTimerHandler (GR_EVENT *event, NX_WIDGET *widget);
/*public function*/

#endif //_NXLISTBOX_H_
