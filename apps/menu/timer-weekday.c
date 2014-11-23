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
#include "timer-weekday.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "nxutils.h"
#include "Nw-menu.h"

#undef _
#define _(x) x

#define LABLE_LEFT 0
#define LABLE_TOP  0

#define MAX_WEEK 7

static LABEL *weekey_lable;
static LABEL *onoff_lable;
static int weekday;
static int iweek;

static int endOfModal = -1;
static int iMsgReturn = MSGBX_BTN_CANCEL;

static TIMER *key_timer;
static int long_pressed_key = -1;

static const char* weekStr[] =
{
	_("Sunday"),
	_("Monday"),
	_("Tuesday"),
	_("Wednesday"),
	_("Thursday"),
	_("Friday"),
	_("Saturday"),
};

static const char* onoffStr[] =
{
	_("Off"),
	_("On"),
};

static void setWeekdayLableText(int iweek)
{
	NeuxLabelSetText(weekey_lable, gettext(weekStr[iweek]));
}

static void setOnoffLableText(int weekday, int iweek)
{
	int on = (weekday & (1<<iweek))?1:0;
	NeuxLabelSetText(onoff_lable, gettext(onoffStr[on]));
}

static void playItemPrompt(void)
{
	char str[128];
	snprintf(str, sizeof(str), "%s:%s", NeuxLabelGetText(weekey_lable), NeuxLabelGetText(onoff_lable));

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
			if(iweek >= (MAX_WEEK-1)){
				endOfModal = 1;
				iMsgReturn = MSGBX_BTN_OK;
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, gettext("Enter"));
				break;
			}else{
				iweek ++;
				setWeekdayLableText(iweek);
				setOnoffLableText(weekday, iweek);
				playItemPrompt();
			}
		}
		break;
	case MWKEY_LEFT:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(iweek <= 0){
				endOfModal = 1;
				iMsgReturn = MSGBX_BTN_CANCEL;
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, gettext("Cancel."));
				break;
			}else{
				iweek --;
				setWeekdayLableText(iweek);
				setOnoffLableText(weekday, iweek);
				playItemPrompt();
			}
		}
		break;	
	case MWKEY_UP:
	case MWKEY_DOWN:	
		{
			weekday ^= (1<<iweek);
			setOnoffLableText(weekday, iweek);
			
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, NeuxLabelGetText(onoff_lable));
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

	case MWKEY_MENU:
		DBGMSG("MENU down\n");
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;
#endif
	case MWKEY_INFO:
		{
			char str[128];
			snprintf(str, sizeof(str), "%s:%s", NeuxLabelGetText(weekey_lable), NeuxLabelGetText(onoff_lable));
			voice_prompt_abort();
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, str);
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
		break;

	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;

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

static void OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}

int set_timer_weekday(int dir)
{
	FORM* form;
	widget_prop_t wprop;
	label_prop_t lprop;
	int y,h;

	weekday = timer.weekday;
	DBGMSG("weekday:%d\n",weekday);
	iMsgReturn = MSGBX_BTN_CANCEL;

	form = NeuxFormNew( MAINWIN_LEFT,
						MAINWIN_TOP,
						MAINWIN_WIDTH,
						MAINWIN_HEIGHT);

	y = LABLE_TOP;
	h = sys_font_size+6;
	weekey_lable = NeuxLabelNew(form,
						LABLE_LEFT,y,
						MAINWIN_WIDTH,h, NULL);

	y += h;
	onoff_lable = NeuxLabelNew(form,
						LABLE_LEFT,y,
						MAINWIN_WIDTH,h, NULL);
	
	key_timer = NeuxTimerNewWithCallback(form, 0, 0, OnKeyTimer);
	
	NeuxWidgetGetProp(weekey_lable, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(weekey_lable, &wprop);
	
	NeuxLabelGetProp(weekey_lable, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(weekey_lable, &lprop);
	
	NeuxWidgetSetFont(weekey_lable, sys_font_name, sys_font_size);
	
	NeuxWidgetShow(weekey_lable, TRUE);
	
	NeuxWidgetGetProp(onoff_lable, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(onoff_lable, &wprop);

	NeuxLabelGetProp(onoff_lable, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(onoff_lable, &lprop);
	
	NeuxWidgetSetFont(onoff_lable, sys_font_name, sys_font_size);

	NeuxWidgetShow(onoff_lable, TRUE);

	NeuxFormSetCallback(form, CB_KEY_DOWN, OnWindowKeydown);
//	NeuxFormSetCallback(form, CB_KEY_UP, OnWindowKeyup);
//	NeuxFormSetCallback(form, CB_FOCUS_IN, OnWindowGetFocus);
//	NeuxFormSetCallback(form, CB_DESTROY,  OnWindowDestroy);

	NeuxFormShow(form,TRUE);
	NeuxWidgetFocus(form);

	if(dir){
		iweek = MAX_WEEK -1;
	}else{
		iweek = 0;
	}
	setWeekdayLableText(iweek);
	setOnoffLableText(weekday,iweek);
	
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, gettext("Select the day of the week."));
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, gettext("Press Right or Left key to select the day of week.Press Up or Down key to set ON or OFF."));

	playItemPrompt();
	
	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	NeuxFormDestroy(&form);
	if(MSGBX_BTN_OK == iMsgReturn){
		timer.weekday = weekday;
		return 0;
	}else{
		return -1;
	}
}


