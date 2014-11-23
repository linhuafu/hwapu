#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "microwin/nano-X.h"
#define  DEBUG_PRINT	1
//#include "info_print.h"
#include "audio_tts.h"
//#include "voice.h"
//#include "ttsplus.h"
#include "audio_play.h"
#include "audio_extern.h"
#include "load_preset.h"
//#include "keydef.h"
//#include "widget.h"
#include "widgets.h"
//#include "system.h"
//#include "sys_config.h"
//#include "tipvoice.h"
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "resource.h"
#include "libvprompt.h"
#include "plextalk-i18n.h"
#include "vprompt-defines.h"
#include "audio_draw.h"
#include "cld.h"


//#include "dmalloc.h"




#define _(S)    gettext(S)


static char bu_path[256];
static char pu_path[256];
static char cancel_path[256];
static char usb_on_path[256];
static char usb_off_path[256];
static char power_on_path[256];
static char power_off_path[256];
static char kon_path[256];


extern struct nano_data nano;

static volatile int* is_running;


extern struct audio_player player;



 struct voice_prompt_cfg vprompt_cfg;



char *sys_get_res_dir(void)
{
	char *respath = (char *)malloc(512);

	snprintf(respath, 512, "/opt/res");

	return respath;
}

char *sys_get_bin_dir(void)
{
	char *binpath = (char *)malloc(512);


	snprintf(binpath, 512, "/opt/bin");

	DBGMSG("bin path=%s\n", binpath);
	return binpath;

}



void 
audio_tts_init (int* tts_running)
{	


	strcpy(bu_path,VPROMPT_AUDIO_BU);
	strcpy(pu_path, VPROMPT_AUDIO_PU);
	strcpy(cancel_path,  VPROMPT_AUDIO_CANCEL);
	strcpy(kon_path,VPROMPT_AUDIO_KON);


	strcpy(usb_on_path, VPROMPT_AUDIO_USB_ON );
	strcpy(usb_off_path,VPROMPT_AUDIO_USB_OFF );

	strcpy(power_on_path, VPROMPT_AUDIO_POWER_ON );
	
	strcpy(power_off_path, VPROMPT_AUDIO_POWER_OFF );
	

//	audio_role = sys_get_cur_tts_lang();
	//audio_volume = sys_get_global_voice_volume();

	
	voice_prompt_init();

//	voice_prompt_abort();	这里可能打断desktop的pu的声音
	
	is_running = tts_running;
	vprompt_cfg.volume = 12;
	vprompt_cfg.speed = 1;
	//vprompt_cfg.equalizer = 1;

	*is_running = 0;
}

/*void audio_tts_change_volume(void)
{
	audio_volume = sys_get_global_voice_volume();
}*/


void 
audio_tts_destroy (void)
{
	//voice_prompt_abort();		//this may abort VBUS's hotplug voice!@!
	voice_prompt_uninit();
}


/* Wait for tts stopped */
void 
wait_tts (void) 
{	
	while (1)
	{
		if (!(*is_running))
			break;

		usleep(20000);
	}
}


static void
usb_on_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, usb_on_path);
}


static void
usb_off_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, usb_off_path);
}




static void
power_off_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, power_off_path);
}


static void
power_on_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, power_on_path);
}


 void
normal_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, pu_path);
}


 void 
error_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, bu_path);
}


 void 
volume_error_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, kon_path);
}


static void
cancel_beep (void)
{
	voice_prompt_abort();
	*is_running = 1;
	voice_prompt_music(1,  &vprompt_cfg, cancel_path);
}
	

static char* 
get_navi_str (int data)
{
	char* navi_str[] = {_("bookmark"),_("30seconds"),
						_("10minutes"),_("track"),
						_("album")};	

	if ((data < 0) || (data > 4)) 
	{
		DBGMSG("ERROR: return NULL!\n");
		
		data=0;
	}

	return navi_str[data];
}





void 
get_volume_str (char *pstrl,char *pvol,int vol)
{

	char numstr[10]={0};
	
	DBGMSG("Enter  get_volume_str!\n");
	
	//strcpy(pstrl,pvol);

	//sprintf(numstr, "%d", vol);
	
	sprintf(pstrl, "%s %d",_("Volume"), vol);


//	DBGMSG("Enter  get_volume_str=%s!\n",numstr);
	
//	strcat(pstrl,numstr);
	
	
	DBGMSG("vol str=%s\n",pstrl);
}


static char* 
get_seek_bookmark_str (int data, int* beep_type)
{
	static char buf[64];	

	if (data == -1) {
		*beep_type =  0;
		return _("no bookmark");
	}
	else  
		snprintf(buf, 64, "%s%d", _("bookmark"), data);
	
	*beep_type = 1;
	return buf;
}


static char*
get_repeat_mode_str (int data)
{
	char* mode_str[] = { _("Off"), _("Track repeat"), _("Album repeat"),
						 _("All album repeat"), _("Shuffle repeat")};

	if ((data < 0) || (data > 4)) {
		DBGMSG("ERROR:return NULL!\n");
		return NULL;
	}
	
	return mode_str[data];
}


static char*
get_pause_str (int data)
{
	char* p_str[] = {_("pause"), _("recover")};

	if ((data < 0) || (data > 1)) {
		DBGMSG("ERROR: return NULL!\n");
		return NULL;
	}
	
	return p_str[data];
}


// 0 represent for success , 1 represet failure
static char*
get_del_track_str (int data, int* beep_type)
{
	char* del_str[] = {_("delete track success"), _("delete track failure")};

	if ((data < 0) || (data > 1)) {
		DBGMSG("ERROR: return  NULL!\n");
		return NULL;
	}

	if (data == 0)
		*beep_type = 1;
	else
		*beep_type = 0;

	return del_str[data];
}


static char*
get_del_all_bmk_str (int data, int* beep_type)
{
	if (data == -1) {
		*beep_type = 0;
		return _("no bookmark");		//这里是不是别这样提示
	}
	else {
		*beep_type = 1;
		return _("all bookmark removed.");
	}
}


static char*
get_del_cur_bmk_str (int data, int* beep_type)
{	
	static char buf[64];
	
	if (data == -1) {
		*beep_type = 0;
		return _("no bookmark");
	}
	else {
		snprintf(buf, 64, "%s%d%s", "bookmark", data, "removed");	
		*beep_type = 1;
		return buf;
	}
}


static char* 
get_del_album_str (int data, int* beep_type)
{
	char* album_str[] = {_("delete album success"), _("delete album failure")};

	if ((data < 0) || (data > 1)) {
		DBGMSG("ERROR: return NULL!\n");
		return NULL;
	}

	if (data)
		*beep_type = 0;
	else
		*beep_type = 1;

	return album_str[data];
}


//0 represet success, 1 represet failure
static char*
get_set_bmk_str (int data, int* beep_type)
{
	char* bmk_str[] = {_("set bookmark success"), _("set bookmark failure")};

	if ((data < 0) || (data > 1)) {
		DBGMSG("ERROR: return NULL!\n");
		return NULL;
	}

	if (data)
		*beep_type = 0;
	else
		*beep_type = 1;

	return bmk_str[data];
}


static char*
get_track_name (int data)
{
	const char* fpath = (const char*)data;

	char* title_p = strrchr(fpath, '/');

	if (!title_p) {
//		printf("There is no / in the title");
		return _("error");							//this shoulde never happend
	}

	return title_p + 1;
}


static char* 
tts_album_name (char* dir_path)
{
//你这里不能修改的
//这里的dir_path该声明为const,是不能修改这个的
	static char album_name[256];
	char src_dir[256];

	memset(src_dir, 0, 256);
	strncpy(src_dir, dir_path, 256);
	
	memset(album_name, 0, 256);
	
	int size = strlen(src_dir);
	if (*(src_dir + size - 1) == '/') {
		*(src_dir + size - 1) = 0;
	}
	char* p = strrchr(src_dir, '/');
	int dir_size = p - src_dir + 1;
	int name_size = size - dir_size;
	strncpy(album_name, p + 1, name_size);

	return album_name;
}


static char*
get_album_name (int data)
{
//	printf("[tts]:data = %s\n",(char*)data);
	
	char* res = tts_album_name((char*)(data));

//	printf("[tts]get album name = %s\n", res);

	return res;
}


static char*
get_speed_set_str(int data)
{
#if 0
	static char buf[64];
	snprintf(buf, 64,"%s%d%s", _("new speed:"), data, _("percent."));
	return buf;
#endif
	return _("Set speed");
}


static char*
get_equ_str (int data)
{
	return (char*)effect_string(data); 
}




int voice_prompt_string_music(char* str)
{
	int lang;
	int len;

	len=strlen(str);
	
//	 lang = CldDetectLanguage(str, len);

	 lang =  VoicePromptDetectLanguage(str, len);

#if 0
		switch (lang)
		{
		case ENGLISH:
			lang = ivTTS_LANGUAGE_ENGLISH;
			break;
		case CHINESE:
			lang = ivTTS_LANGUAGE_CHINESE;
			break;
		case HINDI:
			lang = ivTTS_LANGUAGE_HINDI;
			break;
		default:
			/* Oh, my god! I don't support the language now */
			lang = tts_lang;
			break;
		}
		
#endif

		DBGMSG("enter voice_prompt_string_music fun volume  =%d   speed =%d\n",vprompt_cfg.volume,vprompt_cfg.speed);

		if (vprompt_cfg.volume < 0 ||vprompt_cfg.volume > 25)
		{
			vprompt_cfg.volume=10;

		}
		if (vprompt_cfg.speed < -2 || vprompt_cfg.speed > 8)
		{
			vprompt_cfg.speed=0;
		}
				
		voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,lang, plextalk_get_tts_role(lang), str, len);
					

}

	


	
//all the tts should be played via data
void 
audio_tts (enum tts_mode mode, int data)
{


	char * str_buf;
	char  volume_buf[50];
		
	Break_Music(&player,0,0);	
	
	int beep_type = 0;
	switch (mode)
	{
		case TTS_LOGO:
			/* This won't give a beep */
			voice_prompt_abort();
			str_buf = _("music mode");
			*is_running = 1;
			DBGMSG("enter   TTS_LOGO branch =%s!\n",str_buf);

			voice_prompt_string_music(str_buf);

			
		
			
			break;
		
		case TTS_PREPARE:
			/* Tts prepare */
			break;

		case TTS_KEYLOCK:
			
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			str_buf = _("key lock");
			DBGMSG("audio_tts TTS_KEYLOCK!\n");
			voice_prompt_string_music(str_buf);
			
		
			
			break;

			
		case TTS_WAIT:
			DBGMSG("audio_tts TTS_WAIT!\n");
			voice_prompt_abort();
			str_buf = _("Please Wait");
			*is_running = 1;
			
			voice_prompt_string_music(str_buf);
			
			break;
 			
		case TTS_NAVIGATION:
			DBGMSG("audio_tts TTS_NAVIGATION!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0,  &vprompt_cfg, pu_path);
			str_buf = get_navi_str(data);
			voice_prompt_string_music(str_buf);
			
			break;

        case TTS_VOLUME:
        		DBGMSG("audio_tts TTS_VOLUME!\n");
        		voice_prompt_abort();
        		memset(volume_buf,0x00,sizeof(volume_buf));
			*is_running = 1;
			voice_prompt_music(0,  &vprompt_cfg, pu_path);
			
			get_volume_str(volume_buf,_("Volume"),data);
			voice_prompt_string_music(volume_buf);
			
			break;
			

        		
		case TTS_SEEK_BOOKMARK:
			DBGMSG("audio_tts TTS_SEEK_BOOKMARK!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_seek_bookmark_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0,  &vprompt_cfg, pu_path);
			else 
				voice_prompt_music(0,  &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf);

			
			break;

		case TTS_SEEK_TRACK:
			DBGMSG("audio_tts TTS_SEEK_TRACK!\n");
			voice_prompt_abort();
			*is_running = 1;
			DBGMSG(" Enter  TTS_SEEK_TRACK =%s\n",pu_path);
		//	voice_prompt_music(0,  &vprompt_cfg, pu_path);
			str_buf = get_track_name(data);
			DBGMSG(" Enter  TTS_SEEK_TRACK =%s\n",str_buf);
			voice_prompt_string_music(str_buf);
			
			break;

		case TTS_SEEK_ALBUM:
			DBGMSG("audio_tts TTS_SEEK_ALBUM!\n");
			voice_prompt_abort();
			*is_running = 1;
			
			voice_prompt_music(0,  &vprompt_cfg, pu_path);
			str_buf = get_album_name(data);
			voice_prompt_string_music(str_buf);
			DBGMSG(" Enter  TTS_SEEK_ALBUM END =%s\n",str_buf);
		
			break;

		case TTS_PAUSE_RECOVER:
			DBGMSG("audio_tts TTS_PAUSE_RECOVER!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = get_pause_str(data);
			voice_prompt_string_music(str_buf);
			
			break;

		case TTS_DELETE_TRACK:
			DBGMSG("audio_tts TTS_DELETE_TRACK!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_del_track_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0, &vprompt_cfg, pu_path);
			else
				voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf);
			
			break;

		case TTS_REPEAT_MODE:		
			DBGMSG("audio_tts TTS_REPEAT_MODE!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = get_repeat_mode_str(data);
			voice_prompt_string_music(str_buf);
			
			break;

		case TTS_DELETE_CUR_BMK:
			DBGMSG("audio_tts TTS_DELETE_CUR_BMK!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_del_cur_bmk_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0, &vprompt_cfg, pu_path);
			else
				voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf); 

			
			break;
			
		case TTS_DELETE_ALL_BMK: 

			DBGMSG("audio_tts TTS_DELETE_ALL_BMK!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_del_all_bmk_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0, &vprompt_cfg, pu_path);
			else
				voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf);

		
			break;

		case TTS_DELETE_ALBUM:

			DBGMSG("audio_tts TTS_DELETE_ALBUM!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_del_album_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0, &vprompt_cfg, pu_path);
			else
				voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf);

		
			break;

		case TTS_SPEED_SET:

			DBGMSG("audio_tts TTS_SPEED_SET!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = get_speed_set_str(data);
			voice_prompt_string_music(str_buf);

		
			break;

		case TTS_SET_BMK:

			DBGMSG("audio_tts TTS_SET_BMK!\n");
			voice_prompt_abort();
			*is_running = 1;
			str_buf = get_set_bmk_str(data, &beep_type);
			if (beep_type)
				voice_prompt_music(0, &vprompt_cfg, pu_path);
			else
				voice_prompt_music(0, &vprompt_cfg, bu_path);
			voice_prompt_string_music(str_buf);

			
			break;

		case TTS_SET_EQU:		

			DBGMSG("audio_tts TTS_SET_EQU!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, pu_path);
			str_buf = get_equ_str(data);
			voice_prompt_string_music(str_buf); 

			
			break;

		case TTS_PLEASE_WAIT:
			DBGMSG("audio_tts TTS_PLEASE_WAIT!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, cancel_path);
			str_buf = _("Please wait");
			voice_prompt_string_music(str_buf);

		
				
		case  TTS_BEGIN_MUSIC:

			DBGMSG("audio_tts TTS_BEGIN_MUSIC!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, cancel_path);
			str_buf = _("Beginning of Music");
			voice_prompt_string_music(str_buf);

		
			break;
					
			
		case TTS_END_MUSIC:	

			DBGMSG("audio_tts TTS_END_MUSIC!\n");
			voice_prompt_abort();
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, cancel_path);
			str_buf = _("end of music");
			voice_prompt_string_music(str_buf);
		
			break;
		case	TTS_READ_MEDIA_ERROR:

			DBGMSG("audio_tts TTS_READ_MEDIA_ERROR!\n");
			voice_prompt_abort();
			
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			str_buf = _("Read error.It may not access the media correctly.");
			voice_prompt_string_music(str_buf);

		
			break;

		case	TTS_READ_MEDIA_ERROR1:

			DBGMSG("audio_tts TTS_READ_MEDIA_ERROR1!\n");
			voice_prompt_abort();
			
			*is_running = 1;
			voice_prompt_music(0, &vprompt_cfg, bu_path);
			str_buf = _("Failed to prepare for playback.");
			voice_prompt_string_music(str_buf);

			
			break;
		case TTS_USBON_BEEP:
			
			DBGMSG("audio_tts TTS_USBON_BEEP!\n");	
			usb_on_beep();
			break;

			
		case 	TTTS_USBOFF_BEEP:

			DBGMSG("audio_tts TTTS_USBOFF_BEEP!\n");	
			usb_off_beep();
			break;
			
		case TTS_ERROR_BEEP:

			DBGMSG("audio_tts TTS_ERROR_BEEP!\n");	
			error_beep();
			break;

	case TTS_VOLUME_ERROR:

			DBGMSG("audio_tts TTS_VOLUME_ERROR!\n");	
			volume_error_beep();
			break;
			
		case TTS_NORMAL_BEEP:
			DBGMSG("audio_tts TTS_NORMAL_BEEP!\n");
			normal_beep();
			break;
			
		case TTS_CANCEL_BEEP:

			DBGMSG("audio_tts TTS_CANCEL_BEEP!\n");
			cancel_beep();
			break;
	}


	DBGMSG("audio tts player.countforsound=%d\n",player.countforsound);
}





