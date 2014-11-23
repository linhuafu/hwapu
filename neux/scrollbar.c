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
 * Scrollbar widget support routines.
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
 * Function creates scrollbar widget.
 *
 * @param owner
 *      Scrollbar owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param actvItem
 *       actively viewing item
 * @param pageSize
 *       number of items in one page.
 * @param totalNum
 *       total number of items.
 * @param style
 *      0: horizontal bar, 1, vertical bar.
 * @return
 *      Scrollbar widget if successful, otherwise NULL.
 *
 */
SCROLLBAR * NeuxScrollbarNew(WIDGET * owner, int x, int y, int w, int h,
						 int actvItem, int pageSize, int totalNum, int style)
{
	NX_WIDGET * scrollbar;

	scrollbar = NxNewWidget(WIDGET_TYPE_PROGRESSBAR, (NX_WIDGET *)owner);

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(scrollbar);
	}
	return scrollbar;
}



/**
 * Function sets scrollbar controls.
 *
 * @param scrollbar
 *       Scrollbar widget handle.
 * @param actvItem
 *       actively viewing item
 * @param pageSize
 *       number of items in one page.
 * @param totalNum
 *       total number of items.
 *
 */
void NeuxScrollbarSetControls(SCROLLBAR * scrollbar, int actvItem,
						 int pageSize, int totalNum)
{

}


