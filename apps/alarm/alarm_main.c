#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#define MWINCLUDECOLORS
#include <microwin/device.h>

#include <microwin/nano-X.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "alarm_voice.h"

#include "libvprompt.h"
#include "plextalk-statusbar.h"
#include "plextalk-keys.h"
#include "desktop-ipc.h"
#include "plextalk-theme.h"
#include "application-ipc.h"
#include "plextalk-ui.h"
#include "Sysfs-helper.h"
#include "amixer.h"
#include "plextalk-config.h"
#include "plextalk-constant.h"

#include "Vprompt-defines.h"


#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif

#define KEYLOCK  "/sys/devices/platform/gpio-keys/on_switches"

#define HEADPHONE_JACK1 	"/sys/devices/platform/jz-codec/headphone_jack"

#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))

#define EXIT_TIME_OUT	(1000*60)
#define PLAYAGAIN_TIME_OUT	(1000)

static int key_locked;
static FORM *  formMain;
#define widget formMain
struct voice_prompt_cfg vprompt_cfg;

#if 0
/* For Key long_press judgement */
struct long_press {
	int l_flag;
	int pressed;
};
#endif

struct alarm_nano {
	char alarm_path[256];
	int soundid;
	int curtimeid;
	struct AudioVoice *audio;
	TIMER* elapseTimer;
	TIMER* againTimer;
	int isquit;
	int preIcon;
};

char* alarm_audio_path[3]=
{
	VPROMPT_SOUND_A,
	VPROMPT_SOUND_B,
	VPROMPT_SOUND_C
};


static struct alarm_nano g_alarm_nano;
//static struct long_press menu_key; 
struct plextalk_language_all all_langs;


/* do not show this window */
//static GR_WINDOW_ID h_wid;

/* Use for Key_lock */
static int key_locked;

//static long master_vol = -1, dac_vol, mute, output;
//static int change_vol, change_output;
//static int last_vp_vol;


static int grab_key_table[] = {
	MWKEY_POWER,
	VKEY_BOOK, 
	VKEY_MUSIC,
	VKEY_RADIO,
	MWKEY_ENTER,
	MWKEY_UP,
	MWKEY_DOWN,
	MWKEY_LEFT,
	MWKEY_RIGHT,
	MWKEY_VOLUME_UP,
	MWKEY_VOLUME_DOWN,
	VKEY_RECORD,
	MWKEY_MENU,
	MWKEY_INFO,
};
static const char* output_mux[] = {
	[PLEXTALK_OUTPUT_SELECT_SIDETONE] = PLEXTALK_OUTPUT_SIDETONE,
	[PLEXTALK_OUTPUT_SELECT_BYPASS] = PLEXTALK_OUTPUT_BYPASS,
	[PLEXTALK_OUTPUT_SELECT_DAC] = PLEXTALK_OUTPUT_DAC,
};


static void
help_menu_key_longpress_handler (void);



void DesktopScreenSaver(GR_BOOL activate)
{
}


static int VoicePromptSaveVolume(int vol)
{
#if 0//音量后来的需求又改为一个界面一个音量，因此不需要再保存恢复了

	snd_mixer_t *handle;
	
	handle = amixer_open(NULL);
	
	amixer_get_playback_volume(handle, PLEXTALK_PLAYBACK_VOL_MASTER, 0, &master_vol, NULL);
	amixer_get_playback_volume(handle, PLEXTALK_PLAYBACK_VOL_DAC, 0, &dac_vol, NULL);
	mute = amixer_get_playback_switch(handle, PLEXTALK_PLAYBACK_MUTE, 0);
	output = amixer_get_playback_enum(handle, PLEXTALK_OUTPUT_MUX, 0);
	
	change_vol = mute ||
				 master_vol != plextalk_sound_volume[vol].master ||
				 dac_vol != plextalk_sound_volume[vol].DAC;
	change_output = output != PLEXTALK_OUTPUT_SELECT_DAC;
	
	if (change_vol) {
		if (!mute)
			amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);
		if (master_vol != plextalk_sound_volume[vol].master)
			amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, plextalk_sound_volume[vol].master);
		if (dac_vol != plextalk_sound_volume[vol].DAC)
			amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_DAC, plextalk_sound_volume[vol].DAC);
	}
	if (change_output)
		amixer_sset_str(handle, VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);
	amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);
	
	amixer_close(handle);
#else
	snd_mixer_t *handle;

	handle = amixer_open(NULL);

	amixer_sset_str(handle, VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);

	amixer_close(handle);
	return 0;


#endif
	

}

static int VoicePromptRestoreVolume(void)
{
#if 0//音量后来的需求又改为一个界面一个音量，因此不需要再保存恢复了
	snd_mixer_t *handle;

	if (change_vol || change_output) {

		handle = amixer_open(NULL);
		
		amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);
		if (master_vol != plextalk_sound_volume[last_vp_vol].master)
			amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, master_vol);
		if (dac_vol != plextalk_sound_volume[last_vp_vol].DAC)
			amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_DAC, dac_vol);
		if (change_output)
			amixer_sset_str(handle, VOL_RAW, PLEXTALK_OUTPUT_MUX, output_mux[output]);
		if (!mute)
			amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);
		
		amixer_close(handle);
	}
#endif	
	return 0;

}





static void
alarm_grab_key()
{
	int i;

	for(i = 0; i < ARRAY_SIZE(grab_key_table); i++)
		NeuxWidgetGrabKey(widget, grab_key_table[i], NX_GRAB_HOTKEY_EXCLUSIVE);
}


static void
alarm_ungrab_key ()
{
	int i;
	for(i = 0; i < ARRAY_SIZE(grab_key_table); i++)
		NeuxWidgetUngrabKey(widget, grab_key_table[i]);
}

static int 
headphone_on (void)
{
	int hp_online;
	
	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);
	
	info_debug("mic_headphone hp_online :%d",hp_online);
	
	return hp_online;

}
void alarm_end_toDestroy(void)
{
	DBGLOG("Alarm App Stop!!\n");
	if(g_alarm_nano.againTimer)
		TimerDisable(g_alarm_nano.againTimer);
	//plextalk_schedule_unlock();
	alarm_ungrab_key();
	if(g_alarm_nano.audio) {
		audio_voice_stop(g_alarm_nano.audio);
		audio_voice_destroy(g_alarm_nano.audio);
	}
	NeuxAppStop();
	
}



static void 
event_headphone_handler(void)
{
#if 0
	if (headphone_on()) {
		help_p_mixer_set(SPEAKER_OFF, 0);
		help_p_mixer_set(HEADPHONE_ON, 0);
	} else {
		help_p_mixer_set(SPEAKER_ON, 0);
		help_p_mixer_set(HEADPHONE_OFF, 0);
	}
#endif

}

static void 
alarm_OnTtimer()
{
	TimerDisable(g_alarm_nano.elapseTimer);
	alarm_end_toDestroy();

}

static void 
alarm_OnAgainTtimer()
{
	if(!audio_voice_isruning(g_alarm_nano.audio)){
		/*replay*/
		audio_voice_play(g_alarm_nano.audio, g_alarm_nano.alarm_path);
	}
}



static void
alarm_CreateTimer()
{
	g_alarm_nano.elapseTimer= NeuxTimerNew(widget, EXIT_TIME_OUT, 1);
	NeuxTimerSetCallback(g_alarm_nano.elapseTimer, alarm_OnTtimer);
	DBGLOG("alarm_CreateTimer nano->rec_timer:%d",g_alarm_nano.elapseTimer);	
}

static void
alarm_CreatePlayAgainTimer()
{
	g_alarm_nano.againTimer= NeuxTimerNew(widget, PLAYAGAIN_TIME_OUT, -1);
	NeuxTimerSetCallback(g_alarm_nano.againTimer, alarm_OnAgainTtimer);
	DBGLOG("alarm_CreatePlayAgainTimer nano->rec_timer:%d",g_alarm_nano.againTimer);	
}



static void
alarm_key_down_handler (int key_val)
{
}


static void
alarm_menu_key_longpress_handler (void)
{
}


static void
alarm_menu_key_shortpress_handler (void)
{

}




static void
alarm_OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);
	//一有按按键就退出
	alarm_end_toDestroy();


}

static void
alarm_OnWindowKeyup(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeyup %d.\n",key.ch);

}

static void
alarm_OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);

	plextalk_suspend_unlock();
	
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, g_alarm_nano.preIcon);
	
	VoicePromptRestoreVolume();
	plextalk_global_config_close();

#if 0	
	TimerDelete(s_nano.rec_timer);
	NeuxFormDestroy(widget);	
	widget = NULL;
#endif	
}



static void
alarm_onSWOn(int sw)
{
#if 1
	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 1;
		break;
	case SW_HP_INSERT:
		event_headphone_handler();
		break;
	default:
		return;
	}
	alarm_end_toDestroy();//任何有提示的热插拔都要退出
#endif	
}

static void
alarm_onSWOff(int sw)
{
#if 1
	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 0;
		break;
	case SW_HP_INSERT:
		event_headphone_handler();
		break;
	default:
		return;
	}
	alarm_end_toDestroy();//任何有提示的热插拔都要退出
#endif	
}

static void
alarm_read_keylock()
{
	char buf[16];
	int res;
	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0")){
	key_locked = 1;
	}else{
	key_locked = 0;
	}
	DBGMSG("keylock:%d\n",key_locked);
}



static void
alarm_initApp(struct alarm_nano* nano)
{
	int res;

	/* global config */
	//plextalk_global_config_init();
	plextalk_global_config_open();

	/* theme */
	//CoolShmWriteLock(g_config_lock);
	//NeuxThemeInit(g_config->setting.lcd.theme);
	//CoolShmWriteUnlock(g_config_lock);
#if 0	/* language */
	//plextalk_get_all_lang("plextalk.xml", &all_langs);

	//CoolShmReadLock(g_config_lock);
	res = plextalk_find_lang(&all_langs, g_config->setting.lang.lang);
	//CoolShmReadUnlock(g_config_lock);
	if (res < 0) {
		WARNLOG("Current setted language is not in languages list.\n");
		//CoolShmWriteLock(g_config_lock);
		strlcpy(g_config->setting.lang.lang, all_langs.langs[0].lang, PLEXTALK_LANG_MAX);
		//CoolShmWriteUnlock(g_config_lock);
	}
#endif
	//CoolShmReadLock(g_config_lock);
	plextalk_update_lang(g_config->setting.lang.lang, "alarm");
	//CoolShmReadUnlock(g_config_lock);

	//plextalk_update_sys_font();

	//nano->is_running = 0;
	//rec_tts_init(&(nano->is_running));
	//rec_tts(TTS_USAGE);

	//nano->menu_pressed = 0;
	//nano->menu_l_flag = 0;
	//nano->is_running = 0;

	/* Abort in case of other app's tts */
	voice_prompt_abort();

	DBGMSG("alarm start curtimeid:%d\n",nano->curtimeid);
	nano->soundid = g_config->setting.timer[nano->curtimeid].sound;
	DBGMSG("alarm start soundid:%d\n",nano->soundid);
	if(nano->soundid>2 || nano->soundid<0)
	{
		nano->soundid = 0;
	}
	DBGMSG("alarm start soundid2:%d\n",nano->soundid);
	memset(nano->alarm_path,0x00,256);
	snprintf(nano->alarm_path, 255, alarm_audio_path[nano->soundid]);

	//amixer_direct_sset_integer(VOL_RAW,PLEXTALK_PLAYBACK_VOL_MASTER,plextalk_sound_volume[g_config->volume.guide].master);
	//amixer_direct_sset_integer(VOL_RAW,PLEXTALK_PLAYBACK_VOL_DAC,plextalk_sound_volume[g_config->volume.guide].DAC);
	//VoicePromptSaveVolume(g_config->volume.guide);
	
	nano->isquit = 0;

	//alarm_read_keylock();
}

static void
alarm_InitForm()
{
	alarm_grab_key();
	VoicePromptSaveVolume(g_config->volume.guide);//紧挨着发音

	g_alarm_nano.audio = audio_voice_create();
	DBGMSG("alarm start sound:%s\n",g_alarm_nano.alarm_path);
	audio_voice_play(g_alarm_nano.audio, g_alarm_nano.alarm_path);
	//创建连续播放定时器
	alarm_CreatePlayAgainTimer();
}


static void
alarm_onHotplug(WID__ wid, int device, int index, int action)
{
	DBGLOG("onHotplug device: %d ,index=%d,action=%d\n", device,index,action);

	if (device == MWHOTPLUG_DEV_MMCBLK 
		|| device == MWHOTPLUG_DEV_SCSI_DISK
		|| device == MWHOTPLUG_DEV_VBUS
		|| device == MWHOTPLUG_DEV_UDC
		|| device == MWHOTPLUG_DEV_SCSI_CDROM) {
		
		alarm_end_toDestroy();//任何有提示的热插拔都要退出
	}
	
}



/* Function creates the main form of the application. */
void
alarm_CreateFormMain ()
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, alarm_OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 alarm_OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  alarm_OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_HOTPLUG,  alarm_onHotplug);

//创建超时退出定时器
	alarm_CreateTimer();


#if 0

	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(widget, &wprop);

	NeuxFormShow(widget, FALSE);

	//创建四个LABEL用于显示字符串
	Rec_CreateLabels(widget,nano,data);
	//创建定时器用于计时和显示
	Rec_CreateTimer(widget,nano);
#endif	

	alarm_InitForm();

}

//收到RESTORE消息后继续运行下一个ALARM
static void alarm_onRestoreRun(char *pName)
{
	DBGMSG("alarm_onRestoreRun g_alarm_nano.curtimeid:%d,pName:%s\n",g_alarm_nano.curtimeid,pName);

	//let's stop current voice and restart an alarm
	if(g_alarm_nano.againTimer)
		TimerDelete(g_alarm_nano.againTimer);
	if(g_alarm_nano.elapseTimer)
		TimerDelete(g_alarm_nano.elapseTimer);
	
	if(g_alarm_nano.audio) {
		audio_voice_stop(g_alarm_nano.audio);
		audio_voice_destroy(g_alarm_nano.audio);
	}

	if(pName) {
		char *p = pName+strlen(pName)-1;
		DBGMSG("p:%s\n",p);
		if(*p == 0x31) {
			g_alarm_nano.curtimeid = 1;
		} else if(*p == 0x30) {
			g_alarm_nano.curtimeid = 0;
		} else {
			g_alarm_nano.curtimeid = !g_alarm_nano.curtimeid;
		}
	} else {
		g_alarm_nano.curtimeid = !g_alarm_nano.curtimeid;//两个闹钟，肯定是1，0和0，1的情况
	}
	
	g_alarm_nano.soundid = g_config->setting.timer[g_alarm_nano.curtimeid].sound;
	DBGMSG("alarm start soundid:%d\n",g_alarm_nano.soundid);
	if(g_alarm_nano.soundid>2 || g_alarm_nano.soundid<0) {
		g_alarm_nano.soundid = 0;
	}
	DBGMSG("alarm start soundid2:%d\n",g_alarm_nano.soundid);
	
	memset(g_alarm_nano.alarm_path,0x00,256);
	snprintf(g_alarm_nano.alarm_path, 255, alarm_audio_path[g_alarm_nano.soundid]);
	
	g_alarm_nano.isquit = 0;

	//创建退出定时器
	alarm_CreateTimer();
	g_alarm_nano.audio = audio_voice_create();
	DBGMSG("alarm start sound:%s\n",g_alarm_nano.alarm_path);
	audio_voice_play(g_alarm_nano.audio, g_alarm_nano.alarm_path);
	//创建连续播放定时器
	alarm_CreatePlayAgainTimer();
	
}


static void onAppMsg(const char * src, neux_msg_t * msg)
{
	app_rqst_t *rqst;
	char *pname;
	DBGMSG("onAppMsg id:%d\n",msg->msg.msg.msgId);

	switch(msg->msg.msg.msgId){
	case APP_MSG_RESTORE_ARGS://接收到此消息需要继续响铃
		pname = msg->msg.msg.msgTxt;
		DBGMSG("restore args:%s\n",pname);
		alarm_onRestoreRun(msg->msg.msg.msgTxt);
		break;
	case PLEXTALK_APP_MSG_ID:
		{
			rqst = msg->msg.msg.msgTxt;
			DBGMSG("onAppMsg rqst:%d\n",rqst->type);
			switch (rqst->type) {
			case APPRQST_LOW_POWER:
				DBGMSG("received lowpower message\n");
				alarm_end_toDestroy();
				break;
			}
		}
		break;
	default:
		break;
	}

}


int
main(int argc,char **argv)

{

	printf("Enter alarm main fun!\n");
	
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	if(argv[1] == NULL)
	{
		g_alarm_nano.curtimeid = 0;
	}
	else if(!strcmp(argv[1],"0"))
	{
		g_alarm_nano.curtimeid = 0;
	}
	else
	{
		g_alarm_nano.curtimeid = 1;
	}
	if(argv[1])
		DBGMSG("alarm main:%s\n",argv[1]);
	else
		DBGMSG("alarm main null\n");
	DBGMSG("alarm id:%d\n",g_alarm_nano.curtimeid);

	alarm_initApp(&g_alarm_nano);
	DBGMSG("application initialized\n");

	plextalk_suspend_lock();//alarm界面不suspend
	
	DBGMSG("NeuxAppCreate app create success!\n");
	//get input window id
	//g_help_nano.wid = NeuxAppMsgWID();
	DBGLOG("get wid success!\n");

	CoolShmReadLock(g_config_lock);
	g_alarm_nano.preIcon = g_config->statusbar_category_icon;
	CoolShmReadUnlock(g_config_lock);	
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_TIMER);

	alarm_CreateFormMain();
	DBGMSG("create form success!\n");
	NeuxAppSetCallback(CB_MSG,	 onAppMsg);
	//NeuxAppSetCallback(CB_HOTPLUG, onHotplug);
	NeuxAppSetCallback(CB_SW_ON,	alarm_onSWOn);
	NeuxAppSetCallback(CB_SW_OFF,	alarm_onSWOff);
	
	//this function must be run after create main form ,it need set schedule time
	DBGMSG("mainform shown\n");
	
	// start application, events starts to feed in.
	NeuxAppStart();
	
	return 0;

}
