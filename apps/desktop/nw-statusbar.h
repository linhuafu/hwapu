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
 * Form StatusBar header.
 *
 * REVISION:
 *
 * 2) new struct. ----------------------------------- 2006-06-01 EY
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NW_STATUSBAR_H_
#define _NW_STATUSBAR_H_

#include "widgets.h"
#include "rtc-helper.h"

#define STATUSBAR_CATEGORY_INDEX	0
#define STATUSBAR_CONDITION_INDEX 	1
#define STATUSBAR_MEDIA_INDEX		2
#define STATUSBAR_VOLUME_INDEX		3
#define STATUSBAR_BATTERY_INDEX		4
#define STATUSBAR_TIMER_INDEX 		5
#define STATUSBAR_KEYLOCK_INDEX		6
#define STATUSBAR_AMPM_INDEX		7	//lhf
#define STATUSBAR_CLOCK_INDEX		8


enum {
	SBAR_VOLUME_ICON_NO,	// unvisible
	SBAR_VOLUME_ICON_LEVEL0,
	SBAR_VOLUME_ICON_LEVEL1,
	SBAR_VOLUME_ICON_LEVEL2,
	SBAR_VOLUME_ICON_LEVEL3,
	SBAR_VOLUME_ICON_LEVEL4,
	SBAR_VOLUME_ICON_LEVEL5,
	SBAR_VOLUME_ICON_LEVEL6,
	SBAR_VOLUME_ICON_LEVEL7,
};

enum {
	SBAR_BATTERY_ICON_NO,	// unvisible
	SBAR_BATTERY_ICON_LEVEL0,
	SBAR_BATTERY_ICON_LEVEL1,
	SBAR_BATTERY_ICON_LEVEL2,
	SBAR_BATTERY_ICON_LEVEL3,
	SBAR_BATTERY_ICON_LEVEL4,
	SBAR_BATTERY_ICON_CHARGING,
	SBAR_BATTERY_ICON_ACIN,
};

enum {
	SBAR_TIMER_ICON_NO,	// unvisible
	SBAR_TIMER_ICON_TIMER1,
	SBAR_TIMER_ICON_TIMER2,
	SBAR_TIMER_ICON_TIMER12,
};

enum {
	SBAR_KEYLOCK_ICON_NO,	// unvisible
	SBAR_KEYLOCK_ICON_LOCK,
};

enum {//lhf start
	SBAR_AMPM_ICON_NO,	// unvisible
	SBAR_AMPM_ICON_AM,
	SBAR_AMPM_ICON_PM,
};//lhf end

void
CreateStatusBar (FORM *parent);
void
StatusBarSetIcon(int index, int icon_id);
int
StatusBarRereshTime(sys_time_t *stm);
void
RefreshStatusBar(int confirm);	//ztz

#endif //_NWSTATUSBAR_H_
