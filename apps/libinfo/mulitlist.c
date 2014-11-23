#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linebreak.h>
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include "glib.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "mulitlist.h"
#include "widgets.h"
#include "Nx-widgets.h"
#include "plextalk-i18n.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"

//定义滚动条长度
#define SCROLL_BAR_WIDTH 5

void MultiSbarDraw(struct MultilineList *thiz);

void multiline_list_free_item(void *ctx, void *data)
{
	DList *list = (DList *)data;

	if(list)
	{
		dlist_destroy(list);
	}
}

void multiline_list_handle_faile(struct MultilineList *thiz)
{
	if(thiz){
//		if(thiz->bar){
//			scroll_bar_destroy(thiz->bar);
//		}
		if(thiz->list){
			dlist_destroy(thiz->list);
			thiz->list = NULL;
		}

		if(thiz->scr_str)
		{
			free(thiz->scr_str);
			thiz->scr_str = NULL;
		}
		
		if(thiz->gc){
			GrDestroyGC(thiz->gc);
			thiz->gc = 0;
		}
		
		if(thiz->font_id)
		{
			GrDestroyFont(thiz->font_id);
			thiz->font_id = 0;
		}
		
		free(thiz);
		thiz = NULL;
	}
}


int multiline_list_cal_text_len(struct MultilineList *thiz, char *text, int txtlen)
{
	GR_SIZE gwidth, gheight, gbase;

	GrGetGCTextSize(thiz->gc, (void*)text, txtlen, MWTF_UTF8,
		&gwidth, &gheight, &gbase);
	
	return gwidth;
}

void multiline_line_item_free(struct MLineItem *thiz)
{
	if(thiz){
		free(thiz);
		thiz = NULL;
	}
}

void free_line_item(void *ctx, void*data)
{
	struct MLineItem *thiz = (struct MLineItem *)data;

	multiline_line_item_free(thiz);
	DBGMSG("free_line_item \n");
}


static int utf8_text_to_fit(GR_GC_ID gc, GR_SIZE max_width,
					const char *text, size_t len, GR_SIZE* width)
{
	GR_SIZE w, h, b;
	int first = 0;
	GR_SIZE first_width = 0;

	while (len > 0) {
		int half = len / 2;
		while ((text[first + half] & 0xc0) == 0x80)
			half--;
		if (half == 0)
			break;

		GrGetGCTextSize(gc, (char*)text + first, half, MWTF_UTF8, &w, &h, &b);
		if (first_width + w > max_width)
			len = half;
		else {
			first_width += w;
			first += half;
			len -= half;
		}
	}

	if (width)
		*width = first_width;
	return first;
}


static int parse_multiline(struct MultilineList *thiz, char *text, int len)
{
	GR_SIZE w, h, b, last_w;
	char *brks = NULL;
	struct MLineItem *item = NULL;
	int i, line_start, last_word;
	int line_count = 0;
	int width = thiz->show_width;

	brks = malloc(len);
	if (brks == NULL)
		return -1;
	thiz->tmplist = dlist_create(NULL, NULL);
	if(thiz->tmplist == NULL)
	{
		free(brks);
		return -1;
	}

	GrGetGCTextSize(thiz->gc, (void*)text, len, MWTF_UTF8, &w, &h, &b);

	/* Show the breaking points */
    set_linebreaks_utf8((const utf8_t *)text, len, getenv("LANG"), brks);	

	line_start = 0;
	last_word = 0;
	for (i = 1; i <= len; i++) {
		switch (brks[i-1]) {
		case LINEBREAK_MUSTBREAK:
		case LINEBREAK_ALLOWBREAK:
			GrGetGCTextSize(thiz->gc, text + line_start, i - line_start, MWTF_UTF8, &w, &h, &b);
			if (w > width) {
				if (last_word > line_start) {
					item = &thiz->mLine.pitem[thiz->mLine.cur];
					thiz->mLine.cur++;
					if (w - last_w > width) {
						int partial_len, partial_w;
						partial_len = utf8_text_to_fit(thiz->gc, width - last_w,
									text + last_word, i - last_word, &partial_w);
						last_word += partial_len;
						last_w += partial_w;
					}

					item->start = text + line_start;
					item->len = last_word - line_start;
					dlist_append(thiz->tmplist, (void*)item);
					line_count++;

					line_start = last_word;
					w = w - last_w;
				}

				/* too long word */
				while (w > width) {
					item = &thiz->mLine.pitem[thiz->mLine.cur];
					thiz->mLine.cur++;
					item->start = text + line_start;
					item->len = utf8_text_to_fit(thiz->gc, width, text + line_start, i - line_start, &last_w);
					dlist_append(thiz->tmplist, (void*)item);
					line_count++;

					line_start += item->len;
					w = w - last_w;
				}
			}
			if (brks[i-1] == LINEBREAK_MUSTBREAK || w == width) {
				item = &thiz->mLine.pitem[thiz->mLine.cur];
				thiz->mLine.cur++;
				item->start = text + line_start;
				item->len = i - line_start;
				dlist_append(thiz->tmplist, (void*)item);
				line_count++;

				line_start = i;
				last_word = line_start;
				last_w = 0;
			} else {
				last_word = i;
				last_w = w;
			}
			break;
		}
	}

	free(brks);
	brks = NULL;

	if (line_start < len) {
		item = &thiz->mLine.pitem[thiz->mLine.cur];
		thiz->mLine.cur++;
		item->start = text+line_start;
		item->len = len - line_start;
		dlist_append(thiz->tmplist, (void*)item);
		line_count++;
	}

	return line_count;
}


/*Fixme bar width*/
void multiline_list_add_item(struct MultilineList *thiz, char *text)
{	
	if(thiz){
		int width;
		/*parse text for multiline*/
		width = multiline_list_cal_text_len(thiz, text, strlen(text));

		DBGMSG("width = %d, text= %s\n", width, text);
		
		if(width > thiz->show_width){/*multi line*/
			/**/
			parse_multiline(thiz, text, strlen(text));
			dlist_append(thiz->list, (void*)thiz->tmplist);
			thiz->total_line += dlist_length(thiz->tmplist);
			DBGMSG("multiline\n");
		}
		else{/*one line*/

			DList *tmplist = dlist_create(NULL, NULL);
			if(tmplist == NULL)
				return;
			struct MLineItem *item ;
			item = &thiz->mLine.pitem[thiz->mLine.cur];
			thiz->mLine.cur++;
			item->start = text;
			item->len = strlen(text);
			dlist_append(tmplist, (void*)item);
			thiz->total_line++;
			dlist_append(thiz->list, (void*)tmplist);
		}
		
		if(thiz->total_line > thiz->line_count){
			thiz->bscoll = 1;
			thiz->range = thiz->total_line;//- thiz->line_count;
			
		}else{
			thiz->bscoll = 0;
		}
		
		thiz->total_item ++;
	}
}


static int filterinvalidChar(char *pstr, int len)
{
	char *pEnd = pstr + len-1;
	while(pEnd >= pstr){
		if((0x0d == *pEnd || 0x0a == *pEnd))
			*pEnd = 0x20;
		pEnd --;
	}
	len = pEnd - pstr +1;
	return len;
}


/*Fixme bar width*/
int multiline_list_show(struct MultilineList *thiz)
{
	int show_len;
	char text[256];

	NxSetGCForeground(thiz->gc, thiz->bg_color);
	GrClearWindow(thiz->wid, GR_FALSE); 
	NxSetGCForeground(thiz->gc, thiz->fg_color);

	GrCopyArea(thiz->wid, thiz->gc, 
		0, 0, thiz->width, thiz->height, 0, 0, 0, 0);
	/*
	* show list
	*/
	show_len = thiz->total_line - thiz->global_start;
	if(show_len > thiz->line_count){
		show_len = thiz->line_count;
	}
	
	int total = dlist_length(thiz->list);

	DBGMSG("total = %d\n", total);

	int i = 0;
	int j = 0;
	int count = 0;
	for(i = 0; i < total; i++){
		DList *item;
		int len;
		dlist_get_by_index(thiz->list, i, (void**)&item);
		len = dlist_length(item);

		for(j = 0; j < len; j++){
			struct MLineItem *titem;
			dlist_get_by_index(item, j, (void**)&titem);
			memset(text, 256, 0);
			strncpy(text, titem->start, titem->len);
			text[titem->len] = 0;
			filterinvalidChar(text, strlen(text));

			if((count >= thiz->global_start) &&
				(count < show_len +thiz->global_start))
			{
				int height;
				height = thiz->font_size*(count - thiz->global_start);
				
				GrSetGCForeground(thiz->gc, thiz->fg_color);
				GrSetGCBackground(thiz->gc, thiz->bg_color);
				DBGMSG("text = %s, height = %d\n", text, height);
				if(j == 0){
					GrText(thiz->wid, 
						thiz->gc, 
						0, height,
						text, strlen(text), 
						MWTF_UTF8 | MWTF_TOP);
				}else{
					GrText(thiz->wid, 
						thiz->gc, 
						0, height,
						text, strlen(text), 
						MWTF_UTF8 | MWTF_TOP);
				}

			}
			count++;
		}
	}
	/* update to window */
	GrCopyArea(thiz->wid, thiz->gc, 
		0, 0, thiz->width, thiz->height, 0, 0, 0, 0);

	DBGMSG("show end 1\n");
	if(thiz->bscoll)
	{
		DBGMSG("thiz->bscoll\n");
		MultiSbarDraw(thiz);
	}

	DBGMSG("show end 2\n");
	return 0;
}

int multiline_list_key_up(struct MultilineList *thiz)
{
	int total = dlist_length(thiz->list);

	if(total <= 0){
		return -1;
	}
	thiz->next_lines = 0;
	
	if(thiz->global_select -1 >= 0){
		thiz->global_select--;
		
		thiz->global_start = 0;//line
		int i = 0;
		//cal start line
		for(i = 0; i < thiz->global_select; i++){
			DList *item;
			dlist_get_by_index(thiz->list, i, (void**)&item);
			thiz->global_start += dlist_length(item);
		}
	}else{

		if(thiz->nloop)
		{
			thiz->global_select = total - 1;
		
			thiz->global_start = 0;//line
			int i = 0;
			//cal start line
			for(i = 0; i < thiz->global_select; i++){
				DList *item;
				dlist_get_by_index(thiz->list, i, (void**)&item);
				thiz->global_start += dlist_length(item);
			}
		}
		else{
			if(thiz->total_line <= thiz->line_count){
				return -1;
			}

			if(thiz->global_start -1 >= 0){
					thiz->global_start--;
			}
		}
	}

	if((thiz->global_select+1) < total){
		thiz->end_flag = 0;
	}else{
		thiz->end_flag = 1;
	}
	
//	scroll_bar_set_value(thiz->bar, thiz->global_start);
//	multiline_list_show(thiz);
	GrClearWindow(thiz->wid, 1);
	
	return 0;
}

int multiline_list_key_down(struct MultilineList *thiz)
{
	int total = dlist_length(thiz->list);
	if(total <= 0){
		return -1;
	}

	thiz->next_lines = 0;
	
	if(thiz->global_select +1 < total){
		thiz->end_flag = 0;
		thiz->global_select++;
		thiz->global_start = 0;//line
		int i = 0;
		//cal start line
		for(i = 0; i < thiz->global_select; i++){
			DList *item;
			dlist_get_by_index(thiz->list, i, (void**)&item);
			thiz->global_start += dlist_length(item);
		}
	}
	else{
		thiz->end_flag = 1;
		if(thiz->nloop)
		{
			thiz->global_select = 0;
			thiz->global_start = 0;
		}
		else{
			if(thiz->total_line <= thiz->line_count){
				return -1;
			}
			
			if(thiz->global_start + thiz->line_count < thiz->total_line){
				thiz->global_start++;
			}
		}
	}
	//scroll_bar_set_value(thiz->bar, thiz->global_start);
//		multiline_list_show(thiz);
	GrClearWindow(thiz->wid, 1);
	return 0;
}

int mulitline_list_get_lineScreen(struct MultilineList *thiz)
{
	if(thiz)
	{
		return thiz->line_count;
	}
	return -1;
}


int multiline_list_isNextScreen(struct MultilineList *thiz)
{
	DList *list = NULL;

	if(thiz)
	{
	 	if(thiz->list)
	 	{
			dlist_get_by_index(thiz->list, thiz->global_select, (void**)&list);

			if(list)
			{
				thiz->next_lines = dlist_length(list);
				if(thiz->next_lines > thiz->line_count)
				{
					return thiz->next_lines;
				}
				else{
					return 0;
				}
			}
	 	}
	}
	return -1;
	
}

void multiline_list_keydown_nextScreen(struct MultilineList *thiz, int nextline)
{
	if(thiz)
	{
		thiz->global_start += nextline;
		DBGMSG("thiz->global_start = %d\n", thiz->global_start);
		GrClearWindow(thiz->wid, 1);
	}
}

char *multiline_list_get_current_NextScreen_text(struct MultilineList *thiz, int cur_line)
{
	if(thiz){
		if(thiz->list){
			DList *list;
			dlist_get_by_index(thiz->list, thiz->global_select, (void**)&list);			
			
			if(list){
				struct MLineItem *item;
				int i, len = 0;
				char *text = NULL;
				for(i = cur_line; (i< cur_line + thiz->line_count && i < thiz->next_lines); i++)
				{	
					dlist_get_by_index(list, i, (void**)&item);
					len += item->len;
				}
				
				if(thiz->scr_str){
					free(thiz->scr_str);
					thiz->scr_str = NULL;
				}
				text = (char *)malloc(len+2);
				if(text){
					memset(text, 0x00, len+2);
					dlist_get_by_index(list, cur_line, (void**)&item);
					memcpy(text, item->start, len);
				}
				thiz->scr_str = text;
				return text;
			}
			return NULL;
		}

		return NULL;
	}

	return NULL;

}


char *multiline_list_get_current_text(struct MultilineList *thiz)
{
	if(thiz){
		if(thiz->list){
			DList *list;
			
			dlist_get_by_index(thiz->list,
				 thiz->global_select, (void**)&list);			
			
			if(list){
				struct MLineItem *item;
				dlist_get_by_index(list, 0, 
					(void**)&item);
				return item->start;
			}
			return NULL;
		}

		return NULL;
	}

	return NULL;

}

int multiline_list_get_end(struct MultilineList *thiz)
{
	if(thiz){
		return thiz->end_flag;
	}
	return 0;
}

void multiline_list_destroy(struct MultilineList *thiz)
{
	multiline_list_handle_faile(thiz);
}

struct MultilineList *multiline_list_create(FORM* form, GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_COLOR fg_color, GR_COLOR bg_color)
{
	struct MultilineList *thiz = (struct MultilineList *)malloc(
		sizeof(struct MultilineList));
	if(!thiz){
		goto faile;
	}
	memset(thiz, 0, sizeof(struct MultilineList));

	thiz->form = form;
	
	thiz->x = x;
	thiz->y = y;
	thiz->width = width;
	thiz->height = height;
	
	//thiz->wid
	thiz->wid = parent;
	
	thiz->font_size = sys_font_size;

	thiz->gc = GrNewGC();
	thiz->font_id = GrCreateFont((GR_CHAR*)sys_font_name, sys_font_size, NULL);
	 
	GrSetGCUseBackground(thiz->gc ,GR_FALSE);
 	GrSetGCForeground(thiz->gc, fg_color);
 	GrSetGCBackground(thiz->gc, bg_color);
 	GrSetFontAttr(thiz->gc , GR_TFKERNING, 0);
 	GrSetGCFont(thiz->gc, thiz->font_id); 
	
	thiz->line_count = thiz->height/thiz->font_size;

	DBGMSG("thiz->line_count = %d\n", thiz->line_count);

//	thiz->show_width = MAX_SHOW_LEN;
	thiz->show_width = thiz->width - SCROLL_BAR_WIDTH - 2;

	thiz->list = dlist_create((DataDestroyFunc)multiline_list_free_item, NULL);
	if(thiz->list == NULL)
	{
		goto faile;
	}

	thiz->view_select = 0;
	thiz->global_select = 0;
	thiz->global_start = 0;
	thiz->total_item = 0;
	thiz->next_lines = 0;

	thiz->fg_color = fg_color;
	thiz->bg_color = bg_color;

//	thiz->bar = scroll_bar_create(thiz->wid,thiz->width-SCROLL_BAR_WIDTH, 0, 
//		SCROLL_BAR_WIDTH, thiz->height, 
//		bar_path_bg, bar_path_block);
//	scroll_bar_set_range(thiz->bar, 100);

	DBGMSG("create list ok\n");
	return thiz;
faile:
	multiline_list_handle_faile(thiz);
	return NULL;
}

void multiline_list_set_loop(struct MultilineList *thiz)
{
	thiz->nloop = 1;
}

void MultiSbarDraw(struct MultilineList *thiz)
{
#if 0
	int s, b;
	int y = 0, w = SCROLL_BAR_WIDTH, x = MAINWIN_WIDTH - SCROLL_BAR_WIDTH;

	DBGMSG("thiz->range = %d\n", thiz->range);
	if (thiz->global_start >= thiz->range)	
	{
		DBGMSG("out\n");
		return;	
	}
	NxSetGCForeground(thiz->gc, BLACK);
	GrFillRect(thiz->wid, thiz->gc, x, 0, w, MAINWIN_HEIGHT);	
	NxSetGCForeground (thiz->gc, GR_RGB(0xFF, 0x66, 0x00));	
	//GrFillRect(thiz->wid, thiz->gc, x, 0, w, w);	
	//GrFillRect(thiz->wid, thiz->gc, x, 0 + MAINWIN_HEIGHT - w, w, w);
	b = (MAINWIN_HEIGHT - w * 3) * thiz->line_count / thiz->range;
	b = (b < w) ? w : b;
	if(thiz->range > 1)
	{
		s = thiz->global_start * (MAINWIN_HEIGHT - w * 3 - b) / (thiz->range - 1);
	}
	else{
		s = 0;
	}
	y += s + w * 3 / 2;
	y = y;
	DBGMSG("y  = %d\n", y);
	NxSetGCForeground(thiz->gc, GR_RGB(0xCC, 0xCC, 0xCC));	
	GrFillRect(thiz->wid, thiz->gc, x, y, w, b);

#else

	struct scrollbar sbar = {
		.width = SBAR_WIDTH,
		.fgcolor = theme_cache.foreground,
		.bgcolor = theme_cache.background,
		.bordercolor = theme_cache.foreground,
	};
//	
//	NxSbarDraw(thiz->form, &sbar, thiz->global_select, thiz->line_count, 
//		thiz->total_line, MAINWIN_WIDTH - SCROLL_BAR_WIDTH, 0, MAINWIN_HEIGHT);
	DBGMSG("range:%d line_count:%d global_start:%d\n",thiz->range,thiz->line_count,thiz->global_start);
	int v;
	if((thiz->global_start + thiz->line_count) >= thiz->range){
		v = thiz->range - 1;
	}else{
		v = thiz->global_start;
	}
	NxSbarDraw(thiz->form, &sbar, v, /*1 */thiz->line_count, 
		thiz->range, MAINWIN_WIDTH - SCROLL_BAR_WIDTH, 0, MAINWIN_HEIGHT);
#endif
}

int multiline_list_get_curItem(struct MultilineList *thiz)
{
	if(thiz)
	{
		DBGMSG("thiz->global_select  = %d\n",thiz->global_select);
		return thiz->global_select;
	}
	return -1;
}

int initCurrentItem(struct MultilineList *thiz)
{
	thiz->startLine = 0;
	thiz->showLines = 0;
	thiz->screenLines = mulitline_list_get_lineScreen(thiz);
	thiz->itemLines = multiline_list_isNextScreen(thiz);
	return 1;
}

int mullistIsLastItem(struct MultilineList *thiz)
{
	return multiline_list_get_end(thiz);
}
	
int mullistNextItem(struct MultilineList *thiz)
{
	multiline_list_key_down(thiz);
	initCurrentItem(thiz);
	return 1;
}

int mullistPrevItem(struct MultilineList *thiz)
{
	multiline_list_key_up(thiz);
	initCurrentItem(thiz);
	return 1;
}

char *getItemContent(struct MultilineList *thiz)
{
	char *curtext;
	if(thiz->itemLines > thiz->startLine)
	{
		multiline_list_keydown_nextScreen(thiz, thiz->showLines);
		
		curtext = multiline_list_get_current_NextScreen_text(thiz, thiz->startLine);
		if(thiz->startLine + thiz->screenLines > thiz->itemLines)
		{
			thiz->showLines = thiz->itemLines - thiz->startLine;
			thiz->startLine = thiz->itemLines;
		}
		else
		{
			thiz->startLine += thiz->screenLines;
			thiz->showLines = thiz->screenLines;
		}
	}
	else{
		curtext = multiline_list_get_current_text(thiz);
	}
	return curtext;
}

char *getMoreContent(struct MultilineList *thiz)
{
	if(thiz->itemLines > 0 && 
		thiz->itemLines > thiz->startLine)
	{
		return getItemContent(thiz);
	}
	return NULL;
}


