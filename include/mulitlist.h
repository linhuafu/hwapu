
#ifndef MULITLIST_H
#define MULITLIST_H

#include <stdio.h>
#include <stdint.h>
#include <linebreak.h>
#include "nano-X.h"
#include "dlist.h"
#include "widgets.h"

//DECLS_BEGIN
#define MAX_SHOW_LEN 158
#define MAX_SHOW_NUM 24

#define MAX_ITEM_LEN     256

struct MLineItem{
	char *start;
	int len;
};

struct MLine{
	int cur;
	struct MLineItem pitem[MAX_ITEM_LEN];
};

struct MultilineList
{
	FORM* form;
	GR_WINDOW_ID	wid;
	GR_GC_ID        gc;
	
	GR_COORD   x;
	GR_COORD   y;
	GR_SIZE    width;
	GR_SIZE    height;
	DList      *list;
	DList      *tmplist;
	int        bscoll;
	int        font_id;
	int        show_width;
	S32	       view_select;
	S32	       global_start;
	S32	       global_select;
	int	       end_flag;
	int	       total_line;
	int	       total_item;
	int	       line_count;
	int	       font_size;
	int        show_count;
	int        range;
	int		   nloop;
	int        next_lines;
	GR_COLOR   fg_color;
	GR_COLOR   bg_color;

	struct MLine mLine;
	//for item show
	int        startLine;
	int        showLines;
	int        screenLines;
	int        itemLines;
	char		*scr_str;
};


char *multiline_list_get_current_text(struct MultilineList *thiz);
void multiline_list_add_item(struct MultilineList *thiz, char *text);
int multiline_list_key_down(struct MultilineList *thiz);
int multiline_list_get_end(struct MultilineList *thiz);

struct MultilineList *multiline_list_create(FORM* form, GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
	GR_SIZE width, GR_SIZE height, GR_COLOR fg_color, GR_COLOR bg_color);

int multiline_list_show(struct MultilineList *thiz);

int multiline_list_key_up(struct MultilineList *thiz);

int multiline_list_key_down(struct MultilineList *thiz);

int mulitline_list_get_lineScreen(struct MultilineList *thiz);

char *multiline_list_get_current_text(struct MultilineList *thiz);

int multiline_list_get_end(struct MultilineList *thiz);

void multiline_list_set_loop(struct MultilineList *thiz);

void multiline_list_destroy(struct MultilineList *thiz);

int multiline_list_isNextScreen(struct MultilineList *thiz);

void multiline_list_keydown_nextScreen(struct MultilineList *thiz, int nextline);

//这个buffer需要在外面释放
char *multiline_list_get_current_NextScreen_text(struct MultilineList *thiz, int cur_line);

int mulitline_list_get_lineScreen(struct MultilineList *thiz);

int multiline_list_get_curItem(struct MultilineList *thiz);


int initCurrentItem(struct MultilineList *thiz);

int mullistIsLastItem(struct MultilineList *thiz);

int mullistNextItem(struct MultilineList *thiz);

int mullistPrevItem(struct MultilineList *thiz);

char *getItemContent(struct MultilineList *thiz);

char *getMoreContent(struct MultilineList *thiz);
//DECLS_END

#endif/*DLIST*/

