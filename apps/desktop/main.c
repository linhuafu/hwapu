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
#include "file-helper.h"
#include "nw-statusbar.h"
#include "nw-titlebar.h"
#include "nw-main.h"
#include "plextalk-keys.h"
#include "nxutils.h"
#include "plextalk-constant.h"
#include "volume.h"
#include "file-helper.h"
#include "platform.h"

struct voice_prompt_cfg vprompt_cfg;

static int signal_fd;
int last_msgid;
int vprompt_change_state;	//zzx
int radio_record_active;
int desktop_theme_index;	//ztz
int desktop_tts_brunning;//km
int auto_off_flag;//km 

int specical_for_info;		//ztz

int auto_off_count_down_active;		//ztz

static TIMER *suspendTimer;

static void
signal_handler(int signum)
{
	WPRINT("------- signal caught:[%d] --------\n", signum);
	if ((signum == SIGINT) || (signum == SIGTERM) || (signum == SIGQUIT))
	{
		WPRINT("------- global config destroyed --------\n");
		NeuxWMAppStop();
		WPRINT("------- NEUX stopped            --------\n");
		exit(0);
	}
}

static
void signalInit(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sa, NULL)){}
    if (sigaction(SIGTERM, &sa, NULL)){}
    if (sigaction(SIGQUIT, &sa, NULL)){}
}

//ztz	--start--
static void resyncTimerSetting (void) 
{
	DBGMSG("resync Timer Setting!\n");
	disableAllTimer();
	StatusBarSetIcon(STATUSBAR_TIMER_INDEX, SBAR_TIMER_ICON_NO);
}
//ztz	--end--

static void resyncSetting(void)
{
	CoolShmReadLock(g_config_lock);

	NeuxThemeInit(g_config->setting.lcd.theme);

	plextalk_update_lang(g_config->setting.lang.lang, PLEXTALK_IPC_NAME_DESKTOP);
	plextalk_update_sys_font(g_config->setting.lang.lang);
	plextalk_update_tts(g_config->setting.lang.lang);

	vprompt_cfg.volume    = g_config->volume.guide;
	vprompt_cfg.speed     = g_config->setting.guide.speed;

	desktop_theme_index = g_config->setting.lcd.theme;

	DBGMSG("resyncSetting saver:%d,offtimer:%d\n",g_config->setting.lcd.screensaver,plextalk_get_offtimer_disable());
#if 0//自动关机有问题，先关掉
	if(g_config->setting.lcd.screensaver == PLEXTALK_SCREENSAVER_DEACTIVE && !plextalk_get_offtimer_disable()){
		NeuxWMAppSetScreenSaverTime(60*g_config->setting.timer[2].elapse);
	} else 
#endif	
	{
		NeuxWMAppSetScreenSaverTime(plextalk_screensaver_timeout[g_config->setting.lcd.screensaver]);
	}

	if (g_config->setting.lcd.screensaver == PLEXTALK_SCREENSAVER_ACTIVENOW) {
		sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 4);
		sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 4);
		//NhelperDisplay(0);
	} else {
		sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 0);
		sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 0);
		sysfs_write_integer(PLEXTALK_LCD_BRIGHTNESS, plextalk_lcd_brightless[g_config->setting.lcd.backlight]);
	}
		
	CoolShmReadUnlock(g_config_lock);
}

//ztz 	--start--

static int SystemInHostMode (void)
{
	dir_node_t node;
	int dir_num;
	int i;
	int ret = 0;
	
	PlextalkScanDir("/sys/class/block/", &node, PlextalkFilterAll);
	DBGMSG("Scandir exit, node.num = %d\n", node.num);
	dir_num = node.num;

	for (i = 0 ; i < node.num; i ++ ) {
		if (StringStartWith(node.namelist[i]->d_name, "sr") 
			|| (StringStartWith(node.namelist[i]->d_name, "sd"))) {
			ret = 1;
			break;
		}
	}
	PlextalkDestroyDir(&node);

	DBGMSG("SystemInHostMode : ret = %d\n", ret);
	return ret;
}

 
/* special for plextalk_v1 auto_off */
void AutoOffSetCountDown (int ture)
{
	if (!ture) {
		DBGMSG("Auto Off disable!\n");
		sysfs_write_integer(PLEXTALK_AUTO_OFF_SYS_FILE, 0);
		
		auto_off_count_down_active = 0;
	} else {
		DBGMSG("Auto Off Enable!!!\n");
		int auto_off_timer;

		CoolShmReadLock(g_config_lock);
		auto_off_timer = g_config->auto_poweroff_time * 60;
		CoolShmReadUnlock(g_config_lock);
		
		sysfs_write_integer(PLEXTALK_AUTO_OFF_SYS_FILE, auto_off_timer);

		DBGMSG("Auto off set time = %d seconds\n", auto_off_timer);
		
		auto_off_count_down_active = 1;
	}
}

#ifdef _USE_FD_READ_

static int autooff_fsfd;

void AutoOffFdRead (void *dummy)
{
	char buf[4];

	memset(buf, 0, 4);
	read(autooff_fsfd, buf, 4);
	lseek(atooff_fsfd)
	DBGMSG("Auto off fd read, val = %s, do poweroff!!!\n", buf);

	if (atoi(buf) == 1) {
		DBGMSG("Auto off rd read 1, do poweroff!!!\n");
		DesktopPowerOff();
	} else 
		DBGMSG("Auto off read 0,no poweroff!\n");
}

void RegisterAutoOffFd (void)
{
	autooff_fsfd = open(PLEXTALK_AUTO_OFF_ARRIVED, O_RDONLY);

	NeuxAppFDSourceRegister(autooff_fsfd, NULL, AutoOffFdRead, NULL);
	NxAppSourceActivate(autooff_fsfd, 1, 0);
}
#endif	//_USE_FD_READ_


static void initApp(void)
{
	extern WIDGET * mainApp;

	/* global config init */
	plextalk_global_config_init();
	DisplayOff = 0;				//ztz
	voice_prompt_init();
	InitOffTimer();
	resyncSetting();

	//ztz
	if (SystemInHostMode())
		plextalk_suspend_lock();
	//ztz

	//km
	//lowpowerTimer = NeuxTimerNew(mainApp, 1000*60, 0);
	//NeuxTimerSetCallback(lowpowerTimer, OnLowPowerTimer);
	//km
}

static void onDestroy(WID__ wid)
{
    DBGLOG("---------------window destroy %d.\n",wid);
    voice_prompt_uninit();
    NeuxAppFDSourceUnregister(signal_fd);
   	plextalk_global_config_close();
}


enum { SHOULD_ENABLE_COUNT_DOWN, SHOULD_DISABLE_COUNT_DOWN, SHOULD_IGNORE};

static int ShouldDoCountDown(int cur_app, SBAR_REQUEST type, int icon)
{
//	return ((cur_app == M_BOOK || cur_app == M_MUSIC) && (type = SRQST_SET_CONDITION_ICON) && 
//		(icon == SBAR_CONDITION_ICON_PAUSE || icon == SBAR_CONDITION_ICON_STOP
//		|| icon == SBAR_CONDITION_ICON_SELECT)) ;

	if (cur_app == M_BOOK || cur_app == M_MUSIC) {
		if (type == SRQST_SET_CONDITION_ICON) {
			if (icon== SBAR_CONDITION_ICON_PAUSE || icon == SBAR_CONDITION_ICON_STOP || icon == SBAR_CONDITION_ICON_SELECT)
				return SHOULD_ENABLE_COUNT_DOWN;
			else
				return SHOULD_DISABLE_COUNT_DOWN;
		} else {
			// no mater for count (for example: battery, ac_in, time_tick...)
			return SHOULD_IGNORE;
		}
	} else {
		//other app: ignore icon change...shoudle disable this
		//(for example, when in music, pressed menu key)
		return SHOULD_DISABLE_COUNT_DOWN;
	}
	
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	int countdown_mode;

	switch (msg->msg.msg.msgId)
	{
	case APP_MSG_TOPAPP_CHANGE:
		TopAppChange(*(int*)&msg->msg.msg.msgTxt, ((void*)&msg->msg.msg.msgTxt) + sizeof(int));
		break;
	case PLEXTALK_TITLEBAR_MSG_ID:
		TitleBarSetContent(msg->msg.msg.msgTxt);
		break;
	case PLEXTALK_STATUSBAR_MSG_ID:
		{
			sbar_rqst_t * req = (sbar_rqst_t*)msg->msg.msg.msgTxt;
			switch (req->type)
			{
			case SRQST_SET_CATEGORY_ICON:
				StatusBarSetIcon(STATUSBAR_CATEGORY_INDEX, req->set_icon.icon);
				countdown_mode = ShouldDoCountDown(active_app,req->type,req->set_icon.icon);
				switch (countdown_mode) {
					case SHOULD_ENABLE_COUNT_DOWN:
						AutoOffSetCountDown(1);
						break;
					case SHOULD_DISABLE_COUNT_DOWN:
						AutoOffSetCountDown(0);
						break;
					default:
						break;
				}
#if 0//自动关机有问题，先关掉
				//appk 直接依据当前ICON的消息来判断
				if(isNeedNoOfftimer(active_app,req->type,req->set_icon.icon)) {
					plextalk_set_offtimer_disable(1);
					if(isOffTimerSettingEnable())
						OffTimerSettingChange(0,-1,1);
						
				} else {
					plextalk_set_offtimer_disable(0);
					if(auto_off_flag && !isOffTimerSettingEnable()){
						auto_off_flag = 0;
						OffTimerSettingChange(1,g_config->auto_poweroff_time,1);
					}
				}
				//appk
#endif				
				
				break;
			case SRQST_SET_CONDITION_ICON:
				StatusBarSetIcon(STATUSBAR_CONDITION_INDEX, req->set_icon.icon);
				countdown_mode = ShouldDoCountDown(active_app,req->type,req->set_icon.icon);
				switch (countdown_mode) {
					case SHOULD_ENABLE_COUNT_DOWN:
						AutoOffSetCountDown(1);
						break;
					case SHOULD_DISABLE_COUNT_DOWN:
						AutoOffSetCountDown(0);
						break;
					default:
						break;
				}
#if 0//自动关机有问题，先关掉

				//appk 直接依据当前ICON的消息来判断
				if(isNeedNoOfftimer(active_app,req->type,req->set_icon.icon)) {
					plextalk_set_offtimer_disable(1);
					if(isOffTimerSettingEnable())
						OffTimerSettingChange(0,-1,1);

				} else {
					plextalk_set_offtimer_disable(0);
					if(auto_off_flag && !isOffTimerSettingEnable()){
						auto_off_flag = 0;
						OffTimerSettingChange(1,g_config->auto_poweroff_time,1);
					}

				}
				//appk
#endif

				break;
			case SRQST_SET_MEDIA_ICON:
				StatusBarSetIcon(STATUSBAR_MEDIA_INDEX, req->set_icon.icon);
				break;
			case SRQST_SET_TIME:
				HandleSetTimeMsg();
				break;
			}
			break;
		}
		break;

	case PLEXTALK_TIMER_MSG_ID:
		TimerSettingChange(*((int*)msg->msg.msg.msgTxt));
		break;
#if 0//comment by appk
	case PLEXTALK_OFFTIMER_MSG_ID:
		OffTimerSettingChange(*((int*)msg->msg.msg.msgTxt),0);
		break;
#endif

	case PLEXTALK_FILELIST_MSG_ID:
		HandleFilelistModeMsg(*((int*)msg->msg.msg.msgTxt));
		break;

	case PLEXTALK_BOOK_CONTENT_MSG_ID:
		HandleBookContentMsg(*((int*)msg->msg.msg.msgTxt));
		break;

	case PLEXTALK_MENU_GUIDE_VOL_MSG_ID:
		HandleGuideVolMenuMsg(*((int*)msg->msg.msg.msgTxt));
		break;

	case PLEXTALK_RECORD_ACTIVE_MSG_ID:
		HandleRecordingMsg(*((int*)msg->msg.msg.msgTxt));
		break;

	case PLEXTALK_RECORD_PAUSE_MSG_ID://appk
		HandleRecordingNowMsg(*((int*)msg->msg.msg.msgTxt));
		break;	

	case PLEXTALK_RADIO_RECORD_ACTIVE_MSG_ID:
		radio_record_active = *((int*)msg->msg.msg.msgTxt);
		break;

	case PLEXTALK_INFO_MSG_ID:
		HandleuInfoMsg(*((int*)msg->msg.msg.msgTxt));
		break;

	case PLEXTALK_APP_MSG_ID:
		{
			app_rqst_t *rqst = msg->msg.msg.msgTxt;
			switch (rqst->type) {
			case APPRQST_GUIDE_VOL_CHANGE:
				HandleGuideVolChangeMsg(rqst->guide_vol.volume);
				break;
			case APPRQST_GUIDE_SPEED_CHANGE:
				vprompt_cfg.speed = rqst->guide_speed.speed;
				break;
			case APPRQST_SET_RADIO_OUTPUT:
				HandleRadioOutputMsg(rqst->radio_output.output);
				break;
			case APPRQST_SET_FONTSIZE:
				break;
			case APPRQST_SET_THEME:
			//ztz
				if (rqst->theme.confirm) {
					CoolShmReadLock(g_config_lock);
					NeuxThemeInit(g_config->setting.lcd.theme);
					desktop_theme_index = g_config->setting.lcd.theme;
					CoolShmReadUnlock(g_config_lock);
				} else {
					NeuxThemeInit(rqst->theme.index);
					desktop_theme_index = rqst->theme.index;
				}
				RefreshFormMain(rqst->theme.confirm);
			//ztz
				break;
			case APPRQST_SET_LANGUAGE:
				plextalk_update_lang(rqst->language.lang, "desktop");
				plextalk_update_sys_font(rqst->language.lang);
				/*fall through*/
			case APPRQST_SET_TTS_VOICE_SPECIES:
			case APPRQST_SET_TTS_CHINESE_STANDARD:
				plextalk_update_tts(getenv("LANG"));
				break;
			case APPRQST_RESYNC_SETTINGS:
				resyncSetting();
				resyncTimerSetting();		//ztz
				RefreshFormMain(1);			//modify ztz
				InitVolume();
				break;
			}
		}
		break;
	}
}

static void
onSWOn(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		if (desktop_state == DESKTOP_STATE_OFF)
			return;

		if (active_app != M_DESKTOP) {
			NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
			usleep(SET_SDATE_DELAY);
		}
		voice_prompt_abort();
		last_msgid = voice_prompt_string2(1, &vprompt_cfg,
						ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
						_("Keylock on"));
		vprompt_change_state = 1;
		StatusBarSetIcon(STATUSBAR_KEYLOCK_INDEX, SBAR_KEYLOCK_ICON_LOCK);
		break;
	case SW_HP_INSERT:
		if (desktop_state != DESKTOP_STATE_OFF)
			DoVpromptUsbSound(1);		//added by ztz
		HpSwitch(1);
		break;
	}
}

static void
onSWOff(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		if (desktop_state == DESKTOP_STATE_OFF)
			return;

		if (active_app != M_DESKTOP) {
			NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
			usleep(SET_SDATE_DELAY);
		}
		voice_prompt_abort();
		last_msgid = voice_prompt_string2(1, &vprompt_cfg,
						ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
						_("Keylock off"));
		vprompt_change_state = 1;
		StatusBarSetIcon(STATUSBAR_KEYLOCK_INDEX, SBAR_KEYLOCK_ICON_NO);
		break;
	case SW_HP_INSERT:
		if (desktop_state != DESKTOP_STATE_OFF)	
			DoVpromptUsbSound(0);		//added by ztz
		HpSwitch(0);
		break;
	}
}

static void
OnSuspendTimer(WID__ wid)
{
	NeuxTimerSetEnabled(suspendTimer, FALSE);

	if (!plextalk_suspend_is_locked()) {
		/* specially handle countdown timer */
		TimerSuspendEnterHook();
		NhelperSuspend();
		TimerSuspendExitHook();

		/* reset screen saver */
		NeuxSsaverPauseResume(1);
		NeuxSsaverPauseResume(0);

		CoolShmReadLock(g_config_lock);
		if (g_config->setting.lcd.screensaver != PLEXTALK_SCREENSAVER_ACTIVENOW)
			NhelperDisplay(1);
		CoolShmReadUnlock(g_config_lock);
	}
}

static void
onScreenSaver(GR_BOOL activate)
{
	int screensaver;
	int auto_time;
	
	CoolShmReadLock(g_config_lock);
	screensaver = g_config->setting.lcd.screensaver;
	auto_time =  g_config->auto_poweroff_time;
	
	if (g_config->setting.lcd.screensaver != PLEXTALK_SCREENSAVER_ACTIVENOW)
	{
		if(screensaver != PLEXTALK_SCREENSAVER_DEACTIVE)
			NhelperDisplay(!activate);
	}

	CoolShmReadUnlock(g_config_lock);

#if 0 //自动关机换中方式
	auto_off_flag = 0;//不需要再重新设置

//appk start
	DBGMSG("onScreenSaver activate:%d,screensaver=%d,auto_time=%d,offtimer:%d\n",activate,screensaver,auto_time,plextalk_get_offtimer_disable());
	if(activate){
		if(!plextalk_get_offtimer_disable())
		{
			if(screensaver == PLEXTALK_SCREENSAVER_DEACTIVE || plextalk_screensaver_timeout[screensaver] >= auto_time*60) 
			{
				DesktopPowerOff();
			}
			else 
			{
				
				OffTimerSettingChange(1,auto_time - plextalk_screensaver_timeout[screensaver]/60,1);
			}
		}
		else
		{
			auto_off_flag = 1;//需要再重新设置
		}


	} else {
		if(isOffTimerSettingEnable())
			OffTimerSettingChange(0,-1,1);
	}
//appk end	

#endif

	/* we do delay suspend, avoid break voice prompt */
	if(screensaver != PLEXTALK_SCREENSAVER_DEACTIVE)
	{
		if (activate && !plextalk_suspend_is_locked()) {
			int auto_off_time;
			int suspend_time_r;
			
			CoolShmReadLock(g_config_lock);
			auto_off_time = g_config->auto_poweroff_time;	//defined as minutes
			CoolShmReadUnlock(g_config_lock);
			suspend_time_r = (auto_off_time + 1) * 60 * 1000;	//auto_off_time + 1 minutes after
			NeuxTimerSetControl(suspendTimer, suspend_time_r, 1);

			//NeuxTimerSetControl(suspendTimer, 1000*70, 1);//appk 1000*60->1000*70
		} else
			NeuxTimerSetEnabled(suspendTimer, FALSE);
	}
}

void
CreateSuspendTimer(WIDGET * owner)
{
	suspendTimer = NeuxTimerNew(owner, 0, 0);
	NeuxTimerSetCallback(suspendTimer, OnSuspendTimer);
}


void
DesktopSignalRead(void *dummy)
{
	struct signalfd_siginfo fdsi;
	ssize_t ret;

	ret = read(signal_fd, &fdsi, sizeof(fdsi));
	if (ret < 0) {
		WARNLOG("Read signal fd faild.\n");
		return;
	}

	DBGMSG("Voice prompt %d has terminated.\n", fdsi.ssi_int);
	desktop_tts_brunning = 0;//km

	if (last_msgid == fdsi.ssi_int) {
		if (desktop_state == DESKTOP_STATE_OFF) {
			TimerPoweroffHook();

//			if (HasAlarmTimerPending()) {
//				/* disable all keys except power key */
//				sysfs_write("/sys/device/platform/gpio-keys/disable-keys",
//							"28,59-62,103,105-106,108,114-115,139,358");
//				NhelperSuspend();
//				/* enable all keys */
//				sysfs_write("/sys/device/platform/gpio-keys/disable-keys", "");
//				/* check alarm */
//				// todo ....
//				/* if poweron wakeup, check key lock.
//				   if keypad is locked, resuspend again.
//				   else wait 2s, then go
//				*/
//			} else
				for (;;)
					NhelperPowerOff();
		}

		if (active_app != M_DESKTOP && !WmAppSerialIsLocked() && vprompt_change_state) {
			vprompt_change_state = 0;
			if (specical_for_info) {
				NhelperSetAppStateTwo(FUNC_MAIN_STR[active_app], 0, 1);
				specical_for_info = 0;
			} else
				NhelperSetAppState(FUNC_MAIN_STR[active_app], 0);
		}
	}
}

int
main(int argc,char **argv)
{
	signalInit();

	// create desktop as the WM/desktop
	if (NeuxWMAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	initApp();
	DBGMSG("application initialized\n");

	/* test */
	//TitleBarSetContent(NULL);

	NeuxWMAppSetCallback(CB_SSAVER,  onScreenSaver);
	NeuxWMAppSetCallback(CB_DESTROY, onDestroy);
	NeuxWMAppSetCallback(CB_MSG,     onAppMsg);
	NeuxWMAppSetCallback(CB_SW_ON,   onSWOn);
	NeuxWMAppSetCallback(CB_SW_OFF,  onSWOff);

	signal_fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(signal_fd, NULL, DesktopSignalRead, NULL);
	NxAppSourceActivate(signal_fd, 1, 0);

#ifdef _USE_FD_READ_

	RegisterAutoOffFd();	//ztz

#endif	//_USE_FD_READ_

	// the very first time, show main window.
	// once system is up, it always comes back to where
	// it was.

	CreateFormMain();
	DBGMSG("FormMain showed.\n");

	//this function must be run after create main form ,it need set schedule time
//	GetNextAlarmForStartRecord();

	DBGMSG("mainform shown\n");

	// start application, events starts to feed in.
	NeuxAppStart();

	return 0;
}
