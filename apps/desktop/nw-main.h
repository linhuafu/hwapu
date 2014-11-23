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
 * Form Main header.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NWMAIN_H_
#define _NWMAIN_H_

#include "widgets.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-config.h"

enum {
	DESKTOP_STATE_NULL,
	DESKTOP_STATE_ALARM,
	DESKTOP_STATE_PC_CONNECTOR,
	DESKTOP_STATE_FREE_RUN,
	DESKTOP_STATE_OFF,
};
extern int desktop_state;

enum
{
	M_DESKTOP,
	M_BOOK,
	M_MUSIC,
	M_RADIO,
	M_RECORD,
	M_MENU,
	M_HELP,
	M_FILE_MANAGEMENT,
	M_CALCULATOR,
	M_BACKUP,
	M_ALARM,
	M_PC_CONNECTOR,
	M_FORMAT,
};
extern const char *FUNC_MAIN_STR[];

extern int active_app;
extern struct voice_prompt_cfg vprompt_cfg;

extern int radio_record_active;

extern int last_msgid;
extern int desktop_theme_index;	//ztz

extern TIMER *lowpowerTimer;

#define SET_SDATE_DELAY			200000		//set state delay for 0.2s

void
OnLowPowerTimer(WID__ wid);

void
DesktopPowerOff(void);
void
AppLauncher(int id, int exclude, int voice_prompt, const char* args);
void
TopAppChange(int is_new, const char* app);
void
CreateSuspendTimer(WIDGET * owner);

void
CreateFormMain(void);
FORM * 
GetFormMain(void);
void
RefreshFormMain(int confirm);		//ztz 

//ztz --start--
void 
DoVpromptUsbSound (int on);
//ztz --end--

#endif //_NWMAIN_H_
