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
 * Progressbar widget support routines.
 *
 * REVISION:
 *
 * 2) Implemnet function. ----------------------------------- 2006-05-19 EY
 * 1) Initial creation.   ----------------------------------- 2006-05-04 MG
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"


/*
  x, y coordinates are container widget coordinates.
 */

/**
 * Function creates progressbar widget.
 *
 * @param owner
 *      Progressbar owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param current
 *       current value.
 * @param min
 *       minimum value.
 * @param max
 *       maximum value.
 * @param style
 *      0: horizontal bar, 1, vertical bar.
 * @return
 *      Progressbar widget if successful, otherwise NULL.
 *
 */
PROGRESSBAR * NeuxProgressbarNew(WIDGET * owner, int x, int y, int w, int h,
							 int current, int min, int max, int style)
{
	NX_WIDGET * progressbar;
	progressbar = NxNewWidget(WIDGET_TYPE_PROGRESSBAR, (NX_WIDGET *)owner);
	progressbar->base.posx = x;
	progressbar->base.posy = y;
	progressbar->base.width = w;
	progressbar->base.height = h;

	progressbar->spec.progressbar.value = current;
	progressbar->spec.progressbar.min = min;
	progressbar->spec.progressbar.max = max;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(progressbar);
	}
	return progressbar;
}



/**
 * Function sets progressbar controls.
 *
 * @param progressbar
 *       Progressbar widget handle.
 * @param current
 *       current value.
 * @param min
 *       minimum value.
 * @param max
 *       maximum value.
 *
 */
void NeuxProgressbarSetControls(PROGRESSBAR * progressbar, int current,
							int min, int max)
{
	NX_WIDGET *me = (NX_WIDGET *)progressbar;
	NxSetProgressBarValue(me, current);
}

void NeuxProgressbarGetProp(PROGRESSBAR * progressbar, progressbar_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)progressbar;
	prop->blockcolor = me->spec.progressbar.blockcolor;
}

void NeuxProgressbarSetProp(PROGRESSBAR * progressbar, const progressbar_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)progressbar;
	me->spec.progressbar.blockcolor = prop->blockcolor;
}
