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
 * NeuxMessageboxNew widget support routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-08-08 NW
 *
 */
//define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "nxlist.h"
#include "file-helper.h"
#include "plextalk-config.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "utf8_utils.h"
#include <stdio.h>
#include <math.h>
#include "./nx/nx-widgets.h"


#define NOT_USE_MULTI_SCREEN	1	//不使用多屏显示，而使用一行水平流动
#define MESSAGEBOX_SCROLL_TIME	500


//ztz
static void
MessageBoxSbarDraw (	WIDGET *widget, msgbox_context_t *cntx)
{	
	struct scrollbar sbar = {
		.width = SBAR_WIDTH,
		.fgcolor = theme_cache.foreground,
		.bgcolor = theme_cache.background,
		.bordercolor = theme_cache.foreground,	
	};

	//give 1 for rows
	NxSbarDraw(widget, &sbar, cntx->multi_count, 1 /*cntx->multi_screen_perlines*/,
		cntx->nr_lines - cntx->multi_screen_perlines, MAINWIN_WIDTH - SBAR_WIDTH, 0, MAINWIN_HEIGHT);
}
//ztz


static int 
MultiScreen (int max_height, int font_height, int linespace, int line_count)
{
	return ((font_height + linespace) * line_count) > (max_height + linespace) ? 1 : 0;
}


static int
MultiScreenPerlines (int max_height, int font_height, int linespace)
{
	//to be considered...
	return lroundf((float)(max_height + linespace) / (float)(font_height + linespace));
}


static void
onKeyDown(NX_WIDGET *sender, DATA_POINTER ptr)
{
	msgbox_context_t *cntx = NxGetWidgetData(sender);

	if (NeuxAppGetKeyLock(0)) {
		if (!sender->base.event.keystroke.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
		}
		return;
	}

	switch (sender->base.event.keystroke.ch) {

//modify by ztz  ---start----
#if NOT_USE_MULTI_SCREEN//appk
	case MWKEY_UP:
		if (cntx->multi_screen) {
			if (-- cntx->multi_count < 0)
				cntx->multi_count = 0;

			GrClearWindow(sender->base.wid, 1);
		}

		else if(cntx->item_lable) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		}
		break;
		
	case MWKEY_DOWN:
		DBGMSG("cntx->multi_count = %d\n", cntx->multi_count);
		DBGMSG("cntx->nr_lines = %d\n", cntx->nr_lines);
		DBGMSG("cntx->multi_screen_perlines = %d\n", cntx->multi_screen_perlines);
		if (cntx->multi_screen) {
			if (++ cntx->multi_count > cntx->nr_lines - cntx->multi_screen_perlines - 1)
				cntx->multi_count = cntx->nr_lines - cntx->multi_screen_perlines - 1;

			GrClearWindow(sender->base.wid, 1);
		}
		else if(cntx->item_lable) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		}
		break;
#endif		
//modify by ztz  ---end----

	case MWKEY_RIGHT:
	case MWKEY_ENTER:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Enter"));
		cntx->msgReturn = MSGBX_BTN_OK;
		cntx->endOfModal = 1;
		break;
	case MWKEY_LEFT:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Cancel"));
		cntx->msgReturn = MSGBX_BTN_CANCEL;
		cntx->endOfModal = 1;
		break;
	}

	if (!cntx->endOfModal  && ptr != NULL) {
		KEY__ tempkey;
		tempkey.ch = sender->base.event.keystroke.ch;
		tempkey.hotkey = sender->base.event.keystroke.hotkey;
		(*(KeydownCbfptr)ptr)(sender->base.wid, tempkey);
	}
}

/*
static void
OnWindowGetFocus(WID__ wid)
{
}
*/
static void
OnWindowRedraw(WID__ wid)
{
	NX_WIDGET *w = (NX_WIDGET *)NxGetFromRegistry(wid);
	msgbox_context_t *cntx = NxGetWidgetData(w);
	int font_height = NxGetFontHeight(w->base.fontid);
	text_line_t *pline ;

	int x = 0, y = 0;
	int i;

#if NOT_USE_MULTI_SCREEN//appk
	if(cntx->item_lable) {
		return;
	}
#endif

	pline = container_of(cntx->lines_head.next,	text_line_t, list);

	// modify by ztz 	---start---
	/* skip n lines */
	for (i = 0; i < cntx->multi_count; i ++)
		pline = container_of(pline->list.next, text_line_t, list);
	// modify by ztz 	----end----

	if (cntx->nr_lines == 1) {
		x = (MAINWIN_WIDTH - pline->width) / 2;
		y = (MAINWIN_HEIGHT - font_height) / 2;
	} else {
		int h = cntx->nr_lines * font_height + (cntx->nr_lines - 1) * cntx->line_spacing;
		if (h <= MAINWIN_HEIGHT)
			y = (MAINWIN_HEIGHT - h) / 2;
	}
	
	for (i = 0; i < cntx->nr_lines; i++) {
		GrText(w->base.wid, w->base.gc,
				x, y,
				pline->text, pline->len, MWTF_UTF8 | GR_TFTOP);
		y += font_height + cntx->line_spacing;

		pline = container_of(pline->list.next, text_line_t, list);
	}
	
	if (cntx->multi_screen)
		MessageBoxSbarDraw(w, cntx);		//ztz
}

static void
NeuxMessageboxSetCallback(FORM * form, int cbId, void * fptr)
{
	NX_WINDOW *me = &((NX_WIDGET *)form)->spec.container.spec.window;

	switch (cbId)
	{
	case CB_KEY_DOWN:
		NxRegisterCallBack(&me->cb_keydown, onKeyDown, fptr);
		break;
	default:
		if (fptr != NULL)
			NeuxFormSetCallback(form, cbId, fptr);
		break;
	}
}

/*
  NeuxMessageboxNew is modal, thus messagebox will not return before any button
  is clicked.
 */

/**
 * Function creates messagebox widget.
 *
 * @param title
 *      messagebox title if existed.
 * @param controls
 *      controls property in messagebox form.
 * @param controlcount
 *      controls count
 * @param timeout
 *      timeout time in seconds if nonzero.
 * @return
 *      button clicked.
 *
 */
int NeuxMessageboxNew(KeydownCbfptr keydown_cb, HotplugCbfptr hotplug_cb,
					int line_spacing, const char *format, ...)
{
	WIDGET *widget;
	widget_prop_t wprop;
	form_prop_t fprop;
	label_prop_t lprop;//appk
	msgbox_context_t *cntx;
	va_list args;
	ssize_t n;
	int ret;

	cntx = malloc(sizeof(*cntx));
	if (cntx == NULL)
		return -1;
	memset(cntx, 0, sizeof(*cntx));

	cntx->text = malloc(128);
	if (cntx->text == NULL) {
		free(cntx);
		return -1;
	}
	va_start(args, format);
	n = vsnprintf(cntx->text, 128, format, args);
	va_end(args);
	if (n >= 128) {
		const char *tmp = realloc(cntx->text, n + 1);
		if (tmp == NULL) {
			free(cntx->text);
			free(cntx);
			return -1;
		}

		va_start(args, format);
		vsnprintf(cntx->text, n + 1, format, args);
		va_end(args);
	}
	cntx->text_len = n;

	widget = NeuxFormNew( MAINWIN_LEFT,
						  MAINWIN_TOP,
						  MAINWIN_WIDTH,
						  MAINWIN_HEIGHT);

	//NeuxWidgetGetProp(widget, &wprop);
	//wprop.fgcolor = textcolor;
	//NeuxWidgetSetProp(widget, &wprop);
	NeuxFormGetProp(widget, &fprop);
	fprop.transparent = GR_TRUE;
	NeuxFormSetProp(widget, &fprop);

	NeuxMessageboxSetCallback(widget, CB_KEY_DOWN, keydown_cb);
	NeuxMessageboxSetCallback(widget, CB_HOTPLUG,  hotplug_cb);
//	NeuxMessageboxSetCallback(widget, CB_FOCUS_IN, OnWindowGetFocus);
	NeuxMessageboxSetCallback(widget, CB_EXPOSURE, OnWindowRedraw);

	NeuxWidgetSetFont(widget, sys_font_name, sys_font_size);

	NxSetWidgetData(widget, cntx);

	cntx->lines_head.next = NULL;
	cntx->nr_lines = utf8_fit_width(getenv("LANG"), ((NX_WIDGET *)widget)->base.gc,
				MAINWIN_WIDTH, cntx->text, cntx->text_len, &cntx->lines_head);
	cntx->line_spacing = line_spacing;


//	added by ztz	-----start----
	cntx->multi_screen = MultiScreen(MAINWIN_HEIGHT, NxGetFontHeight(((NX_WIDGET *)widget)->base.fontid), 
		cntx->line_spacing, cntx->nr_lines);

	DBGMSG("cntx->multi_screen = %d\n", cntx->multi_screen);

#if NOT_USE_MULTI_SCREEN//appk
	if(cntx->multi_screen) {
		int h,x,y;
		cntx->multi_screen = 0;

		h = sys_font_size+6;
		y = (MAINWIN_HEIGHT - h)/2;
		
		cntx->item_lable = NeuxLabelNew(widget,0,y,MAINWIN_WIDTH,h, cntx->text);
		
		NeuxWidgetGetProp(cntx->item_lable, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(cntx->item_lable, &wprop);
		
		NeuxLabelGetProp(cntx->item_lable, &lprop);
		lprop.align = LA_LEFT;
		lprop.autosize = GR_FALSE;
		NeuxLabelSetProp(cntx->item_lable, &lprop);

		NeuxLabelSetScroll(cntx->item_lable, MESSAGEBOX_SCROLL_TIME);
		NeuxWidgetSetFont(cntx->item_lable, sys_font_name, sys_font_size);		
		NeuxWidgetShow(cntx->item_lable, TRUE);
		

		
	} else {
		cntx->item_lable = NULL;
	}
#endif
	

	if (cntx->multi_screen) {
		cntx->nr_lines = utf8_fit_width(getenv("LANG"), ((NX_WIDGET *)widget)->base.gc,
			MAINWIN_WIDTH - SBAR_WIDTH, cntx->text, cntx->text_len, &cntx->lines_head);
		cntx->multi_screen_perlines = MultiScreenPerlines(MAINWIN_HEIGHT, NxGetFontHeight(((NX_WIDGET *)widget)->base.fontid), 
			cntx->line_spacing);
		DBGMSG("cntx->multi_screen_perlines = %d\n", cntx->multi_screen_perlines);

		MessageBoxSbarDraw(widget, cntx);
	}
//	ztz		----end---

	voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, cntx->text, cntx->text_len);

	NeuxWidgetShow(widget, TRUE);
	NxSetWidgetFocus(widget);

	ret = NeuxDoModal(&cntx->endOfModal, &cntx->msgReturn);

	NeuxFormDestroy(&widget);

	nxSlistFree(&cntx->lines_head, offsetof(text_line_t, list));
	free(cntx->text);
	free(cntx);

	return ret;
}
