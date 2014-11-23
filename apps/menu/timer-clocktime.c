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
#include "timer-clocktime.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "Plextalk-config.h"
#include "plextalk-ui.h"
#include "nxutils.h"
#include "Rtc-helper.h"
#include "Nw-menu.h"

#undef _
#define _(x) x

#define LABLE_LEFT 0
#define LABLE_TOP  0


extern int voice_prompt_end;

static LABEL *item_lable;
static LABEL *time_lable;
static TIMER *key_timer;

static int keymask;
static int timerEnable;
static int keycount;

static int endOfModal = -1;
static int iMsgReturn = MSGBX_BTN_CANCEL;

typedef enum{
	FORMAT_12H = 0,
	FORMAT_24H,
}HOURFORMAT;

typedef struct {
	char *itemStr;
	int value;
	const char **valStr;
	int minValue;
	int maxValue;
}TIME;

#define MAX_ITEM  3

static const char* valStr[] =
{
	_("AM"),
	_("PM"),
};

static TIME timeinfo[MAX_ITEM] =
{
	{_("Hour"), 0, 0, 0,23},
	{_("Minute"), 0, 0, 0,59},
	{_(" "), 0, valStr, 0,ARRAY_SIZE(valStr)-1},
};

static int iItem = 0;
static int iMaxItem = MAX_ITEM;
static HOURFORMAT format = FORMAT_24H;

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

static void setItemLableText(int iItem)
{
	NeuxLabelSetText(item_lable, gettext(timeinfo[iItem].itemStr));
}

static void setTimeLableText(int iItem)
{
	if(NULL == timeinfo[iItem].valStr){
		char str[10];
		snprintf(str, 10, "%.2d", timeinfo[iItem].value);
		NeuxLabelSetText(time_lable, str);
	}else{
		char *pstr = gettext(timeinfo[iItem].valStr[timeinfo[iItem].value]);
		NeuxLabelSetText(time_lable, pstr);
	}
	
}

static void playItemPrompt(void)
{
	char str[128];
	snprintf(str, 128, "%s: %s", NeuxLabelGetText(item_lable), NeuxLabelGetText(time_lable));

//	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, str);
}

static void
OnWindowKeydown(WID__ wid, KEY__ key)
{
	if (menu_exit) {
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
			tts_lang, tts_role, gettext("Keylock"));
		return;
	}

    switch (key.ch)
    {
	case MWKEY_ENTER:
	case MWKEY_RIGHT:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(iItem >= (iMaxItem-1)){
				endOfModal = 1;
				iMsgReturn = MSGBX_BTN_OK;
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, gettext("Enter"));
				break;
			}else{
				iItem ++;
				setItemLableText(iItem);
				setTimeLableText(iItem);
				
				playItemPrompt();
			}
		}
		break;
	case MWKEY_LEFT:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(iItem <= 0){
				endOfModal = 1;
				iMsgReturn = MSGBX_BTN_CANCEL;
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, gettext("Cancel."));
				break;
			}else{
				iItem --;
				setItemLableText(iItem);
				setTimeLableText(iItem);
				
				playItemPrompt();
			}
		}
		break;
		
	case MWKEY_INFO:
		{
			char str[128];
			
			snprintf(str, sizeof(str), "%s: %s", NeuxLabelGetText(item_lable), NeuxLabelGetText(time_lable));
			voice_prompt_abort();
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, str);
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		break;
		
//	case MWKEY_MENU:
//		DBGMSG("MENU down\n");
	case MWKEY_DOWN:
	case MWKEY_UP:
		if(timerEnable == 0){
			keymask = key.ch;
			TimerEnable(key_timer);
			timerEnable = 1;
			keycount = 0;
		}
		break;

#if 0
	case MWKEY_ENTER:
		{
			endOfModal = 1;
			iMsgReturn = MSGBX_BTN_OK;

			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		break;
#endif

	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;

	}
}


static void opKeyUp(int isKeyUp)
{
	if(timeinfo[iItem].value < timeinfo[iItem].maxValue){
		timeinfo[iItem].value ++;
	}else{
		timeinfo[iItem].value = timeinfo[iItem].minValue;
	}
	setTimeLableText(iItem);

	if(isKeyUp){
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, NeuxLabelGetText(time_lable));
	}
}

static void opKeyDown(int isKeyUp)
{
	if(timeinfo[iItem].value > timeinfo[iItem].minValue){
		timeinfo[iItem].value --;
	}else{
		timeinfo[iItem].value = timeinfo[iItem].maxValue;
	}
	setTimeLableText(iItem);
	
	if(isKeyUp){
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, NeuxLabelGetText(time_lable));
	}
}

static void OnWindowKeyup(WID__ wid,KEY__ key)
{
	if (menu_exit) {
		return;
	}
	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;
	
	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		return;
	}

	if(timerEnable == 0){
		return;
	}
	TimerDisable(key_timer);
	timerEnable = 0;

    switch (key.ch)
    {
	case MWKEY_UP:
		//if(keycount <= 0){
			opKeyUp(1);
		//}
		keycount = 0;
		break;
	case MWKEY_DOWN:
		//if(keycount <= 0){
			opKeyDown(1);
		//}
		keycount = 0;
		break;
#if 0		
	case MWKEY_MENU:
		if(keycount <= 0){
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, gettext("Cancel."));
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, gettext("Close the timer setting menu."));
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			MenuExit(0);
		}
		keycount = 0;
		break;	
		
	case MWKEY_INFO:
		if(keycount <= 0){
			char str[128];
			
			snprintf(str, 128, "%s:%s", NeuxLabelGetText(item_lable), NeuxLabelGetText(time_lable));
			voice_prompt_abort();
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, str);
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		keycount = 0;
		break;
#endif
	default:
		break;

    }

}

static void key_timerout(void)
{
	DBGMSG("keymask=0x%x\n", keymask);
	if(menu_exit){
		return;
	}
	keycount ++;
	switch(keymask){
	case MWKEY_UP:
		if(1==keycount){
			voice_prompt_abort();
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_end = 0;
		}else if(voice_prompt_end){
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		opKeyUp(0);
		break;
	case MWKEY_DOWN:
		if(1==keycount){
			voice_prompt_abort();
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_end = 0;
		}else if(voice_prompt_end){
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		opKeyDown(0);
		break;

		break;
	}
}

int set_timer_clocktime(int dir)
{
	FORM* form;
	widget_prop_t wprop;
	label_prop_t lprop;
	int y,h;
	sys_time_t stm;
	sys_get_time(&stm);
	NhelperStatusBarReflushTime();

	if(-1 == timer.hour){
		timeinfo[0].value = stm.hour;
		timeinfo[1].value = stm.min;
	}else{
		timeinfo[0].value = timer.hour;
		timeinfo[1].value = timer.minute;
	}
	
	iMsgReturn = MSGBX_BTN_CANCEL;

	form = NeuxFormNew( MAINWIN_LEFT,
						MAINWIN_TOP,
						MAINWIN_WIDTH,
						MAINWIN_HEIGHT);

	y = LABLE_TOP;
	h = sys_font_size+6;
	item_lable = NeuxLabelNew(form,
						LABLE_LEFT,y,
						MAINWIN_WIDTH,h, NULL);

	y += h;
	time_lable = NeuxLabelNew(form,
						LABLE_LEFT,y,
						MAINWIN_WIDTH,h, NULL);
	
	key_timer = NeuxTimerNew(form, 500, -1);
	NeuxTimerSetCallback(key_timer, key_timerout);
	TimerDisable(key_timer);
	timerEnable = 0;
	
	NeuxWidgetGetProp(item_lable, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(item_lable, &wprop);
	
	NeuxLabelGetProp(item_lable, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(item_lable, &lprop);
	
	NeuxWidgetSetFont(item_lable, sys_font_name, sys_font_size);
	
	NeuxWidgetShow(item_lable, TRUE);
	
	NeuxWidgetGetProp(time_lable, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(time_lable, &wprop);

	NeuxLabelGetProp(time_lable, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(time_lable, &lprop);
	
	NeuxWidgetSetFont(time_lable, sys_font_name, sys_font_size);

	NeuxWidgetShow(time_lable, TRUE);

	NeuxFormSetCallback(form, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(form, CB_KEY_UP, OnWindowKeyup);
//	NeuxFormSetCallback(form, CB_FOCUS_IN, OnWindowGetFocus);
//	NeuxFormSetCallback(form, CB_DESTROY,  OnWindowDestroy);

	NeuxFormShow(form,TRUE);
	NeuxWidgetFocus(form);

	CoolShmReadLock(g_config_lock);
	format = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);
 
	if(FORMAT_12H == format){
		int ampm;
		iMaxItem = 3;
		timeinfo[0].value = hour24To12(timeinfo[0].value, &ampm);
		timeinfo[2].value = ampm;
		timeinfo[0].minValue = 1;
		timeinfo[0].maxValue = 12;
	}else{
		iMaxItem = 2;
	}

	if(dir){
		iItem = iMaxItem-1;
	}else{
		iItem = 0;
	}
	setItemLableText(iItem);
	setTimeLableText(iItem);

	if(dir){
		playItemPrompt();
	}else{
		char str[128];
		snprintf(str, sizeof(str), "%s: %.2d", gettext(timeinfo[0].itemStr), timeinfo[0].value);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str);

		snprintf(str, sizeof(str), "%s: %.2d", gettext(timeinfo[1].itemStr), timeinfo[1].value);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,tts_lang, tts_role, str);

		if(FORMAT_12H == format){
			strlcpy(str, gettext(timeinfo[2].itemStr),sizeof(str));
			strlcat(str, ": ",sizeof(str));
			strlcat(str, gettext(timeinfo[2].valStr[timeinfo[2].value]),sizeof(str));
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,tts_lang, tts_role, str);
		}
	}
	
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,tts_lang, tts_role,
		gettext("Press Right or Left key to select hour or minute.Press Up or Down key to set the value."));
	
	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	NeuxFormDestroy(&form);
	if(MSGBX_BTN_OK == iMsgReturn){
		if(FORMAT_12H == format){
			//timer.hour = timeinfo[0].value + timeinfo[2].value*12;
			timer.hour = hour12To24(timeinfo[0].value,timeinfo[2].value);
		}else{
			timer.hour = timeinfo[0].value;
		}
		timer.minute = timeinfo[1].value;
		return 0;
	}else{
		return -1;
	}
}


