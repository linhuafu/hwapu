/*
 *  Copyright(C) 2006-2007 Neuros Technology International LLC.
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
 * Window manager main routines.
 *
 * REVISION:
 *
 * 3) WM centrally manages the system level background ------- 2007-08-03 MG
 * 2) Algorithm to run application in serial.----------------- 2007-05-30 MG
 * 1) Initial creation.--- ----------------------------------- 2007-04-02 MG
 *
 */

//#define OSD_DBG_MSG
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include "nc-err.h"
#include "nc-type.h"
#include "file-helper.h"
#include "sem-helper.h"
#include "wm.h"
#include "shared-mem.h"
#include "neux.h"

static const unsigned MAX_CMD_ARGS=16;	/**< Largest number of arguments */

#define BLOCK_TIME 800 // 800ms
#define MAX_RETRY_TIME 3

wm_global_t * wm; /** global window manager data object pointer. */

static void stop_app(int msgid, const char * app)
{
	neux_msg_t msg;

	DBGLOG("Sending STOPAPP msg to %s\n", app);

	msg.msg.msg.msgId = SYS_MSG_STOPAPP;
	msg.bytes = strlen(app) + 1;
	memcpy((void*)&msg.msg.msg.msgTxt, app, msg.bytes);
	msg.bytes += sizeof(msg.msg.msg.msgId);

	GrSendMsg(msgid, (void*)&msg.msg, msg.bytes);
}

static int is_pid_valid(int pid)
{
	//FIXME: can not detect ZOMBIE process this way!!!
	if (kill(pid, 0))
	{
#if 0
		DBGLOG("checking pid: %d err: %d\n", pid, errno);
		switch (errno)
		{
		case ESRCH:
			DBGLOG("pid does not exist!\n");
			break;
		case EINVAL:
			DBGLOG("invalid signal was specified.\n");
			break;
		case EPERM:
			DBGLOG("process  does not have permission to send the signal.\n");
			break;
		}
#endif
		WARNLOG("process has been removed.\n");
		return 0;
	}

	return 1;
}

// get filename off the full command path.
static char * appname_from_command(char * name, const char * cmd)
{
	const char *start;
	const char *end = NULL;

	if (*cmd == '\'' || *cmd == '\"')
		end = strchr(cmd + 1, *cmd);
	if (end == NULL)
		end = strchrnul(cmd, ' ');

	start = memrchr(cmd, '/', end - cmd);
	if (start == NULL)
		start = cmd;
	else
		start += 1;

	strlcpy(name, start, min(end - start + 1, WM_APP_NAME_LEN));
	DBGLOG("registered appname = %s\n", name);
	return name;
}

#define for_each_app(cur) \
	for (cur = wm->appz_top; wm->appz[cur].valid; cur = wm->appz[cur].next)

#define for_each_app_safe(cur, next) \
	for (cur = wm->appz_top, next = wm->appz[cur].next; wm->appz[cur].valid; cur = next, next = wm->appz[cur].next)

static inline void remove_app(wm_global_t * wm, int cur)
{
	int prev, next;

	prev = wm->appz[cur].prev;
	next = wm->appz[cur].next;
	wm->appz[prev].next = next;
	wm->appz[next].prev = prev;
}

static inline void insert_app(wm_global_t * wm, int cur, int new)
{
	int prev;

	prev = wm->appz[cur].prev;

	wm->appz[new].prev = prev;
	wm->appz[new].next = cur;
	wm->appz[cur].prev = new;
	wm->appz[prev].next = new;
}

// wm registration saint checkup.
// checkup is needed because application may crash and leave the registration
// table in a dirty state.
static void reg_sanity_check(void)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	shm_lock_t * shmlock;

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
	if (-1 == wm->appz_top) goto no_app;

	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (!is_pid_valid(regs->pid))
		{
			wm->appz[cur].valid = 0;

			// removing application from zorder table.
			if (cur == wm->appz_top)
			{
				int top = wm->appz_top;
				wm->appz_top = wm->appz[top].next;

				top = wm->appz_top;
				if (wm->appz[top].valid == 0)
				{
					WARNLOG("No valid neux application found!\n");
					wm->appz_top = -1;
				}
			}
			else
			{
				// remove app from z-order entry table.
				DBGLOG("Remove application from zorder table.\n");

				// detach form link
				remove_app(wm, cur);
				// insert cur before top.
				insert_app(wm, wm->appz_top, cur);
			}
		}
	}
 no_app:
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

/**
 * Window manager main entry.
 *
 *        0 if window manager successfully initialized, non-zero otherwise.
 */
int WmInitialize(void)
{
	int first;
	shm_lock_t * shmlock;

	// initialize WM globals.
	wm = CoolShmGet(WM_SHM_ID, sizeof(wm_global_t), &first, &shmlock);
	if (!wm || !first)
	{
		WARNLOG("Unable to initialize memory, failed to initialize WM.\n");
		return -1;
	}
	else
	{
		CoolShmWriteLock(shmlock);
		memset(wm, 0, sizeof(wm_global_t));

		// init appz pointers
		{
			int ii;
			for (ii = 0; ii < WM_MAX_APP_NUM; ii++)
			{
				wm->appz[ii].next = ii + 1;
				wm->appz[ii].prev = ii - 1;
				wm->appz[ii].valid = 0;
			}
			wm->appz[0].prev = WM_MAX_APP_NUM - 1;
			wm->appz[WM_MAX_APP_NUM - 1].next = 0;
			wm->appz_top = -1;
			wm->desktop_pid = -1;
			wm->desktop_msgWid = -1;

			// application serialization semaphore is created off the wm shared mem ID.
			if (-1 == CoolSemCreateBinary(STATE_FILE WM_SHM_ID, (int)'s', &wm->app_serial_lock))
			{
				ERRLOG("unable to create application serialization lock.\n");
			}
		}
		CoolShmWriteUnlock(shmlock);
	}
	return 0;
}

/**
 * Window manager main exit point to clean up memory.
 *
 */
void WmExit(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int app_serial_lock;

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

	CoolShmReadLock(shmlock);
	app_serial_lock = wm->app_serial_lock;
	CoolShmReadUnlock(shmlock);

	CoolSemDestroy(app_serial_lock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	CoolShmPut(WM_SHM_ID);
}


/**
 * Register application with WM.
 *
 * @param name
 *        application name.
 * @return
 *        0 if successful, otherwise non-zero number.
 */
int WmRegisterApp(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int first;
	int top;
	int cur;
	char fname[WM_APP_NAME_LEN];
	shm_lock_t * shmlock;

	reg_sanity_check();

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

	appname_from_command(fname, name);

	// register with z-order.
	CoolShmWriteLock(shmlock);

	if (wm->appz_top == -1)
	{
		top = 0;
	}
	else
	{
		// first check if application is already running.
		for_each_app(cur)
		{
			regs = &wm->appz[cur].reg;

			if (!strcmp(regs->appname, fname))
			{
				CoolShmWriteUnlock(shmlock);
				goto bail_no_shm;
			}
		}

		// no, not running, register it.
		top = wm->appz[wm->appz_top].prev;
	}

	wm->appz_top = top;
	wm->appz[top].valid = 1;
	regs = &wm->appz[top].reg;
	regs->pid = getpid();
	regs->formnum = 0;
	regs->msgWid = -1;
	strlcpy(regs->appname, fname, sizeof(regs->appname));

	DBGLOG("wm-top = %d\n", wm->appz_top);
	DBGLOG("top app: %s\n", regs->appname);
	CoolShmWriteUnlock(shmlock);

	return 0;

 bail:
	//CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return -1;
}

/**
 * Unregister application from WM.
 *
 */
void WmUnregisterApp(void)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int pid;
	int first;
	shm_lock_t * shmlock;

	reg_sanity_check();

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

	pid = getpid();

	CoolShmWriteLock(shmlock);
	//search app registration
	for_each_app(cur)
	{
		if (wm->appz[cur].reg.pid == pid)
		{
			wm->appz[cur].valid = 0;

			// removing application from zorder table.
			if (cur == wm->appz_top)
			{
				int new_top;

				DBGLOG("remove application.\n");

				// bring up next application.
				new_top = wm->appz[cur].next;
				if (wm->appz[new_top].valid == 0)
				{
					WARNLOG("no more neux application running.\n");
					wm->appz_top = -1;
				}
				else
				{
					int fid;

					// bring up next app.
					DBGLOG("+++++++++++++++++++++ bring up next application.\n");

					wm->appz_top = new_top;

					regs = &wm->appz[new_top].reg;
					fid = regs->formid[0];

					CoolShmWriteUnlock(shmlock);
					if (fid > GR_ROOT_WINDOW_ID)
					{
						GrRaiseWindow(fid);
						GrSetFocus(fid);
					}
					else
					{
						WARNLOG("invalid window ID: %d\n", fid);
					}
					CoolShmWriteLock(shmlock);
				}
			}
			else
			{
				// silently remove app from z-order entry table.
				DBGLOG("silently remove application.\n");

				// detach form link
				remove_app(wm, cur);
				// insert cur before top.
				insert_app(wm, wm->appz_top, cur);
			}

			if (-1 != wm->appz_top)
			{
				DBGLOG("wm-top = %d\n", wm->appz_top);
				DBGLOG("top app: %s\n", wm->appz[wm->appz_top].reg.appname);
			}
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	// deref the shared memory reference created at registration time.
	DBGLOG("releasing wm shared memory.\n");
	CoolShmPut(WM_SHM_ID);
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

/**
 * Add form to application. Newly added form takes least z-order.
 *
 * @param wid
 *        Form window ID.
 * @return
 *        0 if successfully added form, -1 otherwise.
 */
int WmAddForm(int wid)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int pid;
	int first;
	int ret = -1;
	shm_lock_t * shmlock;

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

	pid = getpid();

	CoolShmWriteLock(shmlock);
	//search app registration
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;
		if (regs->pid == pid)
		{
			regs->formid[regs->formnum] = wid;
			regs->formnum++;
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

	ret = 0;

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * Remove form from application, set focus to next form in stack if removed form
 * currently has focus and there is more form in stack.
 *
 * @param wid
 *        Form window ID.
 * @param restore
 *        1 to restore previous FORM per the z-order. 0 to destroy current form only.
 */
void WmRemoveForm(int wid, int restore)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int pid;
	int first;
	int focus_reset = 0;
	shm_lock_t * shmlock;

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

	pid = getpid();

	CoolShmWriteLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;
		if (regs->pid == pid)
		{
			int jj;
			for (jj = 0; jj < regs->formnum; jj++)
			{
				if (regs->formid[jj] == wid)
				{
					regs->formnum--;
					if ((jj == 0) && regs->formnum)
						focus_reset = 1;

					for (; jj < regs->formnum; jj++)
						regs->formid[jj] = regs->formid[jj + 1];

					if (focus_reset && restore)
					{
						int fid;

						// always resume to top application by default if top
						// form is destroyed.
						regs = &wm->appz[wm->appz_top].reg;
						fid = regs->formid[0];

						CoolShmWriteUnlock(shmlock);
						if (fid > GR_ROOT_WINDOW_ID)
						{
							GrRaiseWindow(fid);
							GrSetFocus(fid);
						}
						else
						{
							WARNLOG("invalid window ID: %d\n", fid);
						}
						CoolShmWriteLock(shmlock);
					}
					break;
				}
			}
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}


/** Change form z-order
 * Reoder form focus stack, send current focus widget on top of stack.
 *
 * @param wid
 *        Current focus form window ID.
 *
 */
void WmChangeFormZorder(int wid)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int pid;
	int first;
	shm_lock_t * shmlock;

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

	pid = getpid();

	CoolShmWriteLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;
		if (regs->pid == pid)
		{
			if (regs->formid[0] != wid)
			{
				int jj;

				for (jj = regs->formnum; jj-- > 0; )
				{
					if (regs->formid[jj] == wid)
					{
						for (; jj > 0; jj--)
							regs->formid[jj] = regs->formid[jj - 1];
						regs->formid[0] = wid;
						break;
					}
				}
			}
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

/** Return current focus form window ID.
 *
 * @return
 *        Current focus form window ID.
 */
int WmFocusForm(void)
{
	wm_global_t * wm;
	//wm_regist_t * regs;
	//int ii;
	int first;
	shm_lock_t * shmlock;
	int wid = -1;

	reg_sanity_check();

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

	CoolShmReadLock(shmlock);
	if (wm->appz_top != -1)
		wid = wm->appz[wm->appz_top].reg.formid[0];
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return wid;
}

/** Return total number of forms available in current application.
 *
 * @return
 *       number of forms.
 */
int WmTotalForm(void)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int pid;
	int first;
	shm_lock_t * shmlock;
	int num = -1;

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

	pid = getpid();

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (regs->pid == pid)
		{
			num = regs->formnum;
			break;
		}
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return num;
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
int WmIsAppRunning(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	int ret = 0;
	shm_lock_t * shmlock;

	reg_sanity_check();

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

	appname_from_command(fname, name);

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (!strcmp(regs->appname, fname))
		{
			ret = 1;
			break;
		}
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * Check to see if application itself.
 *
 * @param name
 *        application name.
 * @return
 *        1 if application is itself, otherwise zero.
 *
 */
int WmIsAppItself(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	int ret = 0;
	shm_lock_t * shmlock;
	int pid;

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

	appname_from_command(fname, name);

	pid = getpid();
	DBGLOG("current process id = %d\n", pid);

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (!strcmp(regs->appname, fname))
		{
		    /* compare the process ID */
		    if (regs->pid == pid)
			{
				ret = 1;
				break;
			}
		}
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

void WmAppSerialUnlock(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int app_serial_lock = -1;

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

	CoolShmReadLock(shmlock);
	app_serial_lock = wm->app_serial_lock;
	CoolShmReadUnlock(shmlock);

bail:
	CoolShmPut(WM_SHM_ID);
bail_no_shm:
	if (-1 != app_serial_lock) {
		DBGLOG("app_serial_lock up\n");
		CoolSemUp(app_serial_lock);
	}
}

int WmAppSerialIsLocked(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int app_serial_lock = -1;

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

	CoolShmReadLock(shmlock);
	app_serial_lock = wm->app_serial_lock;
	CoolShmReadUnlock(shmlock);

bail:
	CoolShmPut(WM_SHM_ID);
bail_no_shm:
	if (-1 != app_serial_lock) {
		int lock_val = 1;
		CoolSemGetValue(app_serial_lock, &lock_val);
		return !lock_val;
	}
	return 0;
}

/**
 * Run an application.
 *
 * @param name
 *        application name.
 *
 */
void WmRunApp(const char * name)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int app_serial_lock = -1;
	int desktop_msgWid = -1;

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

	CoolShmReadLock(shmlock);
	app_serial_lock = wm->app_serial_lock;
	CoolShmReadUnlock(shmlock);

	if (-1 != app_serial_lock) {
		DBGLOG("app_serial_lock down\n");
		CoolSemDown(app_serial_lock, 1);
	}

	CoolShmReadLock(shmlock);
	if (getpid() == wm->desktop_pid)
	{
		CoolShmReadUnlock(shmlock);
		CoolShmPut(WM_SHM_ID);
		WmRunAppImmediately(name);
		goto bail_no_shm;
	}
	desktop_msgWid = wm->desktop_msgWid;
	CoolShmReadUnlock(shmlock);

	// send msg to desktop to run this app.
	{
		neux_msg_t msg;

		DBGLOG("---------------------------------WM desktop found!\n");

		msg.msg.msg.msgId = SYS_MSG_RUNAPP;
		msg.bytes = strlen(name) + 1;
		memcpy((void*)&msg.msg.msg.msgTxt, name, msg.bytes);
		msg.bytes += sizeof(msg.msg.msg.msgId);

		GrSendMsg(desktop_msgWid, (void*)&msg.msg, msg.bytes);
	}

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

#include <sys/wait.h>  /* define wait(), etc.   */
#include <signal.h>    /* define signal(), etc. */

// here is the code for the signal handler
static void catch_child(int sig_num)
{
    /* when we get here, we know there's a zombie child waiting */
    int child_status;

    DBGLOG("child exited--------------------------------------.\n");

    wait3(&child_status, WNOHANG, NULL);
}

void WmRestoreAppArgs(const char * name)
{
	int wid;

	wid = WmGetAppMsgWid(name);
	if (wid > 0)
	{
		neux_msg_t msg;

		msg.msg.msg.msgId = APP_MSG_RESTORE_ARGS;
		msg.bytes = strlen(name) + 1;
		memcpy((void*)&msg.msg.msg.msgTxt, name, msg.bytes);
		msg.bytes += sizeof(msg.msg.msg.msgId);

		GrSendMsg(wid, (void*)&msg.msg, msg.bytes);
	}
}
/*
static inline void run_app(const char * name)
{
	const char *argv[4];
	int ret;

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = name;
	argv[3] = NULL;

	execv(argv[0], argv);
}
*/

static inline void run_app(const char * name)
{
	const char *argv[MAX_CMD_ARGS];
	char *cmd, *parg;
	int i;

	cmd = strdup(name);
	if (cmd == NULL) {
		WARNLOG("chdir faled (%s)!\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	parg = cmd;
	for (i = 0; i < MAX_CMD_ARGS - 1 && *parg != '\0'; i++) {
		if (*parg == '\'' || *parg == '\"') {
			argv[i] = parg + 1;
			parg = strchr(parg + 1, *parg);
			if (parg == NULL) {
				WARNLOG("illegal command line (%s)!\n", name);
				free(cmd);
				/* Log the failure */
				exit(EXIT_FAILURE);
			}
		} else {
			char *p;

			argv[i] = parg;
			p = strchr(parg, ' ');
//			if (p == NULL)
//				p = strchr(parg, '\t');
			if (p == NULL) {
				i++;
				break;
			}
			parg = p;
		}

		*parg++ = '\0';
		while (*parg == ' ' /*|| *parg == '\t'*/)
			parg++;
	}
	argv[i] = NULL;

	execv(argv[0], argv);

	// the program should not reach here, or it means error occurs
	free(cmd);
}

/**
 * Run an application.
 *
 * @param name
 *        application name.
 *
 */
void WmRunAppImmediately(const char * name)
{
	if (WmIsAppRunning(name))
	{
		DBGLOG("bring up app: %s from background!\n", name);
		WmAppSerialUnlock();
		WmShowApp(name);
		WmRestoreAppArgs(name);
		WmNotifyTopAppChange(TRUE, FALSE);
	}
	else
	{
		pid_t pid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0)
		{
			WARNLOG("fork failed (%s)!\n", strerror(errno));
			goto _exit;
		}

		/* If we got a good PID, then we continue the parent process. */
		if (pid > 0)
		{
			signal(SIGCHLD, catch_child);
		}
		else
		{
			/* now in child process. */
			pid_t sid;
			int i;

			/* Create a new SID for the child process */
			sid = setsid();
			if (sid < 0)
			{
				WARNLOG("setsid faled (%s)!\n", strerror(errno));
				/* Log the failure */
				goto _exit;
			}

			/* Change the current working directory */
			if ((chdir("/")) < 0)
			{
				WARNLOG("chdir faled (%s)!\n", strerror(errno));
				/* Log the failure */
				goto _exit;
			}

			/* Close all files and remap the standard file descriptors */
			// leave stdin / out / err open.
			for (i = getdtablesize(); i >= 3; --i)
				close(i);

			run_app(name);

			// the program should not reach here, or it means error occurs
			WARNLOG("run \"%s\" failed (%s)!\n", name, strerror(errno));

_exit:
			WmAppSerialUnlock();
			exit(EXIT_FAILURE);
		}
	}
}

/** Notify WM the running of a new application.
 *
 */
void WmStartApp(void)
{
	static int app_started = 0;

	if (!app_started)
	{
		wm_global_t * wm;
		int first;
		shm_lock_t * shmlock;
		int app_serial_lock = -1;
		int desktop_pid;

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

		CoolShmReadLock(shmlock);
		app_serial_lock = wm->app_serial_lock;
		desktop_pid = wm->desktop_pid;
		CoolShmReadUnlock(shmlock);

	bail:
		CoolShmPut(WM_SHM_ID);
	bail_no_shm:

		app_started = 1;

		if (-1 != app_serial_lock && desktop_pid != getpid())
			CoolSemUp(app_serial_lock);
	}
}

/**
 * Bring up the desktop.
 *
 *
 */
void WmGotoDesktop(void)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	shm_lock_t * shmlock;

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
	//search app registration
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (regs->pid == wm->desktop_pid)
		{
			int fid = regs->formid[0];

			DBGLOG("WM registered application found!\n");
			CoolShmWriteUnlock(shmlock);

			if (fid > GR_ROOT_WINDOW_ID)
			{
				GrRaiseWindow(fid);
				GrSetFocus(fid);
			}
			else
			{
				WARNLOG("invalid window ID: %d\n", fid);
			}
			CoolShmWriteLock(shmlock);

			// check to change app z-order.
			if (cur != wm->appz_top)
			{
				DBGLOG("bring app to top.\n");
				DBGLOG("wm-top = %d\n", wm->appz_top);
				DBGLOG("top app: %s\n", wm->appz[wm->appz_top].reg.appname);

				// detach form link
				remove_app(wm, cur);
				// insert cur before top.
				insert_app(wm, wm->appz_top, cur);

				wm->appz_top = cur;
			}
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

/**
 * Show a running application and bring it to top.
 *
 * @param name
 *        application name.
 *
 */
void WmShowApp(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	shm_lock_t * shmlock;
//	int app_serial_lock = -1;

	//NOT NEEDED: caller has to guarantee that WmIsAppRunning is called first.
	//reg_sanity_check();

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

	appname_from_command(fname, name);

	CoolShmWriteLock(shmlock);
//	app_serial_lock = wm->app_serial_lock;

	//search app registration
	for_each_app(cur)
	{
		int fid;
		regs = &wm->appz[cur].reg;
		if (!strcmp(regs->appname, fname))
		{
			DBGLOG("WM registered application found!\n");
			fid = regs->formid[0];
			if (fid > GR_ROOT_WINDOW_ID)
			{
				if (cur != wm->appz_top)
				{
					GR_WINDOW_INFO info;

					CoolShmWriteUnlock(shmlock);
					GrRaiseWindow(fid);
					GrGetWindowInfo(fid, &info);
					if (info.mapped)
						GrSetFocus(fid);
					CoolShmWriteLock(shmlock);
				}
			}
			else
			{
				WARNLOG("invalid window ID: %d\n", fid);
			}

			// check to change app z-order.
			if (cur != wm->appz_top)
			{
				DBGLOG("bring app to top.\n");
				DBGLOG("wm-top = %d\n", wm->appz_top);
				DBGLOG("top app: %s\n", wm->appz[wm->appz_top].reg.appname);

				// detach form link
				remove_app(wm, cur);
				// insert cur before top.
				insert_app(wm, wm->appz_top, cur);

				wm->appz_top = cur;
			}
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
//	if (-1 != app_serial_lock)
//		CoolSemUpBinary(app_serial_lock);
	return;
}

const char *WmTopApp(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
//	int app_serial_lock = -1;
	char *res = NULL;

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
//	app_serial_lock = wm->app_serial_lock;
	res = wm->appz[wm->appz_top].reg.appname;
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
//	if (-1 != app_serial_lock)
//		CoolSemUpBinary(app_serial_lock);
	return res;
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

int WmIsAppOnTop(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	shm_lock_t * shmlock;
//	int app_serial_lock = -1;
	int ret = 0;

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

	appname_from_command(fname, name);

	CoolShmWriteLock(shmlock);
//	app_serial_lock = wm->app_serial_lock;

	//search app registration
	cur = wm->appz_top;
	regs = &wm->appz[cur].reg;
	if (!strcmp(regs->appname, fname))
		ret = 1;

	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
//	if (-1 != app_serial_lock)
//		CoolSemUpBinary(app_serial_lock);
	return ret;
}

int WmTopAppIsSelf(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int ret = -1;
	int pid;

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

	pid = getpid();

	CoolShmWriteLock(shmlock);
	if (wm->appz_top != -1)
		ret = (wm->appz[wm->appz_top].reg.pid == pid);
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * Hide a running application and bring up next applicatin in stack.
 *
 *
 */
void WmHideApp(void)
{
	wm_global_t * wm;
	int first;
	shm_lock_t * shmlock;
	int cur;
	int prev;
	int next;
	int found = -1;
	int pid;
	int quit = 0;

	reg_sanity_check();

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

	pid = getpid();

	// logic to protect desktop from being minimized.
	CoolShmReadLock(shmlock);
	if (pid == wm->desktop_pid)
	{
		WARNLOG("Can not minimize desktop.\n");
		quit = 1;
	}
	CoolShmReadUnlock(shmlock);
	if (quit) goto bail;

	CoolShmWriteLock(shmlock);
	//search app registration
	for_each_app(cur)
	{
		if (wm->appz[cur].reg.pid == pid)
		{
			if (wm->appz[wm->appz[cur].next].valid == 0)
			{
				if (cur == wm->appz_top)
					DBGLOG("You are trying to minimize the desktop!\n");
				else
					DBGLOG("Application is already minimized!\n");
				goto no_app;
			}
			else
			{
				wm_regist_t * regs;
				int fid;

				// move application to the end of the zorder table.
				DBGLOG("moving application to bottom zorder.\n");
				found = cur;

				regs = &wm->appz[cur].reg;
				fid = regs->formid[0];

				CoolShmWriteUnlock(shmlock);
				if (fid > GR_ROOT_WINDOW_ID)
				{
					GrLowerWindow(fid);
				}
				else
				{
					WARNLOG("invalid window ID: %d\n", fid);
				}
				CoolShmWriteLock(shmlock);
			}

			if (cur == wm->appz_top)
			{
				int top;
				wm_regist_t * regs;
				int fid;

				wm->appz_top = wm->appz[cur].next;
				top = wm->appz_top;

				// bring up top app.
				regs = &wm->appz[top].reg;
				fid = regs->formid[0];

				CoolShmWriteUnlock(shmlock);
				if (fid > GR_ROOT_WINDOW_ID)
				{
					GrRaiseWindow(fid);
					GrSetFocus(fid);
				}
				else
				{
					WARNLOG("invalid window ID: %d\n", fid);
				}
				CoolShmWriteLock(shmlock);

				DBGLOG("bring up app: form %d\n", regs->formid[0]);
				DBGLOG("bring up app: %s\n", wm->appz[wm->appz_top].reg.appname);
			}

			DBGLOG("wm-top = %d\n", wm->appz_top);
			DBGLOG("top app: %s\n", wm->appz[wm->appz_top].reg.appname);
		}
	}

	if (found != -1)
	{
		// detach form link
		remove_app(wm, found);
		// insert cur after tail.
		insert_app(wm, cur, found);

		DBGLOG("z-order adjust completed.\n");
	}
	else
		WARNLOG("Unable to find application and hide it!\n");

 no_app:
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

int WmGetTopAppMsgWid(void)
{
	wm_global_t * wm;
	int cur;
	int first;
	int ret = -1;
	shm_lock_t * shmlock;

	reg_sanity_check();

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

	CoolShmReadLock(shmlock);
	ret = wm->appz[wm->appz_top].reg.msgWid;
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * Check to get application message WID if application running.
 *
 * @param name
 *        application name.
 * @return
 *        message WID if application is running and is valid to take messages,
 *        otherwise less than or equal to zero.
 *
 */
int WmGetAppMsgWid(const char * name)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	int ret = -1;
	shm_lock_t * shmlock;

	reg_sanity_check();

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

	appname_from_command(fname, name);

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (!strcmp(regs->appname, fname))
		{
			ret = regs->msgWid;
			break;
		}
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * Check to get application message WID if application running.
 *
 * @param name
 *        application name.
 * @return
 *        message WID if application is running and is valid to take messages,
 *        otherwise less than or equal to zero.
 *
 */
int WmGetOtherAppMsgWids(int * msgWid, const char * exclude)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	char fname[WM_APP_NAME_LEN];
	int ret = -1;
	shm_lock_t * shmlock;
	int pid = getpid();

	reg_sanity_check();

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

	ret = 0;

	if (exclude != NULL)
		appname_from_command(fname, exclude);

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;
		if (regs->pid == pid)
			continue;
		if (exclude != NULL && !strcmp(regs->appname, fname))
			continue;
		msgWid[ret++] = regs->msgWid;
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return ret;
}

/**
 * return current application name.
 *
 * @param name
 *        application name buffer.
 * @param max_name_len
 *        maximum application name length in bytes.
 *
 * @return
 *        pointer to name buffer if name found, otherwise NULL.
 */
char * WmGetAppName(char * name, int max_name_len)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	int pid = getpid();
	shm_lock_t * shmlock;
	char * appname = NULL;

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

	CoolShmReadLock(shmlock);
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (regs->pid == pid)
		{
			strlcpy(name, regs->appname, max_name_len);
			appname = name;
			break;
		}
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return appname;
}

/**
 * Check to set application message WID.
 *
 * @param msgWid
 *        application message window ID.
 *
 */
void WmSetAppMsgWid(int msgWid)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur;
	int first;
	int pid = getpid();
	shm_lock_t * shmlock;

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
	for_each_app(cur)
	{
		regs = &wm->appz[cur].reg;

		if (regs->pid == pid)
		{
			regs->msgWid = msgWid;
			break;
		}
	}
	CoolShmWriteUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}

static always_inline int WmStopEverythingExceptDesktop(BOOL do_not_stop_self, BOOL block)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int cur, next;
	int first;
	int pid_self = 0;
	shm_lock_t * shmlock;
    int desktop_fid = 0;
	int itemp = 0;
	int app_pid[WM_MAX_APP_NUM];
	int ret = 0;
	int app_serial_lock = -1;

	reg_sanity_check();

	if (do_not_stop_self)
		pid_self = getpid();

	// get WM globals.
	wm = CoolShmGet(WM_SHM_ID, sizeof(wm_global_t), &first, &shmlock);
	if (!wm)
	{
		WARNLOG("Unable to fetch WM shared memory!\n");
		ret = -1;
		goto bail_no_shm;
	}
	if (first)
	{
		WARNLOG("WM shared memory in wrong state.\n");
		ret = -1;
		goto bail;
	}

	CoolShmReadLock(shmlock);
	app_serial_lock = wm->app_serial_lock;
	CoolShmReadUnlock(shmlock);

	if (-1 != app_serial_lock) {
		DBGLOG("app_serial_lock down\n");
		CoolSemDown(app_serial_lock, 1);
	}

	CoolShmReadLock(shmlock);
	for_each_app_safe(cur, next)
	{
		regs = &wm->appz[cur].reg;
		if (regs->pid == wm->desktop_pid)
		{
 		    desktop_fid = regs->formid[0];
		    continue;
		}
		if (do_not_stop_self && regs->pid == pid_self)
			continue;

		if (block)
			app_pid[itemp++] = regs->pid;

		CoolShmReadUnlock(shmlock); // Need unlock to stop application

		stop_app(regs->msgWid, regs->appname);

		CoolShmReadLock(shmlock);
	}
	CoolShmReadUnlock(shmlock);

	if (!do_not_stop_self)
	{
	    // brings up desktop if it is not already on top.
	    CoolShmWriteLock(shmlock);
	    GrRaiseWindow(desktop_fid);
	    GrSetFocus(desktop_fid);
	    CoolShmWriteUnlock(shmlock);
	}

	if (block)
	{
		int i;
		for (i = 0; i < itemp; i++)
		{
			int retry = 0;
			while (is_pid_valid(app_pid[i]))
			{
				retry++;
				if (retry > MAX_RETRY_TIME)
				{
					WARNLOG("Unable to stop application : %d!\n", app_pid[i]);
					ret = -1;
					goto bail_timeout;
				}
				usleep(BLOCK_TIME * 1000);
			}
		}
bail_timeout: ;
	}

	if (-1 != app_serial_lock) {
		DBGLOG("app_serial_lock up\n");
		CoolSemUp(app_serial_lock);
	}

bail:
	CoolShmPut(WM_SHM_ID);
bail_no_shm:
	return ret;
}


/**
 * Stop all applications except desktop.
 */
int WmStopAllApp(BOOL block)
{
	return WmStopEverythingExceptDesktop(FALSE, block);
}

/**
 * Stop all applications, except the one that called this and desktop.
 * @param block,
 *	TRUE : block, FALSE : non-block
 * @return
 *	0 : stop all other apps successfully. otherwise, has some error
 */
int WmStopAllOtherApp(BOOL block)
{
	return WmStopEverythingExceptDesktop(TRUE, block);
}


/**
 * Stop an application
 * @param app,
 *	application name
 */
void WmStopSpecificApp(const char *app)
{
	int wid;

	wid = WmGetAppMsgWid(app);
	if (wid > 0)
		stop_app(wid, app);
}

void WmSetSsaverTimeout(int timeout)
{
	int wid;

	wid = WmGetAppMsgWid(WM_APP_NAME);
	if (wid > 0)
	{
		neux_msg_t msg;

		msg.msg.msg.msgId = SYS_MSG_SET_SSAVER_TIMEOUT;
		msg.bytes = sizeof(timeout);
		memcpy((void*)&msg.msg.msg.msgTxt, &timeout, msg.bytes);
		msg.bytes += sizeof(msg.msg.msg.msgId);

		GrSendMsg(wid, (void*)&msg.msg, msg.bytes);
	}
}


void WmSsaverPauseResume(SSAVER_PR_STATUS pr)
{
	int wid;

	wid = WmGetAppMsgWid(WM_APP_NAME);
	if (wid > 0)
	{
		neux_msg_t msg;

		msg.msg.msg.msgId = SYS_MSG_SSAVER_PAUSE_RESUME;
		msg.bytes = sizeof(pr);
		memcpy((void*)&msg.msg.msg.msgTxt, &pr, msg.bytes);
		msg.bytes += sizeof(msg.msg.msg.msgId);

		GrSendMsg(wid, (void*)&msg.msg, msg.bytes);
	}
}

void WmNotifyTopAppChange(BOOL always, BOOL is_new)
{
	wm_global_t * wm;
	wm_regist_t * regs;
	int first;
	int pid = getpid();
	shm_lock_t * shmlock;

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

	CoolShmReadLock(shmlock);
	if (always || pid != wm->desktop_pid) {
		neux_msg_t msg;

		regs = &wm->appz[wm->appz_top].reg;

		DBGLOG("Sending TOPAPP_CHANGE (%s) msg to desktop\n", regs->appname);

		msg.msg.msg.msgId = APP_MSG_TOPAPP_CHANGE;
		msg.bytes = sizeof(int) + strlen(regs->appname) + 1;
		*(int*)(&msg.msg.msg.msgTxt) = is_new;
		memcpy(((void*)&msg.msg.msg.msgTxt) + sizeof(int), regs->appname, msg.bytes);
		msg.bytes += sizeof(msg.msg.msg.msgId);

		GrSendMsg(wm->desktop_msgWid, (void*)&msg.msg, msg.bytes);
	}
	CoolShmReadUnlock(shmlock);

 bail:
	CoolShmPut(WM_SHM_ID);
 bail_no_shm:
	return;
}
