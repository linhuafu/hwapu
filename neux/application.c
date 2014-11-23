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
 * Application widget support routines.
 *
 * REVISION:
 *
 * 3) Added in background preference support. ------------- 2007-08-07 MG
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-05-04 MG
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "wm.h"
#include "shared-mem.h"
#include <signal.h>
#include "neux.h"
#include "events.h"

// global application handle, seen by Neux internal widgets.
NX_WIDGET * mainApp;

// private APIs
static void signal_handler(int signum)
{
	WARNLOG("------- signal caught:[%d] --------\n", signum);
	NeuxAppStop();
	exit(0);
}

static void signal_init(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

    if (sigaction(SIGHUP, &sa, NULL)){}
    if (sigaction(SIGINT, &sa, NULL)){}
    if (sigaction(SIGQUIT, &sa, NULL)){}
    if (sigaction(SIGABRT, &sa, NULL)){}
    if (sigaction(SIGTERM, &sa, NULL)){}
}


//TBD: fix below APIs.
/**
 * Adds a fd to mainloop
 */
void
NeuxAppFDSourceRegister(int fd, void *data, void (*cb_read)(void *), void (*cb_write)(void *))
{
	NxAppSourceRegister(fd, data, cb_read, cb_write);
}

void
NeuxAppFDSourceUnregister(int fd)
{
	NxAppSourceUnregister(fd);
}

void
NeuxAppFDSourceActivate(int fd, int want_read, int want_write)
{
	NxAppSourceActivate(fd, want_read, want_write);
}


static void
onMsg(NX_WIDGET *sender, DATA_POINTER ptr)
{
	sender->base.event.msg.bytes -= MAX_MSG_APP_NAME_LEN;
	(*(neux_msg_handle_t)ptr)((const char*)(&sender->base.event.msg.msg + sender->base.event.msg.bytes), (neux_msg_t *)(&sender->base.event.msg.bytes));
}

static void
OnScreenSaver(NX_WIDGET *app, DATA_POINTER ptr)
{
	DBGLOG("screen saver \n");
	(*(SSaverCbfptr)ptr)(app->base.event.screensaver.activate);
}

void NeuxAppSetCallback(int cbId, void * fptr)
{
	NX_APPLICATION *me = &((NX_WIDGET *)mainApp)->spec.application;

	switch (cbId)
	{
//	case CB_IDLE:
//		NxRegisterCallBack(&me->cb_idle, OnKeyDown, fptr);
//		break;
	case CB_MSG:
		NxRegisterCallBack(&me->cb_msg, onMsg, fptr);
		break;
	case CB_SSAVER:
		if (me->iswm)
			NxRegisterCallBack(&me->cb_screensaver, OnScreenSaver, fptr);
		break;
	case CB_DESTROY:
		NxRegisterCallBack(&me->cb_destroy, OnDestroy, fptr);
		break;
	case CB_SW_ON:
		NxRegisterCallBack(&me->cb_sw_on, OnSWOn, fptr);
		break;
	case CB_SW_OFF:
		NxRegisterCallBack(&me->cb_sw_off, OnSWOff, fptr);
		break;
	}
}
//add by lhf
void* NeuxAppGetCallback(int cbId)
{
	NX_APPLICATION *me = &((NX_WIDGET *)mainApp)->spec.application;
	void *fp = NULL;
	switch (cbId)
	{
//	case CB_IDLE:
//		NxRegisterCallBack(&me->cb_idle, OnKeyDown, fptr);
//		break;
	case CB_MSG:
		fp = me->cb_msg.dptr;
		break;
	case CB_SSAVER:
		fp = me->cb_screensaver.dptr;
		break;
	case CB_DESTROY:
		fp = me->cb_destroy.dptr;
		break;
	case CB_SW_ON:
		fp = me->cb_sw_on.dptr;
		break;
	case CB_SW_OFF:
		fp = me->cb_sw_off.dptr;
		break;
	}
	return fp;
}
//lhf end
//----------------------------------------------------------------------
// public APIs
//----------------------------------------------------------------------

/**
 * Function creates application widget.
 *
 * @param name
 *        application default name if available.
 * @return
 *      0 if successful, otherwise -1.
 */
int NeuxAppCreate(const char * name)
{
	if (-1 == WmRegisterApp(name)) {
		WmAppSerialUnlock();
		exit(-1);
	}
	signal_init();
	mainApp = NxAppInitialize(0);
	WmNotifyTopAppChange(FALSE, TRUE);
	WmSetAppMsgWid(mainApp->spec.application.ewid);
//	WmAppSerialUnlock();
	return 0;
}

/**
 * Start application.
 *
 */
void NeuxAppStart(void)
{
    NxAppLoop();
}

/**
 * Stop application.
 *
 */
void NeuxAppStop(void)
{
	NxAppStop();
}

/**
 * Unregister application.
 *
 */
void NeuxAppUnregister(void)
{
	NxAppUnregister();
}

/**
 * if call NeuxAppUnregister Unregister application must call
 * this function.
 *
 */
void NeuxAppExit(void)
{
	NxAppExit();
}

/**
 * Minimize appliation, bring up next application in z-order.
 *
 */
void NeuxAppMinimize(void)
{
	WmHideApp();
	WmNotifyTopAppChange(TRUE, FALSE);
}

/**
 * Check to see if application on top.
 *
 * @param name
 *        application name.
 * @return
 *        1 if application is on top, otherwise zero.
 *
 */

int NeuxIsAppMinimized(const char * app)
{
	return !WmIsAppOnTop(app);
}

/**
 * Bring up desktop.
 *
 */
void NeuxGotoDesktop(void)
{
	WmGotoDesktop();
	WmNotifyTopAppChange(FALSE, FALSE);
}

int NeuxAppMsgWID(void)
{
	return mainApp->spec.application.ewid;
}

int
NeuxAppGetKeyLock(int read_hw_always)
{
	return NxAppGetKeyLock(read_hw_always);
}

void NeuxSetSsaverTimeout(int timeout)
{
	WmSetSsaverTimeout(timeout);
}

void NeuxSsaverPauseResume(int pause)
{
	WmSsaverPauseResume(pause ? spsPause : spsResume);
}


//----------------------------------------------------------------------
// Public APIs for WM / Desktop only.
//----------------------------------------------------------------------

/**
 * Function creates application widget for WM / desktop.
 *
 * @param name
 *        application default name if available.
 * @return
 *      0 if successful, otherwise -1.
 */
int NeuxWMAppCreate(const char * name)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;

	if (WmInitialize()) exit(-1);
	if (-1 == WmRegisterApp(name)) exit(-1);

	mainApp = NxAppInitialize(1);
	WmSetAppMsgWid(mainApp->spec.application.ewid);

	// get WM globals.
	wm = CoolShmGet(WM_SHM_ID, sizeof(wm_global_t), &first, &shmlock);
	if (!wm)
	{
		WARNLOG("Unable to fetch WM shared memory!\n");
		goto bail_no_shm;
	}
	if (first)
	{
		WARNLOG("WM shared memory in wrong state.\n");
		goto bail;
	}

	CoolShmWriteLock(shmlock);
	wm->desktop_pid = getpid();
	wm->desktop_msgWid = mainApp->spec.application.ewid;
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return 0;
}

/**
 * Start application.
 *
 */
void NeuxWMAppStart(void)
{
    NxAppLoop();
}

/**
 * Stop application.
 *
 */
void NeuxWMAppStop(void)
{
    DBGLOG("Terminating application\n");
    NxAppTerminate();
    DBGLOG("Unregistering application\n");
	WmUnregisterApp();
    DBGLOG("WM exiting\n");
	WmExit();
	exit(0);
}


// WM applications running support wrpper functions.

/**
 * Show a running application and bring it to top.
 *
 * @param name
 *        application name.
 *
 */
void NeuxWMAppReshow(const char * app)
{
	WmShowApp(app);
	WmNotifyTopAppChange(FALSE, FALSE);
}

/**
 * Run an application.
 *
 * @param name
 *        application name.
 *
 */
void NeuxWMAppRun(const char * app)
{
	WmRunApp(app);
}

/**
 * Check to see if application running.
 *
 * @param name
 *        application name.
 * @return
 *        1 if application is running, otherwise zero.
 *
 */
int NeuxWMAppIsRunning(const char * app)
{
	return WmIsAppRunning(app);
}

/**
 * Check to see if application itself.
 * use for plugins know which application call it.
 *
 * @param name
 *        application name.
 * @return
 *        1 if application is itself, otherwise zero.
 *
 */
int NeuxWMAppIsItself(const char * app)
{
    return WmIsAppItself(app);
}

/**
 * Stop all application except the desktop.
 */
int NeuxStopAllApp(BOOL block)
{
	return WmStopAllApp(block);
}

/**
 * Stop all application, except the one that called this and the desktop.
 * @param block,
 *	TRUE : block, FALSE : non-block
 * @return
 *	0 : stop all other apps successfully. otherwise, has some error
 */
int NeuxStopAllOtherApp(BOOL block)
{
	return WmStopAllOtherApp(block);
}

/**
 * Stop an application
 * @param app,
 *	application name
 */
void NeuxWMAppStopSpecific(const char * app)
{
	if (WmIsAppRunning(app))
		WmStopSpecificApp(app);
}

void NeuxWMAppSetScreenSaverTime(int t)
{
	NxSetAppScreenSaverTimeout(t);
}

void NeuxWMAppEnableScreenSaver(int enable)
{
	NxSetAppScreenSaverEnabled(enable);
}

const char* NeuxWmTopApp(void)
{
	return WmTopApp();
}

int NeuxWmGetTopAppMsgWid(void)
{
	return WmGetTopAppMsgWid();
}

void NeuxWaitEventTimeout(GR_BOOL (*done_cb)(GR_EVENT *ep), int timeout)
{
	GR_EVENT ev;

	do {
		GrGetNextEventTimeout(&ev, timeout);
		NxEventResolveRoutine(&ev);
	} while (!done_cb(&ev));
}
