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
 * Neuros X form support routines.
 *
 * REVISION:
 *
 * 3) Pull in WM/Desktop support. ------------------------- 2007-04-03 MG
 * 2) Implement function. --------------------------------- 2006-05-19 EY
 * 1) Initial creation. ----------------------------------- 2006-05-16 MG
 *
 */

#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"
#include "application.h"
#include "wm.h"

/*
  Form is the entry point of any GUI, with its parent as the main
  application.
  Form can be a container of other widgets. Form shall maintain focus
  for its children.
  Form is created to be hidden.
*/


/**
 * Function creates a new form with given size at specified location.
 *
 * @param x
 *       x coordinate
 * @param y
 *       y coordinate
 * @param w
 *       width in pixel.
 * @param h
 *       height in pixel.
 * @return
 *       Form handle if successful, otherwise NULL.
 *
 */
FORM * NeuxFormNew(int x, int y, int w, int h)
{
	extern NX_WIDGET * mainApp;

	NX_WIDGET * form;
	NX_WIDGET * owner = (NX_WIDGET *)mainApp;

	form = NxNewWidget(WIDGET_TYPE_WINDOW, owner);//GR_ROOT_WINDOW_ID
	form->base.posx = x;
	form->base.posy = y;
	form->base.width = w;
	form->base.height = h;

	form->spec.container.base.layout = NLT_NULL_LAYOUT;

	NxCreateWidget(form);
	if (WmAddForm(form->base.wid))
	{
		//FIXME: error handling.
	}

	return form;
}

/** Function destroys current form without restoring previous FORM, in other
 * words, this is called to destroy current FORM and bring up a new one next.
 *
 * @param form
 *       target form.
 *
 */
void NeuxFormDestroyOnly(FORM **form)
{
	NX_WIDGET **me = (NX_WIDGET **)form;

	WmRemoveForm((*me)->base.wid, 0);
	NxDestroyWidget(me);
}

/** Function destroys current form and automatically restores the previous
 * FORM per the z-order.
 *
 * @param form
 *       target form.
 *
 */
#include <execinfo.h>

void NeuxFormDestroy(FORM **form)
{
	NX_WIDGET **me = (NX_WIDGET **)form;

	WmRemoveForm((*me)->base.wid, 1);
	NxDestroyWidget(me);
}

/** Function sets call back of the form.
 *
 * @param form
 *       target form.
 * @param cbId
 *       call back function ID.
 * @param fptr
 *       call back function pointer.
 *
 */
void NeuxFormSetCallback(FORM * form, int cbId, void * fptr)
{
	NX_WINDOW *me = &((NX_WIDGET *)form)->spec.container.spec.window;

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
	case CB_DESTROY:
		NxRegisterCallBack(&me->cb_destroy, OnDestroy, fptr);
		break;
	case CB_HOTPLUG:
		NxRegisterCallBack(&me->cb_hotplug, OnHotplug, fptr);
		break;
	}
}

void
NeuxFormGetProp(FORM * form, form_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)form;

	prop->transparent = me->spec.container.spec.window.transparent;
	prop->layout = me->spec.container.base.layout;
}

void
NeuxFormSetProp(FORM * form, const form_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)form;
	GR_WM_PROPERTIES props;

	me->spec.container.spec.window.transparent = prop->transparent;
	me->spec.container.base.layout = prop->layout;

	GrGetWMProperties (me->base.wid, &props);
	props.flags = GR_WM_FLAGS_PROPS;
	if (prop->transparent)
		props.props |= GR_WM_PROPS_NOBACKGROUND;
	else
		props.props &= ~GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties (me->base.wid, &props);

	if (props.title)
		free(props.title);
}
