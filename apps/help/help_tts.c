#include <microwin/device.h>

#include <stdio.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "help_tts.h"


#include "libvprompt.h"

#include "plextalk-config.h"
#include "Amixer.h"
#include "plextalk-statusbar.h"
#include "plextalk-keys.h"
#include "desktop-ipc.h"
#include "plextalk-theme.h"
#include "application-ipc.h"
#include "plextalk-ui.h"
#include "Vprompt-defines.h"
#include "Sysfs-helper.h"

#define OSD_DBG_MSG
#include "nc-err.h"



#define	 _(S)	gettext(S)


/* The path for each beep */
static char bu_path[256];
static char pu_path[256];
static char cancel_path[256];
//struct plextalk_guide guide_cache;
//struct plextalk_volume volume_cache[2];
struct voice_prompt_cfg vprompt_cfg;


static int help_handle_fd;
/* Indicate the current tts stopped or not */
static int tts_stop = 1;
static enum tts_end_state g_tts_end_callback;

extern void help_tts_end_toDestroy(void);
extern ivUInt32 tts_role;
extern ivUInt32 tts_lang;


void help_tts_set_stop (void)
{
	tts_stop = 1;
}

int help_tts_get_stop (void)
{
	return tts_stop;
}

/* for tts text */
#define KEY_NUM	TTS_MAX_NUM			//focus on this change when you add tts_text!!!
static char* tts_text[KEY_NUM];

static void 
tts_text_init(void)
{
	tts_text[TTS_POWER] = _("Power key, Press and hold to turn power on or off.");
	tts_text[TTS_BOOK] = _("Book key, Press to start the book mode.");	
	tts_text[TTS_MUSIC] = _("Music key, press to start music mode.");			
	tts_text[TTS_RADIO] = _("Radio key, press to start radio mode.");		
	tts_text[TTS_PLAY_STOP] = _("Play Stop key, Press to start or stop playback," \
							"or to select an option in a menu or list.");
	tts_text[TTS_UP] = _("Up key, Scroll up.");		
	tts_text[TTS_DOWN] = _("Down key, Scroll down.");
	tts_text[TTS_LEFT] = _("Left key, Move back or Cancel.");		
	tts_text[TTS_RIGHT] = _("Right key, Move forward or Enter.");	
	tts_text[TTS_VOL_INC] = _("Volume Up key.Slide up  to increase the volume."	\
							"Hold to quickly increase the volume.");	
	tts_text[TTS_VOL_DEC] = _("Volume Down key.Slide down  to decrease the volume." \
							"Hold to quickly decrease the volume.");	
	tts_text[TTS_RECORD] = _("Recording key, Press twice to record."	\
							"Pause recording with another press.Press Enter to stop."),	
	tts_text[TTS_MENU] = _("Menu key, Press to open or close the menu."	\
							"Press and hold to turn on or off Key describer.");
	tts_text[TTS_INFO] = _("Information key.Press repeatedly to the current time,battery level," \
							"power source, and other information about the current title," \
							"such as the time elapsed in audio or percentage in text.");		
	tts_text[TTS_BEGIN] = _("Key describe on.");
	tts_text[TTS_EXIT] = _("Key describe off.");

	tts_text[TTS_LOCK] = _("Lock switch key");

	//help_role = sys_get_cur_tts_lang();
	//help_volume = sys_get_global_voice_volume();
}


void
help_init_tts_prop()
{
	int help_hp_online;
	//sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &help_hp_online);
	
	CoolShmReadLock(g_config_lock);
	//guide_cache = g_config->setting.guide;
	//lcd_cache = g_config->setting.lcd;
	//lang_cache = g_config->setting.lang;
	//tts_cache = g_config->setting.tts;
	//volume_cache[0] = g_config->volume[0];
	//volume_cache[1] = g_config->volume[1];
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed	  = g_config->setting.guide.speed;
//	vprompt_cfg.equalizer = g_config->setting.guide.equalizer;

	
	CoolShmReadUnlock(g_config_lock);

	

	plextalk_update_tts(getenv("LANG"));

	DBGLOG("rec_init_tts_prop VOL:%d\n",vprompt_cfg.volume);

}


void help_set_tts_end_state(enum tts_end_state state)
{
	g_tts_end_callback = state;
}

enum tts_end_state help_get_tts_state()
{
	return g_tts_end_callback;
}


void 
help_tts_init (void)
{
	snprintf(bu_path, 256, VPROMPT_AUDIO_BU);
	snprintf(pu_path, 256, VPROMPT_AUDIO_PU);
	snprintf(cancel_path, 256, VPROMPT_AUDIO_CANCEL);


	help_init_tts_prop();
	voice_prompt_init();
	//voice_prompt_abort();
	tts_text_init();
	g_tts_end_callback = TTS_END_STATE_NONE;
	help_on_tts_end_callback();
}


void 
help_tts_end (void)
{
	g_tts_end_callback = TTS_END_STATE_NONE;
	voice_prompt_abort();
	voice_prompt_uninit();
}


void help_tts (int mode)
{
	char* str_buf = tts_text[mode];

	voice_prompt_abort();
	tts_stop = 0;

	if (mode == TTS_ERROR)
		voice_prompt_music(0, &vprompt_cfg, bu_path);
	else if (mode == TTS_EXIT) {
		voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			tts_role, str_buf, strlen(str_buf)); 
		voice_prompt_music(1, &vprompt_cfg, cancel_path);
		
		help_set_tts_end_state(TTS_END_STATE_EXIT);
	} else if(mode == TTS_LOCK) {
	
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
	}else {
		if(mode != TTS_BEGIN){
			voice_prompt_music(0, &vprompt_cfg, pu_path);
		}
		voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			tts_role, str_buf, strlen(str_buf)); 
	}


}


void wait_tts (void)
{
	return;
	
	while (1) {
		if (!help_tts_get_stop())
			usleep(10000);		//usleep 0.01s
		else
			break;
	}
}


/* 回调函数 */
void fd_read(void *pdate)
{
   struct signalfd_siginfo fdsi;

   ssize_t ret = read(help_handle_fd, &fdsi, sizeof(fdsi));

   //if (ret != sizeof(fdsi))
	//	handle_error();

	DBGLOG("fd_read func:\n");
   
//	close(rec_handle_fd);
	
	if(g_tts_end_callback == TTS_END_STATE_EXIT)
	{
		help_tts_end_toDestroy();
		g_tts_end_callback = TTS_END_STATE_NONE;
	}
	
}



void help_on_tts_end_callback (void)
{
	//char* pdata;	 //private data
	int sta = 0;

	/* 得到fd */
	help_handle_fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(help_handle_fd, &sta, fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(help_handle_fd, 1, 0);

	DBGLOG("help_on_tts_end_callback \n");
}




