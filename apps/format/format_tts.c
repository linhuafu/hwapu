#include <stdio.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "format_tts.h"

#include "microwin/nano-X.h"
#define  DEBUG_PRINT	1

#include "plextalk-config.h"
#include "Sysfs-helper.h"
#include "vPrompt-defines.h"

#include "libvprompt.h"


#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


#define _(S)   gettext(S)

/* The path for each beep */
static char bu_path[256];
static char pu_path[256];
static char cancel_path[256];
int format_handle_fd;
static enum tts_end_state g_tts_end_callback;


/**/
static int* tts_is_running = NULL;

static int cal_hp_online;
int cal_handle_fd;

extern ivUInt32 tts_role;
extern ivUInt32 tts_lang;

//struct plextalk_guide guide_cache;
//struct plextalk_volume volume_cache[2];
struct voice_prompt_cfg vprompt_cfg;

static void format_tts_set_stop (void)
{
	*tts_is_running = 0;
}

static int format_tts_get_stop (void)
{
	return *tts_is_running;
}

enum tts_end_state format_get_tts_end_flag(void)
{
	return g_tts_end_callback;
}
enum tts_end_state format_set_tts_end_flag(enum tts_end_state status)
{
	enum tts_end_state old = g_tts_end_callback;
	g_tts_end_callback = status;
	return old;
}



void
format_init_tts_prop()
{
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
	//sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &cal_hp_online);
	

	plextalk_update_tts(getenv("LANG"));

	DBGLOG("rec_init_tts_prop VOL:%d\n",vprompt_cfg.volume);

}

/* 回调函数 */
void fd_read(void *pdate)
{
   struct signalfd_siginfo fdsi;

   ssize_t ret = read(format_handle_fd, &fdsi, sizeof(fdsi));

   //if (ret != sizeof(fdsi))
	//	handle_error();

   format_tts_set_stop();

	DBGLOG("fd_read func:%d\n",g_tts_end_callback);

   if(g_tts_end_callback == TTS_END_STATE_EXIT || TTS_END_STATE_SUCCESS == g_tts_end_callback)
   {	
   		 DBGLOG("!!wait_format_finish!!\n");
   		 wait_format_finish();
		 DBGLOG("format finish !\n");
		 g_tts_end_callback = TTS_END_STATE_NONE;
		 NxAppStop();
   }


   

//	close(rec_handle_fd);
}



void format_on_tts_end_callback (void)
{
	//char* pdata;	 //private data
	int sta = 0;

	/* 得到fd */
	format_handle_fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(format_handle_fd, &sta, fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(format_handle_fd, 1, 0);

	DBGLOG("format_on_tts_end_callback \n");
}



void 
format_tts_init (int* is_running)
{	

	snprintf(bu_path, 256, VPROMPT_AUDIO_BU);
	snprintf(pu_path, 256, VPROMPT_AUDIO_PU);
	snprintf(cancel_path, 256, VPROMPT_AUDIO_CANCEL);

	tts_is_running = is_running;
	//cal_on_tts_end_callback();

	format_init_tts_prop();
	voice_prompt_init();

	format_on_tts_end_callback();
		
	voice_prompt_abort();
}


void 
format_tts_destroy (void)
{
	voice_prompt_abort();
	//voice_prompt_uninit();
}


void 
static cancel_beep (void)
{
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(0, &vprompt_cfg, cancel_path);
}


void
static error_beep (void)
{	
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(0, &vprompt_cfg, bu_path);
}


void 
static normal_beep (void)
{	
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(0, &vprompt_cfg, pu_path);
}


void 
format_tts (enum tts_type_format type)
{

	DBGMSG("[in]Enter format_tts!\n");
	//return;
 
	char* str_buf = NULL;
	char buf[256];
	
	switch (type) {

		case TTS_ENTER_FORMAT:
			voice_prompt_abort();
			str_buf = _("Enter");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Please wait"));
			*tts_is_running = 1;
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 
			break;

		case TTS_REMOVE_ERROR:	
			voice_prompt_abort();
			str_buf = _("The media is removed.Removing media while accessing could destroy data on the card.");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Close the menu"));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 

			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			
			g_tts_end_callback = TTS_END_STATE_EXIT;
			break;
		case TTS_ERROR:	
			voice_prompt_abort();
			str_buf = _("error!");
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			g_tts_end_callback = TTS_END_STATE_EXIT;
			break;	
		case TTS_SUCCESS:	
			voice_prompt_abort();
			str_buf = _("Finished");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Close the menu"));
			*tts_is_running = 1;
			
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 
			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			
			g_tts_end_callback = TTS_END_STATE_SUCCESS;
			break;		
		case TTS_READONLY:
			voice_prompt_abort();
			str_buf = _("Read only media");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Close the menu"));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 

			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			
			g_tts_end_callback = TTS_END_STATE_EXIT;
			break;
		case TTS_MEDIALOCKED:
			voice_prompt_abort();
			str_buf = _("The media is locked");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Close the menu"));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 

			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			
			g_tts_end_callback = TTS_END_STATE_EXIT;
			break;	
		case TTS_WRITEERROR:
			voice_prompt_abort();
			str_buf = _("Write error.It may not access the media correctly");
			memset(buf,0x00,256);
			snprintf(buf,256,"%s:%s",str_buf,_("Close the menu"));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 

			voice_prompt_music(1, &vprompt_cfg, cancel_path);
			
			g_tts_end_callback = TTS_END_STATE_EXIT;
			break;

		case TTS_FORMAT_WAIT_BEEP:	
			voice_prompt_abort();
			*tts_is_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			break;
		case TTS_FORMAT_WAIT_BEEP_NO_ABORT:	
			//voice_prompt_abort();
			*tts_is_running = 1;
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			break;	
		case TTS_FORMAT_WAIT:
			voice_prompt_abort();
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, _("Processing, please wait.")); 

			break;		

		case TTS_FORMAT_LOCK:
			 voice_prompt_abort();
			 *tts_is_running = 1;
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string_ex2(1, 
				PLEXTALK_OUTPUT_SELECT_DAC,
				&vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role, _("Keylock"), strlen(_("Keylock")));
			break;	
		case TTS_FORMAT_MENU_WAIT:
			voice_prompt_abort();
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, _("Processing, please wait.")); 
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			break;
	}
}
