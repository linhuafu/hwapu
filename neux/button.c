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
 * Button widget support routines.
 *
 * REVISION:
 *
 * 3) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 2) Implemnet function. --------------------------------- 2006-05-19 EY
 * 1) Initial creation. ----------------------------------- 2006-05-16 MG
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"

/*
  x, y coordinates are container widget coordinates.
  if x or y is equal to -1, its position will be determined by container layout manager.
  button does nothing if callback is not set.
 */

/**
 * Function creates button widget.
 *
 * @param owner
 *      Button owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param text
 *      button text.
 * @return
 *      Button widget if successful, otherwise NULL.
 *
 */
BUTTON * NeuxButtonNew(WIDGET * owner, int x, int y, int w, int h,
				   const char * text)
{
	NX_WIDGET *button;

	button = NxNewWidget(WIDGET_TYPE_BUTTON, (NX_WIDGET *)owner);
	button->base.posx = x;
	button->base.posy = y;
	button->base.width = w;
	button->base.height = h;
	button->spec.button.caption = (char *)text;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(button);
	}

	return button;
}


/** Function sets call back of the button.
 *
 * @param button
 *       target button.
 * @param cbId
 *       call back function ID.
 * @param fptr
 *       call back function pointer.
 *
 */
void NeuxButtonSetCallback(BUTTON * button, int cbId, void * fptr)
{
	NX_BUTTON *me = &((NX_WIDGET *)button)->spec.button;

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
	case CB_GETIMAGE:
		me->cb_getimage = fptr;
		break;
	}
}

/**
 * Reset button text.
 *
 * @param button
 *       button widget handle.
 * @param text
 *       new text.
 */
void NeuxButtonSetText(BUTTON *button, const char *text)
{
	NxSetButtonCaption((NX_WIDGET *)button, text);
}

/**
 * Get button text.
 *
 * @param button
 *       button widget handle.
 * @return
 *       button text.
 */
char * NeuxButtonGetText(BUTTON * button)
{
	char *text;
	NxGetButtonCaption((NX_WIDGET *)button, &text);
	return text;
}


void NeuxButtonGetProp(BUTTON * Button, button_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)Button;
	prop->type = me->spec.button.type;
	prop->selectedcolor = me->spec.button.selectedcolor;
	prop->shadowdcolor = me->spec.button.shadowdcolor;
	prop->align = me->spec.button.align;
}

void NeuxButtonSetProp(BUTTON * Button, const button_prop_t *prop)
{
	NX_WIDGET *me = (NX_WIDGET *)Button;
	me->spec.button.type = prop->type;
	me->spec.button.selectedcolor = prop->selectedcolor;
	me->spec.button.shadowdcolor = prop->shadowdcolor;
	me->spec.button.align = prop->align;
	NxSetButtonProp(me);
}

// extra button contructors.
/**
 * Function creates button widget with given property.
 *
 * @param owner
 *      Button owner widget.
 * @param x
 *      x coordinate.
 * @param y
 *      y coordinate
 * @param w
 *      display width in pixel.
 * @param h
 *      display height in pixel.
 * @param text
 *      button text.
 * @param align
 *      button text alignment
 * @param font
 *      font descriptor
 * @param fontsize
 *      font size
 * @param kdn
 *      keydown call back function.
 * @return
 *      Button widget if successful, otherwise NULL.
 *
 */
BUTTON * NeuxButtonNewExt(WIDGET * owner, int x, int y, int w, int h,
						  const char * text,
						  BUTTON_ALIGN align,
						  const char * font,
						  int fontsize,
						  KeydownCbfptr kdn)
{
	BUTTON * button;
	button_prop_t buttonprompt;

	button = NeuxButtonNew(owner, x, y, w, h, text);

	NeuxButtonGetProp(button, &buttonprompt);
	buttonprompt.align = align;
	NeuxButtonSetProp(button, &buttonprompt);

	NeuxWidgetSetFont(button, font, fontsize);
	NeuxButtonSetCallback(button, CB_KEY_DOWN, kdn);
	NeuxWidgetShow(button, TRUE);

	return button;
}
