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
 * Recorder configuration header.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-04-30 JW
 *
 */
#ifndef _DESKTOP_IPC_H_
#define _DESKTOP_IPC_H_

#include "neux.h"

#define PLEXTALK_IPC_NAME_DESKTOP			"desktop"
#define PLEXTALK_IPC_NAME_BOOK				"book"
#define PLEXTALK_IPC_NAME_MUSIC				"music"
#define PLEXTALK_IPC_NAME_RADIO				"radio"
#define PLEXTALK_IPC_NAME_RECORD			"recorder"
#define PLEXTALK_IPC_NAME_MENU				"menu"
#define PLEXTALK_IPC_NAME_HELP				"help"
#define PLEXTALK_IPC_NAME_ALARM				"alarm"
#define PLEXTALK_IPC_NAME_CALCULATOR		"calculator"
#define PLEXTALK_IPC_NAME_BACKUP			"backup"
#define PLEXTALK_IPC_NAME_FILE_MANAGEMENT	"file-management"
#define PLEXTALK_IPC_NAME_PC_CONNECTOR		"pc-connector"
#define PLEXTALK_IPC_NAME_FORMAT			"format"
#define PLEXTALK_IPC_NAME_JUMP_PAGE			"jump-page"

#define PLEXTALK_TITLEBAR_MSG_ID		NEUX_USR_MSG(0x231)
#define PLEXTALK_STATUSBAR_MSG_ID		NEUX_USR_MSG(0x232)
/* notify desktop to reschdule timers immediately when timer
 * setting has been changed */
#define PLEXTALK_TIMER_MSG_ID 			NEUX_USR_MSG(0x233)
/* notify desktop applicarion enter or exit file list mode */
#define PLEXTALK_FILELIST_MSG_ID		NEUX_USR_MSG(0x234)
/* notify desktop current book is audio or text */
#define PLEXTALK_BOOK_CONTENT_MSG_ID	NEUX_USR_MSG(0x235)
/* notify desktop menu enter or exit guide volume setting */
#define PLEXTALK_MENU_GUIDE_VOL_MSG_ID	NEUX_USR_MSG(0x236)
/* notify desktop recorder enter or exit recording */
#define PLEXTALK_RECORD_ACTIVE_MSG_ID	NEUX_USR_MSG(0x237)
/* notify desktop radio enter or exit recording */
#define PLEXTALK_RADIO_RECORD_ACTIVE_MSG_ID	NEUX_USR_MSG(0x238)
/* notify desktop applicarion enter or exit information mode */
#define PLEXTALK_INFO_MSG_ID			NEUX_USR_MSG(0x239)
/* notify desktop recorder pause or continue recording ,used to switch the headphone and speaker*/
#define PLEXTALK_RECORD_PAUSE_MSG_ID	NEUX_USR_MSG(0x240)//appk

/* notify desktop to reschdule timers immediately when timer
 * setting has been changed */
#define PLEXTALK_OFFTIMER_MSG_ID			NEUX_USR_MSG(0x241)//appk


#define PLEXTALK_APP_MSG_ID				NEUX_USR_MSG(0x250)


#endif //_DESKTOP_IPC_H_
