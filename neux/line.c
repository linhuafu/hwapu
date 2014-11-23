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
 * Line widget support routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2006-05-29 EY
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"

LINE * NeuxLineNew(WIDGET * owner, int x, int y, int w, int h)
{
	NX_WIDGET * line;

	line = NxNewWidget(WIDGET_TYPE_LINE, (NX_WIDGET *)owner);
	line->base.posx = x;
	line->base.posy = y;
	line->base.width = w;
	line->base.height = h;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(line);
	}
	return line;
}
