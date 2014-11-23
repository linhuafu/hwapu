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
 * Widget Label routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "nxutils.h"

void
NxLabelCreate (NX_WIDGET *widget)
{
	GR_WM_PROPERTIES props;

	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if(widget->base.fontid<=0)
	{
		widget->base.fontid=NxGetFontID(widget->base.fontname, widget->base.fontsize);
		GrSetGCFont (widget->base.gc, widget->base.fontid);
	}
	if (widget->base.height == 0)
		widget->base.height = LABEL_HEIGHT;
	if (widget->base.width == 0)
		widget->base.width = LABEL_WIDTH;

	widget->base.wid = GrNewWindow(widget->parent->base.wid,
						widget->base.posx, widget->base.posy,
						widget->base.width, widget->base.height, 0,
						widget->base.bgcolor, widget->base.fgcolor);

	if (widget->base.name == NULL) {
		char str[WIDGET_NAME_LENGTH];
		snprintf(str, WIDGET_NAME_LENGTH, "Label%d", widget->base.wid);
		widget->base.name = strdup(str);
	}

	if (widget->spec.label.caption != NULL)
		widget->spec.label.caption = strdup(widget->spec.label.caption);	//eyu 060330

    widget->spec.label.captionwidth  = 0;
    widget->spec.label.captionheight = 0;

	widget->spec.label.cb_exposure.fp		= NULL;
	widget->spec.label.cb_exposure.inherit	= GR_TRUE;

	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOFOCUS;
	if (widget->spec.label.transparent)
		props.props |= GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOBACKGROUND;
	else
        props.props |= GR_WM_PROPS_NODECORATE;
    GrSetWMProperties(widget->base.wid, &props);

	if (!widget->spec.label.transparent && widget->base.bgpixmap > 0)
		GrSetBackgroundPixmap(widget->base.wid, widget->base.bgpixmap, GR_BACKGROUND_CENTER);

    GrSetGCUseBackground(widget->base.gc, GR_FALSE);
	GrSelectEvents (widget->base.wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_TIMER);

	DBGLOG("Create Label %d\n", widget->base.wid);
}

void
NxLabelTimerHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    if (widget->spec.label.mshowt++ < 1)  //sleep times
        return;

    widget->spec.label.mshowx -= widget->spec.label.captionheight / 2;
    if (-widget->spec.label.mshowx >= widget->spec.label.captionwidth)
    {
        widget->spec.label.mshowt = 0;
        widget->spec.label.mshowx = 0;
    }

    GrClearWindow(widget->base.wid, GR_FALSE);
    NxLabelDraw(widget);
}

void
NxLabelExposureHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    if (widget->spec.label.cb_exposure.inherit)
        NxLabelDraw(widget);
    if (widget->spec.label.cb_exposure.fp != NULL)
        (*(widget->spec.label.cb_exposure.fp))(widget, widget->spec.label.cb_exposure.dptr);
}

void
NxLabelDraw (NX_WIDGET * label)
{
    GR_SIZE captionx = 0;
    int fontid = label->base.fontid;

    if (label->base.visible == GR_FALSE || label->spec.label.caption == NULL)
        return;

    if (label->spec.label.transparent) {
    	label->spec.label.transb = !label->spec.label.transb;
    	if (!label->spec.label.transb)
    		return;
   	}

    if (label->spec.label.captionheight == 0 && label->spec.label.captionwidth == 0)
    {
        GR_SIZE base;
        NxGetGCTextSize (label->base.gc, label->spec.label.caption, strlen(label->spec.label.caption), MWTF_UTF8,
                         &label->spec.label.captionwidth, &label->spec.label.captionheight, &base);
		label->spec.label.captionheight += base;
    }

    if (label->spec.label.autosize)
    {
    	if ((label->spec.label.captionwidth != label->base.width || label->spec.label.captionheight != label->base.height) &&
    		label->spec.label.captionheight > 0 && label->spec.label.captionwidth > 0)
        {
            label->base.width = label->spec.label.captionwidth;
            label->base.height = label->spec.label.captionheight;
            GrResizeWindow (label->base.wid, label->spec.label.captionwidth, label->spec.label.captionheight);
        }
    }
    else
    {
        if (label->spec.label.automshow && label->base.width < label->spec.label.captionwidth)
        {
        	if (label->spec.label.timerid <= 0) {
        		label->spec.label.timerid = GrCreateTimer(label->base.wid, label->spec.label.interval);
				label->spec.label.mshowx = 0;
				label->spec.label.mshowt = 0;

				if (!label->spec.label.transparent && label->base.bgpixmap <= 0) {
					label->spec.label.mshowpixmap = GrNewPixmap(label->spec.label.captionwidth, label->spec.label.captionheight, NULL);

					NxSetGCForeground (label->base.gc, label->base.bgcolor);
					GrFillRect(label->spec.label.mshowpixmap, label->base.gc, 0, 0,
					           label->spec.label.captionwidth, label->spec.label.captionheight);

					NxSetGCForeground (label->base.gc, label->base.fgcolor);
					GrText (label->spec.label.mshowpixmap, label->base.gc, 0, 0,
	            			label->spec.label.caption, strlen (label->spec.label.caption),
	           				MWTF_UTF8 | GR_TFTOP);
				}
			}

            captionx = label->spec.label.mshowx;
        }
        else
        {
            switch (label->spec.label.align)
            {
            case LA_LEFT:
                break;
            case LA_CENTER:
                captionx = (label->base.width - label->spec.label.captionwidth) / 2;
                break;
            case LA_RIGHT:
                captionx = label->base.width - label->spec.label.captionwidth;
                break;
            }
        }
    }

	if (label->spec.label.mshowpixmap > 0) {
		GrCopyArea(label->base.wid, label->base.gc, 0, 0,
		            min(label->base.width, label->spec.label.captionwidth + captionx), label->spec.label.captionheight,
           			label->spec.label.mshowpixmap, -captionx, 0, 0);
	} else {
	    NxSetGCForeground (label->base.gc, label->base.fgcolor);
	    if (label->spec.label.transparent)
	    {
	        GrUnmapWindow(label->base.wid);
	        GrMapWindow(label->base.wid);
	    }

	    GrText (label->base.wid, label->base.gc, captionx, 0,
	            label->spec.label.caption, strlen (label->spec.label.caption),
	            MWTF_UTF8 | GR_TFTOP);
	}
}

void
NxLabelDestroy(NX_WIDGET *widget)
{
	if(widget->spec.label.imageid>0)
		GrFreeImage(widget->spec.label.imageid);
	if (widget->spec.label.caption != NULL)
		free(widget->spec.label.caption);
	if (widget->spec.label.timerid > 0)
		GrDestroyTimer(widget->spec.label.timerid);
	if (widget->spec.label.mshowpixmap > 0)
		GrDestroyWindow(widget->spec.label.mshowpixmap);

	NxDeleteFromRegistry(widget);
}

int
NxSetLabelCaption(NX_WIDGET *widget, const char *caption)
{
    GR_SIZE base;

	if (widget->base.type != WIDGET_TYPE_LABEL)
		return -1;

	if (widget->spec.label.automshow)
	{
		if (widget->spec.label.timerid > 0)
			GrDestroyTimer(widget->spec.label.timerid);
		widget->spec.label.timerid = 0;
		if (widget->spec.label.mshowpixmap > 0)
			GrDestroyWindow(widget->spec.label.mshowpixmap);
		widget->spec.label.mshowpixmap = 0;
	}

	if (widget->spec.label.caption != NULL)
		free(widget->spec.label.caption);
	if (caption == NULL)
		widget->spec.label.caption = NULL;
	else
		widget->spec.label.caption = strdup(caption);
	widget->spec.label.captionheight = 0;
	widget->spec.label.captionwidth = 0;

    GrClearWindow(widget->base.wid, GR_TRUE);

	return 1;
}

int
NxGetLabelCaption(NX_WIDGET *widget, char **caption)
{
	if (widget->base.type != WIDGET_TYPE_LABEL)
		return -1;
	*caption = widget->spec.label.caption;
	return 1;
}

int
NxSetLabelMShow(NX_WIDGET *widget, int interval)
{
	if (widget->base.type != WIDGET_TYPE_LABEL)
		return -1;
	if (widget->spec.label.autosize)
		return -2;

	if (widget->spec.label.automshow)
	{
		if (widget->spec.label.timerid > 0)
			GrDestroyTimer(widget->spec.label.timerid);
		widget->spec.label.timerid = 0;
		if (widget->spec.label.mshowpixmap > 0)
			GrDestroyWindow(widget->spec.label.mshowpixmap);
		widget->spec.label.mshowpixmap = 0;
	}

	if (interval <= 0) //disable
	{
		widget->spec.label.automshow = GR_FALSE;
		widget->spec.label.interval = 0;
	} else {
		widget->spec.label.automshow = GR_TRUE;
		widget->spec.label.interval = interval;
	}

	return 1;
}
