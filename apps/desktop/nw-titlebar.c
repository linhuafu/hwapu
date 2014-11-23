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
 * Form TitleBar routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
#define OSD_DBG_MSG
#include "nc-err.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"
#include "nw-titlebar.h"
#include "nw-main.h"

#define TITLEBAR_FONT_HEIGHT	12
#define TITLEBAR_SCROLL_TIME	500

static LABEL *label1;
static WIDGET *line1;

//added by ztz
static GR_WINDOW_ID label1_pix;
static GR_WINDOW_ID line1_pix;


void
CreateTitleBar(FORM *widget)
{
	widget_prop_t wprop;
	label_prop_t lbprop;

	/*
	 * title
	 */
	//modify by ztz
	label1 = NeuxLabelNew(widget, TITLEBAR_LEFT, TITLEBAR_TOP, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1, NULL);

	//added by ztz
	label1_pix = GrNewPixmap(TITLEBAR_WIDTH, TITLEBAR_HEIGHT - 1, NULL);

	NeuxWidgetGetProp(label1, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(label1, &wprop);

	NeuxLabelGetProp(label1, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_LEFT;
	lbprop.transparent = FALSE;//TRUE;
	NeuxLabelSetProp(label1, &lbprop);

	NeuxWidgetSetFont(label1, /*sys_font_name*/"arialuni.ttf", TITLEBAR_FONT_HEIGHT);
	NeuxLabelSetScroll(label1, TITLEBAR_SCROLL_TIME);

	NeuxWidgetShow(label1, FALSE);

	/*
	 * line
	 */
	line1 = NeuxLineNew(widget, TITLEBAR_LEFT, TITLEBAR_TOP + TITLEBAR_HEIGHT - 1, TITLEBAR_WIDTH, 1);

	//added by ztz
	line1_pix = GrNewPixmap(TITLEBAR_WIDTH, 1, NULL);

	NeuxWidgetGetProp(line1, &wprop);
	wprop.bgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(line1, &wprop);

	NeuxWidgetShow(line1, TRUE);
}


void
TitleBarSetContent(const char* title)
{
	NeuxThemeInit(desktop_theme_index);	//ztz

	DBGMSG("Desktop set titlebar content called!!!!\n");
	
	if (title == NULL || *title == '\0') {
//		NeuxWidgetShow(label1, FALSE);
//		NeuxLabelSetText(label1, NULL);

		DBGMSG("Set title bar to NULL!!!!\n");
		/* No unmap, just Fill it with background */
		GR_GC_ID gc = GrNewGC();
		GR_WINDOW_ID wid = NeuxWidgetWID(label1);
		
		GrSetGCForeground(gc, theme_cache.background);
		GrSetGCBackground(gc, theme_cache.background);
		GrFillRect(wid, gc, 0, 0, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1);		
		GrDestroyGC(gc);
	} else {
		DBGMSG("Set title bar text, title = %s\n", title);
		NeuxLabelSetText(label1, title);
		NeuxWidgetShow(label1, TRUE);

		/* make sure it will flush */
		GrSetGCForeground(NeuxWidgetGC(label1), theme_cache.background);
		GrSetGCBackground(NeuxWidgetGC(label1), theme_cache.background);
		GrFillRect(label1_pix, NeuxWidgetGC(label1), 
			0, 0, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1);		
		GrCopyArea(NeuxWidgetWID(label1), NeuxWidgetGC(label1), 
			0, 0, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1,
			label1_pix, 0, 0, 0);
		GrSetGCForeground(NeuxWidgetGC(label1), theme_cache.foreground);
		GrText(NeuxWidgetWID(label1), NeuxWidgetGC(label1), 0, 0, NeuxLabelGetText(label1),
			strlen(NeuxLabelGetText(label1)), MWTF_UTF8 | MWTF_TOP);
		GrFlush();
	}
}


//Modify by ztz	-----start---
void
RefreshTitlebar(int confirm)
{
	widget_prop_t wprop;
	label_prop_t lbprop;

	if (confirm) {
		NeuxWidgetGetProp(label1, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(label1, &wprop);

		NeuxWidgetGetProp(line1, &wprop);
		wprop.bgcolor = theme_cache.foreground;
		NeuxWidgetSetProp(line1, &wprop);

		if (NeuxLabelGetText(label1))
			NeuxWidgetShow(label1, TRUE);
		NeuxWidgetShow(line1, TRUE);
	} else {

		NeuxWidgetGetProp(label1, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(label1, &wprop);

		NeuxWidgetGetProp(line1, &wprop);
		wprop.bgcolor = theme_cache.foreground;
		NeuxWidgetSetProp(line1, &wprop);

		/* Refresh label1 with GrCopyArea */
		GrSetGCForeground(NeuxWidgetGC(label1), theme_cache.background);
		GrSetGCBackground(NeuxWidgetGC(label1), theme_cache.background);
		GrFillRect(label1_pix, NeuxWidgetGC(label1), 
			0, 0, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1);		
		GrCopyArea(NeuxWidgetWID(label1), NeuxWidgetGC(label1), 
			0, 0, TITLEBAR_WIDTH , TITLEBAR_HEIGHT - 1,
			label1_pix, 0, 0, 0);

		GrSetGCForeground(NeuxWidgetGC(label1), theme_cache.foreground);
		GrText(NeuxWidgetWID(label1), NeuxWidgetGC(label1), 0, 0, NeuxLabelGetText(label1),
			strlen(NeuxLabelGetText(label1)), MWTF_UTF8 | MWTF_TOP);


		/* Refresh line1 with GrCopyArea */
		GrSetGCForeground(NeuxWidgetGC(line1), theme_cache.foreground);
		GrSetGCBackground(NeuxWidgetGC(line1), theme_cache.foreground);
		GrFillRect(line1_pix, NeuxWidgetGC(line1), 0, 0, TITLEBAR_WIDTH, 1);
		GrCopyArea(NeuxWidgetWID(line1), NeuxWidgetGC(line1), 0, 0, 
			TITLEBAR_WIDTH, 1, line1_pix, 0, 0, 0);
	}
}
//Modify by ztz	-----end---
