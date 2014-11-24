#include <stdio.h>
#include <libintl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "microwin/nano-X.h"
#define DEBUG_PRINT 1
#include "info_print.h"
#include "widgets.h"
#include "book_prompt.h"
#include "libvprompt.h"
#include "Shared-mem.h"
#include "Vprompt-defines.h"
#include "Plextalk-config.h"
#include "Plextalk-setting.h"
#include "Sysfs-helper.h"
#include "Plextalk-constant.h"
#include "Plextalk-i18n.h"
#include "dbmalloc.h"

/* tts is running */
static int* tts_running = NULL;

struct voice_prompt_cfg vprompt_cfg;

void book_prompt_settings(void)
{
	CoolShmReadLock(g_config_lock);
	vprompt_cfg.speed = g_config->setting.guide.speed;
	vprompt_cfg.volume = g_config->volume.guide;
	CoolShmReadUnlock(g_config_lock);
	info_debug("speed=%d\n",vprompt_cfg.speed);
	info_debug("volume=%d\n",vprompt_cfg.volume);
}

void book_prompt_init (int* is_running)
{
	book_prompt_settings();
	
	voice_prompt_init();
	
	tts_running = is_running;
	*tts_running = 0;
}

void book_prompt_destroy (void)
{
	//voice_prompt_abort();		//this may abort VBUS's hotplug voice!@!
	voice_prompt_uninit();
}

void book_prompt_stop(void){
	*tts_running = 0;
	voice_prompt_abort();
}

void book_prompt_play(enum tts_type type,char *prompt)
{
	char* str_buf;

	switch (type) {
		case BOOK_MODE_OPEN:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Book mode");
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;

		case BOOK_TITLE_OPEN:
			//voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Preparing for playback.");
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;

		case BOOK_OPEN_ERR:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Failed to prepare for playback.");
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
			
		case BOOK_OPEN_ERR_NOT_ENOUGH_WORK_AREA:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("The content size is too large. Can not play this content");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
			
		case BOOK_OPEN_ERR_CONVERT_CHARSET:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("The content code page is not match. Can not play this content");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
		case BOOK_BEGIN:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Beginning of book.");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
			
		case BOOK_END:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("End of book.");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
			
		case BOOK_KEYLOCK:
			voice_prompt_abort();
			*tts_running = 1;
			str_buf = _("Keylock");
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
			
		case ERROR_BEEP:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_BU);
			break;

		case NORMAL_BEEP:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
			break;

		case CANCEL_BEEP:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			break;
		
		case LIMIT_BEEP:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_KON);
			break;

		case BOOK_PROMPT:
			if(0 !=prompt && 0!=strlen(prompt))
			{
				voice_prompt_abort();
				*tts_running = 1;
				voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
						tts_role, prompt, strlen(prompt)); 
			}
			break;
		case BOOK_VOLUMN:
			voice_prompt_abort();
			*tts_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(0 !=prompt && 0!=strlen(prompt)){
				voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, prompt, strlen(prompt)); 
			}
			break;
		default:
			break;
	}
}

