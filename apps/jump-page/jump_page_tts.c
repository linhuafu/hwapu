#include <stdio.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "jump_page_tts.h"

#include "microwin/nano-X.h"
#include "plextalk-config.h"
#include "Sysfs-helper.h"
#include "vPrompt-defines.h"

#include "libvprompt.h"

#define OSD_DBG_MSG
#include "nc-err.h"


#define _(S)   gettext(S)

/* The path for each beep */
static char bu_path[256];
static char pu_path[256];
static char cancel_path[256];

static int tts_exit_flag = 0;
/**/
static int* tts_is_running = NULL;

static int page_hp_online;
int page_handle_fd;

extern ivUInt32 tts_role;
extern ivUInt32 tts_lang;

//struct plextalk_guide guide_cache;
//struct plextalk_volume volume_cache[2];
struct voice_prompt_cfg vprompt_cfg;

void page_tts_set_stop (void)
{
	*tts_is_running = 0;
}

int page_tts_get_stop (void)
{
	return *tts_is_running;
}

int page_get_exit_flag()
{
	return tts_exit_flag;
}

/* 回调函数 */
void fd_read(void *pdate)
{
   struct signalfd_siginfo fdsi;

   ssize_t ret = read(page_handle_fd, &fdsi, sizeof(fdsi));

   //if (ret != sizeof(fdsi))
	//	handle_error();
   DBGLOG("page fd_read  tts_exit_flag:%d\n",tts_exit_flag);
   page_tts_set_stop();

   if(tts_exit_flag)
   {
		tts_exit_flag = 0;
		NxAppStop();
   }
//	close(rec_handle_fd);
}

void page_on_tts_end_callback (void)
{
	//char* pdata;	 //private data
	int sta = 0;

	/* 得到fd */
	page_handle_fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(page_handle_fd, &sta, fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(page_handle_fd, 1, 0);

	DBGLOG("page_on_tts_end_callback \n");
}

void
page_init_tts_prop()
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
	//sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &page_hp_online);
	

	plextalk_update_tts(getenv("LANG"));

	DBGLOG("rec_init_tts_prop VOL:%d\n",vprompt_cfg.volume);

}

void 
page_tts_init (int* is_running)
{	

	snprintf(bu_path, 256, VPROMPT_AUDIO_BU);
	snprintf(pu_path, 256, VPROMPT_AUDIO_PU);
	snprintf(cancel_path, 256, VPROMPT_AUDIO_CANCEL);

	tts_is_running = is_running;
	page_on_tts_end_callback();

	page_init_tts_prop();
	voice_prompt_init();
	voice_prompt_abort();
}


void 
page_tts_destroy (void)
{
	voice_prompt_abort();
	//voice_prompt_uninit();
}


void 
cancel_beep (void)
{
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(1, &vprompt_cfg, cancel_path);
}


void
error_beep (void)
{	
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(1, &vprompt_cfg, bu_path);
}


void 
normal_beep (void)
{	
	voice_prompt_abort();
	*tts_is_running = 1;
	voice_prompt_music(1, &vprompt_cfg, pu_path);
}


static const char* 
get_element_str (const char* data)
{
	const char* p = NULL;
	
	switch (*data) {
		case 'E':
			p = _("Enter");
			break;
			
		case 'C':
			p = _("clear");
			break;
			
		default:
			p = data;
			break;
	}
	return p;
}


void 
page_tts (enum tts_type type, char* data)
{

	DBGMSG("[in]Enter page_tts!\n");
	//return;
 
	char* str_buf = NULL;
	char buf[256];

	if(tts_exit_flag)
	{
		DBGMSG("will not reponse any key pagetts will exit!!\n");
		return;
	}
	
	switch (type) {

		case TTS_ENTER_JUMP_PAGE:
			voice_prompt_abort();
			str_buf = _("Jump by page mode");
			*tts_is_running = 1;
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;

		case TTS_ELEMENT:			
			voice_prompt_abort();
			str_buf = (char*)get_element_str(data);
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;

		case TTS_ERROR:	
			voice_prompt_abort();
			str_buf = _("error!");
			*tts_is_running = 1;
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf); 
			break;

		case TTS_NUMBER:			//this tts report the operator number
			voice_prompt_abort();
			str_buf = data;
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf);
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

		case TTS_INPUTSET:
			voice_prompt_abort();
			if (!data)
				str_buf = _("No number");
			else
				str_buf = (char*)get_element_str(data);
			
			memset(buf,0x00,256);
			snprintf(buf,256,"%s %s",str_buf,_("Set"));
			
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf));  
			break;

		case TTS_CLEARALL:
			voice_prompt_abort();
			memset(buf,0x00,256);
			snprintf(buf,256,"%s: %s",_("clean all"),_("Set"));
			
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf, strlen(buf)); 
			break;

		case TTS_NOINPUT:
			voice_prompt_abort();	

			str_buf = _("no number");
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
			break;
		case TTS_KEYLOCK:
			 voice_prompt_abort();
			 *tts_is_running = 1;
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			 voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Keylock"));
			 break;
		case TTS_BACKCANCEL:
			voice_prompt_abort();

			if (!data)
				str_buf = _("No number");
			else
				str_buf = data;
			
			*tts_is_running = 1;
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			 voice_prompt_string2(0 , &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Cancel"));
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf); 
			break;
		case TTS_TOPFIGURE:
			 voice_prompt_abort();
			 *tts_is_running = 1;
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			 voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("Top figure"));
			break;

		case TTS_MAXINPUTVALUE:
			 voice_prompt_abort();
			 *tts_is_running = 1;
			 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			 voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						 tts_lang, tts_role, _("The figures number reached to the upper limit."));
			break;

		case TTS_EXIT_JUMP_PAGE:
			voice_prompt_abort();
			*tts_is_running = 1;
			//voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, _("Colse the menu.")); 
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			tts_exit_flag = 1;
			break;
		case TTS_FAILED_MOVE_PAGE:
			voice_prompt_abort();
			memset(buf,0x00,256);
			snprintf(buf,256,"%s: %s",_("Failed to move the page."),_("Clear."));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf); 
			break;
			
		case TTS_THERE_NO_PAGE:
			voice_prompt_abort();
			memset(buf,0x00,256);
			snprintf(buf,256,"%s: %s",_("There is no page."),_("Colse the menu."));
			*tts_is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, buf);
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			tts_exit_flag = 1;
			break;
		case TTS_PLEASE_WAIT:
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Please wait."));

	}
}
