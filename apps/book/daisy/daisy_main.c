#include <stdio.h>
#include <string.h>
//#include "menu_notify.h"
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <microwin/device.h>
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#define OSD_DBG_MSG
#include "nc-err.h"
#include "typedef.h"
//#include "keydef.h"
//#include "sys_config.h"
//#include "explorer.h"
//#include "menu.h"
//nclude "system.h"
#include "audio.h"
#include "ttsplus.h"
//#include "tipvoice.h"
#include "libvprompt.h"
//#include "p_mixer.h"
//#include "resource.h"
//#include "arch_def.h"
#include <gst/gst.h>
//#include "libinfo.h"
#include "book_prompt.h"
#include "tchar.h"
#include "daisy_parse.h"
#include "daisy_show.h"
#include "daisy_mark.h"
//#include "nx-base.h"
#include "widgets.h"
#include "PlexDaisyAnalyzeAPI.h"
#include "application-ipc.h"
#include "desktop-ipc.h"
#include "application.h"
#include "Plextalk-config.h"
#include "Plextalk-statusbar.h"
#include "Plextalk-keys.h"
#include "Plextalk-theme.h"
#include "Plextalk-titlebar.h"
#include "Plextalk-i18n.h"
#include "Plextalk-ui.h"
#include "Plextalk-constant.h"
#include "File-helper.h"
#include "Nx-widgets.h"
#include "neux.h"
#include "Key-value-pair.h"
#include "book.h"
#include "Libinfo.h"
#include "daisy_info.h"
#include "dbmalloc.h"

#define __(x) x

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))
#endif

typedef enum{
	EXIT_MEDIA_REMOVE,
	EXIT_NORMAL,
}EXIT_TYPE;

typedef enum{
	MODE_NULL = 0,
	MODE_PAUSE ,
	MODE_PLAYING,
	MODE_STOP,
	MODE_END_STOP,/*end of book and stop*/
}PLAY_MODE;

typedef enum{
	AUDIO_MODE_TTS = 0,
	AUDIO_MODE_AUDIO,
}AUDIO_MODE;

typedef enum{
	DAISY_OPENING = 0,
	DAISY_OPEN_ERR = 1,
	DAISY_OPEN_OK = 2,
}OPEN_STATE;

struct LstnBook;

typedef int (*Ju_Func)(struct LstnBook *thiz, int play);
typedef int (*PromptCallback)(struct LstnBook *thiz);

struct LstnBook{
	DaisyShow			daisy_show;
	DaisyParse			*daisy_parse;

	char reserved[32];		//hopeless
	//system set
	struct plextalk_setting setting;
	
	EXIT_TYPE			exit_type;
	
	char 				daisy_dir[PATH_MAX];	//daisy dir 

 	AUDIO_MODE			audio_mode;
	OPEN_STATE			open_state;

	char reserved2[32];		//hopeless
	struct BookAudio 	*audio;
	char reserved3[32];		//hopeless
	
	int					audio_play;
	char				audio_play_file[PATH_MAX];
	
	struct TtsObj 		*book_tts;
	int					tts_play;
	int					tts_tip;
	int					tts_mode;
	
	TIMER				*audiotimer;
	TIMER				*keytimer;
	int					timerEnable;
	int					keymask;
	int					longcount;
	int					fast_cnt;
	int					fastPlay;
	
	PLAY_MODE			playmode;
	PLAY_MODE			playmode_backup;

	PromptCallback		prompt_callback;

	int					select_book;
	
	int					info_module;
	int					info_pause;
	
	int					daisy_pause;
	
	Ju_Func				*prev_ju_func;
	Ju_Func				*next_ju_func;
	int					*hlevel;
	char**				func_name;
	int					func_count;
	int					func_cur;

	bool				jump_page;
//try for timeout for tts'end signal
	int wait_tts_active ;
	int wait_count_out ;
};

static struct LstnBook *gbook = 0;

static int dsMain_close_cur_book(struct LstnBook *thiz, int bDelFile);
static int dsMain_open_new_book(struct LstnBook *thiz);

static void dsMain_stop_prompt(void);
static int dsMain_stop_daisy(struct LstnBook *thiz);
static int dsMain_play_daisy(struct LstnBook *thiz);
static int dsMain_resume_daisy(struct LstnBook *thiz);
static int dsMain_pause_daisy(struct LstnBook *thiz);

static int dsMain_fast_rewind(struct LstnBook *thiz);
static int dsMain_fast_forward(struct LstnBook *thiz);
static void OnWindowRedraw(int clrBg);
static int dsMain_get_page_prompt(struct LstnBook *thiz,char *tipstr, int iMaxLen);
static void dsMain_play_prompt(enum tts_type type,char *prompt,
	PromptCallback callback);


static int dsMain_isOpen(struct LstnBook *thiz)
{
	int ret = 0;
	if(thiz && thiz->daisy_parse){
		ret = 1;
	}
	return ret;
}

int dsMain_getDaisyFile(const char *dir,char *file, int len)
{
	DIR *dp;
	struct dirent *ep;
	const size_t ncc_htm_len = strlen("ncc.htm");
	char *ext;
	int ret = 0;

	dp = opendir(dir);
	if (dp == NULL)
		return 0;

	while (ep = readdir (dp)) {
		if (ep->d_type == DT_DIR)
			continue;

		/* this catch "ncc.htm" and "ncc.html" */
		if (!strncasecmp(ep->d_name, "ncc.htm", ncc_htm_len) &&
			(ep->d_name[ncc_htm_len] == '\0' ||
			 ((ep->d_name[ncc_htm_len] | 0x20) == 'l' && ep->d_name[ncc_htm_len + 1] == '\0')))
		{
			ret = 1;
			break;
		}

		/* now catch "*.opf" */
		ext = strrchr(ep->d_name, '.');
		if (ext == NULL)
			continue;
		if (!strcasecmp(ext + 1, "opf")) {
			ret = 1;
			break;
		}
	}
	if(ret){
		snprintf(file, len, "%s/%s", dir, ep->d_name);
	}
	closedir (dp);
	return ret;
}

static int dsMain_show_screen(void)
{
#if 0
	struct LstnBook *thiz = gbook;
	return_val_if_fail(thiz != NULL, 0);
	GrClearWindow(thiz->daisy_show.wid, 1);
#endif
	DBGLOG("showscreen\n");
	OnWindowRedraw(1);
	return 0;
}

static void dsMain_delCurTitle(void)
{
	DBGMSG("dsMain_delCurTitle\n");
	struct LstnBook*thiz = gbook;
	if(!thiz || !thiz->daisy_parse){
		DBGMSG("failed\n");
		NhelperDeleteResult(APP_DELETE_ERR);
		return;
	}
	
	if(isMediaReadOnly(thiz->daisy_dir)){
		DBGMSG("lock\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);
		return;
	}
//	dsMain_stop_prompt();

	dsMain_stop_daisy(thiz);
	dsMain_close_cur_book(thiz,1);
	
	NhelperDeleteResult(APP_DELETE_OK);
	
	if(thiz->daisy_pause){
		DBGMSG("pause now\n");
		return;
	}

	thiz->exit_type = EXIT_MEDIA_REMOVE;
	NeuxClearModalActive();
	return;
}

static void dsMain_set_book_speed(void)
{
	struct LstnBook*thiz = gbook;
	return_if_fail(thiz != NULL);
	if(thiz->book_tts){
		tts_obj_set_speed(thiz->book_tts,thiz->setting.book.speed);
	}
	if(thiz->audio){
		book_audio_set_speed(thiz->audio,thiz->setting.book.speed);
	}

}

static void dsMain_set_book_equalizer(void)
{
	struct LstnBook*thiz = gbook;
	return_if_fail(thiz != NULL);
	if(thiz->book_tts){
		tts_obj_set_band(thiz->book_tts, thiz->setting.book.text_equalizer);
	}
	
	if(thiz->audio){
		book_audio_set_band(thiz->audio,thiz->setting.book.audio_equalizer);
	}
}

static void dsMain_msg_pause(void)
{
	struct LstnBook* thiz = gbook;
	return_if_fail(thiz != NULL);


	int for_fast_stop = 0 ;

	/* In case of alarm comes but still in key timer */
	if (thiz->timerEnable) {
		TimerDisable(thiz->keytimer);
		thiz->timerEnable = 0;
		thiz->longcount = 0;
		thiz->fastPlay = 0;
		thiz->playmode = MODE_PAUSE;
		thiz->prompt_callback = 0;
		thiz->playmode_backup = MODE_PLAYING;		//recorver play
		for_fast_stop = 1;
	}
	
	if(thiz->select_book){
		info_debug("select book, not pause\n");
		return;
	}

	if(thiz->info_module){
		//in info module
		if(!thiz->info_pause){
			thiz->info_pause = 1;
			info_pause();
		}
		return;
	}

	//in daisy main
	if(thiz->daisy_pause){
		info_debug("had pause\n");
		return;
	}
	DBGMSG("dsMain_msg_pause\n");
	
	PromptCallback callback = thiz->prompt_callback;
	dsMain_stop_prompt();	//stop prompt

	if (!for_fast_stop) {
		thiz->prompt_callback = callback;	//for back resume
		thiz->playmode_backup = thiz->playmode;
	}
	
	dsMain_pause_daisy(thiz);

	thiz->daisy_pause = 1;
}

static void dsMain_msg_resume(void)
{
	struct LstnBook* thiz = gbook;
	return_if_fail(thiz != NULL);

	if(thiz->select_book){
		info_debug("select book, not resume\n");
		return;
	}

	if(thiz->info_module){
		//in info module
		if(thiz->info_pause){
			thiz->info_pause = 0;
			info_resume();
		}
		return;
	}

	if(!thiz->jump_page){
		if(!thiz->daisy_pause){
			info_debug("non resume\n");
			return;
		}
	}
	thiz->daisy_pause = 0;

	DBGMSG("dsMain_msg_resume\n");
//	if(!dsMain_isOpen(thiz)){
//		DBGMSG("close\n");
//		thiz->exit_type = EXIT_MEDIA_REMOVE;
//		NeuxClearModalActive();
//		return;
//	}
	
	if(!PlextalkIsFileExist(thiz->daisy_dir)){
		DBGMSG("file remove\n");
		thiz->exit_type = EXIT_MEDIA_REMOVE;
		NeuxClearModalActive();
		return;
	}
	
	if(thiz->jump_page){
		char tipstr[256];
		DBGMSG("jump page\n");
		int ret = dsMain_get_page_prompt(thiz,tipstr,256);
		if(ret< 0){
			dsMain_play_daisy(thiz);
		}else{
			dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_play_daisy);
		}
		thiz->jump_page = 0;
		return;
	}
	
	if(thiz->prompt_callback){
		PromptCallback callback = thiz->prompt_callback;
		thiz->prompt_callback = 0;
		callback(thiz);//in callback maybe reset prompt_callback
	}else if(MODE_PLAYING == thiz->playmode_backup){
		thiz->playmode_backup = MODE_NULL;
		dsMain_resume_daisy(thiz);
	}
}


static void dsMain_showStateIcon(PLAY_MODE playmode)
{
	if(MODE_PLAYING == playmode)
	{
		plextalk_set_volume_vprompt_disable(1);
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON,SBAR_CONDITION_ICON_PLAY);
	}
	else if(MODE_PAUSE== playmode)
	{
		plextalk_set_volume_vprompt_disable(0);
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON,SBAR_CONDITION_ICON_PAUSE);
	}
	else
	{	
		plextalk_set_volume_vprompt_disable(0);
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON,SBAR_CONDITION_ICON_STOP);
	}
	return;
}

static void dsMain_showTitleBar(struct LstnBook *thiz)
{
	if(NULL == thiz || 0==strlen(thiz->daisy_dir))
	{
		NhelperTitleBarSetContent(_("Read daisy"));
		return;
	}

	char *daisy_title = strrchr(thiz->daisy_dir, '/');
	if(daisy_title){
		daisy_title++;
		NhelperTitleBarSetContent(daisy_title);
		return;
	}else{
		NhelperTitleBarSetContent(_("Read daisy"));
	}
}

static void dsMain_showCateIcon(struct LstnBook *thiz)
{ 
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON,SBAR_CATEGORY_ICON_BOOK_DAISY); 
}


static void dsMain_showMediaIcon(char *path)
{
	if(NULL == path || 0==strlen(path)){
		return;
	}
	if(StringStartWith(path, MEDIA_INNER)){
		info_debug("file in inter\n");
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON,SBAR_MEDIA_ICON_INTERNAL_MEM);
	}
	else if(StringStartWith(path, MEDIA_SDCRD)){
		info_debug("file in sdcard\n");
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON,SBAR_MEDIA_ICON_SD_CARD);
	}
	else {
		info_debug("file in usb\n");
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON,SBAR_MEDIA_ICON_USB_MEM);
	}
	return;
}

static int dsMain_stop_tts(struct LstnBook *thiz)
{
	//info_debug("book daisy tts stop\n");
	thiz->tts_play = 0;
	tts_obj_stop(thiz->book_tts);
	return 0;
}

static int dsMain_start_tts(struct LstnBook *thiz)
{
	//info_debug("dsMain_start_tts start\n");
	int len;
	char *tts_text;
	thiz->tts_play = 0;
	tts_obj_stop(thiz->book_tts);

	tts_text = dsShow_getCurScrText(&thiz->daisy_show);
	if(!tts_text){
		info_debug("text is null\n");
		return -1;
	}

	len = strlen(tts_text);
	info_debug("len=%d\n", len);
	//info_debug("%s\n", (char*)tts_text);

	//tts_obj_start(thiz->book_tts, thiz->tts_mode, (char*)tts_text, len, ivTTS_CODEPAGE_UTF8, tts_role, tts_lang);

	tts_obj_start(thiz->book_tts, (char*)tts_text, len, ivTTS_CODEPAGE_UTF8);
	thiz->tts_play = 1;
	//info_debug("dsMain_start_tts end\n");
	return 0;

}

static int dsMain_play_mp3(struct LstnBook *thiz)
{
	return_val_if_fail(NULL != thiz, -1);
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	if (!PlextalkIsFileExist(thiz->daisy_parse->strPath)){
		DBGMSG("invalid:%s\n",thiz->daisy_parse->strPath);
		return 0;
	}

	if(book_audio_isruning(thiz->audio)){
		if(0 == strcmp(thiz->audio_play_file, thiz->daisy_parse->strPath)){
			DBGMSG("only seek\n");
			book_audio_seek(thiz->audio, thiz->daisy_parse->time.begtime, INVALID_TIME);
			thiz->audio_play = 1;
			return;
		}
	}

	DBGMSG("play audio\n");
	strlcpy(thiz->audio_play_file,thiz->daisy_parse->strPath, PATH_MAX);
	book_audio_play(thiz->audio, thiz->audio_play_file, thiz->daisy_parse->time.begtime, INVALID_TIME);
	thiz->audio_play = 1;
	return 0;
}

static int dsMain_pause_audio(struct LstnBook *thiz)
{
	if(thiz->playmode != MODE_PLAYING)
	{
		return 0;
	}
	
	if(thiz->audio_mode == AUDIO_MODE_AUDIO)
	{
		if(thiz->audio_play)
		{
			if(!book_get_pause(thiz->audio))
			{
				book_audio_pause(thiz->audio);
				thiz->audio_play = 0;
			}
		}
	}
	else
	{
		dsMain_stop_tts(thiz);
	}
	thiz->playmode = MODE_PAUSE;
	return 1;
}

static int dsMain_resume_audio(struct LstnBook *thiz)
{
	if(thiz->playmode != MODE_PAUSE)
	{
		return 0;
	}
	  
	if(thiz->audio_mode == AUDIO_MODE_AUDIO)
	{
	
		//if(book_get_pause(thiz->audio))
		//{
		
			book_audio_pause(thiz->audio);
			thiz->audio_play = 1;
		//}
	}
	else
	{
		dsMain_start_tts(thiz);
	}
	
	thiz->playmode = MODE_PLAYING;
	return 0;
}

static int dsMain_play_audio(struct LstnBook *thiz)
{ 
	if(thiz->audio_mode == AUDIO_MODE_AUDIO)
	{
		dsMain_play_mp3(thiz);
	}
	else
	{
		dsMain_start_tts(thiz);
	}
	thiz->playmode = MODE_PLAYING;
	return 0;
}

static int dsMain_stop_audio(struct LstnBook *thiz)
{
	if(MODE_END_STOP == thiz->playmode){
		return 0;
	}
	if(thiz->audio_mode == AUDIO_MODE_AUDIO)
	{
		thiz->audio_play = 0;
		book_audio_stop(thiz->audio);
	}
	else
	{
		dsMain_stop_tts(thiz);
	}
	thiz->playmode = MODE_STOP;
	return 0;
}

static int dsMain_stop_daisy(struct LstnBook *thiz){
	dsMain_stop_audio(thiz);
	dsMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int dsMain_play_daisy(struct LstnBook *thiz){
	dsMain_play_audio(thiz);
	dsMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int dsMain_resume_daisy(struct LstnBook *thiz){
	DBGMSG("resume\n");
	dsMain_resume_audio(thiz);
	dsMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int dsMain_pause_daisy(struct LstnBook *thiz){
	DBGMSG("pause\n");
	dsMain_pause_audio(thiz);
	dsMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int dsMain_end_stop(struct LstnBook *thiz)
{
	dsMain_stop_audio(thiz);
	thiz->playmode = MODE_END_STOP;
	dsMain_showStateIcon(thiz->playmode);
	return 0;
}

static void dsMain_play_prompt(enum tts_type type,char *prompt,
	PromptCallback callback){
	DBGMSG("play\n");
	
	struct LstnBook *thiz = gbook;
	//try timeout for not recived tts end signal
	DBGMSG("Start count !!\n");
	thiz->wait_tts_active = 1;
	thiz->wait_count_out = 0;

	thiz->tts_tip = 1;
	thiz->prompt_callback = callback;
	if(BOOK_PROMPT == type && NULL ==prompt){
		//for callback
	}else{
		book_prompt_play(type,prompt);
	}
	DBGMSG("end\n");
}

static void dsMain_stop_prompt(void){

	struct LstnBook *thiz = gbook;
	thiz->tts_tip = 0;
	thiz->prompt_callback = 0;
	book_prompt_stop();
}

//check tts prompt play end
static void dsMain_prompt_check(struct LstnBook *thiz)
{
	if(!thiz->tts_tip){
		return;
	}

//do _time out!
	if (thiz->wait_tts_active) {
		thiz->wait_count_out ++;
		//printf("DEBUG: wait_tts_active & thiz->wait_count-out = %d\n",thiz->wait_count_out);

		if (thiz->wait_count_out >= 30)	{	//30 * 0.3 = 9s, time_out, reset;
			//printf("DEBUG: this->wait tts active and wait_count > 20!!!\n");
			thiz->wait_tts_active = 0;
			thiz->wait_count_out = 0;
			//printf("DEBUG: time_out, force set tip_is_running to 0 !!\n");
			tip_is_running = 0;
		}
	}
//do _time out end!!
	if(tip_is_running){
		return;
	}

	//if tts has be end (no matter signal or time out), flags shoule be clear
	if (thiz->wait_tts_active) {
		thiz->wait_tts_active = 0;
		thiz->wait_count_out = 0;
	}
	
	thiz->tts_tip = 0;
	info_debug("tipvoice end\n");
	//info_debug("callback:%x\n",thiz->prompt_callback);
	if(thiz->prompt_callback){
		PromptCallback callback = thiz->prompt_callback;
		thiz->prompt_callback = 0;
		callback(thiz);//in callback maybe reset prompt_callback
	}

}

static int dsMain_get_page_prompt(struct LstnBook *thiz,char *tipstr, int iMaxLen)
{
	unsigned long ulCurPage,ulTotalPage;
	int ret = daisy_getPageInfo(thiz->daisy_parse,&ulCurPage,&ulTotalPage);
	if(ret<0){
		return -1;
	}
	if(ulCurPage == PLEX_DAISY_PAGE_NONE){
		return -1;
	}else if(ulCurPage == PLEX_DAISY_PAGE_FRONT){
		snprintf(tipstr, iMaxLen, "%s", _("front page"));
	}else if(ulCurPage == PLEX_DAISY_PAGE_SPECIAL){
		snprintf(tipstr, iMaxLen, "%s", _("special page"));
	}else{
		snprintf(tipstr, iMaxLen, "%s %ld ", _("page"), ulCurPage);
	}
	return 0;
}


static int dsMain_replayScrText(struct LstnBook *thiz)
{
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	if(BMODE_TEXT_ONLY != thiz->daisy_parse->mode)
	{
		return -1;
	}

	if(MODE_PLAYING != thiz->playmode){
		return -1;
	}
	//replay tts
	dsMain_stop_audio(thiz);
	dsMain_play_audio(thiz);
	return 0;
}

static int dsMain_reCalShowScr(struct LstnBook *thiz)
{
	return_val_if_fail(NULL != thiz->daisy_parse, -1);

	if( 0 == thiz->daisy_parse->strText[0])
	{
		return -1;
	}

	if(thiz->daisy_show.total_line<=0){
		return -1;
	}

	char *pstr = thiz->daisy_show.pText + thiz->daisy_show.lineStart[thiz->daisy_show.start_line];
	dsShow_parseText(&thiz->daisy_show, pstr, wcslen(pstr)*4);
	dsMain_show_screen();
	return 0;
}

static int dsMain_getDaisyInfo(struct LstnBook *thiz, unsigned long curtime)
{
	int ret,ret2;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	ret = daisy_getPhaseText(thiz->daisy_parse);
	ret2 = daisy_getPhaseTime(thiz->daisy_parse);
	if(ret2>=0){
		if( (unsigned long)-1 == curtime ){
			thiz->daisy_parse->time.curtime = thiz->daisy_parse->time.pasttime;
		}else{
			if(curtime < thiz->daisy_parse->time.pasttime){
				info_err("new curtime=%ld\n",curtime);
			}
			thiz->daisy_parse->time.curtime = curtime;
			thiz->daisy_parse->time.begtime += (curtime - thiz->daisy_parse->time.pasttime);
			thiz->daisy_parse->time.pasttime = curtime;
			if(thiz->daisy_parse->time.begtime>thiz->daisy_parse->time.endtime){
				info_err("new curtime=%ld\n",curtime);
			}
		}
	}
	
	if(ret<0 && ret2<0){
		info_err("get err\n");
		return -1;
	}
	
	ret = dsShow_parseText(&thiz->daisy_show, 
		thiz->daisy_parse->strText, wcslen(thiz->daisy_parse->strText)*4);
	info_debug("ret=%d\n", ret);
	return 0;
}

static int dsMain_first_screen(struct LstnBook *thiz, int play)
{
	int ret;
	
	ret = daisy_resumeMark(thiz->daisy_parse);
	if( ret < 0)
	{//no resume book mark
		ret = daisy_jumpHead(thiz->daisy_parse);
		if(ret < 0){
			info_err("jump head error\n");
			return -1;
		}
		//skip head
		ret = daisy_nextPhrase(thiz->daisy_parse);
		if(ret < 0){
			info_err("error\n");
			return -1;
		}
	}
	ret = dsMain_getDaisyInfo(thiz, -1);
	if(ret < 0)
	{	
		info_err("error\n");
		return -1;
	}

	info_debug("bmode=%d\n", thiz->daisy_parse->mode);
	if(thiz->daisy_parse->mode == BMODE_TEXT_ONLY){
		thiz->audio_mode = AUDIO_MODE_TTS;
		info_debug("text only tts\n");
	}
	else{
		thiz->audio_mode = AUDIO_MODE_AUDIO;
	}
	dsMain_show_screen();
	return 0;
}


static int dsMain_jump_head(struct LstnBook *thiz)
{
	int ret;
	info_debug("jump head\n");
	ret = daisy_jumpHead(thiz->daisy_parse);
	if(ret < 0){
		info_err("error\n");
		return -1;
	}
	//skip head
	ret = daisy_nextPhrase(thiz->daisy_parse);
	if(ret < 0){
		info_err("error\n");
		return -1;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}

static int dsMain_jump_tail(struct LstnBook *thiz)
{
	int ret;
	info_debug("jump tail\n");
	ret = daisy_jumpTail(thiz->daisy_parse);
	if(ret < 0){
		info_err("error\n");
		return -1;
	}
	ret = dsMain_getDaisyInfo(thiz, -1);
	if( ret < 0 ){
		return -1;
	}
	dsMain_show_screen();
	//dsMain_play_daisy(thiz);
	return 0;
}

static int dsMain_jumpNextError(struct LstnBook *thiz)
{
	DBGMSG("JUMP NEXT ERR\n");
//	if(MODE_END_STOP == thiz->playmode){
//		dsMain_play_prompt(BOOK_END, 0, 0);
//	}else{
		if(thiz->setting.book.repeat){
			DBGMSG("repeat\n");
			dsMain_play_prompt(ERROR_BEEP, 0, dsMain_jump_head);
		}else{
			dsMain_end_stop(thiz);
			dsMain_jump_tail(thiz);
			dsMain_play_prompt(BOOK_END, 0, 0);
		}
//	}
	return -1;
}

static int dsMain_jumpPrevError(struct LstnBook *thiz)
{
	DBGMSG("jump prev err\n");
//	if(thiz->setting.book.repeat){
//		DBGMSG("repeat\n");
		dsMain_play_prompt(BOOK_BEGIN, 0, dsMain_jump_head);
//	}else{
//		dsMain_play_prompt(BOOK_BEGIN, 0, dsMain_play_daisy);
//	}
	return -1;
}

static int dsMain_next_phrase(struct LstnBook *thiz, int play)
{
	int ret;
	
	ret = daisy_nextPhrase(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			dsMain_play_daisy(thiz);
		}
	}
	return 0;
}

static int dsMain_prev_phrase(struct LstnBook *thiz, int play)
{
	int ret;
	
	ret = daisy_prevPhrase(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			dsMain_play_daisy(thiz);
		}
	}
	return 0;
}

static int dsMain_jump_page(struct LstnBook *thiz, unsigned long ulpage)
{
	int ret;
	struct DMark mark;
	DBGMSG("jump_page\n");
	thiz->jump_page = false;
	if(!thiz || !thiz->daisy_parse){
		NhelperJumpPageResult(APP_JUMP_ERR);
		return -1;
	}

	ret = daisy_JumpPage(thiz->daisy_parse, ulpage);
	DBGMSG("jump_page ret=%d\n",ret);
	if(ret < 0){
		if(PLEX_ERR_NOT_EXIST_TARGET == ret){
			NhelperJumpPageResult(APP_JUMP_NOT_EXIST_PAGE);
		}else{
			NhelperJumpPageResult(APP_JUMP_ERR);
		}
		return ret;
	}
	
	dsMain_getDaisyInfo(thiz, -1);

	NhelperJumpPageResult(APP_JUMP_OK);

	thiz->jump_page = true;
	/*
	dsMain_show_screen();
	if(play){
		char tipstr[256];
		int ret = dsMain_get_page_prompt(thiz,tipstr,256);
		if(ret< 0){
			dsMain_play_daisy(thiz);
		}else{
			dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_play_daisy);
		}
	}*/

	return ret;
}

static int dsMain_next_page(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_nextPage(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			char tipstr[256];
			int ret = dsMain_get_page_prompt(thiz,tipstr,256);
			if(ret< 0){
				dsMain_play_daisy(thiz);
			}else{
				dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_play_daisy);
			}
		}
	}

	return 0;
}


static int dsMain_prev_page(struct LstnBook *thiz, int play)
{
	int ret;
	
	ret = daisy_prevPage(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			char tipstr[256];
			int ret = dsMain_get_page_prompt(thiz,tipstr,256);
			if(ret< 0){
				dsMain_play_daisy(thiz);
			}else{
				dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_play_daisy);
			}
		}
	}

	return 0;
}

static int dsMain_next_group(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_nextGroup(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			dsMain_play_daisy(thiz);
		}
	}

	return 0;
}


static int dsMain_prev_group(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_prevGroup(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			dsMain_play_daisy(thiz);
		}
	}
	return 0;
}


static int dsMain_next_10min(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
//	struct DMark mark;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_next10min(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	
	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}


static int dsMain_prev_10min(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_prev10min(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}

	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}


static int dsMain_next_30sec(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_next30sec(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}


static int dsMain_prev_30sec(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_prev30sec(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}

	return 0;
}


static int dsMain_next_5sec(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_next5sec(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}


static int dsMain_prev_5sec(struct LstnBook *thiz, int play)
{
	int ret;
	unsigned long curtime;
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	curtime = thiz->daisy_parse->time.curtime;
	ret = daisy_prev5sec(thiz->daisy_parse, &curtime);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, curtime);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
		
	return 0;
}


static int dsMain_next_screen(struct LstnBook *thiz, int play)
{
	int ret;
	ret = daisy_nextScreen(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}

	return 0;
}

static int dsMain_prev_screen(struct LstnBook *thiz, int play)
{
	int ret;
 
	ret = daisy_prevScreen(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	 
	return 0;
}


static int dsMain_next_para(struct LstnBook *thiz, int play)
{
	int ret;
 
	ret = daisy_nextParagraph(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	return 0;
}

static int dsMain_prev_para(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_prevParagraph(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}

static int dsMain_prev_sen(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_prevSentence(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	
	return 0;
}

static int dsMain_next_sen(struct LstnBook *thiz, int play)
{
	int ret;

	ret = daisy_nextSentence(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		dsMain_play_daisy(thiz);
	}
	 
	return 0;
}


static int dsMain_prev_heading(struct LstnBook *thiz, int play)
{
	int ret; 
	info_debug("daisy prev heading\n");
	ret = daisy_prevHeading(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpPrevError(thiz);
		return ret;
	}
	else{
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();

		if(play){
			dsMain_play_daisy(thiz);
		}
		
	}
	return 0;  
}
static int dsMain_next_heading(struct LstnBook *thiz, int play)
{
	int ret;
	info_debug("daisy next heading\n");
	
	ret = daisy_nextHeading(thiz->daisy_parse);
	if(ret < 0){
		ret = dsMain_jumpNextError(thiz);
		return ret;
	}
	else{
		info_debug("next heading success\n");
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		if(play){
			dsMain_play_daisy(thiz);
		}
	}
	
	return 0; 
}


static int dsMain_prev_mark(struct LstnBook *thiz, int play)
{
	int ret; 
	ret = daisy_prevMark(thiz->daisy_parse);
	if(ret < 0){
		//ret = dsMain_jump_head(thiz);
		info_err("mark error");
		if(MODE_END_STOP == thiz->playmode){
			dsMain_play_prompt(ERROR_BEEP, 0, 0);
		}else{
			dsMain_play_prompt(ERROR_BEEP, 0, dsMain_play_daisy);
		}
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		char tipstr[128];
		snprintf(tipstr, 128, "%s:%d ", _("bookmark"), thiz->daisy_parse->mark->cur+1);
		dsMain_play_prompt(BOOK_PROMPT, tipstr,dsMain_play_daisy);
	} 

	return 0;
}

static int dsMain_next_mark(struct LstnBook *thiz, int play)
{
	int ret;
 
	ret = daisy_nextMark(thiz->daisy_parse);
	if(ret < 0){
		//ret = dsMain_jump_head(thiz);
		info_err("mark error");
		if(MODE_END_STOP == thiz->playmode){
			dsMain_play_prompt(ERROR_BEEP, 0, 0);
		}else{
			dsMain_play_prompt(ERROR_BEEP, 0, dsMain_play_daisy);
		}
		return ret;
	}
	dsMain_getDaisyInfo(thiz, -1);
	dsMain_show_screen();
	if(play){
		char tipstr[128];
		snprintf(tipstr, 128, "%s:%d ", _("bookmark"), thiz->daisy_parse->mark->cur+1);
		dsMain_play_prompt(BOOK_PROMPT, tipstr,dsMain_play_daisy);
		//book_play(thiz);
	}
	
	return 0;
}

static int dsMain_setMark(struct LstnBook *thiz)
{
	int ret;
	struct DMark mark;
	DBGMSG("set mark\n");
	if(!thiz || !thiz->daisy_parse){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
		return -1;
	}

	if(isMediaReadOnly(thiz->daisy_parse->daisy_file)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}

	ret = daisy_phrase2mark(thiz->daisy_parse, &mark);
	if(ret < 0){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
		return -1;
	}
	
	ret = dsMark_checkMark(thiz->daisy_parse->mark, &mark);
	if(ret >= 0){
		if(ret >= MAX_MARK_COUNT){
			NhelperBookMarkResult(APP_BOOKMARK_UPPER_LIMIT_ERR,0);
		}else{
			NhelperBookMarkResult(APP_BOOKMARK_EXIST_ERR,ret+1);
		}
		return -1;
	}
	ret = dsMark_addMark(thiz->daisy_parse->mark, &mark);
	if(ret<0){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_SET_OK,0);
	}
	return ret;
}

static int dsMain_delCurMark(struct LstnBook *thiz)
{
	int ret;
	DBGMSG("del cur mark\n");
	if(!thiz || !thiz->daisy_parse){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
		return -1;
	}
	
	if(isMediaReadOnly(thiz->daisy_parse->daisy_file)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}
	
	ret = dsMark_delCurMark(thiz->daisy_parse->mark);
	if(ret < 0){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);
	}
	return ret;
}

static int dsMain_delAllMark(struct LstnBook *thiz)
{
	int ret;
	DBGMSG("del all mark\n");
	if(!thiz || !thiz->daisy_parse){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
		return -1;
	}

	if(isMediaReadOnly(thiz->daisy_parse->daisy_file)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}
	
	ret = dsMark_delAllMark(thiz->daisy_parse->mark);
	if(ret < 0){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);
	}
	return ret;
}

static int dsMain_heading_levels(struct LstnBook *thiz)
{
	int cur, total; 
	daisy_getHeadingLevel(thiz->daisy_parse, &cur, &total);
	return total;
}

static int dsMain_fast_rewind_cb(struct LstnBook *thiz){
	info_debug("fast_rewind_callback\n");
	if(thiz->longcount > 0){
		//key also hold,continue fast
		dsMain_fast_rewind(thiz);
	}else{
		//key has release,play
		dsMain_play_audio(thiz);
	}
	return 0;
}

static int dsMain_fast_forward_cb(struct LstnBook *thiz){
	info_debug("fast_forward_callback\n");
	if(thiz->longcount > 0){
		//key also hold,continue fast
		dsMain_fast_forward(thiz);
	}else{
		//key has release,play
		dsMain_play_audio(thiz);
	}
	return 0;
}

static int dsMain_fast_rewind(struct LstnBook *thiz)
{
	char tipstr[256];
	int ret = 0;
	
	info_debug("daisy_fast_rewind\n");
	if(!thiz->daisy_parse){
		thiz->fastPlay = 0;
		return -1;
	}

	if(thiz->daisy_parse->mode == BMODE_TEXT_ONLY){
		ret = daisy_prevParagraph(thiz->daisy_parse);
		if(ret < 0){
			info_err("rewind end\n");
			ret = dsMain_jumpPrevError(thiz);
			return ret;
		}
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		thiz->fast_cnt ++;
		if(1 == thiz->fast_cnt){
			snprintf(tipstr, sizeof(tipstr), "%s %d %s", _("minus"),thiz->fast_cnt*5,_("sentences"));
		}else{
			snprintf(tipstr, sizeof(tipstr), "%d %s", thiz->fast_cnt*5,_("sentences"));
		}
		dsMain_play_prompt(BOOK_PROMPT, tipstr, dsMain_fast_rewind_cb);
	}else{//audio
		unsigned long curtime = thiz->daisy_parse->time.curtime;
		ret = daisy_prev5sec(thiz->daisy_parse, &curtime);
		if(ret < 0){
			info_err("forward end\n");
			thiz->fastPlay = 0;
			dsMain_stop_audio(thiz);
			dsMain_jumpPrevError(thiz);
			return ret;
		}
		dsMain_getDaisyInfo(thiz, curtime);
		dsMain_show_screen();
		
		thiz->fast_cnt ++;
		if(thiz->daisy_parse->mode == BMODE_AUDIO_ONLY){
			thiz->fastPlay = 1;
			dsMain_play_audio(thiz);
			//dsMain_play_prompt(BOOK_PROMPT, NULL, dsMain_fast_rewind_cb);
		}else{
			if(1 == thiz->fast_cnt){
				snprintf(tipstr, sizeof(tipstr), "%s %d %s", _("minus"),thiz->fast_cnt*5,_("seconds"));
			}else{
				snprintf(tipstr, sizeof(tipstr), "%d %s", thiz->fast_cnt*5,_("seconds"));
			}
			dsMain_play_prompt(BOOK_PROMPT, tipstr, dsMain_fast_rewind_cb);
		}
	}
	return 0;
}

static int dsMain_fast_forward(struct LstnBook *thiz)
{
	char tipstr[256];
	int ret = 0;
	
	info_debug("daisy forward\n");
	if(!thiz->daisy_parse){
		thiz->fastPlay = 0;
		return -1;
	}
	
	if(thiz->daisy_parse->mode == BMODE_TEXT_ONLY){
		ret = daisy_nextParagraph(thiz->daisy_parse);
		if(ret < 0){
			info_err("forward end\n");
			ret = dsMain_jumpNextError(thiz);
			return ret;
		}
		dsMain_getDaisyInfo(thiz, -1);
		dsMain_show_screen();
		thiz->fast_cnt ++;
		snprintf(tipstr, sizeof(tipstr), "%d %s", thiz->fast_cnt*5,_("sentences"));
		dsMain_play_prompt(BOOK_PROMPT, tipstr, dsMain_fast_forward_cb);
	}else{//audio
		unsigned long curtime = thiz->daisy_parse->time.curtime;
		ret = daisy_next5sec(thiz->daisy_parse, &curtime);
		if(ret < 0){
			info_err("forward end\n");
			thiz->fastPlay = 0;
			dsMain_stop_audio(thiz);
			dsMain_jumpNextError(thiz);
			return ret;
		}
		dsMain_getDaisyInfo(thiz, curtime);
		dsMain_show_screen();
		thiz->fast_cnt ++;
		if(thiz->daisy_parse->mode == BMODE_AUDIO_ONLY){
			thiz->fastPlay = 1;
			dsMain_play_audio(thiz);
			//dsMain_play_prompt(BOOK_PROMPT, NULL, dsMain_fast_forward_cb);
		}else{
			snprintf(tipstr, sizeof(tipstr), "%d %s", thiz->fast_cnt*5,_("seconds"));
			dsMain_play_prompt(BOOK_PROMPT, tipstr, dsMain_fast_forward_cb);
		}
	}
	return 0;
}

static int dsMain_enter_key_pro(struct LstnBook *thiz)
{
	info_debug("mode=%d\n", thiz->playmode);
	if(thiz->playmode == MODE_PLAYING)
	{
		info_debug("!!book pause\n");
		dsMain_pause_daisy(thiz);
		dsMain_play_prompt(CANCEL_BEEP, NULL, NULL);
	}
	else if(thiz->playmode == MODE_PAUSE)
	{
		info_debug("!!book resume\n");
		dsMain_play_prompt(NORMAL_BEEP, NULL, dsMain_resume_daisy);
	}
	else if(thiz->playmode == MODE_STOP)
	{
		info_debug("!!book play\n");
		dsMain_play_daisy(thiz);
	}
	else if(thiz->playmode == MODE_END_STOP)
	{
		info_debug("!!book replay\n");
		dsMain_jump_head(thiz);
	}
	return 0;
}

/*
#define JU_HEADING_FUNC (1<<0)
#define JU_GROUP_FUNC (1<<1)
#define JU_PAGE_FUNC (1<<2)
#define JU_PHRASE_FUNC (1<<3)
#define JU_SCREEN_FUNC (1<<4)
#define JU_PARA_FUNC (1<<5)
#define JU_SEN_FUNC (1<<6)
#define JU_BOOKMARK_FUNC (1<<7)
*/
static int dsMain_setju_func(struct LstnBook *thiz)
{
	int i, total, mtol;
	info_debug("set ju func\n");
	thiz->func_count = 0;
	thiz->func_cur = 0;
	
	if(thiz->prev_ju_func){
		app_free(thiz->prev_ju_func);
		thiz->prev_ju_func = 0;
	}
	if(thiz->next_ju_func){
		app_free(thiz->next_ju_func);
		thiz->next_ju_func = 0;
	}
	if(thiz->func_name){
		app_free(thiz->func_name);
		thiz->func_name = 0;
	}
	if(thiz->hlevel){
		app_free(thiz->hlevel);
		thiz->hlevel = 0;
	}

	total = dsMain_heading_levels(thiz);
	mtol = total + JU_MAX_FUNC;
	thiz->prev_ju_func = app_malloc(
		sizeof(Ju_Func)*mtol);
	thiz->next_ju_func = app_malloc(
		sizeof(Ju_Func)*mtol);
	
	thiz->func_name = app_malloc(
		sizeof(char*)*mtol);
	thiz->hlevel = app_malloc(
		sizeof(int)*mtol);
	
	if(thiz->daisy_parse->funcs & JU_HEADING_FUNC){
		i = 1;
		while(i <= total){
			thiz->prev_ju_func[thiz->func_count] = dsMain_prev_heading;
			thiz->next_ju_func[thiz->func_count] = dsMain_next_heading;
			thiz->hlevel[thiz->func_count] = i;
			thiz->func_name[thiz->func_count] = __("level");
			thiz->func_count++;
			i++;
		}
	}

	if(thiz->daisy_parse->funcs & JU_GROUP_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_group;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_group;
		thiz->func_name[thiz->func_count] = __("group");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->daisy_parse->funcs & JU_PAGE_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_page;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_page;
		thiz->func_name[thiz->func_count] = __("page");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->daisy_parse->funcs & JU_PHRASE_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_phrase;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_phrase;
		thiz->func_name[thiz->func_count] = __("phrase");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}

	if(thiz->daisy_parse->funcs & JU_10MIN_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_10min;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_10min;
		thiz->func_name[thiz->func_count] = __("10minutes");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}

	if(thiz->daisy_parse->funcs & JU_30SEC_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_30sec;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_30sec;
		thiz->func_name[thiz->func_count] = __("30seconds");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}

	if(thiz->daisy_parse->funcs & JU_SCREEN_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_screen;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_screen;
		thiz->func_name[thiz->func_count] = __("screen");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->daisy_parse->funcs & JU_PARA_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_para;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_para;
		thiz->func_name[thiz->func_count] = __("paragraph");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}

	if(thiz->daisy_parse->funcs & JU_SEN_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_sen;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_sen;
		thiz->func_name[thiz->func_count] = __("sentence");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->daisy_parse->funcs & JU_BOOKMARK_FUNC){
		thiz->prev_ju_func[thiz->func_count] = dsMain_prev_mark;
		thiz->next_ju_func[thiz->func_count] = dsMain_next_mark;
		thiz->func_name[thiz->func_count] = __("bookmark");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	
	return 0;
}

static void dsMain_set_audiomode(struct LstnBook *thiz)
{ 
	int audio;
	if(thiz->daisy_parse->mode == BMODE_TEXT_ONLY){
		audio = 0;
	}else{
		audio = 1;
	} 
	book_setAudioMode(audio);
}

static int dsMain_delete_mark_files(char *daisy_dir)
{
	char tmppath[PATH_MAX];
	char cmd[PATH_MAX+10];

	snprintf(tmppath, PATH_MAX, "%s.bmk", daisy_dir);
	info_debug("tmppath=%s\n", tmppath);
	snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmppath);
	system(cmd); 
	return 0;
}

static int dsMain_close_cur_book(struct LstnBook *thiz, int bDelFile)
{
	if(thiz->daisy_parse){
		daisy_close(thiz->daisy_parse);
		info_debug("book_close end\n");
		thiz->daisy_parse = 0;
	}
	
	if(bDelFile){
		char cmd[PATH_MAX+20];
		dsMain_delete_mark_files(thiz->daisy_dir);
		snprintf(cmd,sizeof(cmd),"rm -rf \"%s\"", thiz->daisy_dir);
		system(cmd);
	}

	return 0;
}

static int dsMain_open_end_cb(struct LstnBook *thiz)
{
	info_debug("dsMain_open_end_cb\n");
	if(DAISY_OPEN_OK == thiz->open_state)
	{
		info_debug("open ok\n");
		dsMain_play_daisy(thiz);
	}
	else if(DAISY_OPEN_ERR== thiz->open_state)
	{
		info_debug("open err\n");
		dsMain_play_prompt(BOOK_OPEN_ERR, 0, 0);
		thiz->exit_type = EXIT_NORMAL;
		NeuxClearModalActive();
	}
	return 0;
}

static void dsMain_init_audio(struct LstnBook *thiz)
{
	if(AUDIO_MODE_TTS == thiz->audio_mode){
		thiz->book_tts = booktts;
	}else{
		thiz->audio = book_audio_create();
	}
	dsMain_set_book_speed();
	dsMain_set_book_equalizer();
	return ;
}

//open 
static int dsMain_open_new_book(struct LstnBook *thiz)
{
	int ret = -1; 
	char daisy_file[PATH_MAX];
	
	if(thiz->daisy_parse){
		daisy_close(thiz->daisy_parse);
		thiz->daisy_parse = NULL;
	}

	info_debug("open new file:%s\n", thiz->daisy_dir);
	thiz->open_state = DAISY_OPENING;
	dsMain_play_prompt(BOOK_TITLE_OPEN, NULL, dsMain_open_end_cb);
	
	//2.open book and initial something
 	ret = dsMain_getDaisyFile(thiz->daisy_dir, daisy_file, PATH_MAX);
	if(!ret){
		info_debug("get file err\n");
		goto fail;
	}
	thiz->daisy_parse = daisy_open(daisy_file); 
	if(NULL == thiz->daisy_parse){
		goto fail;
	}

	//3.set book navigation
	dsMain_setju_func(thiz);
	
	//4.get book title for display

	//5.save file for backup
	book_set_backup_path(thiz->daisy_dir);
	
	//6.set file come form:sdcard or usb or storage
	dsMain_showMediaIcon(thiz->daisy_dir);

	//7.change titlebar icon
	dsMain_showCateIcon(thiz);
	
	//8.change supplement display
	//book_change_supplement(thiz);

	//9.save andio mode, adjust volumn
	dsMain_set_audiomode(thiz);
	
	//10.save the current file, for open again
	book_set_last_path(thiz->daisy_dir);
	
	//set title bar
	dsMain_showTitleBar(thiz);
	
	info_debug("first screen\n");
	dsMain_first_screen(thiz, 0);
	info_debug("first screen end\n");

	dsMain_init_audio(thiz);
	
	thiz->open_state = DAISY_OPEN_OK;
	return 0;
fail:
	thiz->open_state = DAISY_OPEN_ERR;
	return -1;
}

#if 0
/*get the last play book*/
static int dsMain_get_lastbook(struct LstnBook *thiz)
{
	//strcpy(thiz->daisy_dir, sys_get_book_title());
	dsMain_get_last_path(thiz->daisy_dir);
	info_debug("get_last book\n");
	if(access(thiz->daisy_dir, F_OK)){
		info_debug("not exist=%s\n", thiz->daisy_dir);
		return -1;
	}

	if(!PlextalkIsDsiayBook(thiz->daisy_dir))
	{
		return -1;
	}
	return 0;
}
#endif

static void dsMain_destroy_book(struct LstnBook *thiz)
{
	return_if_fail(thiz != NULL);
	
	if(thiz){
		if(thiz->prev_ju_func){
			app_free(thiz->prev_ju_func);
			thiz->prev_ju_func = 0;
		}
		if(thiz->next_ju_func){
			app_free(thiz->next_ju_func);
			thiz->next_ju_func = 0;
		}
		if(thiz->func_name){
			app_free(thiz->func_name);
			thiz->func_name = 0;
		}
		if(thiz->hlevel){
			app_free(thiz->hlevel);
			thiz->hlevel = 0;
		}

		if(thiz->audio){
			book_audio_destroy(thiz->audio);
			thiz->audio = 0;
		}

		if(thiz->book_tts){
			tts_obj_stop(thiz->book_tts);
		}

		if(thiz->daisy_parse){
			daisy_close(thiz->daisy_parse);
			thiz->daisy_parse = 0;
		}

		app_free(thiz);
		thiz = NULL;
	}
}


static struct LstnBook *dsMain_create_book(void)
{
	struct LstnBook *thiz;

	thiz = (struct LstnBook *)app_malloc(sizeof(struct LstnBook));
	if(!thiz){
		info_err("malloc struct LstnBook\n");
		return NULL;
	}
	memset(thiz, 0, sizeof(struct LstnBook));
	gbook = thiz;
	return thiz;
}

static int dsMain_set_prev_unit(struct LstnBook *thiz)
{
	int paused;
	PromptCallback callback=NULL;
	char tipstr[256];

	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	if(MODE_NULL == thiz->playmode){
		DBGMSG("MODE:%d\n", thiz->playmode);
		return;
	}else if(MODE_PLAYING == thiz->playmode){
		dsMain_pause_daisy(thiz);
		paused = 1;
	}else{
		callback = thiz->prompt_callback;
		paused = 0;
	}
	
	if(thiz->func_cur - 1 < 0){/*limit*/
		thiz->func_cur = thiz->func_count - 1;
	}
	else{
		thiz->func_cur--;
	}

	if(thiz->hlevel[thiz->func_cur]){ 
		daisy_setHeadingLevel(thiz->daisy_parse,
			thiz->hlevel[thiz->func_cur]);
		
		snprintf(tipstr, sizeof(tipstr), "%s%d",
			gettext(thiz->func_name[thiz->func_cur]),
			thiz->hlevel[thiz->func_cur]);
	}
	else{
		snprintf(tipstr, sizeof(tipstr), "%s", gettext(thiz->func_name[thiz->func_cur]));
	}

	info_debug("unit=%s\n", tipstr);
	if(paused){
		dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_resume_daisy);
	}else{
		dsMain_play_prompt(BOOK_PROMPT,tipstr,callback);
	}
	return 0;

}


static int dsMain_set_next_unit(struct LstnBook*thiz)
{
	int paused;
	char tipstr[256];
	PromptCallback callback=NULL;
	
	return_val_if_fail(NULL != thiz->daisy_parse, -1);
	
	if(MODE_NULL == thiz->playmode){
		DBGMSG("MODE:%d\n", thiz->playmode);
		return;
	}else if(MODE_PLAYING == thiz->playmode){
		dsMain_pause_daisy(thiz);
		paused = 1;
	}else{
		callback = thiz->prompt_callback;
		paused = 0;
	}
	
	if(thiz->func_cur + 1 >= thiz->func_count){
		thiz->func_cur = 0;
	}
	else{
		thiz->func_cur++;
	}

	if(thiz->hlevel[thiz->func_cur]){
		snprintf(tipstr, sizeof(tipstr), "%s%d",
			gettext(thiz->func_name[thiz->func_cur]),
			thiz->hlevel[thiz->func_cur]);
		 
		daisy_setHeadingLevel(thiz->daisy_parse,
			thiz->hlevel[thiz->func_cur]);
	}
	else{
		snprintf(tipstr, sizeof(tipstr), "%s", gettext(thiz->func_name[thiz->func_cur]));
	}
	
	info_debug("unit=%s\n", tipstr);
	if(paused){
		dsMain_play_prompt(BOOK_PROMPT,tipstr,dsMain_resume_daisy);
	}else{
		dsMain_play_prompt(BOOK_PROMPT,tipstr,callback);
	}
	return 0;

}

static int dsMain_prev_unit(struct LstnBook *thiz)
{
	return_val_if_fail(thiz != NULL, -1);
	return_val_if_fail(thiz ->daisy_parse!= NULL, -1);

	dsMain_stop_prompt();
	dsMain_stop_audio(thiz);
	thiz->prev_ju_func[thiz->func_cur](thiz, 1);
	return 0;
}


static int dsMain_next_unit(struct LstnBook *thiz)
{
	return_val_if_fail(thiz != NULL, -1);
	return_val_if_fail(thiz ->daisy_parse!= NULL, -1);
	
	dsMain_stop_prompt();
	dsMain_stop_audio(thiz);
	thiz->next_ju_func[thiz->func_cur](thiz, 1);
	return 0;
}

static int dsMain_infomation(struct LstnBook *thiz)
{
	#define BUF_LEN  256
	#define TEXT_NUM 6
				
	int i, text_num;
	char (*p)[BUF_LEN] = NULL;
	char *info_text[TEXT_NUM];
	char path[PATH_MAX];
	struct vocinfo *pvocInfo[1];
	struct vocinfo vocInfo;
	int voc_num;
	//pause
	if(thiz->info_module){
		info_debug("!!!!!!rein\n");
		return 0;
	}
	info_debug("info\n");
	dsMain_msg_pause();
	thiz->info_module = 1;
	thiz->info_pause= 0;
	pvocInfo[0] = &vocInfo;
	NhelperTitleBarSetContent(_("Information")); 
	
	/* Create Info id */										 
	void *info_id = info_create(); 

	text_num = 0;
	if(NULL != thiz->daisy_parse){
		p = app_malloc(BUF_LEN*sizeof(char)*TEXT_NUM);
		for(i=0; i< TEXT_NUM; i++){
			info_text[i] = p+i;
		}
		memset(p, 0, BUF_LEN*sizeof(char)*TEXT_NUM);
		if(AUDIO_MODE_AUDIO == thiz->audio_mode){
			dsInfo_ElapsedTimeInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
		}else{
			dsInfo_PercentElapseInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
		}
		dsInfo_PagesInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
		vocInfo.cur_item = text_num;
		dsInfo_HeadingInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN, 
			path, PATH_MAX, &vocInfo, &voc_num);
		dsInfo_BookmarkNumberInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
		dsInfo_TittleNameInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
		dsInfo_CreationDateInfo(thiz->daisy_parse, info_text[text_num++], BUF_LEN);
	}
	//DBGMSG("voc_num:%d\n",voc_num);
	if(1==voc_num){
		//DBGMSG("pvocInfo:%x\n",pvocInfo[0]);
		//DBGMSG("btext:%d cur_item:%d\n",pvocInfo[0]->btext, pvocInfo[0]->cur_item);
		//DBGMSG("vocfile:%s start:%d len:%d\n",pvocInfo[0]->vocfile, pvocInfo[0]->start, pvocInfo[0]->len);
	}
	info_start_ex(info_id, info_text, text_num, pvocInfo, voc_num, &tip_is_running);
	
	info_destroy (info_id);
	
	if(NULL != p){
		app_free(p);
		p= NULL;
	}
	//resume
	thiz->info_module = 0;
	thiz->info_pause= 0;
	dsMain_msg_resume();
	
	NeuxFormShow(thiz->daisy_show.form, TRUE);
	NeuxWidgetFocus(thiz->daisy_show.form);
//	book_initKeyLock();
			
	#undef BUF_LEN
	#undef TEXT_NUM
	return 1;
}

#if 0
static void dsMain_volumeKey(struct LstnBook *thiz, int key)
{
	int limit = 0;
	int cur_vol = book_getCurVolumn();
	if(cur_vol == last_vol ){
		if(cur_vol >= PLEXTALK_SOUND_VOLUME_MAX 
			&& MWKEY_VOLUME_UP == key){
			limit = 1;
		}
	}
	
	if(MODE_PLAYING == thiz->playmode){
		DBGMSG("volumn\n");
		if(limit){
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
		}else{
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
	}else{
		if(limit){
			dsMain_play_prompt(LIMIT_BEEP,0 ,thiz->prompt_callback);
		}else{
			char volstr[50];
			DBGMSG("volumn22\n");
			snprintf(volstr, sizeof(volstr), "%s %d",_("Volume"), cur_vol);
			dsMain_play_prompt(BOOK_VOLUMN, volstr ,thiz->prompt_callback);
		}
	}
	last_vol = cur_vol;
}
#endif
static void dsMain_key_timerout(void)
{
	struct LstnBook*thiz = gbook;
	int ret;
	
	if(!thiz || !thiz->daisy_parse){
		return;
	}
	if(thiz->select_book){
		return;
	}
	
	DBGMSG("key=0x%x\n", thiz->keymask);
	thiz->longcount++;

	if( MODE_PLAYING != thiz->playmode && 
		MODE_PAUSE != thiz->playmode ){
		return;
	}

	switch(thiz->keymask){
	case MWKEY_LEFT:
		if(1 == thiz->longcount)
		{	//first in, after for callback--fast_forward_callback
			dsMain_stop_prompt();
			dsMain_stop_audio(thiz);
			dsMain_fast_rewind(thiz);	
		}else{
			if(BMODE_AUDIO_ONLY == thiz->daisy_parse->mode && thiz->fastPlay){
				dsMain_fast_rewind(thiz);
			}
		}
		break;
	case MWKEY_RIGHT:
		if(1 == thiz->longcount)
		{	//first in, after for callback--fast_forward_callback
			dsMain_stop_prompt();
			dsMain_stop_audio(thiz);
			dsMain_fast_forward(thiz);
		}else{
			if(BMODE_AUDIO_ONLY == thiz->daisy_parse->mode && thiz->fastPlay){
				dsMain_fast_forward(thiz);
			}
		}
		break;
	}
}

static int dsMain_key_up_handle(KEY__ key)
{
	int iret = 1;
	struct LstnBook*thiz = gbook;
	return_val_if_fail(thiz != NULL, -1);
	
	/* select file or daisy pause or info moudle cant reponse key*/
	if(thiz->select_book || thiz->info_module || thiz->daisy_pause){
		return -1;
	}

	if(thiz->timerEnable== 0){
		return -1;
	}
	TimerDisable(thiz->keytimer);
	thiz->timerEnable = 0;

	if (NeuxAppGetKeyLock(0)) {
		info_debug("key lock\n");
		return -1;
	}
	
	switch(key.ch){
	case MWKEY_RIGHT:
		if(thiz->longcount <= 0){
			//short press
			dsMain_next_unit(thiz);
		}
		thiz->longcount = 0;
		thiz->fastPlay = 0;
		break;
	case MWKEY_LEFT:
		if(thiz->longcount <= 0){
			//short press
			dsMain_prev_unit(thiz);
		}
		thiz->longcount = 0;
		thiz->fastPlay = 0;
		break;
//	case MWKEY_INFO:
//		if(thiz->longcount <= 0){
//			dsMain_infomation(thiz);
//		}
//		thiz->longcount = 0;
//		break;
	default:
		break;
	}
	return iret;
}


static int dsMain_key_down_handle(KEY__ key)
{
	int iret = 1;
	struct LstnBook*thiz = gbook;
	return_val_if_fail(thiz != NULL, -1);

	/* select file or daisy pause or info moudle cant reponse key*/
	if(thiz->select_book || thiz->info_module || thiz->daisy_pause){
		return -1;
	}

	if (NeuxAppGetKeyLock(0)) {
		info_debug("key lock\n");
		if(MODE_PLAYING == thiz->playmode){
			dsMain_pause_daisy(thiz);
			dsMain_play_prompt(BOOK_KEYLOCK,0,dsMain_resume_daisy);
		}else{
			dsMain_play_prompt(BOOK_KEYLOCK,0,thiz->prompt_callback);
		}
		return -1;
	}

	switch(key.ch){
	case MWKEY_ENTER:
		{
			dsMain_enter_key_pro(thiz);
		}
		break;
	case MWKEY_INFO:
		dsMain_infomation(thiz);
		break;
		
	case MWKEY_RIGHT:
	case MWKEY_LEFT:
		if(thiz->timerEnable == 0){
			info_debug("enable timer\n");
			/*long press timer*/
			thiz->keymask = key.ch;
			TimerEnable(thiz->keytimer);
			thiz->timerEnable = 1;
			thiz->longcount = 0;
			thiz->fast_cnt = 0;
		}
		break;
//	case MWKEY_VOLUME_UP:
//	case MWKEY_VOLUME_DOWN:
//		dsMain_volumeKey(thiz, key.ch);
//		break;
		
	case MWKEY_UP:
		{
			dsMain_set_prev_unit(thiz);
		}
		break;
	case MWKEY_DOWN:
		{
			dsMain_set_next_unit(thiz);
		}
		break;

	case VKEY_BOOK:
		{
			dsMain_stop_prompt();
			dsMain_stop_daisy(thiz);
			//
			dsMain_close_cur_book(thiz, 0);
			//exit, select new file
			thiz->exit_type = EXIT_NORMAL;
			NeuxClearModalActive();
			//dsMain_select_new_file(thiz);
			//dsMain_open_new_book(thiz);
		}
		break;
	default:
		break;
	}

	return iret;
}

static void dsMain_audio_check(struct LstnBook *thiz)
{
	if(!thiz->daisy_parse){
		return;
	}

	if(thiz->audio && thiz->audio_play){
		if(BMODE_AUDIO_ONLY == thiz->daisy_parse->mode && thiz->fastPlay){
			//audio fast play now
			return;
		}
		if(book_audio_isruning(thiz->audio))
		{
			unsigned long bak_time,time;
			time = book_audio_get_time(thiz->audio);
			//info_debug("time:%ld\n",time);
			if(time < thiz->daisy_parse->time.begtime){
				return;
			}
			
			if( (time+ 300) < thiz->daisy_parse->time.endtime)
			{
				bak_time = thiz->daisy_parse->time.curtime;
				thiz->daisy_parse->time.curtime = thiz->daisy_parse->time.pasttime + (time - thiz->daisy_parse->time.begtime);
				if(BMODE_AUDIO_ONLY == thiz->daisy_parse->mode){
					if((bak_time/1000) != (thiz->daisy_parse->time.curtime/1000)){
						dsMain_show_screen();
					}
				}
				return;
			}
		}
		//next
		int ret = daisy_nextPhrase(thiz->daisy_parse);
		if(ret < 0){
			info_debug("book end\n");
			//daisy_jump_tail(thiz->daisy_parse);
			dsMain_end_stop(thiz);
			if(thiz->setting.book.repeat){
				dsMain_play_prompt(BOOK_END,0,dsMain_jump_head);
			}else{
				dsMain_play_prompt(BOOK_END,0,0);
			}
		}
		else{
			dsMain_getDaisyInfo(thiz, -1);
			dsMain_show_screen();
			if(!book_audio_isruning(thiz->audio)){
				dsMain_play_audio(thiz);
				return;
			}
			if(0 != strcmp(thiz->audio_play_file, thiz->daisy_parse->strPath)){
				dsMain_play_audio(thiz);
			}
		}
		
	}
	else if(thiz->book_tts && thiz->tts_play && !tts_obj_isruning(thiz->book_tts)){
		thiz->tts_play = 0;
		int ret = dsShow_nextScreen(&thiz->daisy_show);
		if(ret >= 0){
			dsMain_show_screen();
			dsMain_play_audio(thiz);
			return;
		}
		//next phase
		ret = daisy_nextPhrase(thiz->daisy_parse);
		if(ret < 0){
			info_debug("book end\n");

			dsMain_end_stop(thiz);
			
			CoolShmReadLock(g_config_lock);
			int repeat = g_config->setting.book.repeat;
			CoolShmReadUnlock(g_config_lock);
			DBGMSG("repeat:%d\n",repeat);
			DBGMSG("thiz->setting.book.repeat:%d\n",thiz->setting.book.repeat);
			if(repeat){
				dsMain_play_prompt(BOOK_END,0,dsMain_jump_head);
			}else{
				dsMain_play_prompt(BOOK_END,0,0);
			}
		}
		else{
			info_debug("start\n");
			dsMain_getDaisyInfo(thiz, -1);
			dsMain_show_screen();
			dsMain_play_audio(thiz);
		}
	}
}

static void dsMain_audio_timerout(void)
{
	struct LstnBook*thiz = gbook;
	//info_debug("timer out\n");
	return_if_fail(thiz != NULL);
	if(thiz->select_book){
		return;
	}

	dsMain_prompt_check(thiz);
	dsMain_audio_check(thiz);
}

static int dsMain_setPageInfo()
{
	unsigned long pulCurPage,pulMaxPage,pulLastPage;
	unsigned long pulFrPage,pulSpPage,pulTotalPage;
	PlexResult ret;
	struct LstnBook* thiz = gbook;

	return_val_if_fail(thiz != NULL, -1);

	return_val_if_fail(thiz->daisy_parse!= NULL, -1);
	
	ret = PlexDaisyGetPageInfo(&pulCurPage, &pulMaxPage,&pulLastPage,
		&pulFrPage,&pulSpPage,&pulTotalPage);
	if(ret != PLEX_OK){
		book_del_pageinfo();
		return -1;
	}
	
	book_set_pageinfo(pulCurPage,pulMaxPage,pulFrPage,pulSpPage);
	return 0;
}


static void onAppMsg(const char * src, neux_msg_t * msg)
{
	struct LstnBook* thiz = gbook;
	
	int msgId = msg->msg.msg.msgId;
	DBGLOG("app msg %d .\n", msgId);
	if(PLEXTALK_APP_MSG_ID != msgId){
		return;
	}

	app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;
	switch (app->type)
	{
	case APPRQST_SET_STATE:
		{
			app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
			info_debug("set state:%d\n", rqst->pause);

			/* Special handler for info */
			if (rqst->spec_info) {
				printf("Special msg for info!!\n");
				if (thiz->info_module) {
					if (rqst->pause)
						info_pause();
					else
						info_resume();
				}

				return ;
			}
			
			printf("Normal msg set state!\n");
			if(rqst->pause){
				dsMain_msg_pause();
				dsMain_setPageInfo();
			}else{
				dsMain_msg_resume();
			}
		}
		break;
	case APPRQST_JUMP_PAGE:
		{
			DBGLOG("jump page\n");
			app_jump_page *rqst = (app_jump_page*)app;
			dsMain_jump_page(thiz, rqst->page);
		}
		break;
	case APPRQST_BOOKMARK:
		{
			DBGLOG("set bookmark\n");
			app_bookmark_rqst_t *rqst = (app_bookmark_rqst_t*)app;
	
			if(APP_BOOKMARK_SET == rqst->op)
			{
				dsMain_setMark(thiz);
			}
			else if(APP_BOOKMARK_DELETE == rqst->op)
			{
				dsMain_delCurMark(thiz);
			}
			else if(APP_BOOKMARK_DELETE_ALL== rqst->op)
			{
				dsMain_delAllMark(thiz);
			}
		}
		break;
		
	case APPRQST_DELETE_CONTENT:
		{
			dsMain_delCurTitle();
		}
		break;
		
	case APPRQST_SET_SPEED:
		{
			return_if_fail(thiz != NULL);
			app_speed_rqst_t *rqst = (app_speed_rqst_t*)app;
			thiz->setting.book.speed = rqst->speed;
			dsMain_set_book_speed();
		}
		break;
		
	case APPRQST_SET_REPEAT:
		{
			return_if_fail(thiz != NULL);
			app_repeat_rqst_t *rqst = (app_repeat_rqst_t*)app;
			thiz->setting.book.repeat= rqst->repeat;
		}
		break;
		
	case APPRQST_SET_EQU:
		{
			return_if_fail(thiz != NULL);
			app_equ_rqst_t *rqst = (app_equ_rqst_t*)app;
			if(rqst->is_audio){
				thiz->setting.book.audio_equalizer= rqst->equ;
			}else{
				thiz->setting.book.text_equalizer= rqst->equ;
			}
			dsMain_set_book_equalizer();
		}
		break;

	case APPRQST_SET_FONTSIZE:
		{	//plextalk_lcd
			widget_prop_t wprop;
			return_if_fail(thiz != NULL);
			//change font size
			info_debug("change font size\n");
			
			NeuxWidgetSetFont(thiz->daisy_show.form, sys_font_name, sys_font_size);
			thiz->daisy_show.show_gc = NeuxWidgetGC(thiz->daisy_show.form);
			dsShow_Init(&thiz->daisy_show, sys_font_size);
			//redispplay
			dsMain_reCalShowScr(thiz);
			//replay text tts
			dsMain_replayScrText(thiz);
		}
		break;
	case APPRQST_SET_THEME:
		{	//plextalk_lcd
			return_if_fail(thiz != NULL);
			
			widget_prop_t wprop;
			info_debug("change theme\n");
			
			app_theme_rqst_t *rqst = (app_theme_rqst_t *)app;
			thiz->setting.lcd.theme = rqst->index;
			
			NeuxWidgetGetProp(thiz->daisy_show.form, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(thiz->daisy_show.form, &wprop);
			
			GrSetGCForeground(thiz->daisy_show.show_gc, theme_cache.foreground);
			GrSetGCBackground(thiz->daisy_show.show_gc, theme_cache.background);
			dsMain_show_screen();
		}
		break;
	case APPRQST_SET_LANGUAGE:
		{
			info_debug("set language\n");
			/* Update book tts mode */
			dsMain_show_screen();
		}
		break;

	case APPRQST_RESYNC_SETTINGS:
		{
			info_debug("default settings \n");
			return_if_fail(thiz != NULL);
			widget_prop_t wprop;
			
			CoolShmReadLock(g_config_lock); 
			memcpy(&thiz->setting, &g_config->setting, sizeof(thiz->setting));
			CoolShmReadUnlock(g_config_lock);

			//tts 
			book_prompt_settings();
			//book speed,equ
			dsMain_set_book_speed();
			dsMain_set_book_equalizer();
			//font size
			NeuxWidgetSetFont(thiz->daisy_show.form, sys_font_name, sys_font_size);
			thiz->daisy_show.show_gc = NeuxWidgetGC(thiz->daisy_show.form);
			dsShow_Init(&thiz->daisy_show, sys_font_size);
			//theme
			NeuxWidgetGetProp(thiz->daisy_show.form, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(thiz->daisy_show.form, &wprop);
			GrSetGCForeground(thiz->daisy_show.show_gc, theme_cache.foreground);
			GrSetGCBackground(thiz->daisy_show.show_gc, theme_cache.background);
			//language
			/* Update book tts mode */
			//redisplay and replay audio
			dsMain_reCalShowScr(thiz);
			//replay text tts
			dsMain_replayScrText(thiz);
		}
		break;
	}
}

static void OnWindowKeydown(WID__ wid,KEY__ key)
{
    DBGLOG("OnWindowKeydown:%d\n",key);
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		info_debug("help running\n");
		return;
	}
	
	dsMain_key_down_handle(key);
}

static void OnWindowKeyup(WID__ wid,KEY__ key)
{
    DBGLOG("OnWindowKeyup:%d\n",key);
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		info_debug("help running\n");
		return;
	}

	dsMain_key_up_handle(key);
}

static void OnWindowRedraw(int clrBg)
{
	int i, y;
	struct LstnBook *thiz = gbook;
	GR_GC_ID gc;
	GR_WINDOW_ID wid;
	DaisyParse *parse;
		
	return_if_fail(thiz != NULL);
	if(thiz->select_book){
		return;
	}
	
	gc = thiz->daisy_show.show_gc;
	wid = thiz->daisy_show.wid;
	
	if(wid != GrGetFocus()){
		DBGMSG("NOT FOCUS\n");
		//return;
	}

	if(clrBg){
		NxSetGCForeground(gc, theme_cache.background);
		GrFillRect(wid, gc, 0, 0, thiz->daisy_show.w, thiz->daisy_show.h);
		NxSetGCForeground(gc, theme_cache.foreground);
	}

	parse = thiz->daisy_parse;
	if(NULL == parse)
	{	//NOT OPEN BOOK
		char *pstr = _("BOOK");
		int w,h,b;
		int x,y;
		
		NeuxWidgetSetFont(thiz->daisy_show.form, sys_font_name, 24);
		gc = NeuxWidgetGC(thiz->daisy_show.form);
		NxSetGCForeground(gc, theme_cache.foreground);
		NxSetGCBackground(gc, theme_cache.background);
		
		NxGetGCTextSize(gc, pstr, -1, MWTF_UTF8, &w, &h, &b);
		x = (thiz->daisy_show.w - w)/2;
		y = (thiz->daisy_show.h - 24)/2;
		GrText(wid, gc, x, y, pstr, -1, MWTF_UTF8 | MWTF_TOP);
		
		NeuxWidgetSetFont(thiz->daisy_show.form, sys_font_name, sys_font_size);
		thiz->daisy_show.show_gc = NeuxWidgetGC(thiz->daisy_show.form);
		return;
	}
	
	if(parse->mode == BMODE_AUDIO_ONLY)
	{
		if(-1 != parse->time.curtime && -1!=parse->time.totaltime)
		{
			char elapsetime[100];
			char totaltime[100];
			char *elapse = _("Elapsed");
			char *total = _("Total");
			unsigned long	ulSec, ulMin;
			unsigned long curPage,totalPage;
			int x, y;
			FONT_SIZE font_size = thiz->daisy_show.font_size;
			
			ulSec = parse->time.curtime/1000;
			ulMin = ulSec/60;
			if(FONT_SIZE_24PT == font_size){
				snprintf(elapsetime, 100, "%.2ld:%.2ld:%.2ld", ulMin/60, ulMin%60, ulSec%60);
			}else{
				snprintf(elapsetime, 100, "%s:%.2ld:%.2ld:%.2ld", elapse, ulMin/60, ulMin%60, ulSec%60);
			}
			ulSec = parse->time.totaltime/1000;
			ulMin = ulSec/60;
			if(FONT_SIZE_24PT == font_size){
				snprintf(totaltime, 100, "%.2ld:%.2ld:%.2ld", ulMin/60, ulMin%60, ulSec%60);
			}else{
				snprintf(totaltime, 100, "%s:%.2ld:%.2ld:%.2ld", total, ulMin/60, ulMin%60, ulSec%60);
			}

			x = 2;
			y = 2;
			GrText(wid, gc, x, y,elapsetime, strlen(elapsetime), MWTF_UTF8 | MWTF_TOP);
			y += thiz->daisy_show.line_height;
			GrText(wid, gc, x, y,totaltime, strlen(totaltime), MWTF_UTF8 | MWTF_TOP);
			y += thiz->daisy_show.line_height;
			if(daisy_getPageInfo(parse,&curPage,&totalPage)>=0){
				if((long)curPage >=0 && (long)totalPage>0){
					char page[50];
					if(curPage>totalPage){
						curPage = totalPage;
					}
					snprintf(page, sizeof(page), "P:%ld/%ld", curPage, totalPage);
					GrText(wid, gc, x, y,page, strlen(page), MWTF_UTF8 | MWTF_TOP);
				}
			}
		}
	}
	else{
		dsShow_showScreen(&thiz->daisy_show);
	}
}

static void OnWindowExposure(WID__ wid)
{
	DBGLOG("-------exposure:%d---\n",wid);
	OnWindowRedraw(0);
}

static void OnWindowGetFocus(WID__ wid)
{
	struct LstnBook* thiz = gbook;
	return_if_fail(thiz != NULL);
	DBGLOG("---------------windown focus %d.\n",wid);
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BOOK_DAISY);
	dsMain_showMediaIcon(thiz->daisy_dir);
	dsMain_showStateIcon(thiz->playmode);
	dsMain_showTitleBar(thiz);
}

static void OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	book_del_backup();
	plextalk_suspend_unlock();
	dsMain_destroy_book(gbook);
	gbook = NULL;
}


static void OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	info_debug("Hotplug device=%d index=%d action=%d\n",device,index,action);
	struct LstnBook* thiz = gbook;
	char *path;
	return_if_fail(thiz != NULL);
	
	if(thiz->select_book || thiz->info_module || thiz->daisy_pause){
		DBGLOG("return\n");
		return ;
	}

	if(DAISY_OPEN_ERR == thiz->open_state)
	{
		return;
	}
	
	if(0 == strlen(thiz->daisy_dir))
	{
		return;
	}
	path = thiz->daisy_dir;
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		break;

	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			if(StringStartWith(path, MEDIA_SDCRD))
			{
				info_debug("remove sdcard\n");
				dsMain_stop_daisy(thiz);
				thiz->exit_type = EXIT_MEDIA_REMOVE;
				NeuxClearModalActive();
			}
			break;
		case MWHOTPLUG_DEV_SCSI_DISK:
			if(StringStartWith(path, MEDIA_USB))
			{
				info_debug("remove usb\n");
				dsMain_stop_daisy(thiz);
				thiz->exit_type = EXIT_MEDIA_REMOVE;
				NeuxClearModalActive();
			}
			break;
		case MWHOTPLUG_DEV_SCSI_CDROM:
cdrom_remove:
			if(StringStartWith(path, MEDIA_CDROM))
			{
				info_debug("remove cdrom\n");
				dsMain_stop_daisy(thiz);
				thiz->exit_type = EXIT_MEDIA_REMOVE;
				NeuxClearModalActive();
			}
			break;
		default:
			return;
		}
		break;

	case MWHOTPLUG_ACTION_CHANGE:
		if (device == MWHOTPLUG_DEV_SCSI_CDROM)
		{
			printf("MWHOTPLUG_ACTION_CHANGE !!!, MWHOTPLUG_DEV_SCSI_CDROM!!!\n");
		
			int ret = CoolCdromMediaPresent();
			if (ret <= 0)
				goto cdrom_remove;
		}
		break;
	}
}


#if 0
static int dsMain_grab_table[] = { 
	MWKEY_UP,
	MWKEY_DOWN, 
	MWKEY_ENTER,
	MWKEY_RIGHT, 
	MWKEY_LEFT,
	VKEY_BOOK,
	MWKEY_INFO,
};
#endif

/* Function creates the main form of the application. */
static void dsMain_CreateMainForm (struct LstnBook*thiz)
{
	FORM  *formMain;
//	TIMER *timer;
    widget_prop_t wprop;

	dsShow_Init(&thiz->daisy_show, sys_font_size);
	
	/*创建控件*/
    formMain = NeuxFormNew(thiz->daisy_show.x, thiz->daisy_show.y, thiz->daisy_show.w, thiz->daisy_show.h);
	thiz->daisy_show.form = formMain;
	thiz->daisy_show.wid = NeuxWidgetWID(formMain);
	
	NeuxFormSetCallback(formMain, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(formMain, CB_KEY_UP, OnWindowKeyup);
	NeuxFormSetCallback(formMain, CB_FOCUS_IN, OnWindowGetFocus);
	NeuxFormSetCallback(formMain, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(formMain, CB_EXPOSURE, OnWindowExposure);
	NeuxFormSetCallback(formMain, CB_HOTPLUG, OnWindowHotplug );
	
	NeuxWidgetGetProp(formMain, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(formMain, &wprop);
	
	NeuxWidgetSetFont(formMain, sys_font_name, sys_font_size);
	thiz->daisy_show.show_gc = NeuxWidgetGC(formMain);
	//info_debug("thiz->gc:%d\n",thiz->daisy_show.gc);
	
	//create timer for check audio
	thiz->audiotimer = NeuxTimerNew(formMain, 300 ,-1);
	NeuxTimerSetCallback(thiz->audiotimer, dsMain_audio_timerout);

	//create timer for hold key 
	thiz->keytimer = NeuxTimerNew(formMain, 1000, -1);
	NeuxTimerSetCallback(thiz->keytimer, dsMain_key_timerout);
	TimerDisable(thiz->keytimer);
	thiz->timerEnable = 0;

	/* 显示主窗口 */
	NeuxFormShow(formMain, TRUE);
	NeuxWidgetFocus(formMain);

	NeuxWidgetRedraw(formMain);
	return ;
}

#if 0
static int dsMain_first_open(struct LstnBook *thiz)
{
	info_debug("dsMain_first_open\n");
//	if(dsMain_get_lastbook(thiz)>=0)
//	{	//open
//		dsMain_open_new_book(thiz);
//		return 0;
//	}
//	memset(thiz->daisy_dir, 0, sizeof(thiz->daisy_dir));
//	dsMain_select_new_file(thiz);
	if(dsMain_open_new_book(thiz)>=0)
	{	//if first open success
		return -1;
	}

	
	NeuxClearModalActive();
	return 0;
}
#endif

int daisy_main(char *file, int first, MsgFunc* func_ptr)
{
	int endOfModal;
	struct LstnBook *thiz;

	if(NULL==file || 0 == strlen(file) ){
		return -1;
	}

	thiz = dsMain_create_book();
	if(NULL == thiz){
		info_debug("malloc err\n");
		//FATAL(fmt,arg...);
		return 1;
	}
	//copy file
	memset(thiz->daisy_dir, 0, sizeof(thiz->daisy_dir));
	strlcpy(thiz->daisy_dir, file, sizeof(thiz->daisy_dir));

	//copy system setting
	CoolShmReadLock(g_config_lock);	
	memcpy(&thiz->setting, &g_config->setting, sizeof(thiz->setting));
	CoolShmReadUnlock(g_config_lock);

	//set tts mode though current language

	plextalk_suspend_lock();
	
	dsMain_CreateMainForm(thiz);

//	book_initKeyLock();

	*func_ptr = onAppMsg;
	//NeuxAppSetCallback(CB_MSG, onAppMsg);
	
	if(first)
	{	//first in ,must play bookmode
		dsMain_play_prompt(BOOK_MODE_OPEN, 0, dsMain_open_new_book);
	}
	else
	{
		dsMain_open_new_book(thiz);
	}
	
	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	info_debug("modal exit\n");
	if(EXIT_MEDIA_REMOVE == thiz->exit_type){
		strlcpy(file, PLEXTALK_MOUNT_ROOT_STR, PATH_MAX);
	}
	
	*func_ptr = NULL;
	NeuxFormDestroy(&thiz->daisy_show.form);
	info_debug("daisy main exit\n");
	return 1;
}


