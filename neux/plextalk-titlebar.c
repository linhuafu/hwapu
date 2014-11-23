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
 * Form TitleBar routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-04-24 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "plextalk-titlebar.h"
#include "neux.h"
#include "desktop-ipc.h"

void
NhelperTitleBarSetContent(const char* title)
{
	neux_msg_t msg;
	int len;

	msg.msg.msg.msgId = PLEXTALK_TITLEBAR_MSG_ID;
	if (title == NULL || *title == '\0') {
		msg.msg.msg.msgTxt[0] = '\0';
		len = 1;
	} else {
		strlcpy(msg.msg.msg.msgTxt, title, sizeof(msg.msg.msg.msgTxt));
		len = strlen(msg.msg.msg.msgTxt) + 1;
	}
	msg.bytes = sizeof(msg.msg.msg.msgId) + len;

	NeuxMsgSend(PLEXTALK_IPC_NAME_DESKTOP, &msg);
}
