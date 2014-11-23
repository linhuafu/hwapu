#ifndef NEUX__H
#define NEUX__H
/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
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
 * neux master header.
 *
 * REVISION:
 *
 *
 * 2) Added in background preference support. ------------- 2007-08-07  MG
 * 1) Initial creation. ----------------------------------- 2007-02-23  MG
 */

#include "application.h"
#include "widgets.h"

#define LINUX_START_YEAR		1900
#define	START_YEAR				2013
#define DIFFERENCE_YEAR			(START_YEAR - LINUX_START_YEAR)

#define MAX_MSG_APP_NAME_LEN	64
#define MAX_MSG_LEN				(NANOX_MAX_MSG_LEN - MAX_MSG_APP_NAME_LEN)

// message allocation policy
#define NEUX_SYS_MSG(id)	(0x00000000|(id))
#define NEUX_USR_MSG(id)	(0x00008000|(id))
#define NEUX_APP_MSG(id)	(0x00004000|(id))		// system send to application

#define APP_MSG_RESTORE_ARGS	NEUX_APP_MSG(1)		// send application parameter msg to application
#define APP_MSG_TOPAPP_CHANGE	NEUX_APP_MSG(2)

typedef struct
{
	int  msgId;								/** message ID. */
	char msgTxt[MAX_MSG_LEN - sizeof(int)];	/** message text. */
} neux_id_msg_t;

typedef union
{
	neux_id_msg_t msg;						/** ID partitioned message. */
	char          buf[MAX_MSG_LEN];			/** generic message buffer. */
} neux_msg_ut;

/** Neux generic message structure. */
typedef struct
{
	int         bytes;						/** msg bytes. */
	neux_msg_ut msg;						/** message body. */
} neux_msg_t;

typedef void (*neux_msg_handle_t)(const char * srcApp, neux_msg_t * msg);

int NeuxMsgSend(const char * dstApp, neux_msg_t * msg);

int NeuxMsgBroadcast(const char * exclude, neux_msg_t * msg);

#endif /* NEUX__H */

