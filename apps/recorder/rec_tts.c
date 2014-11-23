#include <stdio.h>
#include <libintl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "microwin/nano-X.h"
//#include "voice.h"
//#include "ttsplus.h"
#include "mic_rec.h"
//#include "widget.h"
//#include "system.h"
#include "mic_rec.h"
#include "rec_tts.h"
//#include "keydef.h"
#include "rec_draw.h"
//#include "sys_config.h"
//#include "tipvoice.h"
//#include "resource.h"

#define OSD_DBG_MSG 1
#define DEBUG_PRINT 1
#include "nc-err.h"
#include "plextalk-config.h"
#include "Sysfs-helper.h"
#include "vPrompt-defines.h"
#include "application-ipc.h"
#include "Amixer.h"



#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


#define _(S) 	gettext(S)


static char bu_path[256];
static char pu_path[256];
static char cancel_path[256];

/* tts is running */
static int* tts_running = NULL;

/* TTS role */
int rec_role;

int rec_handle_fd;

/*TTS volume */
int rec_volume;


extern ivUInt32 tts_role;
extern ivUInt32 tts_lang;

extern void rec_tts_end(void);
#if 0
struct plextalk_radio radio_cache;
struct plextalk_guide guide_cache;
struct plextalk_timer timer_cache[2];
struct plextalk_lcd lcd_cache;
struct plextalk_language lang_cache;
struct plextalk_tts tts_cache;
struct plextalk_volume volume_cache[2];
#endif
struct voice_prompt_cfg vprompt_cfg;

int rec_hp_online;

static enum tts_end_state g_tts_end_callback;


void rec_tts_set_stop (void)
{
	*tts_running = 0;
}

int rec_tts_get_stop (void)
{
	return *tts_running;
}


void rec_set_tts_end_state(enum tts_end_state state)
{
	g_tts_end_callback = state;
}

int rec_is_in_tts_exit_state()
{
	return (TTS_END_STATE_EXIT == g_tts_end_callback);
}


void
rec_init_tts_prop()
{
	CoolShmReadLock(g_config_lock);
	//guide_cache = g_config->setting.guide;
	//lcd_cache = g_config->setting.lcd;
	//lang_cache = g_config->setting.lang;
	//tts_cache = g_config->setting.tts;
	//volume_cache[0] = g_config->volume.record;
	//volume_cache[1] = g_config->volume[1];
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed	  = g_config->setting.guide.speed;
//	vprompt_cfg.equalizer = g_config->setting.guide.equalizer;
	CoolShmReadUnlock(g_config_lock);
	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &rec_hp_online);

	//vprompt_cfg.volume = rqst->guide_vol.volume;
	//vprompt_cfg.volume = volume_cache[rec_hp_online].volume[PLEXTALK_VOL_GUIDE];
	//vprompt_cfg.speed = guide_cache.speed;
	//vprompt_cfg.equalizer = guide_cache.equalizer;

	//当插入耳机并打开monitor时，此时的volume应该为record内的音量
	if(rec_hp_online)
	{
		if(get_mic_rec_monitor_state())
		{
			vprompt_cfg.volume = g_config->volume.record;
		}
	}


	
	//rec_volume = vprompt_cfg.volume;

	plextalk_update_tts(getenv("LANG"));

	DBGLOG("rec_init_tts_prop VOL:%d\n",vprompt_cfg.volume);


}

/* 回调函数 */
void fd_read(void *pdate)
{
   struct signalfd_siginfo fdsi;

   ssize_t ret = read(rec_handle_fd, &fdsi, sizeof(fdsi));

   //if (ret != sizeof(fdsi))
	//  handle_error();

   rec_tts_set_stop();

	DBGLOG("fd_read func:%d\n",g_tts_end_callback);
#if 1
   if(g_tts_end_callback == TTS_END_STATE_EXIT)
   {
  		 g_tts_end_callback = TTS_END_STATE_NONE;
   	 	 rec_tts_end();
   }
   else 
#endif	   	
	if(g_tts_end_callback == TTS_END_STATE_START_RECORD)
	{
		g_tts_end_callback = TTS_END_STATE_NONE;
		rec_start_record();
	}


   

//	close(rec_handle_fd);
}



void rec_on_tts_end_callback (void)
{
	//char* pdata;   //private data
	int sta = 0;

	/* 得到fd */
	rec_handle_fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(rec_handle_fd, &sta, fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(rec_handle_fd, 1, 0);

	DBGLOG("rec_on_tts_end_callback \n");
}



void 
rec_tts_init (int* is_running)
{
	//char *prefix;
	//prefix = sys_get_res_dir();

	snprintf(bu_path, 256, VPROMPT_AUDIO_BU);
	snprintf(pu_path, 256, VPROMPT_AUDIO_PU);
	snprintf(cancel_path, 256, VPROMPT_AUDIO_CANCEL);

	//rec_role = sys_get_cur_tts_lang();
	//rec_volume = sys_get_global_voice_volume();

	rec_init_tts_prop();

	//free(prefix);
	voice_prompt_init();

	tts_running = is_running;

	g_tts_end_callback = TTS_END_STATE_NONE;
	rec_on_tts_end_callback();
	
	/* Abort in case of other app's tts */
	voice_prompt_abort();
}

void rec_tts_change_volume(void)
{
#if 0
	volume_cache[0] = g_config->volume[0];
	volume_cache[1] = g_config->volume[1];
	//CoolShmReadUnlock(g_config_lock);
	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &rec_hp_online);
	
	vprompt_cfg.volume = volume_cache[rec_hp_online].volume[PLEXTALK_VOL_GUIDE];

	rec_volume = vprompt_cfg.volume;//sys_get_global_voice_volume();
#else
	CoolShmReadLock(g_config_lock);
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed	  = g_config->setting.guide.speed;
//	vprompt_cfg.equalizer = g_config->setting.guide.equalizer;
	CoolShmReadUnlock(g_config_lock);
	//当插入耳机并打开monitor时，此时的volume应该为record内的音量
	if(rec_hp_online)
	{
		if(get_mic_rec_monitor_state())
		{
			vprompt_cfg.volume = g_config->volume.record;
		}
	}

#endif
}


void 
rec_tts_destroy (void)
{
	close(rec_handle_fd);
	g_tts_end_callback = TTS_END_STATE_NONE;
	//voice_prompt_abort();		//this may abort VBUS's hotplug voice!@!
	voice_prompt_uninit();
}


static void
normal_beep (void)
{
	voice_prompt_abort();
	*tts_running = 1;
	//voice_prompt_music(1, &vprompt_cfg, pu_path);
	voice_prompt_music2(0, get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC, 
		&vprompt_cfg, pu_path);
}


static void 
error_beep (void)
{
	voice_prompt_abort();
	*tts_running = 1;
	//voice_prompt_music(0, &vprompt_cfg, bu_path);
	voice_prompt_music2(0, get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC, 
		&vprompt_cfg, bu_path);
}


static void
cancel_beep (void)
{
	voice_prompt_abort();
	*tts_running = 1;
	//voice_prompt_music(0, &vprompt_cfg, cancel_path);
	voice_prompt_music2(0, get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC, 
		&vprompt_cfg, cancel_path);
}


//这种wait,tts可以改为信号量唤醒

void 
wait_tts (void) 
{

	return; 

	
	while (1) {

		if (!(*tts_running))
			break;
		
		usleep(100000);		//usleep 0.1s
	}
}

void
rec_tts_switch_to_speaker()
{
	//off the monitor
	if(get_mic_rec_monitor_state())
		mic_rec_monitor_off();

	//change to the speaker
	NhelperRecordingNow(0);
	NhelperRecordingActive(0);
}


void
rec_tts_close_error()
{
	voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("close the recording mode"));

	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);

	DBGMSG("error tts close\n");

}

void 
rec_tts_no_target()
{
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("No recording target media"));
}

void rec_tts_removed()
{
		char* str_buf;

	 	voice_prompt_abort();

		rec_tts_switch_to_speaker();
		
	 	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		str_buf = _("The media is removed.Removing media while Recording could destroy data on the card");
		voice_prompt_string_ex2(0, 
		get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
		&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf)); 

		//voice_prompt_music(1, &vprompt_cfg,VPROMPT_AUDIO_USB_OFF);
		rec_tts_close_error();
		
		rec_set_tts_end_state(TTS_END_STATE_EXIT);

}

void 
rec_tts(enum tts_type type)
{
	char* str_buf;
	char buf[256];

	/*if monitor is on, not the DAC, no tts report*/
	//if (get_mic_rec_monitor_state())
	//	return;
	if(rec_is_in_tts_exit_state()) {
		DBGMSG("will exit rec_tts not response\n");
		return;
	}
	
	switch (type) {
		case CHANGE_STATE_ERROR:
			//tts_speak(_("change state error"));
			break;

		case CHANGE_STATE_PAUSE:			
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = _("pause");
			//voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			//		tts_role, str_buf, strlen(str_buf)); 
			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf));
			break;

		case CHANGE_STATE_RECORDING:
			//tts_speak(_("recorder recording"));
			break;

		case CHANGE_STATE_PREPARE:
			//tts_speak(_("recorder prepared"));
			break;

		case TTS_NO_SDCARD:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			str_buf = _("No SD card");
			//voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			//		tts_role, str_buf, strlen(str_buf)); 
			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf));
			break;

		case TTS_SWITCH_MONITOR_ON:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = _("Monitor output ON");
			//voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			//		tts_role, str_buf, strlen(str_buf)); 
			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf));
			//rec_set_tts_end_state(TTS_END_STATE_MONITOR_ON);
			break;

		case TTS_SWITCH_MONITOR_OFF:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = _("Monitor output OFF");
			//voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			//		tts_role, str_buf, strlen(str_buf)); 
			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf));
			break;
			
		case TTS_USAGE:			
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Microphone recording.Press recording key to start recording, press play/stop key to stop recording");
			//voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			//		tts_role, str_buf, strlen(str_buf)); 
			
			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf));
			break;
			
		case REC_EXIT:			
			voice_prompt_abort();
			*tts_running = 1;
			memset(buf,0x00,256);
			snprintf(buf,256,"%s %s",_("Cancel"),_("close the recording mode"));

			rec_tts_switch_to_speaker();

			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 
			voice_prompt_music(1, &vprompt_cfg, cancel_path);// 退出，不需要切换到SLITONE
			
			rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;
		
		case ERROR_BEEP:
			error_beep();		//this just a beep
			break;

		case NORMAL_BEEP:
			normal_beep();
			break;

		case CANCEL_BEEP:
			cancel_beep();
			break;
		case REC_KEYLOCK:
			 voice_prompt_abort();
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 //voice_prompt_string2_ex2(0, 1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			//			 tts_lang, tts_role, _("Keylock"));
			 voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, _("Keylock"), strlen(_("Keylock")));
			break;
		case REC_AGC:
			// voice_prompt_string2_ex2(0, 1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			//			 tts_lang, tts_role, _("Detected sound"));
			 voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, _("Detected sound"), strlen(_("Detected sound")));
			 break;
		case REC_NOTARGET:
			 voice_prompt_abort();

			 rec_tts_switch_to_speaker();
			 
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("No recording target media"));

			 rec_tts_close_error();
			 
			 rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;
		case REC_NOTENOUGH:// 退出，不需要切换到SLITONE
			 voice_prompt_abort();

			 rec_tts_switch_to_speaker();

			 
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Not enough space on the target media"));

			 rec_tts_close_error();
			 rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;	
		case REC_WRITEERROR:
			 voice_prompt_abort();

			 rec_tts_switch_to_speaker();

			 
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Write error.It may not access the media correctly"));
			 rec_tts_close_error();
			 rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;	
		case REC_REMOVED:
			 voice_prompt_abort();
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			// voice_prompt_string2_ex2(0, 1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			//			 tts_lang, tts_role,
			//			 _("The media is removed.Removing media while Recording could destroy data on the card"));

			str_buf = _("The media is removed.Removing media while Recording could destroy data on the card");

			voice_prompt_string_ex2(0, 
				get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, str_buf, strlen(str_buf)); 
			break;	

		case REC_BIGMAX:// 退出，不需要切换到SLITONE
			 voice_prompt_abort();

			 rec_tts_switch_to_speaker();

			 
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("The file size reached to the upper limit.Cannot continue recording"));
			 rec_tts_close_error();

			rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;	

	   	case REC_STARTRECORD:
			voice_prompt_abort();
			//voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_music2(1, get_mic_rec_monitor_state() ? PLEXTALK_OUTPUT_SELECT_SIDETONE : PLEXTALK_OUTPUT_SELECT_DAC, 
				&vprompt_cfg, pu_path);
			
			rec_set_tts_end_state(TTS_END_STATE_START_RECORD);
			break;	

		case REC_READONLY:
			 voice_prompt_abort();

			 rec_tts_switch_to_speaker();

			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Read only media"));
			 rec_tts_close_error();

			rec_set_tts_end_state(TTS_END_STATE_EXIT);
			break;
	}
}

