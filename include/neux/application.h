#ifndef APPLICATION__H
#define APPLICATION__H
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
 * Application module header.
 *
 * REVISION:
 *
 *
 * 2) Added in background preference support. ------------- 2007-08-07 MG
 * 1) Initial creation. ----------------------------------- 2006-05-12 MG
 *
 */

#include <unistd.h>
#include <signal.h>
#include <nano-X.h>

#include "nc-type.h"

#define APPLICATION void

APPLICATION *
AppGetHandle(void);

void
NeuxAppFDSourceRegister(int fd, void *data, void (*cb_read)(void *), void (*cb_write)(void *));
void
NeuxAppFDSourceUnregister(int fd);
void
NeuxAppFDSourceActivate(int fd, int want_read, int want_write);

//----------------------------------------------------------------------
// public APIs.

int NeuxAppCreate(const char * name);
void NeuxAppStart(void);
void NeuxAppStop(void);
void NeuxAppUnregister(void);
void NeuxAppExit(void);
int NeuxStopAllApp(BOOL block);
int NeuxStopAllOtherApp(BOOL block);
void NeuxAppMinimize(void);
int  NeuxIsAppMinimized(const char * app);
void NeuxGotoDesktop(void);
void NeuxAppSetCallback(int cbId, void * fptr);
void* NeuxAppGetCallback(int cbId);	//add by lhf

int NeuxAppMsgWID(void);

int NeuxAppGetKeyLock(int read_hw_always);

int NxAppGetNeuxScreenHeight(void);
int NxAppGetNeuxScreenWidth(void);

void NeuxSetSsaverTimeout(int timeout);
void NeuxSsaverPauseResume(int pause);


//----------------------------------------------------------------------
// below APIs are only required for WM / Desktop application.
int NeuxWMAppCreate(const char * name);
void NeuxWMAppStart(void);
void NeuxWMAppStop(void);
void NeuxWMAppStopSpecific(const char * app);

void NeuxWMAppReshow(const char * app);
void NeuxWMAppRun(const char * app);
int NeuxWMAppIsRunning(const char * app);
int NeuxWMAppIsItself(const char * app);
void NeuxWMAppSetScreenSaverTime(int t);
void NeuxWMAppEnableScreenSaver(int enable);

#define NeuxWMAppSetCallback NeuxAppSetCallback

const char* NeuxWmTopApp(void);
int NeuxWmGetTopAppMsgWid(void);

void NeuxWaitEventTimeout(GR_BOOL (*done_cb)(GR_EVENT *ep), int timeout);
#define NeuxWaitEvent(done_cb) NeuxWaitEventTimeout(done_cb, 0)

#endif //APPLICATION__H

