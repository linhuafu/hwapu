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
 * Neux msg support routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-05-05 MG
 *
 */
#include <stdio.h>
#include <stdlib.h>

//#define OSD_DBG_MSG
#include "neux.h"
#include "wm.h"

/**
 * Send message to target application.
 *
 * @param dstApp
 *        target application name.
 * @param msg
 *        message.
 * @return
 *        0 if message is successfully sent, -1 otherwise.
 *
 */
int NeuxMsgSend(const char * dstApp, neux_msg_t * msg)
{
	int wid;
	int ret = -1;

	wid = WmGetAppMsgWid(dstApp);
	if (wid > 0)
	{
		char *name = (void*)&msg->msg + msg->bytes;
		WmGetAppName(name, MAX_MSG_APP_NAME_LEN);
		GrSendMsg(wid, (void*)&msg->msg, msg->bytes + strlen(name) + 1);
		ret = 0;
	}
	return ret;
}

int NeuxMsgBroadcast(const char * exclude, neux_msg_t * msg)
{
	int wid[WM_MAX_APP_NUM];
	char *name;
	int ret, i;

	ret = WmGetOtherAppMsgWids(wid, exclude);
	if (ret == -1)
		return -1;

	name = (void*)&msg->msg + msg->bytes;
	WmGetAppName(name, MAX_MSG_APP_NAME_LEN);

	for (i = 0; i < ret; i++)
		GrSendMsg(wid[i], (void*)&msg->msg, msg->bytes + strlen(name) + 1);

	return 0;
}
