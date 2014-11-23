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
#include "timer-comfirm.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "Nw-menu.h"
#include "nxutils.h"
#include "Nw-menu.h"


typedef struct{
	FORM* form;
	TIMER* timer;
	struct MultilineList *list;
	
	int   ok;
	int   timeNo;
}MODAL_INFO;

#define FORMAT_12H 0
#define FORMAT_24H 1

#define TIMER_PERIOD       500

MODAL_INFO modal_info;

static int endOfModal = -1;
static char timer_name[100];
static char timer_state[100];
static char timer_function[100];
static char alarm_sound_name[100];
static char set_time_info[100];
static char set_time_info1[100];
static char repeat_information[100];

static TIMER *key_timer;
static int long_pressed_key = -1;

static int hour24To12(int hour,int *ampm)
{
	 *ampm = (hour>=12);
	 if (hour == 0){
	 	hour = 12;
	 } else if(hour > 12) {
		 hour -= 12;
	 }
	 return hour;
}
static int hour12To24(int hour,int ampm)
{
	if(hour == 12){
		if(ampm)
			return 12;
		else
			return 0;
	}

	if(ampm){
		hour += 12;
	}
	return hour;
}

static void OnWindowKeydown(WID__ wid, KEY__ key)
{
	char *curtext = NULL;
	DBGMSG("key.ch = %d\n", key.ch);
	if (menu_exit) {
		DBGMSG("stop\n");
		voice_prompt_abort();
		NeuxAppStop();
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Keylock"));
		return;
	}

	switch (key.ch)
	{
	case MWKEY_DOWN:
		{
			//TimerDisable(modal_info.timer);
			voice_prompt_abort();
			voice_prompt_end = 1;

			mullistNextItem(modal_info.list);
			curtext = getItemContent(modal_info.list);
			TimerEnable(modal_info.timer);

			DBGMSG("curtext = %s", curtext);
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string(1, &vprompt_cfg,
				ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
				curtext, strlen(curtext));
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_UP:
		{
			//TimerDisable(modal_info.timer);
			voice_prompt_abort();
			voice_prompt_end = 1;

			mullistPrevItem(modal_info.list);
			curtext = getItemContent(modal_info.list);
			TimerEnable(modal_info.timer);
			
			DBGMSG("curtext = %s", curtext);
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string(1, &vprompt_cfg,
				ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
				curtext, strlen(curtext));
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_ENTER:
	case MWKEY_RIGHT:
		DBGMSG("confirm, ok!\n");
		TimerDisable(modal_info.timer);
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Enter"));
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Set"));
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
		
		if (timer.enabled &&
			timer.function == PLEXTALK_TIMER_ALARM &&
			timer.type == PLEXTALK_TIMER_CLOCKTIME &&
			timer.repeat == PLEXTALK_ALARM_REPEAT_WEEKLY &&
			!(timer.weekday & 0x7f)) {
			/* alarm's repeat setting is not validity */
			timer.enabled = 0;
		}
		
		int change = memcmp(&setting.timer[timer_nr], &timer, sizeof(timer)) != 0;
		if (change) {
			CoolShmWriteLock(g_config_lock);
			memcpy(&g_config->setting.timer[timer_nr], &timer, sizeof(timer));
			CoolShmWriteUnlock(g_config_lock);
			NhelperReschduleTimer(timer_nr);
		}
		MenuExit(change);
		break;
	case MWKEY_LEFT:
		DBGMSG("confirm, cancel!");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Cancel."));
		modal_info.ok = 0;
		endOfModal = 1;
		break;
#if 0	
	case MWKEY_MENU:
		DBGMSG("MENU down\n");
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;

	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
#endif

	}
}

static void OnWindowKeyup(WID__ wid, KEY__ key)
{
#if 0
	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		return;
	}

	if (long_pressed_key != key.ch)
		return;

	long_pressed_key = -1;
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch)
	{
	case MWKEY_MENU:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext("Cancel."));
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext("Close the timer setting menu."));
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
		MenuExit(0);
		break;

	default:
		break;
	}
#endif
}

static void OnWindowTimer(WID__ wid)
{
	char *curtext = NULL;
	if(menu_exit){
		return;
	}
	
	if(voice_prompt_end){
		DBGMSG("tips end\n");
		curtext = getMoreContent(modal_info.list);
		if(!curtext){
			mullistNextItem(modal_info.list);
			if(mullistIsLastItem(modal_info.list)){
				DBGMSG("read end\n");
				TimerDisable(modal_info.timer);
				return;
			}
			curtext = getItemContent(modal_info.list);
		}
		
		voice_prompt_string(1, &vprompt_cfg,
			ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
			curtext, strlen(curtext));
		voice_prompt_end = 0;
	}
}


static void OnWindowRedraw(WID__ wid)
{
	if(modal_info.list)
	{
		multiline_list_show(modal_info.list);
	}
}

static void OnWindowGetFocus(WID__ wid)
{
	
}

static void OnWindowDestroy(WID__ wid)
{
	DBGMSG("window destroy\n");
	if(modal_info.list)
	{
		multiline_list_destroy(modal_info.list);
		modal_info.list = NULL;
	}
	DBGMSG("window destroy end\n");
}

static void OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}

static void CreateFormMain(void)
{
    widget_prop_t wprop;

	/* Create a New window */
	modal_info.form = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	modal_info.list = multiline_list_create(modal_info.form, NeuxWidgetWID(modal_info.form), 0, 0, MAINWIN_WIDTH, MAINWIN_HEIGHT, 
		theme_cache.foreground, theme_cache.background);

	multiline_list_set_loop(modal_info.list);

	/* Create timer */
   	modal_info.timer = NeuxTimerNew(modal_info.form, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(modal_info.timer, OnWindowTimer);
	TimerEnable(modal_info.timer);

	key_timer = NeuxTimerNewWithCallback(modal_info.form, 0, 0, OnKeyTimer);
	/* Set callback for window events */	
	NeuxFormSetCallback(modal_info.form, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(modal_info.form, CB_KEY_UP, OnWindowKeyup);
	NeuxFormSetCallback(modal_info.form, CB_EXPOSURE, OnWindowRedraw);
	NeuxFormSetCallback(modal_info.form, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(modal_info.form, CB_FOCUS_IN, OnWindowGetFocus);

	/* Set proproty(color)*/
	NeuxWidgetGetProp(modal_info.form, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(modal_info.form, &wprop);

}

static void
InitComfirm(void)
{

	modal_info.ok = -1;
	modal_info.timeNo = timer_nr;
}

void confirm_add_item()
{
	char *text;
	char *on_off_str[2];
	on_off_str[0] = _("Off");
	on_off_str[1] = _("On");
	char *timer_func[2];
	timer_func[1] = _("Sleep");
	timer_func[0] = _("Alarm");
	char *alrm_sound[3];
	alrm_sound[0] = _("A");
	alrm_sound[1] = _("B");
	alrm_sound[2] = _("C");
	int timeformat = FORMAT_24H;
	char *ampm_info[2];
	ampm_info[0] = _("AM");
	ampm_info[1] = _("PM");
	char *repeat_info[2];
	repeat_info[0] = _("One time");
	repeat_info[1] = _("Everyday");

	char *pre_timer = _("Timer name");
	char *pre_timer_str_name = _("Timer");

	CoolShmReadLock(g_config_lock);
	timeformat = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);

	snprintf(timer_name, 100, "%s:%s%d", pre_timer, pre_timer_str_name,
		modal_info.timeNo+1);
	text = timer_name;
	multiline_list_add_item(modal_info.list, timer_name);

	snprintf(timer_state, 100, "%s:%s", pre_timer_str_name,
	on_off_str[timer.enabled]);
	text = timer_state;
	multiline_list_add_item(modal_info.list, timer_state);

	if(!timer.enabled){
		return;
	}
	
	char *pre_timer_function = _("Timer function");
	snprintf(timer_function, 100, "%s:%s", pre_timer_function,
	timer_func[timer.function]);
	text = timer_function;
	multiline_list_add_item(modal_info.list, timer_function);


	char *clocktime = _("Clock time");
	char *str_timer = _("Timer");
	char *pre_minute = _("minutes");
	char *pre_repeat_info = _("Repeat");
	if(!timer.function){
		char *pre_alarm_sound = _("Alarm sound");
		snprintf(alarm_sound_name, 100, "%s:%s", pre_alarm_sound,
			alrm_sound[timer.sound]);
		text = alarm_sound_name;
		multiline_list_add_item(modal_info.list, alarm_sound_name);
	}
	if(timer.type == PLEXTALK_TIMER_COUNTDOWN){
		int minutes = 0;
		switch(timer.elapse)
		{
		case PLEXTALK_SLEEP_3MIN:
			minutes = 3;
			break;
				case PLEXTALK_SLEEP_5MIN:
			minutes = 5;
			break;
				case PLEXTALK_SLEEP_15MIN:
			minutes = 15;
			break;
				case PLEXTALK_SLEEP_30MIN:
			minutes = 30;
			break;
				case PLEXTALK_SLEEP_45MIN:
			minutes = 45;
			break;
				case PLEXTALK_SLEEP_60MIN:
			minutes = 60;
			break;
				case PLEXTALK_SLEEP_90MIN:
			minutes = 90;
			break;
				case PLEXTALK_SLEEP_120MIN:
			minutes = 120;
			break;
		}
		snprintf(set_time_info, 100, "%s:%d %s", str_timer,
			minutes, pre_minute);
		text = set_time_info;
		multiline_list_add_item(modal_info.list, set_time_info);
	}
	else{
		char *repeatinfo;
		if(timeformat == FORMAT_12H)
		{
			int ampm;
			int hour = hour24To12(timer.hour,&ampm);
			snprintf(set_time_info1, 100, "%s: %.2d:%.2d %s", clocktime,
				hour, timer.minute, ampm_info[ampm]);	
		}else{
			snprintf(set_time_info1, 100, "%s: %.2d:%.2d", clocktime,
				timer.hour, timer.minute);
		}
		text = set_time_info1;
		multiline_list_add_item(modal_info.list, set_time_info1);

		if(PLEXTALK_ALARM_REPEAT_ONCE == timer.repeat){
			repeatinfo = repeat_info[0];
			snprintf(repeat_information, 100, "%s:%s", pre_repeat_info,
			repeatinfo);
		}else if(PLEXTALK_ALARM_REPEAT_DAILY == timer.repeat){
			repeatinfo = repeat_info[1];
			snprintf(repeat_information, 100, "%s:%s", pre_repeat_info,
			repeatinfo);
		}else{
			char *week[7];
			week[0] = _("Sunday");
			week[1] = _("Monday");
			week[2] = _("Tuesday");
			week[3] = _("Wednesday");
			week[4] = _("Thursday");
			week[5] = _("Friday");
			week[6] = _("Saturday");

			int val = timer.weekday;
			int i;
			strcpy(repeat_information, pre_repeat_info);
			strcat(repeat_information,":");
			for(i=0; i< 7;i++){
				if(val & (1<<i)){
					strcat(repeat_information," ");
					strcat(repeat_information,week[i]);
				}
			}

		}
		text = repeat_information;
		multiline_list_add_item(modal_info.list, repeat_information);
	}
}


int confirm_timer_setting()
{
	char *curtext = NULL;
	//DBGMSG("confirm_timer_setting \n");

	InitComfirm();
	//CreateSystemForm();
	CreateFormMain();
	//DBGMSG("confirm_timer_setting 2\n");

	confirm_add_item();
	//DBGMSG("confirm_timer_setting 3\n");
	/* Show window */
    NeuxFormShow(modal_info.form, TRUE);
	NeuxWidgetFocus(modal_info.form);

	initCurrentItem(modal_info.list);
	curtext = getItemContent(modal_info.list);
	
	voice_prompt_string2(0, &vprompt_cfg,ivTTS_CODEPAGE_UTF8,
		tts_lang,tts_role,_("You are about to set the timer.Are you sure?"));
	
	voice_prompt_string(1, &vprompt_cfg,
		ivTTS_CODEPAGE_UTF8, tts_lang,tts_role,
		curtext, strlen(curtext));
	
	voice_prompt_end = 0;
	//DBGMSG("confirm_timer_setting 4\n");

	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	DBGMSG(" exit\n");
	NeuxFormDestroy(&modal_info.form);
	//DBGMSG("confirm_timer_setting 5  :%d\n",modal_info.ok);
	if(modal_info.ok){
		//MenuExit(0);
		return 0;
	}
	else{
		return -1;
	}
}

