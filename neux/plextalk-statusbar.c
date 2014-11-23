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
 * Form StatusBar routines.
 *
 * REVISION:
 *
 * 2) Make statusbar a shared object accross the platform.- 2007-04-30 MG
 * 1) Initial creation. ----------------------------------- 2007-04-24 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "plextalk-statusbar.h"
#include "neux.h"
#include "desktop-ipc.h"

void
NhelperStatusBarSetIcon(SBAR_REQUEST req, int icon)
{
	neux_msg_t msg;
	sbar_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(sbar_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_STATUSBAR_MSG_ID;
	rqst->type = req;
	rqst->set_icon.icon = icon;

	NeuxMsgSend(PLEXTALK_IPC_NAME_DESKTOP, &msg);
}

void
NhelperStatusBarReflushTime(void)
{
	neux_msg_t msg;
	sbar_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(sbar_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_STATUSBAR_MSG_ID;
	rqst->type = SRQST_SET_TIME;

	NeuxMsgSend(PLEXTALK_IPC_NAME_DESKTOP, &msg);
}
