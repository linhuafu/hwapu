#include <stdio.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "backup_tts.h"

#include "microwin/nano-X.h"

#include "plextalk-config.h"
#include "Sysfs-helper.h"
#include "vPrompt-defines.h"

#include "libvprompt.h"


#define OSD_DBG_MSG
#include "nc-err.h"




#define _(S)   gettext(S)



/**/
static int tts_running = 0;


struct voice_prompt_cfg vprompt_cfg;

void
backup_init_tts_prop()
{	
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed = g_config->setting.guide.speed;
	//vprompt_cfg.equalizer = g_config->setting.guide.equalizer;

}

void backup_tts_init ()
{	
	tts_running = 0;
	voice_prompt_init();
	voice_prompt_abort();
	backup_init_tts_prop();
}

void backup_tts_destroy (void)
{
	//voice_prompt_abort();		//this may abort VBUS's hotplug voice!@!
	voice_prompt_uninit();
}

void cancel_beep (void)
{
	voice_prompt_abort();
	tts_running = 1;
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
}

void error_beep (void)
{	
	voice_prompt_abort();
	tts_running = 1;
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_BU);
}

void normal_beep (void)
{	
	voice_prompt_abort();
	tts_running = 1;
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
}

void backup_tts_set_stopped (void)
{
	tts_running = 0;
}

int backup_tts_get_stopped(void)
{
	return !tts_running;
}

int *backup_get_ttsstate(void)
{
	return &tts_running;
}

void backup_tts (enum tts_type type, float data)
{

	DBGMSG("[in]Enter backup_tts!\n");
	
	char* str_buf = NULL;
	
	switch (type) {

		case TTS_ENTER_BACKUP:
			//voice_prompt_abort();
			tts_running = 1;
			str_buf = _("File processing,please wait");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			break;
		case TTS_BG_MP3:
			//voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			break;
		case TTS_MEN_KEY:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("File processing,please wait");
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));			
			break;
		case TTS_MAX_FILELEVEL:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("Cannot paste to the directory.Maximum directory level is 8.");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_MEDIA_LOCKED:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The media is locked");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_REMOVE_MEDIA_READ:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The media is removed.Removing media while accessing could destroy data on the card.");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("Read error.It may not access the media correctly.");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_REMOVE_MEDIA_WRITE:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("The media is removed.Removing media while accessing could destroy data on the card.");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("Write error.It may not access the media correctly");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_CDDA_WRITE_ERROR:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("Write error.It may not access the media correctly");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_NOTMEM:
			voice_prompt_abort();
			tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			str_buf = _("Not enough space on the target media");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_FAILBACKUP:
			voice_prompt_abort();
			tts_running = 1;
			str_buf = _("Failed to backup");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);			
			break;
		case TTS_COMPELTE:
			voice_prompt_abort();
			str_buf = _("backup completed");
			tts_running = 1;
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			str_buf = _("Close the backup mode");
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		case TTS_ELEMENT:
			{
				char present[128];
				memset(present, 0x00, 128);
				voice_prompt_abort();
				sprintf(present, "%f%%", data);
				DBGMSG("present = %s\n", present);
				tts_running = 1;
				voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
						tts_role, present, strlen(present)); 
			}
			break;
		case TTS_ERROR:	
			voice_prompt_abort();
			str_buf = _("error!");
			tts_running = 1;
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
		case TTS_ERRBEEP:		
			error_beep();
			break;
		case TTS_NORBEEP:
			normal_beep();
			break;
		case TTS_CANCEL:
			cancel_beep();
			break;
	}
}
