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
 * Widget Listbox routines.
 *
 * REVISION:
 *
 * 4) Redraw algorithm optimization. ---------------------- 2007-05-16 MG
 * 3) struct modify---------------------------------------- 2006-05-26 EY
 * 2) Added listbox helper function. ---------------------- 2006-05-04 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "utf8_utils.h"
#include "libvprompt.h"
#include "vprompt-defines.h"
#include "plextalk-i18n.h"

#define DEF_CHARS_PER_SCROLL	1
#define DEF_ENDING_SCROLL_TIMES	3
#define DEF_SCROLL_TIME			500 //0.5s

static slist_t*
IsInMultiselectlist(NX_LISTBOX *listbox, int index)
{
	slist_t* pitem = listbox->multiseleclist.next;

	while (pitem)
	{
		struct multiselect *msel = container_of(pitem, struct multiselect, list);
		if (msel->index == index)
			return msel;
		pitem = pitem->next;
	}

	return NULL;
}

static void
RemoveFromMultiselectlist(NX_LISTBOX *listbox, struct multiselect *msel)
{
	nxSlistRemove(&listbox->multiseleclist, &msel->list);
	listbox->selectedcount--;
	free(msel);
}

static GR_BOOL
AddToMultiselectlist(NX_LISTBOX *listbox, int index)
{
	struct multiselect *msel = malloc(sizeof(struct multiselect));
	if (msel == NULL)
		return GR_FALSE;

	msel->index = index;

	nxSlistAdd(&listbox->multiseleclist, &msel->list);
	listbox->selectedcount++;

	return GR_TRUE;
}

static void
ListboxDrawSingleSelectedmark(NX_WIDGET *widget, int index, GR_BOOL drawmark)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;
	if (index < lbx->topitem || index >= (lbx->topitem + lbx->pitems))
		return;

	int markwidth = lbx->itemheight >> 2;
	int markheight = markwidth;
    int y = lbx->itemmargin.top + (index - lbx->topitem) * lbx->itemheight;
	int x = lbx->itemmargin.left + (markwidth / 2);
	if (drawmark)
	{
		NxSetGCForeground (widget->base.gc, lbx->selectmarkcolor);
		GrFillRect(widget->base.wid, widget->base.gc, x + (markwidth / 2), y + ((lbx->itemheight - markheight) >> 1), markwidth, markheight);
	}
	else
	{
		NxSetGCForeground(widget->base.gc, lbx->selectedcolor);
		GrFillRect(widget->base.wid, widget->base.gc, x, y, markwidth * 2, lbx->itemheight);
	}
}

static void
ListboxDrawAllSelectedmark(NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;
	slist_t* pitem = lbx->multiseleclist.next;

	while (pitem)
	{
		struct multiselect *msel = container_of(pitem, struct multiselect, list);
		ListboxDrawSingleSelectedmark(widget, msel->index, GR_TRUE);
		pitem = pitem->next;
	}
}

static void
create_scroll_timer(NX_WIDGET *widget)
{
	scroll_info_t* scroll_info;

	if (!widget->spec.listbox.bScroll)
		return;

	scroll_info = &widget->spec.listbox.scroll_info;
	if (scroll_info->scrolltimer > 0)
		GrDestroyTimer(scroll_info->scrolltimer);
	scroll_info->scrolltimer = GrCreateTimer(widget->base.wid, widget->spec.listbox.scroll_interval);
	scroll_info->scrollcnt = 0;
}

static void
destory_scroll_timer(NX_WIDGET *widget)
{
	scroll_info_t* scroll_info = &widget->spec.listbox.scroll_info;
    if (scroll_info->scrolltimer > 0)
        GrDestroyTimer(scroll_info->scrolltimer);
}

/*
 * Function Name: locate_string_to_fit()
 *@param widget: the target widget
 *@param max_w: the maximum width of the text field
 *@param srcstr: the string to be located
 *@param dststr: the buffer to store the located string
 *@param dstsize: the buffer size
 *@param bsuffix: TRUE, suffix; FALSE, no suffix
 *@return the located index in the srcstr
*/
static int locate_string_to_fit(NX_WIDGET * widget, GR_SIZE max_w,
					const char* srcstr, char* dststr, int dstsize, GR_BOOL bsuffix)
{
#define SUFFIX_STRING  " ..."
#define SUFFIX_STRLEN	strlen(SUFFIX_STRING)

	GR_SIZE w, h, b;
	int len;

	len = strlen(srcstr);
	NxGetGCTextSize(widget->base.gc, (char*)srcstr, len, MWTF_UTF8, &w, &h, &b);
	if (w <= max_w)
		return len;

	if (bsuffix)
	{
		NxGetGCTextSize(widget->base.gc, SUFFIX_STRING, SUFFIX_STRLEN, MWTF_UTF8, &w, &h, &b);
		max_w -= w;
	}

	len = utf8_text_to_fit(widget->base.gc, max_w, srcstr, len, &w);

	if (NULL != dststr)
	{
		memcpy(dststr, srcstr, len);
		if (bsuffix)
			memcpy(&dststr[len], SUFFIX_STRING, SUFFIX_STRLEN + 1);
		else
			dststr[len] = '\0';
	}

	return len;
}

static void
ListboxDrawItem(NX_WIDGET *widget, GR_WINDOW_ID wid, int item, int x, int y, int itemwidth)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;
    int fontheight = NxGetFontHeight(widget->base.fontid);
	char *pStr;
    GR_SIZE textwidth = 0;
    GR_SIZE textheight = 0;
    GR_SIZE textbase = 0;
    GR_SIZE textregionwidth;

	if (lbx->useicon)
	{
		GR_BITMAP *icon = NULL;
		if (lbx->cb_geticon != NULL)
			(*(lbx->cb_geticon))(widget->base.wid, item, (void **)&icon);

		if (lbx->graphictype == ltIcon) {
			if (icon != NULL)
				GrBitmap(wid, widget->base.gc, x, y,
						fontheight, fontheight, icon);
			x += fontheight;
		} else {
			int w = lbx->iconregion.w;
			if (w < 0)
				w = fontheight + lbx->textmargin.top + lbx->textmargin.bottom;
			if (icon != NULL) {
				GR_IMAGE_INFO info;
				int ix = x, iy = y;
				int iw = w, ih = lbx->iconregion.h;
				if (ih < 0)
					ih = fontheight + lbx->textmargin.top + lbx->textmargin.bottom;
				GrGetImageInfo(*icon, &info);
				if (iw >= info.width && ih >= info.height) {
					ix += (iw - info.width) / 2;
					iw = info.width;
					iy += (ih - info.height) / 2;
					ih = info.height;
				} else {
					if (iw * info.height > ih * info.width) {
						ix += (iw - ih * info.width / info.height) / 2;
						iw = ih * info.width / info.height;
					} else {
						iy += (ih - iw * info.height / info.width) / 2;
						ih = iw * info.height / info.width;
					}
				}
				GrDrawImageToFit(wid, widget->base.gc, ix, iy,
								 iw, ih, *icon);
			}
			x += w;
		}
	}

	if (item - lbx->topitem == lbx->indexitem) //selected
	{
		int selwidth = itemwidth - lbx->itemmargin.right - x;
		int selheight = fontheight + lbx->textmargin.top + lbx->textmargin.bottom;
#ifdef FASTGUI
		NxSetGCForeground(widget->base.gc, lbx->selectedcolor);
		GrFillRect(wid, widget->base.gc, x, y, selwidth, selheight);
#else
		NxPixmapMaskColor(wid, widget->base.gc, x, y,
                          itemwidth, itemheight, lbx->selectedcolor);
#endif
		if (lbx->cb_getitemfgcolor != NULL)
			NxSetGCForeground(widget->base.gc, (*(lbx->cb_getitemfgcolor))(widget->base.wid, item));
		else
			NxSetGCForeground (widget->base.gc, lbx->selectedfgcolor);
	}
	else
	{
		if (lbx->cb_getitemfgcolor != NULL)
			NxSetGCForeground(widget->base.gc, (*(lbx->cb_getitemfgcolor))(widget->base.wid, item));
		else
			NxSetGCForeground (widget->base.gc, widget->base.fgcolor);
	}

	x += lbx->textmargin.left;
	y += lbx->textmargin.top;
	textregionwidth = itemwidth - (x + lbx->textmargin.right);

	lbx->cb_getitem(widget->base.wid, item, &pStr);
	NxGetGCTextSize(widget->base.gc, pStr, strlen(pStr),
					MWTF_UTF8, &textwidth, &textheight, &textbase);

	if (textwidth <= textregionwidth || !lbx->bScroll)
		GrText(wid, widget->base.gc, x, y,
				pStr, strlen(pStr), MWTF_UTF8 | GR_TFTOP);
	else {
		char str[PATH_MAX];

		if (item - lbx->topitem == lbx->indexitem) {
			scroll_info_t* scroll_info = &lbx->scroll_info;

			locate_string_to_fit(widget, textregionwidth, pStr, str, PATH_MAX, GR_FALSE);

			scroll_info->textregion.x = x;
			scroll_info->textregion.y = y;
			scroll_info->textregion.w = textregionwidth;
			scroll_info->textregion.h = fontheight;
			scroll_info->text = pStr;
			scroll_info->nextstatus = SCROLL_START;
			create_scroll_timer(widget);
		} else {
			//locate string and append " ..." to the end of string
			locate_string_to_fit(widget, textregionwidth, pStr, str, PATH_MAX, GR_TRUE);
		}

		GrText(wid, widget->base.gc, x, y,
				str, strlen(str), MWTF_UTF8 | GR_TFTOP);
	}
}

static void
ListboxKeyUpdownDraw(NX_WIDGET *widget, int range)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;
    GR_RECT redrawarea;
    int preItemIndex;
    int i;
    int p;
    int itemwidth;
    int itemheight;
    int fontheight = NxGetFontHeight(widget->base.fontid);
    int selwidth, selheight;
    GR_COORD x, y;

    itemwidth = widget->base.width;
    if (lbx->pitems < lbx->numitems)
        itemwidth -= lbx->sbar.width;
//    itemheight = fontheight + lbx->textmargin.top + lbx->textmargin.bottom +
//                 lbx->itemmargin.top + lbx->itemmargin.bottom;
//    lbx->itemheight = itemheight;
	itemheight = lbx->itemheight;

    redrawarea.width = itemwidth;
    redrawarea.height = itemheight * 2;
    redrawarea.x = 0;

    switch (range)
    {
    case 0:
        if (lbx->indexitem == 0)
            preItemIndex = lbx->numitems - 1;
        else
            preItemIndex = 0;
        break;
    case -1:
        preItemIndex = lbx->indexitem + 1;
        redrawarea.y = lbx->indexitem * itemheight;
        break;
    case 1:
        preItemIndex = lbx->indexitem - 1;
        redrawarea.y = (lbx->indexitem - 1) * itemheight;
        break;
    case -2:
        preItemIndex = lbx->indexitem + 1;
        redrawarea.y = 0;
        GrCopyArea(widget->base.wid, widget->base.gc, 0, redrawarea.height,
                   redrawarea.width, (lbx->pitems - 2) * itemheight,
                   widget->base.wid, 0, itemheight, 0);
        break;
    case 2:
        preItemIndex = lbx->indexitem - 1;
        redrawarea.y = (lbx->indexitem - 1) * itemheight;
        GrCopyArea(widget->base.wid, widget->base.gc, 0, 0,
                   redrawarea.width, (lbx->pitems - 2) * itemheight,
                   widget->base.wid, 0, itemheight, 0);
        break;
    default:
        return;
    }

	if (lbx->bScroll)
	{
		scroll_info_t* scroll_info = &lbx->scroll_info;
		scroll_info->nextstatus = SCROLL_IDLE;
		destory_scroll_timer(widget);
	}

    // clear redraw area
    if (range == 0)
    {
        if (widget->base.bgpixmap > 0)
        {
            GrCopyArea(widget->base.wid, widget->base.gc, 0, 0, redrawarea.width, itemheight,
                       widget->base.bgpixmap, 0, 0, 0);
            GrCopyArea(widget->base.wid, widget->base.gc, 0, (lbx->numitems - 1) * itemheight, redrawarea.width, itemheight,
                       widget->base.bgpixmap, 0, (lbx->numitems - 1) * itemheight, 0);
        }
        else
        {
            NxSetGCForeground (widget->base.gc, widget->base.bgcolor);
            GrFillRect(widget->base.wid, widget->base.gc, 0, 0, redrawarea.width, itemheight);
            GrFillRect(widget->base.wid, widget->base.gc, 0, (lbx->numitems - 1) * itemheight, redrawarea.width, itemheight);
        }
    }
    else
    {
        if (widget->base.bgpixmap > 0)
            GrCopyArea(widget->base.wid, widget->base.gc, redrawarea.x, redrawarea.y, redrawarea.width, redrawarea.height,
                       widget->base.bgpixmap, redrawarea.x, redrawarea.y, 0);
        else
        {
            NxSetGCForeground (widget->base.gc, widget->base.bgcolor);
            GrFillRect(widget->base.wid, widget->base.gc, redrawarea.x, redrawarea.y, redrawarea.width, redrawarea.height);
        }
    }

    x = lbx->itemmargin.left;
	if (lbx->ismultiselec)
		x += lbx->itemheight / 2;
    y = lbx->itemmargin.top;

    if (lbx->cb_getitem)
    {
        for (i = 0, p = lbx->topitem;
             i < lbx->pitems && i < lbx->numitems;
             i++, p++, y += itemheight)
        {
            if (p == (lbx->topitem + lbx->indexitem) ||
            	p == (lbx->topitem + preItemIndex))
				ListboxDrawItem(widget, widget->base.wid, p, x, y, itemwidth);
        }

        // draw scrollbar
        if (lbx->pitems < lbx->numitems)
        {
			NxSbarDraw(widget, &lbx->sbar,
					lbx->topitem + lbx->indexitem, lbx->pitems, lbx->numitems,
					widget->base.width - lbx->sbar.width, 0, widget->base.height);
        }
    }

	if (lbx->ismultiselec)
		ListboxDrawAllSelectedmark(widget);
}

// System APIs
void
NxListboxCreate (NX_WIDGET *widget)
{
    GR_WM_PROPERTIES props;
    struct widgetbase * base = &widget->base;
    NX_LISTBOX * lbx = &widget->spec.listbox;

    NxSetGCForeground(base->gc, base->fgcolor);
    NxSetGCBackground(base->gc, base->bgcolor);

    if(base->fontid<=0)
    {
        base->fontid=NxGetFontID(base->fontname, base->fontsize);
        GrSetGCFont (base->gc, base->fontid);
    }

    if (base->height == 0)
        base->height = LISTBOX_HEIGHT;
    if (base->width == 0)
        base->width = LISTBOX_WIDTH;

    base->wid = GrNewWindow(widget->parent->base.wid,
                          base->posx, base->posy,
                          base->width, base->height, 0,
                          base->bgcolor, base->fgcolor);

    props.flags = GR_WM_FLAGS_PROPS;
    props.props = GR_WM_PROPS_NOBACKGROUND;
    GrSetWMProperties (base->wid, &props);

    if (base->name == NULL) {
    	char str[WIDGET_NAME_LENGTH];
    	snprintf(str, WIDGET_NAME_LENGTH, "Listbox%d", base->wid);
        base->name = strdup(str);
    }

    lbx->cb_exposure.fp			= NULL;
    lbx->cb_exposure.inherit	= GR_TRUE;
    lbx->cb_keydown.fp			= NULL;
    lbx->cb_keydown.inherit		= GR_TRUE;
    lbx->cb_keyup.fp			= NULL;
    lbx->cb_keyup.inherit		= GR_TRUE;
    lbx->cb_focusin.fp			= NULL;
    lbx->cb_focusin.inherit		= GR_TRUE;
    lbx->cb_focusout.fp			= NULL;
    lbx->cb_focusout.inherit	= GR_TRUE;
    lbx->cb_destroy.fp			= NULL;
    lbx->cb_destroy.inherit		= GR_TRUE;

    GrSetGCUseBackground(base->gc, GR_FALSE);
    lbx->indexitem = 0;
    lbx->pitems = 0;
    lbx->topitem = 0;
    lbx->numitems = 0;

	if (lbx->bScroll) {
		if (lbx->scroll_interval == 0)
			lbx->scroll_interval = DEF_SCROLL_TIME;
		if (lbx->scroll_endingscrollcnt == 0)
			lbx->scroll_endingscrollcnt = DEF_ENDING_SCROLL_TIMES;
	}

    GrSelectEvents (base->wid, GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
                               GR_EVENT_MASK_EXPOSURE |
                               GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT |
                               GR_EVENT_MASK_TIMER);
}

void
NxListboxDestroy(NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;

	if (!nxSlistEmpty(&lbx->multiseleclist))
		nxSlistFree(&lbx->multiseleclist, offsetof(struct multiselect, list));
	if (lbx->scroll_info.scrolltimer > 0)
        GrDestroyTimer(lbx->scroll_info.scrolltimer);
    if (lbx->cb_destroy.fp != NULL)
        (*(lbx->cb_destroy.fp))(widget, lbx->cb_destroy.dptr);

    NxDeleteFromRegistry(widget);
}

void
NxListboxKeyDownHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;
    int range = 0;
    int left_item = lbx->pitems / 2;

	if (NeuxAppGetKeyLock(0)) {
		if (!event->keystroke.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
		}
		return;
	}

    if (lbx->cb_keydown.inherit && lbx->numitems > 0)
    {
        int topitem = lbx->topitem;
        int indexitem = lbx->indexitem;

        switch (event->keystroke.ch)
        {
        case MWKEY_DOWN:
        	if (!lbx->bWrapAround && lbx->topitem + lbx->indexitem >= lbx->numitems - 1)
        		break;
            lbx->indexitem++;
            if (lbx->topitem + lbx->indexitem > lbx->numitems - 1)
            {
                lbx->indexitem = 0;
                lbx->topitem = 0;
                if (lbx->pitems < lbx->numitems)
                {
                    NxListboxDraw(widget);
                    break;
                }
            }
            else
            {
            	if (lbx->indexitem > left_item && lbx->topitem < lbx->numitems - lbx->pitems)
		 		{
			 		lbx->topitem++;
			 		lbx->indexitem = left_item;
					NxListboxDraw(widget);
					break;
		 		}
				else
				{
	                if (lbx->indexitem >= lbx->pitems)
	                {
	                    range = 2;
	                    lbx->topitem++;
	                    lbx->indexitem = lbx->pitems - 1;
	                    if (widget->base.bgpixmap > 0)
	                    {
	                        NxListboxDraw(widget);
	                        break;
	                    }
	                }
	                else
	                    range = 1;
				}
            }
            if (indexitem != lbx->indexitem || topitem != lbx->topitem)
                ListboxKeyUpdownDraw(widget, range);
            break;
        case MWKEY_UP:
        	if (!lbx->bWrapAround && lbx->topitem == 0 && lbx->indexitem == 0)
        		break;
			if (lbx->topitem > 0)
			{
				if (lbx->indexitem == left_item)
				{
					lbx->topitem--;
					NxListboxDraw(widget);
					break;
				}
				else if (lbx->indexitem >= lbx->numitems - left_item - lbx->topitem)
				{
					lbx->indexitem--;
					range = -1;
				}
				else
				{
					lbx->indexitem--;
					range = -1;
				}
				if (lbx->indexitem < 0)
				{
					lbx->topitem--;
					lbx->indexitem = 0;
					NxListboxDraw(widget);
					break;
				}
			}
			else
			{
				lbx->indexitem--;
	            if (lbx->indexitem < 0)
	            {
	                lbx->topitem--;
	                lbx->indexitem = 0;
	                if (lbx->topitem < 0)
	                {
	                    if (lbx->numitems > lbx->pitems)
	                    {
	                        lbx->topitem = lbx->numitems - lbx->pitems;
	                        lbx->indexitem = lbx->pitems - 1;
	                        NxListboxDraw(widget);
	                        break;
	                    }
	                    else
	                    {
	                        lbx->topitem = 0;
	                        lbx->indexitem = lbx->numitems - 1;
	                    }
	                }
	                else
	                {
	                    range = -2;
	                    if (widget->base.bgpixmap > 0)
	                    {
	                        NxListboxDraw(widget);
	                        break;
	                    }
	                }
	            }
	            else
	                range = -1;
			}
            if (indexitem != lbx->indexitem || topitem != lbx->topitem)
                ListboxKeyUpdownDraw(widget, range);
            break;
        // '*', multi-selection
        case '*':
			if (widget->spec.listbox.ismultiselec)
			{
				slist_t* pitem;
				if ((pitem = IsInMultiselectlist(lbx, lbx->topitem + lbx->indexitem)))
				{
					RemoveFromMultiselectlist(lbx, pitem);
					ListboxDrawSingleSelectedmark(widget, lbx->topitem + lbx->indexitem, GR_FALSE);
				}
				else
				{
					AddToMultiselectlist(lbx, lbx->topitem + lbx->indexitem);
					ListboxDrawSingleSelectedmark(widget, lbx->topitem + lbx->indexitem, GR_TRUE);
				}
			}
			break;
		// page up
		case MWKEY_PAGEUP:
			if (!lbx->bWrapAround && lbx->topitem == 0 && lbx->indexitem == 0)
        		break;
			if (lbx->indexitem == 0)
			{
				lbx->topitem -= lbx->pitems;
				if (lbx->topitem < 0)
					lbx->topitem = 0;
			}
			else
				lbx->indexitem = 0;
			if (indexitem != lbx->indexitem || topitem != lbx->topitem)
				NxListboxDraw(widget);
			break;
		// page down
		case MWKEY_PAGEDOWN:
			if (!lbx->bWrapAround && lbx->topitem + lbx->indexitem >= lbx->numitems - 1)
        		break;
			if (lbx->indexitem == lbx->pitems - 1)
			{
				lbx->topitem += lbx->pitems;
				if (lbx->topitem + lbx->pitems > lbx->numitems)
					lbx->topitem = lbx->numitems - lbx->pitems;
			}
			else
			{
				if (lbx->numitems < lbx->pitems)
					lbx->indexitem = lbx->numitems - 1;
				else
					lbx->indexitem = lbx->pitems - 1;
			}
			if (indexitem != lbx->indexitem || topitem != lbx->topitem)
				NxListboxDraw(widget);
			break;
        }
    }

    if (lbx->cb_keydown.fp != NULL)
        (*(lbx->cb_keydown.fp))(widget, lbx->cb_keydown.dptr);
}

void
NxListboxKeyUpHandler (GR_EVENT *event, NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;

    if (lbx->cb_keyup.fp != NULL)
        (*(lbx->cb_keyup.fp))(widget, lbx->cb_keyup.dptr);
}

static void NxListboxRedrawScrollRegion(NX_WIDGET * widget)
{
	scroll_info_t* scroll_info = &widget->spec.listbox.scroll_info;
    NX_LISTBOX * lbx = &widget->spec.listbox;

#ifdef FASTGUI
	NxSetGCForeground(widget->base.gc, lbx->selectedcolor);
	GrFillRect(widget->base.wid, widget->base.gc,
				scroll_info->textregion.x - lbx->textmargin.left, scroll_info->textregion.y,
				scroll_info->textregion.w + lbx->textmargin.left + lbx->textmargin.right, scroll_info->textregion.h);
#else
	NxPixmapMaskColor(widget->base.wid, widget->base.gc,
				scroll_info.textregion.x - lbx->textmargin.left, scroll_info.textregion.y,
				scroll_info.textregion.w + lbx->textmargin.left + lbx->textmargin.right, scroll_info.textregion.h,
				lbx->selectedfgcolor);
#endif
	if (lbx->cb_getitemfgcolor != NULL)
		NxSetGCForeground(widget->base.gc, (*(lbx->cb_getitemfgcolor))(widget->base.wid, lbx->indexitem));
	else
		NxSetGCForeground(widget->base.gc, lbx->selectedfgcolor);
 	GrText(widget->base.wid, widget->base.gc, scroll_info->textregion.x, scroll_info->textregion.y,
 		   scroll_info->text + scroll_info->offset, scroll_info->showlen, MWTF_UTF8 | GR_TFTOP);

	/* Redraw scroll bar with timer go on */
	NxSbarDraw(widget, &lbx->sbar,
			lbx->topitem + lbx->indexitem, lbx->pitems, lbx->numitems,
			widget->base.width - lbx->sbar.width, 0, widget->base.height);
}

void
NxListboxScrollTimerHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	scroll_info_t* scroll_info = &widget->spec.listbox.scroll_info;

	switch (scroll_info->nextstatus)
	{
	case SCROLL_IDLE:
		return;
	case SCROLL_START:
        scroll_info->offset = 0;
        scroll_info->nextstatus = SCROLL_RUNNING;
    case SCROLL_RUNNING:
		scroll_info->offset = utf8_next_nchar(scroll_info->text, scroll_info->offset, DEF_CHARS_PER_SCROLL);
		scroll_info->showlen = locate_string_to_fit(widget, scroll_info->textregion.w, scroll_info->text + scroll_info->offset, NULL, 0, GR_FALSE);
		if (widget->spec.listbox.bScrollBidi) {
			if (scroll_info->showlen == strlen(scroll_info->text + scroll_info->offset))
				scroll_info->nextstatus = SCROLL_ENDING;
		} else {
			if (scroll_info->showlen == 0) {
				scroll_info->offset = 0;
				scroll_info->showlen = locate_string_to_fit(widget, scroll_info->textregion.w, scroll_info->text, NULL, 0, GR_FALSE);
				scroll_info->nextstatus = SCROLL_ENDING;
			}
		}
		break;
	case SCROLL_ENDING:
		if (widget->spec.listbox.bScrollBidi)
			scroll_info->nextstatus = SCROLL_REVRUNNING;
		else
        	scroll_info->nextstatus = SCROLL_RUNNING;
		return;
	case SCROLL_REVRUNNING:
        scroll_info->offset = utf8_next_nchar(scroll_info->text, scroll_info->offset, -DEF_CHARS_PER_SCROLL);
        scroll_info->showlen = locate_string_to_fit(widget, scroll_info->textregion.w, scroll_info->text + scroll_info->offset, NULL, 0, GR_FALSE);
		if (scroll_info->offset == 0)
			scroll_info->nextstatus = SCROLL_REVENDING;
		break;
	case SCROLL_REVENDING:
		scroll_info->nextstatus = SCROLL_START;
		if (widget->spec.listbox.scroll_endingscrollcnt < 0)
			return;
		if (++scroll_info->scrollcnt == widget->spec.listbox.scroll_endingscrollcnt) {
			scroll_info->nextstatus = SCROLL_IDLE;
			destory_scroll_timer(widget);
		}
		return;
	default:
		return;
	}

    NxListboxRedrawScrollRegion(widget);
}

void
NxListboxTimerHandler (GR_EVENT *event, NX_WIDGET *widget)
{
	NxListboxScrollTimerHandler (event, widget);
}


void
NxListboxExposureHandler(GR_EVENT *event, NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;

    if (lbx->cb_exposure.inherit)
        NxListboxDraw(widget);
    if (lbx->cb_exposure.fp != NULL)
        (*(lbx->cb_exposure.fp))(widget, lbx->cb_exposure.dptr);
}

void
NxListboxFocusInHandler(GR_EVENT *event, NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;

    if (lbx->cb_focusin.fp != NULL)
        (*(lbx->cb_focusin.fp))(widget, lbx->cb_focusin.dptr);
    if (lbx->cb_focusin.inherit)
        ;
}

void
NxListboxFocusOutHandler(GR_EVENT *event, NX_WIDGET *widget)
{
    NX_LISTBOX * lbx = &widget->spec.listbox;

	lbx->scroll_info.nextstatus = SCROLL_IDLE;
	if (lbx->scroll_info.scrolltimer > 0)
		GrDestroyTimer(lbx->scroll_info.scrolltimer);

    if (lbx->cb_focusout.fp != NULL)
        (*(lbx->cb_focusout.fp))(widget, lbx->cb_focusout.dptr);
    if (lbx->cb_focusout.inherit)
        ;
}

void
NxListboxDraw (NX_WIDGET * listbox)
{
    GR_WINDOW_INFO winfo;
    GR_WINDOW_ID tempListboxPixmapID;
    int fontheight;
    int i, p, itemwidth, itemheight;

    GR_COORD x, y;
    NX_LISTBOX * lbx;
    struct widgetbase * base;
    scroll_info_t* scroll_info;

    if (!NxGetWidgetAbsVisible(listbox)) return;

    lbx = &listbox->spec.listbox;
    base = &listbox->base;

    tempListboxPixmapID = GrNewPixmap(base->width, base->height, NULL);
    if (tempListboxPixmapID == 0)
        tempListboxPixmapID = base->wid;

    if (base->bgpixmap > 0 && base->usebgpixmap)
        GrCopyArea(tempListboxPixmapID, base->gc, 0, 0,
                   base->width, base->height,
                   base->bgpixmap, 0, 0, 0);
    else
    {
        NxSetGCForeground (base->gc, base->bgcolor);
        GrFillRect(tempListboxPixmapID, base->gc, 0, 0, base->width, base->height);
        NxSetGCForeground (base->gc, base->fgcolor);
    }

    lbx->numitems = lbx->cb_getitem(base->wid, -1, NULL);

    //replace this with saved info.------------------
    //doesn't support wresize, but we don't need that.
    winfo.width = base->width;
    winfo.height = base->height;
    //-----------------------------------------------

    //replace with local font height routines.-------
    fontheight = NxGetFontHeight(base->fontid);
    //-----------------------------------------------

//    lbx->itemheight = fontheight + lbx->textmargin.top + lbx->textmargin.bottom +
//                      lbx->itemmargin.top + lbx->itemmargin.bottom;
//
//    lbx->pitems = winfo.height / lbx->itemheight;

//    if (lbx->pitems <= lbx->numitems &&
//        lbx->topitem + lbx->pitems > lbx->numitems)
//    {
//        lbx->indexitem += lbx->topitem + lbx->pitems - lbx->numitems;
//        lbx->topitem = lbx->numitems - lbx->pitems;
//    }
//
//    if (lbx->indexitem > lbx->numitems - 1 || lbx->indexitem > lbx->pitems - 1)
//        lbx->indexitem = 0;

    itemheight = lbx->itemheight;
	itemwidth = winfo.width;
    if (lbx->pitems < lbx->numitems)
        itemwidth -= lbx->sbar.width;

    x = lbx->itemmargin.left;
    y = lbx->itemmargin.top;

	if (lbx->bScroll)
	{
		scroll_info = &lbx->scroll_info;
		scroll_info->nextstatus = SCROLL_IDLE;
		destory_scroll_timer(listbox);
	}

	// no item to list
	if (lbx->numitems <= 0)
	{
		int selwidth = itemwidth - lbx->itemmargin.left - lbx->itemmargin.right;
		int selheight = itemheight - lbx->itemmargin.top - lbx->itemmargin.bottom;
#ifdef FASTGUI
		NxSetGCForeground(base->gc, lbx->selectedcolor);
		GrFillRect(tempListboxPixmapID, base->gc, x, y, selwidth, selheight);
#else
		NxPixmapMaskColor(tempListboxPixmapID, base->gc, x, y,
						  selwidth, selheight, lbx->selectedcolor);
#endif
		if (lbx->cb_getitemfgcolor != NULL)
			NxSetGCForeground(base->gc, (*(lbx->cb_getitemfgcolor))(base->wid, 0));
		else
			NxSetGCForeground (base->gc, lbx->selectedfgcolor);

		if (tempListboxPixmapID != 0 && tempListboxPixmapID != base->wid)
		{
			GrCopyArea(base->wid, base->gc, 0, 0, base->width, base->height, tempListboxPixmapID, 0, 0, 0);
			GrDestroyWindow(tempListboxPixmapID);
			tempListboxPixmapID = 0;
		}
		return;
	}

	if (lbx->ismultiselec)
		x += lbx->itemheight / 2;

    //i = 0, x = 3, y = 2
    for (i = 0, p = lbx->topitem;
         i < lbx->pitems && i < lbx->numitems;
         i++, p++, y+= itemheight)
    {
    	ListboxDrawItem(listbox, tempListboxPixmapID, p, x, y, itemwidth);
    }

    if (tempListboxPixmapID != 0 && tempListboxPixmapID != base->wid)
    {
        GrCopyArea(base->wid, base->gc, 0, 0, base->width, base->height, tempListboxPixmapID, 0, 0, 0);
        GrDestroyWindow(tempListboxPixmapID);
        tempListboxPixmapID = 0;
    }

    if (lbx->pitems < lbx->numitems)
    {
		NxSbarDraw(listbox, &lbx->sbar,
				lbx->topitem + lbx->indexitem, lbx->pitems, lbx->numitems,
                winfo.width - lbx->sbar.width, 0, winfo.height);
    }

	if (lbx->ismultiselec)
		ListboxDrawAllSelectedmark(listbox);
}

void
NxListboxRecalcPitems (NX_WIDGET * listbox)
{
    NX_LISTBOX * lbx;
    struct widgetbase * base;
    int fontheight;

    lbx = &listbox->spec.listbox;
    base = &listbox->base;

//    lbx->numitems = lbx->cb_getitem(base->wid, -1, NULL);

    fontheight = NxGetFontHeight(base->fontid);

    lbx->itemheight = fontheight + lbx->textmargin.top + lbx->textmargin.bottom +
                      lbx->itemmargin.top + lbx->itemmargin.bottom;

    lbx->pitems = base->height / lbx->itemheight;
}

void
NxNeuxListboxSetIndex(NX_WIDGET * listbox, int index)
{
    NX_LISTBOX * lbx = &listbox->spec.listbox;

	lbx->numitems = lbx->cb_getitem(listbox->base.wid, -1, NULL);

	if (lbx->numitems <= lbx->pitems) {
		lbx->topitem = 0;
		lbx->indexitem = index;
	} else {
		int left_item = lbx->pitems / 2;
		if (index <= left_item) {
			lbx->topitem = 0;
        	lbx->indexitem = index;
		} else if (index >= lbx->numitems - left_item) {
			lbx->topitem = lbx->numitems - lbx->pitems;
			lbx->indexitem = index - lbx->topitem;
		} else {
			lbx->indexitem = left_item;
			lbx->topitem = index - left_item;
		}
	}

    NxListboxDraw(listbox);
}

void
NxNeuxListboxSetItem(NX_WIDGET * listbox, int index, int top)
{
    NX_LISTBOX * lbx = &listbox->spec.listbox;

    lbx->topitem = top;
    lbx->indexitem = index;

    NxListboxDraw(listbox);
}

int
NxListboxGetMultiSelectedCount(NX_WIDGET * listbox)
{
	if (!listbox->spec.listbox.ismultiselec)
		return 0;

	return listbox->spec.listbox.selectedcount;
}

int
NxListboxGetMultiSelectedData(NX_WIDGET * listbox, int* selectedIndexarray, int arraysize)
{
	if (!listbox->spec.listbox.ismultiselec || arraysize == 0)
		return 0;

	int count = 0;
	slist_t* pitem = listbox->spec.listbox.multiseleclist.next;
	while (pitem && count <= arraysize)
	{
		struct multiselect *msel = container_of(pitem, struct multiselect, list);
		selectedIndexarray[count++] = msel->index;
		pitem = pitem->next;
	}
	return count;
}

void
NxListboxClearMultiSelected(NX_WIDGET * listbox)
{
	if (!listbox->spec.listbox.ismultiselec)
		return;

	if (!nxSlistEmpty(&listbox->spec.listbox.multiseleclist))
	{
		nxSlistFree(&listbox->spec.listbox.multiseleclist, offsetof(struct multiselect, list));
		listbox->spec.listbox.selectedcount = 0;
	}
}
