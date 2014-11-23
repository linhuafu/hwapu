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
 * Widget Application header.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXAPPLICATION_H_
#define _NXAPPLICATION_H_

#include "nx-base.h"

typedef struct
{
	GR_WINDOW_ID	ewid;	//input window id for grab event
	GR_SCREEN_INFO  screen;

	GR_BOOL         iswm;   //Is window manager
	GR_BOOL         isssaveractive;    //0:hide 1:showing
	int      		iScrSaverTimeout;
	CallBackStruct cb_screensaver;

	CallBackStruct cb_idle;
	CallBackStruct cb_msg;
	CallBackStruct cb_destroy;
	CallBackStruct cb_sw_on;
	CallBackStruct cb_sw_off;

} NX_APPLICATION;

int
NxAppGetKeyLock(int read_hw_always);

/*event handler */
void
NxAppScreenSaverHandler (GR_EVENT *, NX_WIDGET *);
void
NxAppIdleHandler (GR_EVENT *, NX_WIDGET *);
void
NxAppMsgHandler (GR_EVENT *, NX_WIDGET *);
void
NxAppSWOnHandler (GR_EVENT *, NX_WIDGET *);
void
NxAppSWOffHandler (GR_EVENT *, NX_WIDGET *);

/*public  functions*/
NX_WIDGET *
NxAppInitialize (int wm);
void
NxAppLoop (void);
void
NxAppTerminate(void);
void
NxAppStop(void);
void
NxAppUnregister(void);
void
NxAppExit(void);
void
NxAppDestroy(NX_WIDGET *widget);
void
NxSetAppScreenSaverTimeout(int);
void
NxGetAppScreenSaverTimeout(int *);
void
NxSetAppScreenSaverEnabled(GR_BOOL);
GR_BOOL
NxGetAppScreenSaverEnabled(void);
void
NxAppSourceRegister(int fd, void *data, void (*cb_read)(void *), void (*cb_write)(void *));
void
NxAppSourceUnregister(int fd);
void
NxAppSourceActivate(int fd, int want_read, int want_write);
void
nxAppSourceEventHandler(int fd, int can_read, int can_write);

int
NxAppGetNeuxScreenHeight(void);
int
NxAppGetNeuxScreenWidth(void);
GR_FONT_ID
NxGetFontID(const char*, GR_SIZE);
void
NxPutFontID(GR_FONT_ID);
int
NxGetFontHeight(GR_FONT_ID fid);

#endif //_NXAPPLICATION_H_
