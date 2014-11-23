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
 * Widget Button routines.
 *
 * REVISION:
 *
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"

void NxButtonFocusDraw (NX_WIDGET * button, GR_BOOL bFocus);

static void NxButtonSetProp(NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	if (widget->spec.button.type != btNormal)
		GrGetWMProperties (widget->base.wid, &props);

	props.flags = GR_WM_FLAGS_PROPS;
	if (widget->spec.button.type != btNormal)
		props.props |= GR_WM_PROPS_NOBACKGROUND;
	else
		props.props = GR_WM_PROPS_NODECORATE;
	GrSetWMProperties (widget->base.wid, &props);
}

void
NxButtonCreate (NX_WIDGET *widget)
{
	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if(widget->base.fontid<=0)
	{
		widget->base.fontid=NxGetFontID(widget->base.fontname, widget->base.fontsize);
		GrSetGCFont (widget->base.gc, widget->base.fontid);
	}
	if (widget->base.height == 0)
		widget->base.height = BUTTON_HEIGHT;
	if (widget->base.width == 0)
		widget->base.width = BUTTON_WIDTH;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Button%d", widget->base.wid);
		widget->base.name = strdup(str);
	}
	if (widget->spec.button.caption == NULL)
		widget->spec.button.caption = strdup(widget->base.name);
	else
		widget->spec.button.caption = strdup(widget->spec.button.caption);

	widget->spec.button.cb_exposure.fp		= NULL;
	widget->spec.button.cb_exposure.inherit	= GR_TRUE;
	widget->spec.button.cb_keydown.fp		= NULL;
	widget->spec.button.cb_keydown.inherit	= GR_TRUE;
	widget->spec.button.cb_keyup.fp			= NULL;
	widget->spec.button.cb_keyup.inherit	= GR_TRUE;
	widget->spec.button.cb_focusin.fp		= NULL;
	widget->spec.button.cb_focusin.inherit	= GR_TRUE;
	widget->spec.button.cb_focusout.fp		= NULL;
	widget->spec.button.cb_focusout.inherit	= GR_TRUE;

	widget->spec.button.st_down = GR_FALSE;

    GrSetGCUseBackground(widget->base.gc, GR_FALSE);
	GrSelectEvents (widget->base.wid, GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
	                                  GR_EVENT_MASK_EXPOSURE |
	                                  GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT);

	NxButtonSetProp(widget);

	DBGLOG("Create Button %d\n", widget->base.wid);
}

void
NxButtonKeyDownHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	if (widget->spec.button.cb_keydown.fp != NULL)
		(*(widget->spec.button.cb_keydown.fp))(widget, widget->spec.button.cb_keydown.dptr);
}

void
NxButtonKeyUpHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	if (widget->spec.button.cb_keyup.fp != NULL)
		(*(widget->spec.button.cb_keyup.fp))(widget, widget->spec.button.cb_keyup.dptr);
}

void
NxButtonExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("bexprosureevent\n");

	if (widget->spec.button.cb_exposure.inherit)
		NxButtonDraw(widget);
	if (widget->spec.button.cb_exposure.fp != NULL)
		(*(widget->spec.button.cb_exposure.fp))(widget, widget->spec.button.cb_exposure.dptr);
}

void
NxButtonFocusInHandler(GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("bfocusinevent %d\n", widget->base.wid);

	if (widget->spec.button.cb_focusin.fp != NULL)
		(*(widget->spec.button.cb_focusin.fp))(widget, widget->spec.button.cb_focusin.dptr);
	if (widget->spec.button.cb_focusin.inherit)
		NxButtonFocusDraw(widget, GR_TRUE);
}

void
NxButtonFocusOutHandler(GR_EVENT *event, NX_WIDGET *widget)
{
	DBGLOG("bfocusoutevent %d\n", widget->base.wid);

	if (widget->spec.button.cb_focusout.fp != NULL)
		(*(widget->spec.button.cb_focusout.fp))(widget, widget->spec.button.cb_focusout.dptr);
	if (widget->spec.button.cb_focusout.inherit)
		NxButtonFocusDraw(widget, GR_FALSE);
}

void
NxButtonDraw (NX_WIDGET * button)
{
	GR_SIZE captionwidth, captionheight, base;
	GR_SIZE captionx, captiony;
	GR_SIZE radius;
	GR_BOOL flag = GR_FALSE;

	if (!button->base.visible)
		return;

	captionx = 0;
	captiony = 0;

	if (button->spec.button.imageid <= 0)
	{
		NxGetGCTextSize(button->base.gc, button->spec.button.caption, strlen(button->spec.button.caption),
						MWTF_UTF8, &captionwidth, &captionheight, &base);
		switch (button->spec.button.align)
		{
			case btLeft:
				break;
			case btCenter:
				captionx = (button->base.width - captionwidth) / 2;
				break;
			case btRight:
				captionx = button->base.width - captionwidth;
			break;
		}

		captiony = (button->base.height - captionheight) / 2;
		if (captionx < 0)
			captionx = 0;		/*if window height or width < */
		if (captiony < 0)
			captiony = 0;		/*caption height or width - reset */

        GR_IMAGE_ID iid = 0;
		switch (button->spec.button.type)
		{
		case btImage:
			if (button->spec.button.cb_getimage != NULL)
				iid = (*(button->spec.button.cb_getimage))(button->base.wid, 0);
			if (iid != 0)
				GrDrawImageToFit(button->base.wid, button->base.gc, 0, 0, button->base.width, button->base.height, iid);
			break;
		case btGeneral:
			NxSetGCForeground (button->base.gc, DKGRAY);
			GrRect (button->base.wid, button->base.gc, 0, 0, button->base.width, button->base.height);
            if (button->base.wid == GrGetFocus())
                NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
            else
                NxSetGCForeground (button->base.gc, button->base.bgcolor);
			GrFillRect (button->base.wid, button->base.gc, 1, 1, button->base.width - 2, button->base.height - 2);
			NxSetGCForeground (button->base.gc, button->base.fgcolor);
			GrText (button->base.wid, button->base.gc, captionx, captiony + 2, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			break;
        case btFlat:
			if (button->spec.button.bredrawflag)
			{
				if (button->base.wid == GrGetFocus())
					flag = GR_TRUE;
				GrUnmapWindow(button->base.wid);
				GrMapWindow(button->base.wid);
				if (flag)
					GrSetFocus(button->base.wid);
				NxSetGCForeground (button->base.gc, button->spec.button.shadowdcolor);
				GrText (button->base.wid, button->base.gc, captionx + 2, captiony + 2, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
				NxSetGCForeground (button->base.gc, button->base.fgcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			}
			button->spec.button.bredrawflag = !button->spec.button.bredrawflag;
			break ;
		case btCircle:
			if (button->spec.button.bredrawflag)
			{
				if (button->base.wid == GrGetFocus())
					flag = GR_TRUE;
				GrUnmapWindow(button->base.wid);
				GrMapWindow(button->base.wid);
				if (flag)
					GrSetFocus(button->base.wid);
				radius = button->base.width > button->base.height ? button->base.height >> 1 : button->base.width >> 1;
				NxSetGCForeground (button->base.gc, button->spec.button.shadowdcolor);
				GrEllipse (button->base.wid, button->base.gc, button->base.width >> 1, button->base.height >> 1, radius, radius);
				NxSetGCForeground (button->base.gc, button->base.bgcolor);
				GrFillEllipse (button->base.wid, button->base.gc, button->base.width >> 1, button->base.height >> 1, radius - 2, radius - 2);
				NxSetGCForeground (button->base.gc, button->base.fgcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, 1, MWTF_UTF8 | GR_TFTOP);
			}
			button->spec.button.bredrawflag = !button->spec.button.bredrawflag;
			break ;
		case btNormal:
		default:
			GrClearWindow(button->base.wid, GR_FALSE);
			NxSetGCForeground (button->base.gc, WHITE);
			GrLine (button->base.wid, button->base.gc, 0, 0, button->base.width - 1, 0);
			GrLine (button->base.wid, button->base.gc, 0, 0, 0, button->base.height - 1);
			NxSetGCForeground (button->base.gc, DKGRAY);
			GrLine (button->base.wid, button->base.gc, button->base.width - 1, 0,
					button->base.width - 1, button->base.height - 1);
			GrLine (button->base.wid, button->base.gc, 0, button->base.height - 1,
					button->base.width - 1, button->base.height - 1);
			if (button->base.wid == GrGetFocus())
			{
				NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
				GrFillRect (button->base.wid, button->base.gc, 1, 1, button->base.width - 2, button->base.height - 2);
				NxSetGCForeground (button->base.gc, button->base.bgcolor);
				GrFillRect (button->base.wid, button->base.gc, 4, 4, button->base.width - 8, button->base.height - 8);
			}
			NxSetGCForeground (button->base.gc, button->base.fgcolor);
			GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			break;
		}
	}
	else
	{
		if (!button->spec.button.st_down)
			GrDrawImageToFit(button->base.wid, button->base.gc, 5, 5, button->base.width - 10, button->base.height - 10, button->spec.button.imageid);
		else
			GrDrawImageToFit(button->base.wid, button->base.gc, 6, 6, button->base.width - 9, button->base.height - 9, button->spec.button.imageid);
	}
}

void
NxButtonFocusDraw (NX_WIDGET * button, GR_BOOL bFocus)
{
	GR_SIZE captionwidth, captionheight, base;
	GR_SIZE captionx, captiony;
	GR_SIZE radius;

	captionx = 0;
	captiony = 0;

	if (button->spec.button.imageid <= 0)
	{
		NxGetGCTextSize(button->base.gc, button->spec.button.caption, strlen(button->spec.button.caption),
						MWTF_UTF8, &captionwidth, &captionheight, &base);
		switch (button->spec.button.align)
		{
			case btLeft:
				break;
			case btCenter:
				captionx = (button->base.width - captionwidth) / 2;
				break;
			case btRight:
				captionx = button->base.width - captionwidth;
			break;
		}

		captiony = (button->base.height - captionheight) / 2;
		if (captionx < 0)
			captionx = 0;		/*if window height or width < */
		if (captiony < 0)
			captiony = 0;		/*caption height or width - reset */

        GR_IMAGE_ID iid = 0;
		switch (button->spec.button.type)
		{
        case btImage:
            if (button->base.wid == GrGetFocus())
            {
                NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
                if (button->spec.button.cb_getimage != NULL)
                    iid = (*(button->spec.button.cb_getimage))(button->base.wid, 1);
            }
            else
            {
                NxSetGCForeground (button->base.gc, button->base.bgcolor);
                if (button->spec.button.cb_getimage != NULL)
                    iid = (*(button->spec.button.cb_getimage))(button->base.wid, 0);
            }
            if (iid != 0)
                GrDrawImageToFit(button->base.wid, button->base.gc, 0, 0, button->base.width, button->base.height, iid);
            break ;
		case btGeneral:
            NxSetGCForeground (button->base.gc, DKGRAY);
			GrRect (button->base.wid, button->base.gc, 0, 0, button->base.width, button->base.height);
            if (button->base.wid == GrGetFocus())
                NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
            else
                NxSetGCForeground (button->base.gc, button->base.bgcolor);
			GrFillRect (button->base.wid, button->base.gc, 1, 1, button->base.width - 2, button->base.height - 2);
			NxSetGCForeground (button->base.gc, button->base.fgcolor);
			GrText (button->base.wid, button->base.gc, captionx, captiony + 2, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			break ;
		case btFlat:
			if (bFocus)
			{
				NxSetGCForeground (button->base.gc, button->spec.button.shadowdcolor);
				GrText (button->base.wid, button->base.gc, captionx + 2, captiony + 2, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
				NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			}
			else
			{
				NxSetGCForeground (button->base.gc, button->spec.button.shadowdcolor);
				GrText (button->base.wid, button->base.gc, captionx + 2, captiony + 2, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
				NxSetGCForeground (button->base.gc, button->base.fgcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			}
			return ;
		case btCircle:
			radius = button->base.width > button->base.height ? button->base.height>>1 : button->base.width>>1;
			if (bFocus)
			{
				NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
				GrFillEllipse (button->base.wid, button->base.gc, button->base.width>>1, button->base.height>>1, radius - 2, radius - 2);
				NxSetGCForeground (button->base.gc, button->base.fgcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, 1, MWTF_UTF8 | GR_TFTOP);
			}
			else
			{
				NxSetGCForeground (button->base.gc, button->base.bgcolor);
				GrFillEllipse (button->base.wid, button->base.gc, button->base.width>>1, button->base.height>>1, radius - 2, radius - 2);
				NxSetGCForeground (button->base.gc, button->base.fgcolor);
				GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, 1, MWTF_UTF8 | GR_TFTOP);
			}
			return ;
		case btNormal:
		default:
			GrClearWindow(button->base.wid, GR_FALSE);
			NxSetGCForeground (button->base.gc, WHITE);
			GrLine (button->base.wid, button->base.gc, 0, 0, button->base.width - 1, 0);
			GrLine (button->base.wid, button->base.gc, 0, 0, 0, button->base.height - 1);
			NxSetGCForeground (button->base.gc, DKGRAY);
			GrLine (button->base.wid, button->base.gc, button->base.width - 1, 0,
					button->base.width - 1, button->base.height - 1);
			GrLine (button->base.wid, button->base.gc, 0, button->base.height - 1,
					button->base.width - 1, button->base.height - 1);
			if (bFocus)
			{
				NxSetGCForeground (button->base.gc, button->spec.button.selectedcolor);
				GrFillRect (button->base.wid, button->base.gc, 1, 1, button->base.width - 2, button->base.height - 2);
				NxSetGCForeground (button->base.gc, button->base.bgcolor);
				GrFillRect (button->base.wid, button->base.gc, 4, 4, button->base.width - 8, button->base.height - 8);
			}
			NxSetGCForeground (button->base.gc, button->base.fgcolor);
			GrSetGCUseBackground(button->base.gc, GR_FALSE);
			GrText (button->base.wid, button->base.gc, captionx, captiony, button->spec.button.caption, strlen(button->spec.button.caption), MWTF_UTF8 | GR_TFTOP);
			break;
		}
	}
	else
	{
		if (!button->spec.button.st_down)
			GrDrawImageToFit(button->base.wid, button->base.gc, 5, 5, button->base.width - 10, button->base.height - 10, button->spec.button.imageid);
		else
			GrDrawImageToFit(button->base.wid, button->base.gc, 6, 6, button->base.width - 9, button->base.height - 9, button->spec.button.imageid);
	}
}

void
NxButtonDestroy(NX_WIDGET *widget)
{
	if (widget->spec.button.imageid > 0)
		GrFreeImage(widget->spec.button.imageid);
	free(widget->spec.button.caption);
	NxDeleteFromRegistry(widget);
}

int
NxSetButtonPixmap(NX_WIDGET *widget, char *filename)
{
	if (widget->base.type != WIDGET_TYPE_BUTTON)
		return -1;
	if (filename == NULL)
		return -1;

	if (widget->spec.button.imageid > 0)
		GrFreeImage(widget->spec.button.imageid);
	widget->spec.button.imageid = GrLoadImageFromFile(filename, 0);
	GrClearWindow(widget->base.wid, GR_FALSE);
	NxButtonDraw(widget);
	return 1;
}

int
NxRemoveButtonPixmap(NX_WIDGET *widget)
{
	if (widget->base.type != WIDGET_TYPE_BUTTON)
		return -1;

	if (widget->spec.button.imageid > 0) {
		GrFreeImage(widget->spec.button.imageid);
		widget->spec.button.imageid = -1;
	}
	GrClearWindow(widget->base.wid, GR_FALSE);
	NxButtonDraw(widget);
	return 1;
}

int
NxSetButtonCaption(NX_WIDGET *widget, const char *caption)
{
	if (widget->base.type != WIDGET_TYPE_BUTTON)
		return -1;

	if (widget->spec.button.caption)
		free(widget->spec.button.caption);
	widget->spec.button.caption = strdup(caption);
	if (widget->base.realized == GR_TRUE)
		GrClearWindow(widget->base.wid, GR_TRUE);
	return 1;
}

int
NxGetButtonCaption(NX_WIDGET *widget, char **caption)
{
	if (widget->base.type != WIDGET_TYPE_BUTTON)
		return -1;

	*caption = widget->spec.button.caption;
	return 1;
}

void
NxSetButtonProp(NX_WIDGET *widget)
{
	if (widget->base.type != WIDGET_TYPE_BUTTON)
		return;

	if (widget->base.realized == GR_TRUE)
		NxButtonSetProp(widget);
}
