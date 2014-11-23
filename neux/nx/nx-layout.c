/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
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
 * Widget layout support routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-04-08 MG
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

/** Apply layout management to given container widget.
 *
 * @param widget
 *        Target container widget.
 *
 */
void NxLayout(NX_WIDGET * widget)
{
	NX_CONTAINER * ctn;
	int ii, x, y, delta;
	NX_WIDGET * ww;

	if ((widget->base.type != WIDGET_TYPE_WINDOW) &&
		(widget->base.type != WIDGET_TYPE_PANEL))
	{
		WARNLOG("I am not a container...\n");
		return;
	}

	ctn = &widget->spec.container;

	// apply the layout logic
	// TODO: only simply box and flow layout are supported so far.
	switch (ctn->base.layout)
	{
	default:
	case NLT_BOX: // layout vertically
		DBGLOG("BOX NxLayout: childnum = %d\n", ctn->base.childNum);
		y = 0;
		for (ii = 0; ii < ctn->base.childNum; ii++)
		{
			ww = ctn->base.childTab[ii];
			y += ww->base.height;
			//DBGLOG("BOX NxLayout: ww: %p, y = %d\n", ww, y);
		}
		delta = (widget->base.height > y) ? (widget->base.height - y) / (ctn->base.childNum + 1) : 0;
		//DBGLOG("BOX NxLayout: delta = %d, H = %d\n", delta, widget->base.height);
		y = delta;
		for (ii = 0; ii < ctn->base.childNum; ii++)
		{
			ww = ctn->base.childTab[ii];
			ww->base.posx = (-1 == ww->base.posx) ?
				(widget->base.width - ww->base.width) / 2 : ww->base.posx;
			ww->base.posy = y;
			//DBGLOG("BOX NxLayout: w: %p, x = %d, y = %d\n", ww, ww->base.posx, ww->base.posy);
			y += delta + ww->base.height;
		}
		break;

	case NLT_FLOW: // layout horizontally
		DBGLOG("FLOW NxLayout: childnum = %d\n", ctn->base.childNum);
		x = 0;
		for (ii = 0; ii < ctn->base.childNum; ii++)
		{
			ww = ctn->base.childTab[ii];
			x += ww->base.width;
		}
		delta = (widget->base.width > x) ? (widget->base.width - x) / (ctn->base.childNum + 1) : 0;
		x = delta;
		for (ii = 0; ii < ctn->base.childNum; ii++)
		{
			ww = ctn->base.childTab[ii];
			ww->base.posx = x;
			ww->base.posy = (-1 == ww->base.posy) ?
				(widget->base.height - ww->base.height) / 2 : ww->base.posy;
			x += delta + ww->base.width;
		}
		break;

	case NLT_GRIDBAG:
	case NLT_GRID:
	case NLT_SPRING:
	case NLT_BORDER:
	case NLT_CARD:
		DBGLOG("layout [%d] not supported.\n", ctn->base.layout);
		break;
	case NLT_NULL_LAYOUT:
		WARNLOG("No layout style specified.\n");
		break;
	}
}
