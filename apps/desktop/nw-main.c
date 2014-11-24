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
 * Form Main routines.
 *
 * REVISION:
 *
 * 2) Main entry becomes the default startup window. ------ 2006-07-24 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#include <microwin/device.h>
#include <pthread.h>
#define OSD_DBG_MSG
#include "nc-err.h"
#include "nxutils.h"
#include "plextalk-theme.h"
#include "nw-titlebar.h"
#include "nw-statusbar.h"
#include "nw-main.h"
#include "plextalk-keys.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "desktop-ipc.h"
#include "plextalk-ui.h"
#include "rtc-helper.h"
#include "sysfs-helper.h"
#include "volume.h"
#include "key-value-pair.h"
#include "voltage.h"
#include "platform.h"
#include "application.h"
#include "plextalk-constant.h"
#include "application-ipc.h"

const char *FUNC_MAIN_STR[] = {
	[M_DESKTOP]         = PLEXTALK_IPC_NAME_DESKTOP,
	[M_BOOK]            = PLEXTALK_IPC_NAME_BOOK,
	[M_MUSIC]           = PLEXTALK_IPC_NAME_MUSIC,
	[M_RADIO]           = PLEXTALK_IPC_NAME_RADIO,
	[M_RECORD]          = PLEXTALK_IPC_NAME_RECORD,
	[M_MENU]            = PLEXTALK_IPC_NAME_MENU,
	[M_HELP]            = PLEXTALK_IPC_NAME_HELP,
	[M_FILE_MANAGEMENT] = PLEXTALK_IPC_NAME_FILE_MANAGEMENT,
	[M_CALCULATOR]      = PLEXTALK_IPC_NAME_CALCULATOR,
	[M_BACKUP]          = PLEXTALK_IPC_NAME_BACKUP,
	[M_ALARM]           = PLEXTALK_IPC_NAME_ALARM,
	[M_PC_CONNECTOR]	= PLEXTALK_IPC_NAME_PC_CONNECTOR,
	[M_FORMAT]			= PLEXTALK_IPC_NAME_FORMAT,
};


//ztz		
#define PC_DBG_MSG(fmt, arg...) do { \
	printf("[DESKTOP-->%s, %d]:"fmt, __func__, __LINE__, ##arg);\
} while (0)

static FORM *  formMain;
#define widget formMain

static int last_app;	// we only record "book, music and radio"
int active_app = M_DESKTOP;

//static TIMER *lowpowerTimer; //ztz

extern int vprompt_change_state;	//zzx

int desktop_state = DESKTOP_STATE_NULL;

TIMER *lowpowerTimer;
static int lowpower;

//added by ztz
static TIMER *WaitPoweroffTimer;
static int desktop_wait_poweroff;
//end

static TIMER *PowerOffForceTimer;

static int menu_is_running; 	//ztz

//added by km
static TIMER *voltageReadTimer;
//added by km
static TIMER *voltageSampleTimer;

extern int desktop_tts_brunning;//km
/*km 统一加到voltage.c中
//static int main_batt_voltage_min, main_batt_voltage_max;
//static int backup_batt_voltage_min, backup_batt_voltage_max;
*/
static int guide_vol_menu_active;

extern int auto_off_count_down_active;		//ztz: for auto power off

void
HandleGuideVolMenuMsg(int active)
{
	guide_vol_menu_active = active;
}
static void
doStopAllApp(void)
{
	NeuxStopAllApp(TRUE);

	TitleBarSetContent(NULL);
	StatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_NO);
	StatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	StatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
}


// << ztz : 开始，启动一个3s的定时器，3s之后，关机
static void 
ForcePowerOff (WID__ wid)
{
	printf("PowerOffForceTimer arrived, force do poweroff!!!\n");

	NhelperPowerOff();
}


//<< ztz: 开始，增加poweroff的延时处理函数
static void
OnWaitPoweroffTimer (WID__ wid)
{
	NeuxTimerSetEnabled(WaitPoweroffTimer, FALSE);
	NhelperPowerOff();
}


void
CreateWaitPowerOffTimer(WIDGET * owner)
{
	WaitPoweroffTimer = NeuxTimerNew(owner, 1000*2, -1);
	NeuxTimerSetCallback(WaitPoweroffTimer, OnWaitPoweroffTimer);
	NeuxTimerSetEnabled(WaitPoweroffTimer, FALSE);
}

void
DesktopDelayPowerOff (void)
{
	doStopAllApp();
	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Power off"));
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_POWER_OFF);

	TimerPoweroffHook();

	desktop_wait_poweroff = 1;
}
//<< ztz: 结束


void
DesktopPowerOff(void)
{
	printf("Desktop do power off!!\n");

	//it's necessary to restore xml file!!!
	plextalk_setting_write_xml(PLEXTALK_SETTING_DIR "setting.xml");

	NeuxTimerSetEnabled(PowerOffForceTimer, GR_TRUE);	

	desktop_state = DESKTOP_STATE_OFF;
	NeuxGotoDesktop();

	doStopAllApp();

	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Power off"));
	last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_POWER_OFF);
	vprompt_change_state = 1;
}

//km start
void
OnLowPowerTimer(WID__ wid)
{

	int sleeptime = 100000;	
	int timeout = 5000000/sleeptime;

	
	DBGMSG("send the mesage desktop_tts_brunning:%d\n",desktop_tts_brunning);
	NhelperLowPower();//send the low power off message
	
	if(widget) {

		while(timeout--) {//wait 5s for all other app stops
			if(NeuxWidgetWID(widget) == GrGetFocus()) {
				break;
			}
			usleep(sleeptime);		//usleep 0.1s
		}
		DBGMSG("I will power off now :%d\n",timeout);
	}
	//doStopAllApp();//appk
	//desktop_state = DESKTOP_STATE_OFF;//appk
	desktop_state = DESKTOP_STATE_OFF;//appk
	NeuxGotoDesktop();//appk
	doStopAllApp();//appk

	
	
	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Low battery. Power off"));
	last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_POWER_OFF);
	vprompt_change_state = 1;
}
//km end

//added by km start
static void EnableVolSampleTimer()
{
	//NeuxTimerSetControl(voltageSampleTimer,1000,SAVE_VOLTAGE_NUM);
	NeuxTimerSetEnabled(voltageSampleTimer, GR_TRUE);
}

static void DisableVolSampleTimer()
{
	NeuxTimerSetEnabled(voltageSampleTimer, GR_FALSE);
}


//定时开启一个秒定时器采集电量，采的次数为SAVE_VOLTAGE_NUM
static void
OnVolStartProcessTimer(WID__ wid)
{
	DBGMSG("OnVolStartProcessTimer\n");
	EnableVolSampleTimer();
}

//appk start
static int LowPowerVoicePromptDoneOrKeyDown(GR_EVENT *ep)
{
	int ret;
	ret =  (ep->type == GR_EVENT_TYPE_FD_ACTIVITY ||
		   ep->type == GR_EVENT_TYPE_KEY_DOWN);

	return (ret || (ep->type == GR_EVENT_TYPE_HOTPLUG && ep->hotplug.device == MWHOTPLUG_DEV_VBUS));
}

//appk end


//秒定时器，进行电量采集
//最后更新电量图标，判断是否低电
static void
OnSaveVolValueTimer(WID__ wid)
{
	static int cnt = 0;
	int interval;
	//DBGMSG("OnSaveVolValueTimer cnt:%d\n",cnt);
	PlextalkBatteryVoltagGet();
	cnt++;
	if(cnt>=SAVE_VOLTAGE_NUM) {
		interval = PlextalkGetVolSampleInterval();
		//DBGMSG("OnSaveVolValueTimer interval:%d\n",interval);
		if(interval) {
			NeuxTimerSetControl(voltageReadTimer,interval,1);
			NeuxTimerSetEnabled(voltageReadTimer, GR_TRUE);
		}
		PlextalkUpdateBatteryIcon();
		if(IsLowVoltageMusic()) {

			if (!lowpower) {
				if (active_app != M_DESKTOP) {
					NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
					printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
					usleep(1000);
				}
							
				voice_prompt_abort();
				last_msgid = voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Low battery.Power off in one minute."));//appk
				vprompt_change_state = 1;//appk	
				
				NeuxTimerSetEnabled(lowpowerTimer, GR_TRUE);
				//NhelperLowPower();//comment km
				lowpower = 1;

				NeuxWaitEvent(LowPowerVoicePromptDoneOrKeyDown);//km
			}
		}
	}
}

static void initLowpowerTimer()
{
	if(IsLowVoltagePowerOff()) {
		NhelperPowerOff();
	} else if (IsLowVoltageMusic()) {

		if (!lowpower) {
			if (active_app != M_DESKTOP) {
				NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
				printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
				usleep(1000);
			}
					
			voice_prompt_abort();
			last_msgid = voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Low battery.Power off in one minute."));//appk
			vprompt_change_state = 1;//appk	
			NeuxTimerSetEnabled(lowpowerTimer, GR_TRUE);
			//NhelperLowPower();//comment km
			lowpower = 1;

			NeuxWaitEvent(LowPowerVoicePromptDoneOrKeyDown);//km
		}		
	} else {
		NeuxTimerSetEnabled(lowpowerTimer, GR_FALSE);
		lowpower = 0;
	}

}


//added by km end

//zzx	--start--
static int
FindApp(const char *app)
{
	int i;
	
	for (i = 0; i < ARRAY_SIZE(FUNC_MAIN_STR); i++) {
		if (!strcmp(FUNC_MAIN_STR[i], app))
			return i;
	}

	return M_DESKTOP;
}

//ztz
static int NextChangeFreshTitleBar = 0;

//for a bug: it's bad idea!!!!
static int 
bad_conditon (void)
{
	int mb_on = (active_app == M_MUSIC || active_app == M_BOOK);
	int menu_on;

	CoolShmReadLock(g_config_lock);
	menu_on = g_config->menu_is_running; 
	CoolShmReadUnlock(g_config_lock);


	printf("!!!!!!!!!!!!!!!!DEBUG::active_app = %d\n", active_app);
	printf("!!!!!!!!!!!!!!!!DEBUG::mb_on = %d\n", mb_on);
	printf("!!!!!!!!!!!!!!!!DEBUG::menu_on = %d\n", menu_on);
	

	return mb_on && menu_on;
}

static int pc_connect_startup;
//ztz	
void
TopAppChange(int is_new, const char* app)
{
	PC_DBG_MSG("TopAppChange, app = %s\n", app);

	int new_active_app = FindApp(WmTopApp());

	if (NextChangeFreshTitleBar) {
		DBGMSG("NextChangeFreshTitleBar!!!\n");
		RefreshStatusBar(1);
		RefreshTitlebar(1);
	}

	if (!strcmp(app, PLEXTALK_IPC_NAME_PC_CONNECTOR)) {
		printf("Top App chage, app is pc-connector!!\n");
		NextChangeFreshTitleBar = 1;
	} else 
		NextChangeFreshTitleBar = 0;

	PC_DBG_MSG("new_active_app = %d\n", new_active_app);
	PC_DBG_MSG("active_app = %d\n", active_app);
	
	if (new_active_app != active_app) {
//		if (active_app != M_DESKTOP) {
//			NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
//printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
//			usleep(1000);
//		}

		PC_DBG_MSG("is_new = %d\n", is_new);

		if (is_new && (new_active_app == M_BOOK || new_active_app == M_MUSIC ||
		               new_active_app == M_RADIO || new_active_app == M_RECORD))
			guide_vol_menu_active = 0;

		active_app = new_active_app;
//		SetVolume(active_app);
		//if (!is_new)
		//	AppChangeVolumeHook(active_app, 0);
		AppChangeVolumeHook(active_app, is_new);
//		radio_record_active = 0;  //add by xlm

		if (active_app == M_DESKTOP) {
			DBGMSG("active_app == M_DESKTOP, so will set titlebar to NULL!\n");
			TitleBarSetContent(NULL);
			StatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_NO);
			StatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
			StatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
		}

//for a bug: bad idea!!!!
		if (!is_new && active_app != M_DESKTOP && !bad_conditon()) {
			NhelperSetAppState(FUNC_MAIN_STR[active_app], 0);
printf("set %s state: resume\n", FUNC_MAIN_STR[active_app]);
		}
		if (!is_new && active_app == M_DESKTOP) {
			if (desktop_state == DESKTOP_STATE_ALARM) {
				int udc_online = 0;
				sysfs_read_integer(PLEXTALK_UDC_ONLINE, &udc_online);
				if (udc_online) {
					plextalk_suspend_lock();
					AppLauncher(M_PC_CONNECTOR, 0, 0, NULL);
					PC_DBG_MSG("desktop_state chage to DESKTOP_STATE_PC_CONNECTOR!!!!!\n");
					desktop_state = DESKTOP_STATE_PC_CONNECTOR;
					return;
				}
			}

			CoolShmReadLock(g_config_lock);
			int menu_running = g_config->menu_is_running;
			CoolShmReadUnlock(g_config_lock);

			PC_DBG_MSG("menu_running = %d\n", menu_running);
			PC_DBG_MSG("pc_connect_startup = %d\n", pc_connect_startup);
			PC_DBG_MSG("desktop_state = %d\n", desktop_state);
			
			if (desktop_state < DESKTOP_STATE_FREE_RUN && !pc_connect_startup && !menu_running) {
				PC_DBG_MSG("launch last app!\n");
				PC_DBG_MSG("last_app = %d\n", last_app);
				
				/* start default of last app */
				AppLauncher(last_app, 0, 0, NULL);
				desktop_state = DESKTOP_STATE_FREE_RUN;
				PC_DBG_MSG("desktop_state chage to DESKTOP_STATE_FREE_RUN!!!!!\n");
			}
		}
	}

	if (!strcmp(app, PLEXTALK_IPC_NAME_PC_CONNECTOR))
		pc_connect_startup = 0;

	/* On Top App change, check if menu is running!! */
//	if (!NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_MENU))
//		menu_is_running = 0;
}
//zzx --end--

void
AppLauncher(int id, int exclude, int voice_prompt, const char *args)
{
	char path[PATH_MAX];

	if (exclude)
		doStopAllApp();
	else if (active_app != M_DESKTOP) {
		NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
		usleep(SET_SDATE_DELAY);
	}

	if (voice_prompt) {
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
	}

	if (args)
		snprintf(path, PATH_MAX, PLEXTALK_BINS_DIR "%s %s", FUNC_MAIN_STR[id], args);
	else
		snprintf(path, PATH_MAX, PLEXTALK_BINS_DIR "%s", FUNC_MAIN_STR[id]);
	NeuxWMAppRun(path);
	AppChangeVolumeHook(id, 1);

	if (id == M_BOOK || id == M_MUSIC || id == M_RADIO) {
    	last_app = id;
    	CoolSetCharValue(PLEXTALK_SETTING_DIR ".app.history", "app",
                     "last", FUNC_MAIN_STR[id]);
	}
}

static TIMER *key_timer;
static int long_pressed_key = -1;
static int menu_key_down_app;

static void
OnWindowKeydown(WID__ wid, KEY__ key)
{
	if (desktop_state == DESKTOP_STATE_OFF)
		return;

	if (long_pressed_key != -1)
		return;
	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
		}
		return;
	}
    if (!key.hotkey)
		return;
	if (WmAppSerialIsLocked() == 1)
		return;
//	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_ALARM))
//		return;

    switch (key.ch) {
    case MWKEY_POWER:
    	/* long pressed (2 seconds) to do poweroff */
    	long_pressed_key = MWKEY_POWER;
    	NeuxTimerSetControl(key_timer, 2000, 1);
    	break;

    case VKEY_BOOK:
		if (plextalk_schedule_is_locked())
			return;
    	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_BOOK))
    		return;
    	AppLauncher(M_BOOK, 1, 1, NULL);
    	break;

    case VKEY_MUSIC:
		if (plextalk_schedule_is_locked())
			return;
		if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_MUSIC))
    		return;
    	AppLauncher(M_MUSIC, 1, 1, NULL);
    	break;

    case VKEY_RADIO:
		if (plextalk_schedule_is_locked())
			return;
		if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_RADIO))
    		return;
    	AppLauncher(M_RADIO, 1, 1, NULL);
    	break;

    case VKEY_RECORD:
		if (plextalk_schedule_is_locked())
			return;
    	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_RECORD) ||
    		NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_RADIO))
    		return;
    	AppLauncher(M_RECORD, 0, 1, NULL);
    	break;

    case MWKEY_MENU:
    	if (active_app == M_PC_CONNECTOR || info_running || active_app == M_DESKTOP)     //add xlm
    		return;
    	else if (active_app == M_RECORD || (active_app == M_RADIO && radio_record_active)) {
			if (recording_active) {
    			voice_prompt_abort();
				last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_BU);
				vprompt_change_state = 1;
    		} else {
    		
	    		CoolShmWriteLock(g_config_lock);
				g_config->menu_is_running = 1;
				DBGMSG("set g_config->menu_is_running to 1!\n");
				CoolShmWriteUnlock(g_config_lock);

     			AppLauncher(M_MENU, 0, 1, FUNC_MAIN_STR[active_app + (active_app == M_RADIO && !!radio_record_active)]);
				menu_key_down_app = M_MENU;
    		}
			return;
    	} else if (!plextalk_schedule_is_locked())
			menu_key_down_app = active_app;
		else
			menu_key_down_app = M_MENU;
//		if (!plextalk_get_help_disable()) {
			/* long pressed to run help */
			long_pressed_key = MWKEY_MENU;
    		NeuxTimerSetControl(key_timer, 500, 1);
//    	} else if (plextalk_get_menu_disable()) {
//    		AppLauncher(M_MENU, 0, 1, FUNC_MAIN_STR[active_app + (active_app == M_RADIO && !!radio_record_active)]);
//    	} else {
//    		voice_prompt_abort();
//			last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_BU);
//			vprompt_change_state = 1;
//		}
		break;

    case MWKEY_VOLUME_UP:
    case MWKEY_VOLUME_DOWN:
		if (!guide_vol_menu_active) {
			if (HandleVolumeKey(key.ch == MWKEY_VOLUME_UP)) {
				long_pressed_key = key.ch;
    			NeuxTimerSetControl(key_timer, 300, -1);
    		}
		} else {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		}
    	break;
	}
}

static void
OnWindowKeyup(WID__ wid, KEY__ key)
{
	if (desktop_state == DESKTOP_STATE_OFF)
		return;

	if (!key.hotkey)
		return;
	if (long_pressed_key != key.ch)
		return;

	long_pressed_key = -1;
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch) {
	case MWKEY_POWER:
		if(active_app != M_HELP){/*lhf for help prompt*/
			voice_prompt_abort();
			last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_BU);
			vprompt_change_state = 1;
		}
		break;
	case MWKEY_MENU:
		//ztz --start--
		DBGMSG("plextalk_get_menu_disable() = %d\n", plextalk_get_menu_disable());
		if ((menu_key_down_app != M_MENU) && (!plextalk_get_menu_disable())) {
			CoolShmWriteLock(g_config_lock);
			g_config->menu_is_running = 1;
			DBGMSG("set g_config->menu_is_running to 1!\n");
			CoolShmWriteUnlock(g_config_lock);

			AppLauncher(M_MENU, 0, 1,
    					active_app >= 0 ? FUNC_MAIN_STR[active_app + (active_app == M_RADIO && !!radio_record_active)] : NULL);
		} //ztz --end--
		break;
	}
}

static void
OnKeyTimer(WID__ wid)
{
	int key = long_pressed_key;
	long_pressed_key = -1;
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key) {
    case MWKEY_VOLUME_UP:
    case MWKEY_VOLUME_DOWN:
    	if (HandleVolumeKey(key == MWKEY_VOLUME_UP)) {
    		NeuxTimerSetEnabled(key_timer, GR_TRUE);
			long_pressed_key = key;
		}
		break;
	case MWKEY_POWER:
		DesktopPowerOff();
		break;
	case MWKEY_MENU:
		/* long pressed to run help */
		AppLauncher(M_HELP, 0, 1, NULL);
		break;
	}
}

// km 换成voltage.c中统一的接口 -----start-------
#if 0
static void
UpdateBattery(void)
{
	int main_batt_present = 0, backup_batt_present = 0;
	static int last_icon = SBAR_BATTERY_ICON_NO;
	int icon = SBAR_BATTERY_ICON_ACIN;

	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);

	if (main_batt_present | backup_batt_present) {
		int vbus_present = 0;
//		int ac_present = 0;

		sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);
//		sysfs_read_integer(PLEXTALK_AC_PRESENT, &ac_present);

		if (vbus_present /*| ac_present*/) {
			if (main_batt_present) {
				char status[32];
				sysfs_read(PLEXTALK_MAIN_BATT_STATUS, status, sizeof(status));
				if (!strcmp(status, "Charging"))
					icon = SBAR_BATTERY_ICON_CHARGING;
			}
		} else {
			int batt_voltage_now;
			int batt_voltage_min, batt_voltage_max;

			if (!backup_batt_present) {
				sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_NOW, &batt_voltage_now);
				batt_voltage_min = main_batt_voltage_min;
				batt_voltage_max = main_batt_voltage_max;
			} else {
				sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_NOW, &batt_voltage_now);
				batt_voltage_min = backup_batt_voltage_min;
				batt_voltage_max = backup_batt_voltage_max;
			}

			icon = (batt_voltage_now - batt_voltage_min)
					/ ((batt_voltage_max - batt_voltage_min) / 5);
			if (icon < 0)
				icon = 0;
			if (icon > 4)
				icon = 4;
			icon += 1;

			if ((batt_voltage_now - batt_voltage_min) * 100 < (batt_voltage_max - batt_voltage_min) * 5) {
				if (active_app != M_DESKTOP) {
					NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
					printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
					usleep(1000);
				}
				voice_prompt_abort();
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Low battery. Power off in one minute."));
				if (!lowpower) {
					NeuxTimerSetEnabled(lowpowerTimer, GR_TRUE);
					NhelperLowPower();
					lowpower = 1;
				}
			}
		}
	}

	if (last_icon != icon) {
		StatusBarSetIcon(STATUSBAR_BATTERY_INDEX, icon);
		last_icon = icon;
	}
}
#endif

//km  换成统一的接口-----end------


//ztz ---start----
void DoVpromptUsbSound (int on)
{
	if(plextalk_get_hotplug_voice_disable()) {//added km
		DBGMSG("hotplug voice disable\n");
		return;
	}
	if (active_app != M_DESKTOP) {
		NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
		//usleep(1000);		//not enough!!!
		usleep(SET_SDATE_DELAY);		//delay 0.2s should be OK
	}
	
	DBGMSG("Do Vprompt USB sound!!!!\n");
	voice_prompt_abort();
	last_msgid = voice_prompt_music(1, &vprompt_cfg, 
		on ? VPROMPT_AUDIO_USB_ON : VPROMPT_AUDIO_USB_OFF);
	vprompt_change_state = 1;
}
//ztz ----end----


//ztz	--start--
static CheckScreenSaver (int device)
{
	//
	if (device == MWHOTPLUG_DEV_BATTERY)
		return;

	if ((DisplayOff) && (g_config->setting.lcd.screensaver != 0)) {
		NhelperDisplay(1);
		NeuxWMAppSetScreenSaverTime(plextalk_screensaver_timeout[g_config->setting.lcd.screensaver]);
		if (auto_off_count_down_active)
			AutoOffSetCountDown(1);
	}
}
//ztz 	--end --


//add by appk start
static int VoicePromptDoneOrKeyDown(GR_EVENT *ep);
static void prepare_pc_connector()
{
	if (active_app == M_RECORD || (active_app == M_RADIO && radio_record_active)){

		plextalk_set_hotplug_voice_disable(FALSE);
		plextalk_set_SD_hotplug_voice_disable(FALSE);
	
		voice_prompt_abort();
		voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
									tts_lang, tts_role, _("close the recording mode"));
		NeuxWaitEvent(VoicePromptDoneOrKeyDown);
		
	}
}
//add by appk end

static void HotplugScsiAction (int index, int action)
{
	DBGMSG("Hot plug scsi --- index = %d, --- action = %d\n", index, action);

	static int ignore_change = 1;
			
	if (action == MWHOTPLUG_ACTION_ADD) {
		DoVpromptUsbSound(1);
		plextalk_suspend_lock();
	} else if (action == MWHOTPLUG_ACTION_REMOVE){
		ignore_change = 1;
		DoVpromptUsbSound(0);
		plextalk_suspend_unlock();
	} else if (action == MWHOTPLUG_ACTION_CHANGE) {
		if (CoolCdromMediaPresent() > 0) {
			ignore_change = 0;
			DoVpromptUsbSound(1);
		} else {
			if (!ignore_change)
				DoVpromptUsbSound(0);
			ignore_change = 1;
		}
	}
}

static void
OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	if (desktop_state == DESKTOP_STATE_OFF)
		return;

	CheckScreenSaver(device);	//ztz

	PC_DBG_MSG(" hotplug!\n");

	switch (device) {
	case MWHOTPLUG_DEV_VBUS:
		PC_DBG_MSG("hotPlug: VBUS!\n");
//	case MWHOTPLUG_DEV_ACIN:
		//added by km --start
		{
			int vbus_present;
			sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);
			if(!vbus_present) {
				DoVpromptUsbSound(0);		//added by ztz
				PlextalkInitBattVol();
				PlextalkBatteryVoltagGet();
				NeuxTimerSetControl(voltageReadTimer,200,1);
				NeuxTimerSetEnabled(voltageReadTimer, GR_TRUE);
			}
			//added by km --end
			//if (action == MWHOTPLUG_ACTION_ADD) 
			else//modified by km
			{
				DoVpromptUsbSound(1);		//added by ztz
				NeuxTimerSetEnabled(lowpowerTimer, GR_FALSE);
				lowpower = 0;
				//added by km
				NeuxTimerSetEnabled(voltageReadTimer, GR_FALSE);
			}
		}
	case MWHOTPLUG_DEV_BATTERY:
		PC_DBG_MSG("hotPlug: Battery!\n");
		//deleted by km
		//UpdateBattery();
		//added by km
		PlextalkBatteryVoltagGet();
		//added by km
		PlextalkUpdateBatteryIcon();

		AppChangeVolumeHook(active_app,0);
		break;

	//ztz --start--
	case MWHOTPLUG_DEV_MMCBLK:
		PC_DBG_MSG("hotPlug: MMCBLK!\n");
		if(!plextalk_get_SD_hotplug_voice_disable())//add appk
			(action == MWHOTPLUG_ACTION_REMOVE) ? DoVpromptUsbSound(0) : DoVpromptUsbSound(1);
		break;
	//ztz --end--

	case MWHOTPLUG_DEV_SCSI_DISK:
	case MWHOTPLUG_DEV_SCSI_CDROM:
		/* we cann't enter suspend mode when OTG in host mode */
		HotplugScsiAction(index, action);
		break;

	case MWHOTPLUG_DEV_UDC:
		PC_DBG_MSG("hotplug : UDC!\n");
		if (action == MWHOTPLUG_ACTION_ONLINE) {
			plextalk_suspend_lock();
			prepare_pc_connector();//add by appk
			pc_connect_startup = 1;
			PC_DBG_MSG("start to launch pc-connector!\n");
			if (GetInfoStat()) {
				plextalk_schedule_unlock();
				SetInfoStat(0);
			}
			AppLauncher(M_PC_CONNECTOR, 1, 0, active_app >= 0 ? FUNC_MAIN_STR[active_app] : NULL);//appk 0->1
			//if (desktop_state == DESKTOP_STATE_ALARM) {
			desktop_state = DESKTOP_STATE_PC_CONNECTOR;
			PC_DBG_MSG("desktop_state chage to DESKTOP_STATE_PC_CONNECTOR!!!!!\n");
			//}
		} else {
			plextalk_suspend_unlock();
		}
		break;
	}
}

//<< ztz: 在关机状态下，focus_in时不起app
static void
OnWindowGetFocus(WID__ wid)
{
	DBGMSG("Desktop window get focus!\n");

	CoolShmReadLock(g_config_lock);
	int menu_running = g_config->menu_is_running;
	DBGMSG("menu_running = %d\n", menu_running);
	CoolShmReadUnlock(g_config_lock);

	if (desktop_state == DESKTOP_STATE_FREE_RUN && (!pc_connect_startup) 
		&& !menu_running) {
		DBGMSG("Window Get Focus, launcher last_app = %d\n", last_app);
		AppLauncher(last_app, 0, 0, NULL);
	}
}
//<< ztz: end 

static void
OnWindowDestroy(WID__ wid)
{
    DBGLOG("---------------window destroy %d.\n",wid);
    widget = NULL;
}

static void InitStatusBar()
{
	char buf[16];
	int res;

	//deleted by km start
#if 0	
	sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MIN_DESIGN, &main_batt_voltage_min);
	sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MAX_DESIGN, &main_batt_voltage_max);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MIN_DESIGN, &backup_batt_voltage_min);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MAX_DESIGN, &backup_batt_voltage_max);

	UpdateBattery();
#endif
	//deleted by km end
	//added by km start
	PlextalkBatteryVoltagGet();
	PlextalkUpdateBatteryIcon();
	//added by km end
	

//	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
//	if (res > 0 && !strcmp(buf, "0"))
	if (NeuxAppGetKeyLock(0))
		StatusBarSetIcon(STATUSBAR_KEYLOCK_INDEX, SBAR_KEYLOCK_ICON_LOCK);
}

static void
InitForm(void)
{
    NeuxWidgetGrabKey(widget, MWKEY_POWER,       NX_GRAB_HOTKEY);

    NeuxWidgetGrabKey(widget, VKEY_BOOK,         NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_MUSIC,        NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_RADIO,        NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_RECORD,       NX_GRAB_HOTKEY);

    NeuxWidgetGrabKey(widget, MWKEY_VOLUME_UP,   NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, MWKEY_VOLUME_DOWN, NX_GRAB_HOTKEY);

    NeuxWidgetGrabKey(widget, MWKEY_MENU,        NX_GRAB_HOTKEY);

   	key_timer = NeuxTimerNew(widget, 500, 0);
	NeuxTimerSetCallback(key_timer, OnKeyTimer);

	//modified by km  0->-1
	lowpowerTimer = NeuxTimerNew(widget, 1000*60, -1);
	//lowpowerTimer = NeuxTimerNew(widget, 1000*60, 0);
	NeuxTimerSetCallback(lowpowerTimer, OnLowPowerTimer);

	//added by km start	
	int interval = PlextalkGetVolSampleInterval();
	voltageReadTimer = NeuxTimerNew(widget, interval, 1);
	NeuxTimerSetCallback(voltageReadTimer, OnVolStartProcessTimer);
	if(interval)
		NeuxTimerSetEnabled(voltageReadTimer, GR_TRUE);
	else
		NeuxTimerSetEnabled(voltageReadTimer, GR_FALSE);

	voltageSampleTimer = NeuxTimerNew(widget, 1000, SAVE_VOLTAGE_NUM);
	NeuxTimerSetCallback(voltageSampleTimer, OnSaveVolValueTimer);
	NeuxTimerSetEnabled(voltageSampleTimer, GR_FALSE);		
	//added by km end

	// added by ztz
	// force do poweroff after 3s
	PowerOffForceTimer = NeuxTimerNew(widget, DELAY_SECOND * 3, -1);
	NeuxTimerSetCallback(PowerOffForceTimer, ForcePowerOff);
	NeuxTimerSetEnabled(PowerOffForceTimer, GR_FALSE);
	// added by ztz
	
}

static void
InitLastApp(void)
{
	char last_app_name[MAX_MSG_APP_NAME_LEN];
	int ret;

	ret = CoolGetCharValue(PLEXTALK_SETTING_DIR ".app.history", "app",
                     "last", last_app_name, MAX_MSG_APP_NAME_LEN);
    if (ret == COOL_INI_OK) {
    	if (!strcmp(last_app_name, FUNC_MAIN_STR[M_BOOK]))
    		last_app = M_BOOK;
    	else if (!strcmp(last_app_name, FUNC_MAIN_STR[M_MUSIC]))
    		last_app = M_MUSIC;
    	else if (!strcmp(last_app_name, FUNC_MAIN_STR[M_RADIO]))
    		last_app = M_RADIO;
	}
	if (last_app == 0)
		last_app = M_BOOK;
}


//modify by ztz		--start-----
void
RefreshFormMain(int confirm)
{
    widget_prop_t wprop;

	printf("DEBUG: refresh form main, confirm = %d\n", confirm);


	if (confirm) {
		NeuxWidgetGetProp(widget, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(widget, &wprop);
	
		GrClearArea(NeuxWidgetWID(widget), 0, 0,
				STATUSBAR_WIDTH, STATUSBAR_HEIGHT + TITLEBAR_HEIGHT, GR_FALSE);

		//GrClearWindow(NeuxWidgetWID(widget), GR_FALSE);
		//GrFillRect(NeuxWidgetWID(widget), NeuxWidgetGC(widget), 0, 0, 
		//	NeuxScreenWidth(), NeuxScreenHeight());
	}
	
	RefreshStatusBar(confirm);
	RefreshTitlebar(confirm);	
}
//modify by ztz		----end


// start of public APIs.

static int VoicePromptDoneOrKeyDown(GR_EVENT *ep)
{
	return ep->type == GR_EVENT_TYPE_FD_ACTIVITY ||
	       ep->type == GR_EVENT_TYPE_KEY_DOWN ||
	       ep->type == GR_EVENT_TYPE_SW_ON ||
	       ep->type == GR_EVENT_TYPE_SW_OFF;
}

#define _USE_TIMER_AUTO_OFF_
#ifdef _USE_TIMER_AUTO_OFF_

static TIMER* checkAutoOffTimer;

static void
OnCheckAutoOffTimer (WID__ wid)
{
	int val;

	sysfs_read_integer(PLEXTALK_AUTO_OFF_ARRIVED, &val);

	if (val) {
		if (plextalk_get_auto_off_disable()) {
			DBGMSG("On check timer bu auto off was disabled!,reset autooff timer!!!\n");

			int auto_off_timer;

			CoolShmReadLock(g_config_lock);
			auto_off_timer = g_config->auto_poweroff_time * 60;
			CoolShmReadUnlock(g_config_lock);
		
			sysfs_write_integer(PLEXTALK_AUTO_OFF_SYS_FILE, auto_off_timer);
			auto_off_count_down_active = 1;
		} else {
			DBGMSG("On check timer of auto off, do poweroff!!!\n");
			NeuxTimerSetEnabled(checkAutoOffTimer, 0);		//disable timer
			DesktopPowerOff();
		}
	}
}
#endif	//_USE_TIMER_AUTO_OFF_

/* Function creates the main form of the application. */
void
CreateFormMain (void)
{
	int i;
    widget_prop_t wprop;
    int udc_online = 0;

    widget = NeuxFormNew(0, 0, NeuxScreenWidth(), NeuxScreenHeight());

	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(widget, &wprop);

	// create, titlebar, status bar
	CreateSuspendTimer(widget);
	CreateStatusBar(widget);
	CreateTitleBar(widget);
	PlextalkInitBattVol();//added by km

	NeuxFormSetCallback(widget, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,   OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_HOTPLUG,  OnWindowHotplug);
	NeuxFormSetCallback(widget, CB_FOCUS_IN, OnWindowGetFocus);

//	//DBGMSG("menuTimeout = %d\n", osdsettings.general.menuTimeout);
//	scheduleTimer = NeuxTimerNewWithCallback(widget, DELAY_SECOND*30, -1,
//                                      OnScheduleTimer);
//	NeuxTimerSetEnabled(scheduleTimer, FALSE);

	InitForm();

    NeuxFormShow(widget, TRUE);
    NeuxWidgetFocus(widget);
	
	InitStatusBar();//appk
	InitVolume();

	//InitStatusBar();//appk

#ifdef _USE_TIMER_AUTO_OFF_

	checkAutoOffTimer = NeuxTimerNew(widget, DELAY_SECOND * 5, -1);		//5s then flush the clock label
	NeuxTimerSetCallback(checkAutoOffTimer, OnCheckAutoOffTimer);
#endif	//_USE_TIMER_AUTO_OFF_
	
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_POWER_ON);
	last_msgid = voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
									tts_lang, tts_role, _("Power On"));
	vprompt_change_state = 1;

	NeuxWaitEvent(VoicePromptDoneOrKeyDown);


	InitLastApp();
	InitTimer(widget);
	//added by km start
	initLowpowerTimer();
	//added by km end

	sysfs_read_integer(PLEXTALK_UDC_ONLINE, &udc_online);
	if (udc_online) {
		plextalk_suspend_lock();
		if (desktop_state == DESKTOP_STATE_NULL) {
			AppLauncher(M_PC_CONNECTOR, 0, 0, NULL);
			desktop_state = DESKTOP_STATE_PC_CONNECTOR;
			PC_DBG_MSG("desktop_state chage to DESKTOP_STATE_PC_CONNECTOR!!!!!\n");

		}
	}

	if (desktop_state == DESKTOP_STATE_NULL) {
		/* start default of last app */
		AppLauncher(last_app, 0, 0, NULL);
		desktop_state = DESKTOP_STATE_FREE_RUN;
		PC_DBG_MSG("desktop_state chage to DESKTOP_STATE_FREE_RUN!!!!!\n");
	}
}
