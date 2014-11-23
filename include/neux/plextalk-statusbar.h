/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
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
 * Form StatusBar header.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-04-24 EY
 *
 */

#ifndef _STATUSBAR_H_
#define _STATUSBAR_H_

#include "widgets.h"
#include "neux.h"

enum {
	SBAR_CATEGORY_ICON_NO,	// unvisible
	SBAR_CATEGORY_ICON_BOOK,
	SBAR_CATEGORY_ICON_BOOK_SELECT,
	SBAR_CATEGORY_ICON_BOOK_DAISY,
	SBAR_CATEGORY_ICON_BOOK_TEXT,
	SBAR_CATEGORY_ICON_MUSIC,
	SBAR_CATEGORY_ICON_RADIO,
	SBAR_CATEGORY_ICON_MENU,
	SBAR_CATEGORY_ICON_INFO,
	SBAR_CATEGORY_ICON_CALC,
	SBAR_CATEGORY_ICON_TIMER,
	SBAR_CATEGORY_ICON_RECODER,
	SBAR_CATEGORY_ICON_BACKUP,
};

enum {
	SBAR_CONDITION_ICON_NO,	// unvisible
	SBAR_CONDITION_ICON_PLAY,
	SBAR_CONDITION_ICON_PAUSE,
	SBAR_CONDITION_ICON_STOP,
	SBAR_CONDITION_ICON_SELECT,
	SBAR_CONDITION_ICON_SEARCH,
	SBAR_CONDITION_ICON_RECORD,
	SBAR_CONDITION_ICON_RECORD_BLINK,
};

enum {
	SBAR_MEDIA_ICON_NO,	// unvisible
	SBAR_MEDIA_ICON_INTERNAL_MEM,
	SBAR_MEDIA_ICON_SD_CARD,
	SBAR_MEDIA_ICON_USB_MEM,
};

typedef enum {
	SRQST_SET_CATEGORY_ICON,
	SRQST_SET_CONDITION_ICON,
	SRQST_SET_MEDIA_ICON,
	SRQST_SET_TIME,
} SBAR_REQUEST;

typedef struct
{
	SBAR_REQUEST	type;
	int				icon;
} sbar_set_icon_rqst_t;

typedef struct
{
	SBAR_REQUEST        type;
} sbar_set_time_rqst_t;

typedef union
{
	SBAR_REQUEST        	type;
	sbar_set_icon_rqst_t	set_icon;
	sbar_set_time_rqst_t	set_time;
} sbar_rqst_t;

void
NhelperStatusBarSetIcon(SBAR_REQUEST req, int icon);

void
NhelperStatusBarReflushTime(void);

#endif //_STATUSBAR_H_
