#ifndef WM__H
#define WM__H
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
 * Window manager support module header.
 *
 * REVISION:
 *
 * 2) WM centrally manages the system level background ---- 2007-08-03  MG
 * 1) Initial creation. ----------------------------------- 2007-02-06  MG
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "nano-X.h"
#include "nc-type.h"

#ifdef __GNUC__
#    define GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > x || __GNUC__ == x && __GNUC_MINOR__ >= y)
#else
#    define GCC_VERSION_AT_LEAST(x,y) 0
#endif

#ifndef always_inline
#if GCC_VERSION_AT_LEAST(3,1)
#    define always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define always_inline __forceinline
#else
#    define always_inline inline
#endif
#endif

#define WM_SHM_ID        "neux-wm"   /** neux window manager shared mem ID. */
#define WM_MAX_APP_NUM   (8)         /** max number of apps in parallel. */
#define WM_APP_NAME_LEN  (64)        /** only first WM_APPNAME_LEN-1 chars are significant. */
#define WM_APP_MAX_FORMS (32)        /** maximum number of forms an application can keep open simultaneously. */


// system level message ID definitions.
#define SYS_MSG_RUNAPP					0
#define SYS_MSG_STOPAPP					1
#define SYS_MSG_SET_SSAVER_TIMEOUT     	2
#define SYS_MSG_SSAVER_PAUSE_RESUME		3

#define WM_APP_NAME	"desktop"

/** WM application registration structure. */
typedef struct
{
	char appname[WM_APP_NAME_LEN];   /** application name. */
	int  pid;                        /** application process ID. */
	int  formid[WM_APP_MAX_FORMS];   /** application loose form focus stack, formid[0]
                                         corresponds to top frame ID. */
	int  formnum;                    /** num of forms in this application. */
	int  msgWid;                     /** message window ID if available. */
} wm_regist_t;


/** WM application z-order management structure. */
typedef struct
{
	short prev;
	short next;
	short valid;
	wm_regist_t reg;
} wm_zorder_t;

/** WM global data structure. */
typedef struct
{
	int           desktop_pid;         /** desktop pid */
	int           desktop_msgWid;      /** desktop msg wid. */

	wm_zorder_t   appz[WM_MAX_APP_NUM];/** application z-order */
	short         appz_top;
	int           app_serial_lock;     /** mutex to guarantee application is run in serial. */

} wm_global_t;


int             WmInitialize(void);
void            WmExit(void);
int             WmRegisterApp(const char * name);
void            WmUnregisterApp(void);
int             WmAddForm(int wid);
void            WmRemoveForm(int wid, int restore);
void            WmChangeFormZorder(int wid);
int             WmFocusForm(void);
int             WmTotalForm(void);
int             WmIsAppRunning(const char * name);
int             WmIsAppItself(const char * name);
void            WmStartApp(void);
void            WmRunApp(const char * name);
void            WmRunAppImmediately(const char * name);
void            WmShowApp(const char * name);
const char     *WmTopApp(void);
int             WmIsAppOnTop(const char * name);
void            WmHideApp(void);
void            WmOnChildUpdate(GR_EVENT_UPDATE * event);
int             WmNewClientWindow(GR_WINDOW_ID wid);
void            WmGotoDesktop(void);
char *          WmGetAppName(char * name, int max_name_len);
int             WmGetTopAppMsgWid(void);
int             WmGetAppMsgWid(const char * name);
int             WmGetOtherAppMsgWids(int * msgWid, const char * exclude);
void            WmSetAppMsgWid(int msgWid);
int             WmStopAllApp(BOOL block);
int             WmStopAllOtherApp(BOOL block);
void            WmStopSpecificApp(const char *app);
void            WmAppSerialUnlock(void);
int             WmAppSerialIsLocked(void);
int             WmTopAppIsSelf(void);

typedef enum
{
	spsPause,
	spsResume,
} SSAVER_PR_STATUS;

void WmSetSsaverTimeout(int timeout);
void WmSsaverPauseResume(SSAVER_PR_STATUS pr);

void WmNotifyTopAppChange(BOOL always, BOOL is_new);

#endif /* WM__H */

