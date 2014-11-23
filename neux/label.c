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
 * Label widget support routines.
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


/*
  x, y coordinates are container widget coordinates.
 */

/**
 * Function creates label widget.
 *
 * @param owner
 *      Label owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param text
 *      label text.
 * @return
 *      Label widget if successful, otherwise NULL.
 *
 */
LABEL * NeuxLabelNew(WIDGET * owner, int x, int y, int w, int h,
				   const char * text)
{
	NX_WIDGET * label;

	label = NxNewWidget(WIDGET_TYPE_LABEL, (NX_WIDGET *)owner);
	label->base.posx = x;
	label->base.posy = y;
	label->base.width = w;
	label->base.height = h;

	label->spec.label.autosize = GR_TRUE;
	label->spec.label.caption = (char *)text;
	label->spec.label.transparent = GR_FALSE;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(label);
	}

	return label;
}

/**
 * Reset label move show stat.
 *
 * @param label
 *       label widget handle.
 * @param interval
 *       if <= 0, means disable move show, else is interval.
 * @return
 *	1: sucess  -1:error type -2:it's autosize
 */
int NeuxLabelSetScroll(LABEL * label, int interval)
{
	return NxSetLabelMShow((NX_WIDGET *)label, interval);
}

/**
 * Reset label text.
 *
 * @param label
 *       label widget handle.
 * @param text
 *       new text.
 */
void
NeuxLabelSetText(LABEL * label, const char * text)
{
	NxSetLabelCaption((NX_WIDGET *)label, text);
}

char *
NeuxLabelGetText(LABEL * label)
{
	char *text;
	NxGetLabelCaption((NX_WIDGET *)label, &text);
	return text;
}

void
NeuxLabelGetProp(LABEL * label, label_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)label;
	prop->transparent = me->spec.label.transparent;
	prop->align = me->spec.label.align;
	prop->autosize = me->spec.label.autosize;
}

void
NeuxLabelSetProp(LABEL * label, const label_prop_t * prop)
{
	NX_WIDGET *me = (NX_WIDGET *)label;

	me->spec.label.transparent = prop->transparent;
	me->spec.label.align = prop->align;
	me->spec.label.autosize = prop->autosize;

    GR_WM_PROPERTIES labelprops;
    labelprops.flags = GR_WM_FLAGS_PROPS;
    labelprops.props = GR_WM_PROPS_NOFOCUS;
    if (me->spec.label.transparent)
        labelprops.props |= (GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOBACKGROUND);
    else
        labelprops.props |= GR_WM_PROPS_NODECORATE;
    GrSetWMProperties(me->base.wid, &labelprops);
}
