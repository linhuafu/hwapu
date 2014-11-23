#include <stdio.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h> 
#include "radio_tts.h"
#include "radio_play.h"
#include "microwin/nano-X.h"
#include "radio_rec_create.h"
#include "radio_preset.h"
#include "radio_play.h"
#include "libvprompt.h"
#include "vprompt-defines.h"
#include "amixer.h"
#include "plextalk-i18n.h"
#include "plextalk-setting.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "libvprompt.h"


//#define _(S)	gettext(S)


/* Indicate the current Output Mux */
int road_flag;

/* indicate if tts is running */
static int tts_running = 0;

struct voice_prompt_cfg vprompt_cfg ;


extern int radio_mute_state;

void radio_tts_set_promptcfg()
{
	vprompt_cfg.volume    = g_config->volume.guide;
	vprompt_cfg.speed = g_config->setting.guide.speed;
//	vprompt_cfg.equalizer = g_config->setting.guide.equalizer;
}


void 
radio_tts_init (void)
{
	voice_prompt_init();
	voice_prompt_abort();
	CoolShmReadLock(g_config_lock);
	radio_tts_set_promptcfg();
	CoolShmReadUnlock(g_config_lock);	
}


void 
radio_reset_tts_cfg (void)
{	
	CoolShmReadLock(g_config_lock);
	radio_tts_set_promptcfg();
	CoolShmReadUnlock(g_config_lock);	
}


void radio_tts_change_volume(void)
{
	//volume = sys_get_global_voice_volume();
	//g_conifg->,,,,
}


void radio_tts_abort(void)
{
	voice_prompt_abort();
}


int* radio_tts_isrunning (void)
{
	return &tts_running;
}


void 
radio_tts_destroy (void)
{
	//voice_prompt_abort();		//this may abort VBUS's hotplug voice!@!
	voice_prompt_uninit();
}


void radio_tts_set_stopped (void)
{
	tts_running = 0;
}


int radio_tts_get_stopped (void)
{
	return !tts_running;
}


static char*
get_freq_str (int data)
{
	static char buf[64];
	double freq = (double)data / 10;

	snprintf(buf, 64, "%g%s", freq, _("mega hertz")); 

	return buf;
}


/* TTS string of preset station success */
static char*
get_preset_success_str(int data)
{
	static char buf[128] = {0};
	// call function radio_get_freq() is not suitable
	snprintf(buf, 128, _("registered %g mega hertz to channel %d"), radio_get_freq(), data);
	return buf;
}


static char*
get_channel_found_str (int data)
{
	static char buf[128];

	snprintf(buf, 128, "%s%d%s%g%s", _("channel"), data, _("frequency"), radio_get_freq(), _("mega hertz"));

	return buf;
}


static char*
get_del_station_str (int data)
{
	static char buf[128];

	snprintf(buf, 128, "%s%g%s", _("frequency"), radio_get_freq(), _("delete from channel"));
	
	return buf;
}


/* Radio Main TTS */
void radio_tts (enum tts_type type, int data)
{
	char* str_buf;
	int select;

	select = radio_mute_state ? PLEXTALK_OUTPUT_SELECT_DAC : PLEXTALK_OUTPUT_SELECT_BYPASS;
		  
	DBGMSG("Radio tts, Output May set to Bypass!!!\n");
	
	switch (type) {
		case RADIO_MODE_START:
			/* This won't give a beep */
			str_buf = _("radio mode");
			tts_running = 1;
			
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_AUTO_FAIL:
			voice_prompt_abort();
			str_buf = _("Cannot find the available frequency");
			tts_running = 1;
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_CUR_FREQ:
			voice_prompt_abort();
			str_buf = get_freq_str(data);
			tts_running = 1;
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));			
			break;
			
		case RADIO_TTS_NO_SDCARD:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("No recording target media");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));			
			break;
			
		case RADIO_SET_STATION_SUCCESS:			
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf = get_preset_success_str(data);
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_SET_STATION_ALREADY:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The frequency is already set to the preset channel.");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_SET_STATION_MAX_NUMS:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The preset channel has reached the maximum.");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_CHANNEL_NOTFOUND:			
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("no preset channels");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_CHANNEL_FOUND:	
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf = get_channel_found_str(data);
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_DEL_STATION_SUCCESS:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf = get_del_station_str(data);
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_DEL_STATION_FAILURE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("no frequency delete from channel");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_DEL_ALL_STATION:			
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("delete all the preset channels");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		/* Recorder TTS */
		case RECORD_USAGE:
			voice_prompt_abort();
			tts_running = 1;
			str_buf = _("Radio recording.Press recording key to start recording, press play/stop key to stop recording");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RECORD_EXIT_ERROR:
			voice_prompt_abort();
			tts_running = 1;
			str_buf = _("close the recording mode");
			voice_prompt_string_ex2(0, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music2(1,select,&vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
			
		case RECORD_EXIT:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf = _("Cancel");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("close the recording mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music2(1, select, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
			
		case RADIO_SET_MUTE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			str_buf = _("mute");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case CHANGE_STATE_ERROR:
			//tts_speak(_("Change state error!"));
			break;
			
		case RADIO_REC_SD_REMOVE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The media is removed.Removing media while Recording could destroy data on the card");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;	
			
		case CHANGE_STATE_RECORDING:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music2(1, select, &vprompt_cfg, VPROMPT_AUDIO_PU);
			break;

		case CHANGE_STATE_PAUSE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf = _("pause");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case RADIO_REC_MONITOR_SWITCH:
			voice_prompt_abort();
			tts_running = 1;
			str_buf = data ? _("Monitor output ON") : _("Monitor output OFF");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_REC_SD_READONLY:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf =  _("Read only media");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_REC_LIMIT:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf =  _("The file size reached to the upper limit.Cannot continue recording");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_REC_WRITE_ERR:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf =  _("Write error.It may not access the media correctly");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case RADIO_REC_NOSPACE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf =  _("Not enough space on the target media");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
			
		case CHANGE_STATE_PREPARE:
			//tts_speak(_("Record prepared!"));
			break;

		/* TTS auto scan of Radio */
		case AUTO_SCAN_ING:			
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf =  _("already in auto scan");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;

		case AUTO_SCAN_BEGIN:			
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			str_buf =  _("Auto search");
			voice_prompt_string_ex2(1, select, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			break;
		
		case TTS_ERROR_BEEP:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music2(1, select, &vprompt_cfg, VPROMPT_AUDIO_BU);
			break;

		case TTS_NORMAL_BEEP:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music2(1, select, &vprompt_cfg, VPROMPT_AUDIO_PU);
			break;
			
		case TTS_CANCEL_BEEP:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music2(1, select, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
	}
}

