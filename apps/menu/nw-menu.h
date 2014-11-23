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

#ifndef _NWMENU_H_
#define _NWMENU_H_

#include "widgets.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-config.h"
#include "application-ipc.h"

extern char* curr_app;
extern struct plextalk_setting setting;
//lhf
extern int menu_exit;
extern int wait_focus_out;

void MenuExit(int sync_setting_file);
//lhf

void ShowFormMenu(void);

//lhf
int bookmark_result(app_bookmark_result* rqst);

int delete_result(app_delete_result* rqst);
//lhf
#endif //_NWMAIN_H_
