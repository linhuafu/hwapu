/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
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
 * Main App routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
#include <signal.h>

#include <nano-X.h>

#define OSD_DBG_MSG
#include "nc-err.h"

#include "amixer.h"
#include "application.h"
#include "neux.h"
#include "widgets.h"
#include "desktop-ipc.h"
#include "application-ipc.h"
#include "plextalk-i18n.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "sysfs-helper.h"
#include "nw-statusbar.h"
#include "nw-titlebar.h"
#include "nw-main.h"
#include "plextalk-keys.h"
#include "nxutils.h"
#include "plextalk-constant.h"

#define TIMER_TYPE_COUNTDOWN	1
#define TIMER_TYPE_ALARM		2

#define PLEXTAL_TIMER_COUNTDOWN			"/sys/bus/i2c/devices/1-0034/axp173-timer/expires"


static struct plextalk_timer timer[3];//appk

static TIMER *tickTimer;
//static TIMER *tickTimer1;
static time_t suspend_time;

static const int PlextalkCountdown[] = {
	3, 5, 15, 30, 45, 60, 90, 120
};

static void
SetTimerIcon()
{
	static int last_icon = SBAR_TIMER_ICON_NO;
	int icon;

	icon = timer[0].enabled + timer[1].enabled * 2;
	if (icon != last_icon) {
		StatusBarSetIcon(STATUSBAR_TIMER_INDEX, icon);
		last_icon = icon;
	}
}

static void
UpdateSettingFile(void)
{
	PlextalkCheckAndCreateDirectory(PLEXTALK_SETTING_DIR);
	plextalk_setting_write_xml(PLEXTALK_SETTING_DIR "setting.xml");
}

static inline int
IsAlarmTimerPending(int index)
{
	return timer[index].enabled &&
	       timer[index].function == PLEXTALK_TIMER_ALARM;
}

static inline int
IsCountDownTimerPending(int index)
{
	return timer[index].enabled &&
	       timer[index].type == PLEXTALK_TIMER_COUNTDOWN;
}

/*
 * get timer's expire interval (unit: minutes)
 */
static int
TimerExpire(sys_time_t *stm, int index,int *type)//appk 增加一个标记表示是countdown还是alarm
{
	if (!timer[index].enabled)
		return -1;

	if (timer[index].type == PLEXTALK_TIMER_COUNTDOWN) {
		*type = TIMER_TYPE_COUNTDOWN;
		return plextalk_get_timer_countdown(index);
	} else {
		*type = TIMER_TYPE_ALARM;
	}

	if (timer[index].repeat == PLEXTALK_ALARM_REPEAT_WEEKLY) {
		int days, wday;

		if (!(timer[index].weekday & 0x7f))
			return -1;

		if (timer[index].weekday & (1U << stm->wday)) {
			if (stm->hour < timer[index].hour ||
				(stm->hour == timer[index].hour && stm->min < timer[index].minute))
				return (timer[index].hour - stm->hour) * 60 +
						(timer[index].minute - stm->min);
		}

		days = 0;
		wday = stm->wday;
		do {
			wday++;
			if (wday >= 7)
				wday = 0;
			days++;
		} while (!(timer[index].weekday & (1U << wday)));

		return days * 24 * 60 +
				(timer[index].hour - stm->hour) * 60 +
				(timer[index].minute - stm->min);
	}
	//once and every day
	if (stm->hour < timer[index].hour ||
		(stm->hour == timer[index].hour && stm->min < timer[index].minute))
		return (timer[index].hour - stm->hour) * 60 +
				(timer[index].minute - stm->min);
//bug
//	if (timer[index].repeat == PLEXTALK_ALARM_REPEAT_ONCE)
//		return -1;
	//every day
	return 24 * 60 +
			(timer[index].hour - stm->hour) * 60 +
			(timer[index].minute - stm->min);
}

static void
ReschduleTimer(sys_time_t *stm)
{
	int next0, next1;
	int type = 0;
	int countdown = -1;
	int alarm = -1;

	SetTimerIcon();

#if 0//appk
	if (!(timer[0].enabled + timer[1].enabled)){
		printf("\n%s disable\n",__FUNCTION__);		//add by lhf
		sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, 0); //add by lhf
		return;
	}

	/* set nearest timer to wakealarm, so we can wakeup in suspend */
	next0 = TimerExpire(stm, 0, &type);
	next1 = TimerExpire(stm, 1, &type);
	if ((unsigned int)next1 < (unsigned int)next0)
		next0 = next1;	
	printf("\n%s=%d\n",__FUNCTION__,next0 * 60);		//add by lhf
	sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, stm->raw + next0 * 60);
#else//appk

	if (!(timer[0].enabled + timer[1].enabled + timer[2].enabled)){
		printf("\n%s disable\n",__FUNCTION__);		//add by lhf
		sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, 0); //add by lhf
		sysfs_write_integer(PLEXTAL_TIMER_COUNTDOWN, 0); 
		return;
	}
	

	/* set nearest timer to wakealarm/HardTimer, so we can wakeup in suspend */
	next1 = TimerExpire(stm, 0, &type);
	if(next1 != -1) {
		if(TIMER_TYPE_ALARM == type) {
			alarm = next1;
		} else {
			countdown = next1;
		}
	}
	next1 = TimerExpire(stm, 1, &type);
	if(next1 != -1) {
		if(TIMER_TYPE_ALARM == type) {
			if ((unsigned int)next1 < (unsigned int)alarm)
				alarm = next1;
		} else {
			if ((unsigned int)next1 < (unsigned int)countdown)
				countdown = next1;
		}
	}
	next1 = TimerExpire(stm, 2, &type);
	if(next1 != -1) {
		if(TIMER_TYPE_ALARM == type) {
			if ((unsigned int)next1 < (unsigned int)alarm)
				alarm = next1;
		} else {
			if ((unsigned int)next1 < (unsigned int)countdown)
				countdown = next1;
		}
	}

	printf("set alarm time:%d\n",alarm * 60);		//add by lhf

//	sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, stm->raw + alarm * 60);	//res

//wrtie with xx:xx:00	
	time_t w_time = stm->raw + alarm * 60;
	time_t s_time = w_time - w_time % 60;

	printf("s_time:%lu\n",s_time);		//add by lhf
	sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, 0); //add by lhf
	sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, s_time);

	printf("set countdown time:%d\n",countdown);//write to Hardware Timer units:minute
	if(countdown == -1)
		sysfs_write_integer(PLEXTAL_TIMER_COUNTDOWN, 0);
	else
		sysfs_write_integer(PLEXTAL_TIMER_COUNTDOWN, countdown);

	if(tickTimer) {
		NeuxTimerSetControl(tickTimer, DELAY_SECOND * 60, -1);//硬定时器与软定时器同步
	}

#endif//appk
}

static int
TimerCheckExpired(sys_time_t *stm, int index, int action)
{
	if (!timer[index].enabled)
		return 0;

	if (timer[index].type == PLEXTALK_TIMER_COUNTDOWN) {
		unsigned int countdown = plextalk_get_timer_countdown(index);
		if (countdown > 0) {

			CoolShmWriteLock(g_config_lock);
			g_config->timer_countdown[index]--;
			CoolShmWriteUnlock(g_config_lock);

			countdown--;
			if (countdown > 0)
				return 0;
		}
	} else {
		if (timer[index].repeat == PLEXTALK_ALARM_REPEAT_WEEKLY &&
			!(timer[index].weekday & (1U << stm->wday)))
			return 0;

		if (stm->min != timer[index].minute ||
			stm->hour != timer[index].hour)
			return 0;

		if (timer[index].repeat != PLEXTALK_ALARM_REPEAT_ONCE)
			goto do_action;
	}

	/* one shot timer */
	timer[index].enabled = 0;

	CoolShmWriteLock(g_config_lock);
	g_config->setting.timer[index].enabled = 0;
	CoolShmWriteUnlock(g_config_lock);

	UpdateSettingFile();

do_action:
	if (action) {
		if (timer[index].function == PLEXTALK_TIMER_ALARM) {
			if (!plextalk_get_alarm_disable()) {
				char alarm_args[] = { '0' + index, '\0', };
				AppLauncher(M_ALARM, 0, 0, alarm_args);
			}
		} else {
			if (!plextalk_sleep_timer_is_locked())
				DesktopPowerOff();
		}
	}

	return 1;
}

static void
OnTickTimer(WID__ wid)
{
	sys_time_t stm;
	int active0, active1,active2;

	sys_get_time(&stm);

#if 0
	// colck_label更新进行判断的方式
	if (StatusBarRereshTime(&stm) == 1)	{ //means time str has changed (unit - minute)
		active0 = TimerCheckExpired(&stm, 0, 1);
		/* timer 0 has high priority */
		active1 = TimerCheckExpired(&stm, 1, !active0);
		active2 = TimerCheckExpired(&stm, 2, !active1);//appk

		DBGMSG("New Time(unit - minute): \n");
		DBGMSG("active0->%d, active1->%d, active2->%d\n", active0, active1, active2);

		if (active0 | active1 | active2)//appk
			ReschduleTimer(&stm);
	}
#endif

	StatusBarRereshTime(&stm);
	active0 = TimerCheckExpired(&stm, 0, 1);
	/* timer 0 has high priority */
	active1 = TimerCheckExpired(&stm, 1, !active0);
	active2 = TimerCheckExpired(&stm, 2, !active1);//appk

	DBGMSG("active0->%d, active1->%d, active2->%d\n", active0, active1, active2);

	if (active0 | active1 | active2)//appk
		ReschduleTimer(&stm);

}

//static void
//OnTickTimer1(WID__ wid)
//{
//	sys_time_t stm;

//	sys_get_time(&stm);
//	StatusBarRereshTime(&stm);
//}

void
TimerSettingChange(int index)
{
	sys_time_t stm;
	DBGMSG("TimerSettingChange\n");
	CoolShmReadLock(g_config_lock);
	timer[index] = g_config->setting.timer[index];
	CoolShmReadUnlock(g_config_lock);

	if (IsCountDownTimerPending(index)) {
		CoolShmWriteLock(g_config_lock);
		g_config->timer_countdown[index] = PlextalkCountdown[timer[index].elapse];
		CoolShmWriteUnlock(g_config_lock);
	}

	sys_get_time(&stm);

	printf("stm.raw:%lu\n",stm.raw);		//add by lhf
	ReschduleTimer(&stm);
}

//appk start
void InitOffTimer()
{
	timer[2].enabled = 0;//must be 0
	timer[2].type = PLEXTALK_TIMER_COUNTDOWN;
	timer[2].function = PLEXTALK_TIMER_SLEEP;
	timer[2].elapse = g_config->auto_poweroff_time;//PLEXTALK_SLEEP_15MIN;
	
	CoolShmWriteLock(g_config_lock);
	g_config->setting.timer[2] = timer[2];
	CoolShmWriteUnlock(g_config_lock);
	
}

void OffTimerSettingChange(int enable,int elapse,int schdule)
{
	sys_time_t stm;

	DBGMSG("OffTimerSettingChange enable=%d,elapse=%d,schdule=%d\n",enable,elapse,schdule);

#if 1//自动关机有问题，先关掉	
	timer[2].enabled = 0;//enable?1:0;
#else
	timer[2].enabled = enable?1:0;
	timer[2].elapse = elapse;
#endif


	if (IsCountDownTimerPending(2)) {
		CoolShmWriteLock(g_config_lock);
		g_config->timer_countdown[2] = timer[2].elapse;//PlextalkCountdown[timer[2].elapse];
		g_config->setting.timer[2] = timer[2];
		CoolShmWriteUnlock(g_config_lock);
	}

#if 1//自动关机有问题，先关掉	
#else
	if(schdule){
		sys_get_time(&stm);
		ReschduleTimer(&stm);
	}
#endif	

	
}

int isOffTimerSettingEnable(void)
{
	int ret;

	ret = timer[2].enabled;
	
	DBGMSG("off timer is:%d\n",ret);
	return ret;
}



//appk end

void
InitTimer(WIDGET * owner)
{
	sys_time_t stm;
	int active = 0;
	int i;

	sys_get_time(&stm);
	StatusBarRereshTime(&stm);

//	tickTimer = NeuxTimerNew(owner, DELAY_SECOND * 5 , -1);		//do this on 5s
	tickTimer = NeuxTimerNew(owner, DELAY_SECOND * 60, - 1);
	NeuxTimerSetCallback(tickTimer, OnTickTimer);
	
//	tickTimer1 = NeuxTimerNew(owner, DELAY_SECOND * 60, -1);
//	tickTimer1 = NeuxTimerNew(owner, DELAY_SECOND * 5, -1);		//5s then flush the clock label
//	NeuxTimerSetCallback(tickTimer1, OnTickTimer1);

	CoolShmReadLock(g_config_lock);
	timer[0] = g_config->setting.timer[0];
	timer[1] = g_config->setting.timer[1];
	CoolShmReadUnlock(g_config_lock);

	for (i = 0; i < 2; i++) {
		if (timer[i].type == PLEXTALK_TIMER_COUNTDOWN)
			timer[i].enabled = 0;
		else {
			active = TimerCheckExpired(&stm, i, !active);
			if (active)
				desktop_state = DESKTOP_STATE_ALARM;
		}
	}
	//InitOffTimer();

	ReschduleTimer(&stm);
}

void
HandleSetTimeMsg(void)
{
	int active0 = 0;
	sys_time_t stm;

	sys_get_time(&stm);
	StatusBarRereshTime(&stm);

	if (timer[0].type != PLEXTALK_TIMER_COUNTDOWN)
		active0 = TimerCheckExpired(&stm, 0, 1);
	if (timer[1].type != PLEXTALK_TIMER_COUNTDOWN)
		TimerCheckExpired(&stm, 1, !active0);

	ReschduleTimer(&stm);
}

int
HasAlarmTimerPending(void)
{
	return IsAlarmTimerPending(0) ||
	       IsAlarmTimerPending(1);
}

void
TimerSuspendEnterHook(void)
{
	if (IsCountDownTimerPending(0) ||
	    IsCountDownTimerPending(1) || 
	    IsCountDownTimerPending(2))//appk
		time(&suspend_time);
}

void
TimerSuspendExitHook(void)
{
	if (IsCountDownTimerPending(0) ||
	    IsCountDownTimerPending(1) ||
	    IsCountDownTimerPending(2)
	    ) {
		time_t wakeup_time;
		int mins_diff;

		time(&wakeup_time);
		mins_diff = (wakeup_time - suspend_time) / 60;

		CoolShmWriteLock(g_config_lock);
		if (IsCountDownTimerPending(0)) {
			if(g_config->timer_countdown[0] > mins_diff)//appk start
				g_config->timer_countdown[0] -= mins_diff;
			else
				g_config->timer_countdown[0] = 0;
		}
		if (IsCountDownTimerPending(1)) {
			if(g_config->timer_countdown[1] > mins_diff)
				g_config->timer_countdown[1] -= mins_diff;
			else
				g_config->timer_countdown[1] = 0;
		}
		if (IsCountDownTimerPending(2)) {
			if(g_config->timer_countdown[2] > mins_diff)
				g_config->timer_countdown[2] -= mins_diff;
			else
				g_config->timer_countdown[2] = 0;//appk end
		}
		CoolShmWriteUnlock(g_config_lock);
	}
}


//add by lhf
static int disableTimer(int index)
{
	if( timer[index].enabled ){
		if(1) {
			timer[index].enabled = 0;
			CoolShmWriteLock(g_config_lock);
			g_config->setting.timer[index].enabled = 0;
			CoolShmWriteUnlock(g_config_lock);
			return 1;
		}
	}
	return 0;
}
void
disableAllTimer(void)
{//add by lhf
	int ret1,ret2,ret3;
	sys_time_t stm;

	ret1 = disableTimer(0);
	ret2 = disableTimer(1);
	ret3 = disableTimer(2);
	if(ret1 || ret2 || ret3){
		UpdateSettingFile();
		sys_get_time(&stm);
		ReschduleTimer(&stm);
	}
	// todo
}

//add by lhf



//add by lhf
static int disableInvalidTimer(int index)
{
	if( timer[index].enabled ){
		if( timer[index].function == PLEXTALK_TIMER_SLEEP || 
			timer[index].type == PLEXTALK_TIMER_COUNTDOWN ) {
			timer[index].enabled = 0;
			CoolShmWriteLock(g_config_lock);
			g_config->setting.timer[index].enabled = 0;
			CoolShmWriteUnlock(g_config_lock);
			return 1;
		}
	}
	return 0;
}//add by lhf

void
TimerPoweroffHook(void)
{//add by lhf
	int ret1,ret2,ret3;
	sys_time_t stm;

	ret1 = disableInvalidTimer(0);
	ret2 = disableInvalidTimer(1);
	ret3 = disableInvalidTimer(2);
	if(ret1 || ret2 || ret3){
		UpdateSettingFile();
		sys_get_time(&stm);
		ReschduleTimer(&stm);
	}
	// todo
}

