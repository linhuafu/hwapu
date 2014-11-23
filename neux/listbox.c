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
 * Listbox widget support routines.
 *
 * REVISION:
 *
 * 2) Implemnet function. ----------------------------------- 2006-05-19 EY
 * 1) Initial creation. ----------------------------------- 2006-05-04 MG
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"


/**
 * Function creates listbox widget.
 *
 * @param owner
 *      listbox owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param getItem
 *      function pointer to return item string.
 *       int getItem(int item, void * text)
 *         item: -1  return total number of entries.
 *                   'text':  don't care.
 *              >= 0 return 0 if successful.
 *                   '*text': points to item text.
 * @param getIcon
 *      function pointer to return item icons if available.
 *       int getIcon(int item, void * icon)
 *         item: -1  error
 *              >= 0 return 0 if successful.
 *                   '*icon': points to bitmap data.
 * @return
 *      listbox widget if successful, otherwise NULL.
 *
 */

LISTBOX * NeuxListboxNew(WIDGET * owner,
					 int x, int y, int w, int h,
					 GetItemCbfptr getItem,
					 GetIconCbfptr getIcon,
					 GetItemFgcolorCbfptr getitemfgcolor)
{
    NX_WIDGET * listbox;

	listbox = NxNewWidget(WIDGET_TYPE_LISTBOX, (NX_WIDGET *)owner);
	listbox->base.posx = x;
	listbox->base.posy = y;
	listbox->base.width = w;
	listbox->base.height = h;

	listbox->spec.listbox.cb_geticon = getIcon;
	listbox->spec.listbox.cb_getitem = getItem;
	listbox->spec.listbox.cb_getitemfgcolor = getitemfgcolor;
	listbox->spec.listbox.sbar.width = SBAR_WIDTH;
	listbox->spec.listbox.useicon = TRUE;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(listbox);
	}
	return listbox;
}

/** Function returns currently topitem of the listbox.
 *
 * @param listbox
 *       target listbox.
 * @return
 *       top item.
 */
int NeuxListboxTopItem(LISTBOX * listbox)
{
	NX_LISTBOX *me = &((NX_WIDGET *)listbox)->spec.listbox;
    return me->topitem;
}

/** Function returns currently selected item of the specified listbox.
 *
 * @param listbox
 *       target listbox.
 * @return
 *       selected item.
 */
int NeuxListboxSelectedItem(LISTBOX * listbox)
{
	NX_LISTBOX *me = &((NX_WIDGET *)listbox)->spec.listbox;
    return (me->topitem + me->indexitem);
}

void
NeuxListboxRecalcPitems (LISTBOX * listbox)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
	NxListboxRecalcPitems(me);
}

void NeuxListboxSetIndex(LISTBOX * listbox, int index)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
    NxNeuxListboxSetIndex(me, index);
}

void NeuxListboxSetItem(LISTBOX * listbox, int index, int top)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
    NxNeuxListboxSetItem(me, index, top);
}

/** Function sets call back of the listbox.
 *
 * @param listbox
 *       target listbox.
 * @param cbId
 *       call back function ID.
 * @param fptr
 *       call back function pointer.
 *
 */
void NeuxListboxSetCallback(LISTBOX * listbox, int cbId, void * fptr)
{
	NX_LISTBOX *me = &((NX_WIDGET *)listbox)->spec.listbox;

	switch (cbId)
	{
	case CB_KEY_DOWN:
		NxRegisterCallBack(&me->cb_keydown, OnKeyDown, fptr);
		break;
	case CB_KEY_UP:
		NxRegisterCallBack(&me->cb_keyup, OnKeyUp, fptr);
		break;
	case CB_EXPOSURE:
		NxRegisterCallBack(&me->cb_exposure, OnExposure, fptr);
		break;
	case CB_FOCUS_IN:
		NxRegisterCallBack(&me->cb_focusin, OnFocusIn, fptr);
		break;
	case CB_FOCUS_OUT:
		NxRegisterCallBack(&me->cb_focusout, OnFocusOut, fptr);
		break;
	}
}

void
NeuxListboxGetProp(LISTBOX * listbox, listbox_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;

	prop->useicon = me->spec.listbox.useicon;
	prop->selectedcolor = me->spec.listbox.selectedcolor;
	prop->selectedfgcolor = me->spec.listbox.selectedfgcolor;
	prop->ismultiselec = me->spec.listbox.ismultiselec;
	prop->selectmarkcolor = me->spec.listbox.selectmarkcolor;

	prop->itemmargin.left = me->spec.listbox.itemmargin.left;
	prop->itemmargin.top = me->spec.listbox.itemmargin.top;
	prop->itemmargin.right = me->spec.listbox.itemmargin.right;
	prop->itemmargin.bottom = me->spec.listbox.itemmargin.bottom;

	prop->textmargin.left = me->spec.listbox.textmargin.left;
	prop->textmargin.top = me->spec.listbox.textmargin.top;
	prop->textmargin.right = me->spec.listbox.textmargin.right;
	prop->textmargin.bottom = me->spec.listbox.textmargin.bottom;

	prop->iconregion.w = me->spec.listbox.iconregion.w;
	prop->iconregion.h = me->spec.listbox.iconregion.h;

	prop->sbar.width = me->spec.listbox.sbar.width;
	prop->sbar.fgcolor = me->spec.listbox.sbar.fgcolor;
	prop->sbar.bgcolor = me->spec.listbox.sbar.bgcolor;
	prop->sbar.bordercolor = me->spec.listbox.sbar.bordercolor;

	prop->indexitem = me->spec.listbox.indexitem;
	prop->itemheight = me->spec.listbox.itemheight;
	prop->graphictype = me->spec.listbox.graphictype;

	prop->bScroll = me->spec.listbox.bScroll;
	prop->bScrollBidi = me->spec.listbox.bScrollBidi;
	prop->scroll_interval = me->spec.listbox.scroll_interval;
	prop->scroll_endingscrollcnt = me->spec.listbox.scroll_endingscrollcnt;

	prop->bWrapAround = me->spec.listbox.bWrapAround;
}

void
NeuxListboxSetProp(LISTBOX * listbox, const listbox_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;

	me->spec.listbox.useicon = prop->useicon;
	me->spec.listbox.selectedcolor = prop->selectedcolor;
	me->spec.listbox.selectedfgcolor = prop->selectedfgcolor;
	me->spec.listbox.ismultiselec = prop->ismultiselec;
	me->spec.listbox.selectmarkcolor = prop->selectmarkcolor;

	me->spec.listbox.itemmargin.left = prop->itemmargin.left;
	me->spec.listbox.itemmargin.top = prop->itemmargin.top;
	me->spec.listbox.itemmargin.right = prop->itemmargin.right;
	me->spec.listbox.itemmargin.bottom = prop->itemmargin.bottom;

	me->spec.listbox.iconregion.w = prop->iconregion.w;
	me->spec.listbox.iconregion.h = prop->iconregion.h;

	me->spec.listbox.textmargin.left = prop->textmargin.left;
	me->spec.listbox.textmargin.top = prop->textmargin.top;
	me->spec.listbox.textmargin.right = prop->textmargin.right;
	me->spec.listbox.textmargin.bottom = prop->textmargin.bottom;

	me->spec.listbox.sbar.width = prop->sbar.width;
	me->spec.listbox.sbar.fgcolor = prop->sbar.fgcolor;
	me->spec.listbox.sbar.bgcolor = prop->sbar.bgcolor;
	me->spec.listbox.sbar.bordercolor = prop->sbar.bordercolor;

	me->spec.listbox.graphictype = prop->graphictype;

	me->spec.listbox.bScroll = prop->bScroll;
	me->spec.listbox.bScrollBidi = prop->bScrollBidi;
	me->spec.listbox.scroll_interval = prop->scroll_interval;
	me->spec.listbox.scroll_endingscrollcnt = prop->scroll_endingscrollcnt;

	me->spec.listbox.bWrapAround = prop->bWrapAround;
}

int
NeuxListboxGetPageItems(LISTBOX * listbox)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
	return me->spec.listbox.pitems;
}

int	NeuxListboxGetMultiSelectedCount(LISTBOX * listbox)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
	return NxListboxGetMultiSelectedCount(me);
}

int NeuxListboxGetMultiSelectedData(LISTBOX * listbox, int* selectedIndexarray, int arraysize)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
	return NxListboxGetMultiSelectedData(me, selectedIndexarray, arraysize);
}

void NeuxListboxClearMultiSelected(LISTBOX * listbox)
{
	NX_WIDGET *me = (NX_WIDGET *)listbox;
	NxListboxClearMultiSelected(me);
}
