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
 * Panel widget support routines.
 *
 * REVISION:
 *
 * 3) Pull in WM/Desktop support. ------------------------- 2007-04-03 MG
 * 2) Implemnet function. --------------------------------- 2006-05-19 EY
 * 1) Initial creation. ----------------------------------- 2006-05-04 MG
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"
#include "plextalk-theme.h"

/*
  panel is a container widget, it thus should manage focus of its children
  if any.
  x, y coordinates are container widget coordinates.
 */

/**
 * Function creates panel widget.
 *
 * @param owner
 *      Panel owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @return
 *      Panel widget if successful, otherwise NULL.
 *
 */
PANEL * NeuxPanelNew(WIDGET * owner, int x, int y, int w, int h)
{
	NX_WIDGET * panel;
	panel = NxNewWidget(WIDGET_TYPE_PANEL, (NX_WIDGET *)owner);
	panel->base.posx = x;
	panel->base.posy = y;
	panel->base.width = w;
	panel->base.height = h;

	panel->spec.container.base.layout = NLT_NULL_LAYOUT;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(panel);
	}

	/*
	 * Apply properties found in our theme.
	 */
	panel->base.fgcolor = theme_cache.foreground;
	panel->base.bgcolor = theme_cache.background;

	return panel;
}

/**
 * Function creates panel widget.
 *
 * @param owner
 *      Panel owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
  * @param bgfile
 *      background filename.
 * @return
 *      Panel widget if successful, otherwise NULL.
 *
 */
PANEL * NeuxPanelNewExt(WIDGET * owner, int x, int y, int w, int h, const char * bgfile)
{
	NX_WIDGET * panel;
	GR_IMAGE_ID bg;

	panel = NxNewWidget(WIDGET_TYPE_PANEL, (NX_WIDGET *)owner);
	panel->base.posx = x;
	panel->base.posy = y;
	panel->base.width = w;
	panel->base.height = h;
	panel->base.bgpixmap = GrNewPixmap(w, h, NULL);
	bg = GrLoadImageFromFile((char *)bgfile, 0);
	GrDrawImageToFit(panel->base.bgpixmap,
		panel->base.gc, 0, 0, panel->base.width,
		panel->base.height, bg);
	GrFreeImage(bg);

	panel->spec.container.base.layout = NLT_NULL_LAYOUT;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(panel);
	}

	return panel;
}


/**
 * Function gets panel widget property.
 *
 * @param panel
 *      Panel widget.
 * @param prop
 *      Property data pointer.
 */
void
NeuxPanelGetProp(PANEL * panel, panel_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)panel;

	prop->layout = me->spec.container.base.layout;
}

/**
 * Function sets panel widget property.
 *
 * @param panel
 *      Panel widget.
 * @param prop
 *      Property data pointer.
 */
void
NeuxPanelSetProp(PANEL * panel, const panel_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)panel;

	me->spec.container.base.layout = prop->layout;
}
