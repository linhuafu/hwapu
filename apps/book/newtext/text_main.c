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
#include "ttsplus.h"
//#include "tipvoice.h"
#include "libvprompt.h"
//#include "p_mixer.h"
//#include "resource.h"s
//#include "arch_def.h"
#include <gst/gst.h>
//#include "libinfo.h"
#include "book_prompt.h"
#include "tchar.h"
#include "text_parse.h"
#include "text_show.h"
#include "text_mark.h"
//#include "nx-base.h"
#include "widgets.h"
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
#include "text_info.h"
#include "Storage.h"
#include "dbmalloc.h"
#include "PlexDaisyAnalyzeAPI.h"

extern PlexResult ConvertHtmlEpubToText(const char *filepath, char *outfile, const int bufsize);

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
	TEXT_OPENING = 0,
	TEXT_OPEN_ERR = 1,
	TEXT_OPEN_ERR_NOT_ENOUGH_WORK_AREA =2,
	TEXT_OPEN_ERR_CONVERT_CHARSET =3,
	TEXT_OPEN_OK = 4,
}OPEN_STATE;

struct TextBook;

typedef int (*Ju_Func)(struct TextBook *thiz, int play);
typedef int (*PromptCallback)(struct TextBook *thiz);

struct TextBook{
	TextShow			text_show;
	TextParse			*text_parse;
	//system set
	struct plextalk_setting setting;

	char 				textfile[PATH_MAX];
	char					genfile[PATH_MAX];
		
	OPEN_STATE			open_state;
	EXIT_TYPE			exit_type;

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

	PLAY_MODE			playmode;
	PLAY_MODE			playmode_backup;

	PromptCallback		prompt_callback;

	int					select_book;
	
	int					info_module;
	int					info_pause;
	
	int					text_pause;
	
	Ju_Func				*prev_ju_func;
	Ju_Func				*next_ju_func;
	int					*hlevel;
	char**				func_name;
	int					func_count;
	int					func_cur;

//wait for tts time_out 
	int 	wait_tts_active;
	int 	wait_tts_count;
	
};

static struct TextBook *gbook = 0;

static int txMain_closeCurBook(struct TextBook *thiz, int bDelFile);
static int txMain_openNewBook(struct TextBook *thiz);

static void txMain_stopPrompt(void);
static int txMain_stopText(struct TextBook *thiz);
static int txMain_playText(struct TextBook *thiz);
static int txMain_resumeText(struct TextBook *thiz);
static int txMain_pauseText(struct TextBook *thiz);

static int txMain_fastRewind(struct TextBook *thiz);
static int txMain_fastForward(struct TextBook *thiz);
static void txMain_grabKey (struct TextBook*thiz);
static void txMain_ungrabKey (struct TextBook*thiz);
static void OnWindowRedraw(int clrBg);
static int txMain_endStop(struct TextBook *thiz);

static int txMain_isOpen(struct TextBook *thiz)
{
	int ret = 0;
	if(thiz && thiz->text_parse){
		ret = 1;
	}
	return ret;
}

static int txMain_showScreen(void)
{
	DBGMSG("show\n");
	OnWindowRedraw(1);
	return 1;
}

static void txMain_delCurTitle(void)
{
	DBGMSG("txMain_delCurTitle\n");
	struct TextBook*thiz = gbook;

	if(!thiz || !thiz->text_parse){
		DBGMSG("failed\n");
		NhelperDeleteResult(APP_DELETE_ERR);
		return;
	}

	if(isMediaReadOnly(thiz->textfile)){
		DBGMSG("lock\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);
		return;
	}
	
//	txMain_stopPrompt();
	txMain_stopText(thiz);
	txMain_closeCurBook(thiz,1);
	
	NhelperDeleteResult(APP_DELETE_OK);

	if(thiz->text_pause){
		DBGMSG("pause now\n");
		return;
	}
	thiz->exit_type = EXIT_MEDIA_REMOVE;
	NeuxClearModalActive();
}

static void txMain_setBookSpeed(void)
{
	struct TextBook*thiz = gbook;
	return_if_fail(thiz != NULL);
	if(thiz->book_tts){
		tts_obj_set_speed(thiz->book_tts, thiz->setting.book.speed);
	}
}

static void txMain_setBookEqualizer(void)
{
	struct TextBook*thiz = gbook;
	return_if_fail(thiz != NULL);
	if(thiz->book_tts){
		tts_obj_set_band(thiz->book_tts, thiz->setting.book.text_equalizer);
	}
}

static void txMain_msgPause(void)
{
	struct TextBook* thiz = gbook;
	return_if_fail(thiz != NULL);

	int for_fast_stop = 0;
	
	/* In case of alarm comes but still in key timer */
	if (thiz->timerEnable) {
		TimerDisable(thiz->keytimer);
		thiz->timerEnable = 0;
		thiz->longcount = 0;
		thiz->playmode = MODE_PAUSE;
		thiz->prompt_callback = 0;
		thiz->playmode_backup = MODE_PLAYING;
		for_fast_stop = 1;
	}
	
	if(thiz->select_book){
		info_debug("select book, not pause\n");
		return;
	}

	if(thiz->info_module){
		//in info module
		//if(!thiz->info_pause){
		//	thiz->info_pause = 1;
			info_pause();
		//}
		return;
	}

	//in text main
	if(thiz->text_pause){
		info_debug("had pause\n");
		return;
	}
	DBGMSG("txMain_msgPause\n");
	txMain_ungrabKey(thiz);
	
	PromptCallback callback = thiz->prompt_callback;
	txMain_stopPrompt();	//stop prompt

	if (!for_fast_stop) {
		thiz->prompt_callback = callback;	//for back resume
		thiz->playmode_backup = thiz->playmode;
	}
	
	txMain_pauseText(thiz);

	thiz->text_pause = 1;
}

static void txMain_msgResume(void)
{
	struct TextBook* thiz = gbook;
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

	if(!thiz->text_pause){
		info_debug("non resume\n");
		return;
	}
	thiz->text_pause = 0;

	DBGMSG("txMain_msgResume\n");
//	if(!txMain_isOpen(thiz)){
//		DBGMSG("txt close\n");
//		thiz->exit_type = EXIT_MEDIA_REMOVE;
//		NeuxClearModalActive();
//		return;
//	}
	
	if(!PlextalkIsFileExist(thiz->textfile)){
		DBGMSG("file remove\n");
		thiz->exit_type = EXIT_MEDIA_REMOVE;
		NeuxClearModalActive();
		return;
	}
	
	txMain_grabKey(thiz);
	if(thiz->prompt_callback){
		PromptCallback callback = thiz->prompt_callback;
		thiz->prompt_callback = 0;
		callback(thiz);//in callback maybe reset prompt_callback
	}else if(MODE_PLAYING == thiz->playmode_backup){
		thiz->playmode_backup = MODE_NULL;
		txMain_resumeText(thiz);
	}

}


static void txMain_showStateIcon(PLAY_MODE playmode)
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

static void txMain_showTitleBar(struct TextBook *thiz)
{
	if(NULL == thiz || 0==strlen(thiz->textfile))
	{
		NhelperTitleBarSetContent(_("Read text"));
		return;
	}

	char *text_title = strrchr(thiz->textfile, '/');
	if(text_title){
		text_title++;
		NhelperTitleBarSetContent(text_title);
		return;
	}else{
		NhelperTitleBarSetContent(_("Read text"));
	}
}

static void txMain_showCateIcon(struct TextBook *thiz)
{ 
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON,SBAR_CATEGORY_ICON_BOOK_TEXT); 
}


static void txMain_showMediaIcon(char *path)
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

static int txMain_stopTts(struct TextBook *thiz)
{
	//info_debug("book text tts stop\n");
	thiz->tts_play = 0;
	tts_obj_stop(thiz->book_tts);
	return 0;
}

static int txMain_startTts(struct TextBook *thiz)
{
	//info_debug("txMain_startTts start\n");
	int len;
	char *tts_text;
	thiz->tts_play = 0;
	tts_obj_stop(thiz->book_tts);

	tts_text = txShow_getCurSentence(&thiz->text_show);
	if(!tts_text){
		info_debug("text is null\n");
		return -1;
	}

	len = strlen(tts_text);
	info_debug("len=%d\n", len);
	//info_debug("%s\n", (char*)tts_text);
	
//	tts_obj_start(thiz->book_tts, thiz->tts_mode, (char*)tts_text, len, ivTTS_CODEPAGE_UTF8, tts_role, tts_lang);

	tts_obj_start(thiz->book_tts, (char*)tts_text, len, ivTTS_CODEPAGE_UTF8);

	thiz->tts_play = 1;
	//info_debug("txMain_startTts end\n");
	return 0;

}

static int txMain_pauseAudio(struct TextBook *thiz)
{
	if(thiz->playmode != MODE_PLAYING){
		return 0;
	}
	 
	txMain_stopTts(thiz);
	thiz->playmode = MODE_PAUSE;
	return 1;
}

static int txMain_resumeAudio(struct TextBook *thiz)
{
	if(thiz->playmode != MODE_PAUSE){
		return 0;
	}
	  
	txMain_startTts(thiz);
	thiz->playmode = MODE_PLAYING;
	return 0;
}

static int txMain_playAudio(struct TextBook *thiz)
{ 
	txMain_startTts(thiz);
	thiz->playmode = MODE_PLAYING;
	return 0;
}

static int txMain_stopAudio(struct TextBook *thiz)
{
	if(MODE_END_STOP == thiz->playmode){
		return 0;
	}
	txMain_stopTts(thiz);
	thiz->playmode = MODE_STOP;
	return 0;
}

static int txMain_stopText(struct TextBook *thiz){
	txMain_stopAudio(thiz);
	txMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int txMain_playText(struct TextBook *thiz){
	txMain_playAudio(thiz);
	txMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int txMain_resumeText(struct TextBook *thiz){
	DBGMSG("resume\n");
	txMain_resumeAudio(thiz);
	txMain_showStateIcon(thiz->playmode);
	return 0;
} 

static int txMain_pauseText(struct TextBook *thiz){
	DBGMSG("pause\n");
	txMain_pauseAudio(thiz);
	txMain_showStateIcon(thiz->playmode);
	return 0;
} 

static void txMain_playPrompt(enum tts_type type,char *prompt,
	PromptCallback callback){

	struct TextBook *thiz = gbook;
	thiz->wait_tts_active = 1;
	thiz->wait_tts_count = 0;
	DBGMSG("txMain play promptd, start do count!\n");

	thiz->tts_tip = 1;
	thiz->prompt_callback = callback;
	book_prompt_play(type,prompt);
}

static void txMain_stopPrompt(void){

	struct TextBook *thiz = gbook;
	thiz->tts_tip = 0;
	thiz->prompt_callback = 0;
	book_prompt_stop();
}

//check tts prompt play end
static void txMain_promptCheck(struct TextBook *thiz)
{
	if(!thiz->tts_tip){
		return;
	}

	if (thiz->wait_tts_active) {
		//DBGMSG("thiz->wait_tts_active = %d\n", thiz->wait_tts_active);
		//DBGMSG("thiz->wait_tts_count = %d\n", thiz->wait_tts_count);

		thiz->wait_tts_count ++;

		if (thiz->wait_tts_count > 30) {	//30 * 0.3s = 9s
			//DBGMSG("Wait TTS TIME OUT!!!\n");
			tip_is_running = 0;
			thiz->wait_tts_count = 0;
			thiz->wait_tts_active = 0;
		}
	}

	if(tip_is_running){
		return;
	}

	if (thiz->wait_tts_active) {
		thiz->wait_tts_active = 0;
		thiz->wait_tts_count = 0;
	}
	
	thiz->tts_tip = 0;
	info_debug("tipvoice end\n");

	if(thiz->prompt_callback){
		PromptCallback callback = thiz->prompt_callback;
		thiz->prompt_callback = 0;
		callback(thiz);
	}
}

static int txMain_replayScrText(struct TextBook *thiz)
{
	return_val_if_fail(NULL != thiz->text_parse, -1);
	if(MODE_PLAYING != thiz->playmode){
		return -1;
	}
	//replay tts
	txMain_stopAudio(thiz);
	txMain_playAudio(thiz);
	return 0;
}

static int txMain_reCalShowScr(struct TextBook *thiz)
{
	return_val_if_fail(NULL != thiz->text_parse, -1);
	DBGMSG("size:%d\n", thiz->text_show.textSize);
	if(thiz->text_show.textSize<=0){
		return -1;
	}
	DBGMSG("line:%d\n", thiz->text_show.total_line);
	if(thiz->text_show.total_line<=0){
		return -1;
	}
	
	char *pstr = thiz->text_show.pScrStart;
	DBGMSG("pstr:%x\n",(int)pstr);
	txShow_parseText(&thiz->text_show, pstr, -1, thiz->text_show.codepage, thiz->text_show.bMoreText);
	txMain_showScreen();
	return 0;
}

static int txMain_getTextInfo(struct TextBook *thiz)
{
	int ret;
	return_val_if_fail(NULL != thiz->text_parse, -1);
	
	ret = text_getText(thiz->text_parse);
	if(ret<0){
		info_err("get err\n");
	}
	
	ret = txShow_parseText(&thiz->text_show, thiz->text_parse->strText, thiz->text_parse->strSize, 
		thiz->text_parse->codepage, !thiz->text_parse->isTextEnd);
	info_debug("ret=%d\n", ret);
	return 0;
}

static int txMain_firstScreen(struct TextBook *thiz, int play)
{
	int ret;
	
	ret = text_resumeMark(thiz->text_parse);
	if( ret < 0)
	{//no resume book mark
		text_jumpHead(thiz->text_parse);
	}
	ret = txMain_getTextInfo(thiz);
	if(ret < 0)
	{	
		info_err("error\n");
		return -1;
	}
	txMain_showScreen();
	return 0;
}


static int txMain_jumpHead(struct TextBook *thiz)
{
	int ret;
	info_debug("jump head\n");
	
	text_jumpHead(thiz->text_parse);
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	txMain_playText(thiz);
	return 0;
}

static int txMain_jumpTail(struct TextBook *thiz)
{
	int ret;
	info_debug("jump tail\n");
	
	text_jumpTail(thiz->text_parse);
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	//txMain_playText(thiz);
	return 0;
}


static int txMain_jumpNextError(struct TextBook *thiz)
{	
	DBGMSG("navigation err\n");
//	if(MODE_END_STOP==thiz->playmode){
//		txMain_playPrompt(BOOK_END, 0, 0);
//		return 0;
//	}
	if(thiz->setting.book.repeat){//repeat
		DBGMSG("repeat\n");
		//txMain_jumpHead(thiz);
		txMain_playPrompt(ERROR_BEEP, 0, txMain_jumpHead);
	}else{//standard play
		DBGMSG("standard play\n");
		txMain_endStop(thiz);
		txMain_jumpTail(thiz);
		txMain_playPrompt(BOOK_END, 0, 0);
	}
	return 0;
}

static int txMain_jumpPrevError(struct TextBook *thiz)
{	
	DBGMSG("navigation err\n");
//	if(thiz->setting.book.repeat){//repeat
//		DBGMSG("repeat\n");
//		txMain_jumpHead(thiz);
//	}else{//standard play
//		DBGMSG("standard play\n");
		txMain_playPrompt(BOOK_BEGIN, 0, txMain_jumpHead);
//	}
	return 0;
}

static int txMain_nextScreen(struct TextBook *thiz, int play)
{
	int ret;
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_nextScreen(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpNextError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}

	return 0;
}

static int txMain_prevScreen(struct TextBook *thiz, int play)
{
	int ret;

 	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_prevScreen(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpPrevError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}
	 
	return 0;
}


static int txMain_nextParagraph(struct TextBook *thiz, int play)
{
	int ret;
 	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_nextParagraph(thiz->text_parse,byteoff);
	if(ret < 0){
		ret = txMain_jumpNextError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}
	return 0;
}

static int txMain_prevParagraph(struct TextBook *thiz, int play)
{
	int ret;
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_prevParagraph(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpPrevError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}
	
	return 0;
}

static int txMain_prevSentence(struct TextBook *thiz, int play)
{
	int ret;
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_prevSentence(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpPrevError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}
	return 0;
}

static int txMain_nextSentence(struct TextBook *thiz, int play)
{
	int ret;
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_nextSentence(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpNextError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		txMain_playText(thiz);
	}
	return 0;
}


static int txMain_prevHeading(struct TextBook *thiz, int play)
{
	int ret; 
	info_debug("text prev heading\n");
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_prevHeading(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpPrevError(thiz);
		return ret;
	}
	else{
		txMain_getTextInfo(thiz);
		txMain_showScreen();

		if(play){
			txMain_playText(thiz);
		}
	}
	return 0;  
}
static int txMain_nextHeading(struct TextBook *thiz, int play)
{
	int ret;
	info_debug("text next heading\n");
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_nextHeading(thiz->text_parse, byteoff);
	if(ret < 0){
		ret = txMain_jumpNextError(thiz);
		return ret;
	}
	else{
		info_debug("next heading success\n");
		txMain_getTextInfo(thiz);
		txMain_showScreen();
		if(play){
			txMain_playText(thiz);
		}
	}
	return 0; 
}


static int txMain_prevMark(struct TextBook *thiz, int play)
{
	int ret; 
	ret = text_prevMark(thiz->text_parse);
	if(ret < 0){
		//ret = txMain_jumpHead(thiz);
		info_err("mark error");
		if(MODE_END_STOP == thiz->playmode){
			txMain_playPrompt(ERROR_BEEP, 0, 0);
		}else{
			txMain_playPrompt(ERROR_BEEP, 0, txMain_playText);
		}
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		char tipstr[128];
		snprintf(tipstr, 128, "%s:%d ", _("bookmark"), thiz->text_parse->mark->cur+1);
		txMain_playPrompt(BOOK_PROMPT, tipstr,txMain_playText);
	} 
	return 0;
}

static int txMain_nextMark(struct TextBook *thiz, int play)
{
	int ret;
 
	ret = text_nextMark(thiz->text_parse);
	if(ret < 0){
		//ret = txMain_jumpHead(thiz);
		info_err("mark error");
		if(MODE_END_STOP == thiz->playmode){
			txMain_playPrompt(ERROR_BEEP, 0, 0);
		}else{
			txMain_playPrompt(ERROR_BEEP, 0, txMain_playText);
		}
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	if(play){
		char tipstr[128];
		snprintf(tipstr, 128, "%s:%d ", _("bookmark"), thiz->text_parse->mark->cur+1);
		txMain_playPrompt(BOOK_PROMPT, tipstr,txMain_playText);
		//book_play(thiz);
	}
	return 0;
}

int txMain_phrase2mark(MARK *mark)
{
	struct TextBook *thiz = gbook;
	return_val_if_fail(thiz, -1);
	return_val_if_fail(thiz->text_parse, -1);
	
	if(MODE_END_STOP == thiz->playmode){
		mark->ulOffset = 0;
		return 0;
	}
	int byteoff = txShow_getScrStartByte(&thiz->text_show);
	DBGMSG("byteoff:%d\n",byteoff);
	mark->ulOffset = text_nextOffset(thiz->text_parse, byteoff);
	DBGMSG("ulOffset:%lld\n",mark->ulOffset);
	return 0;
}

static int txMain_setMark(struct TextBook *thiz)
{
	int ret;
	MARK mark;
	DBGMSG("set mark\n");
	if(!thiz || !thiz->text_parse){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
		return -1;
	}
	
	if(isMediaReadOnly(thiz->text_parse->fname)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}

	ret = txMain_phrase2mark(&mark);
	if(ret < 0){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
		return -1;
	}
	
	ret = mark_checkMark(thiz->text_parse->mark, &mark);
	if(ret >= 0){
		if(ret >= MAX_MARK_COUNT){
			NhelperBookMarkResult(APP_BOOKMARK_UPPER_LIMIT_ERR,0);
		}else{
			NhelperBookMarkResult(APP_BOOKMARK_EXIST_ERR,ret+1);
		}
		return -1;
	}
	
	ret = mark_addMark(thiz->text_parse->mark, &mark);
	if(ret<0){
		NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_SET_OK,0);
	}
	return ret;
}

static int txMain_delCurMark(struct TextBook *thiz)
{
	int ret;
	DBGMSG("del cur mark\n");
	if(!thiz || !thiz->text_parse){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
		return -1;
	}
	
	if(isMediaReadOnly(thiz->text_parse->fname)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}
	
	ret = mark_delCurMark(thiz->text_parse->mark);
	if(ret<0){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);
	}
	return ret;
}

static int txMain_delAllMark(struct TextBook *thiz)
{
	int ret;
	DBGMSG("del all mark\n");
	if(!thiz || !thiz->text_parse){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
		return -1;
	}
	
	if(isMediaReadOnly(thiz->text_parse->fname)){
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);
		return -1;
	}

	ret = mark_delAllMark(thiz->text_parse->mark);
	if(ret<0){
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
	}else{
		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);
	}
	return ret;
}

static int txMain_headingLevels(struct TextBook *thiz)
{
	return 1;
}

static int txMain_fastRewindCb(struct TextBook *thiz){
	info_debug("fast_rewind_callback\n");
	if(thiz->longcount > 0){
		//key also hold,continue fast
		txMain_fastRewind(thiz);
	}else{
		//key has release,play
		txMain_playAudio(thiz);
	}
	return 0;
}

static int txMain_fastForwardCb(struct TextBook *thiz){
	info_debug("fast_forward_callback\n");
	if(thiz->longcount > 0){
		//key also hold,continue fast
		txMain_fastForward(thiz);
	}else{
		//key has release,play
		txMain_playAudio(thiz);
	}
	return 0;
}

static int txMain_fastRewind(struct TextBook *thiz)
{
	char tipstr[256];
	int ret = 0;
	
	info_debug("text_fast_rewind\n");
	return_val_if_fail(NULL != thiz->text_parse, -1);
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_prev5Sentence(thiz->text_parse, byteoff);
	if(ret < 0){
		info_err("rewind end\n");
		ret = txMain_jumpPrevError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	thiz->fast_cnt ++;
	if(1 == thiz->fast_cnt){
		snprintf(tipstr, 256, "%s %d %s", _("minus"),thiz->fast_cnt*5,_("sentences"));
	}else{
		snprintf(tipstr, 256, "%d %s", thiz->fast_cnt*5,_("sentences"));
	}
	txMain_playPrompt(BOOK_PROMPT, tipstr, txMain_fastRewindCb);
	
	return 0;
}

static int txMain_fastForward(struct TextBook *thiz)
{
	char tipstr[256];
	int ret = 0;
	
	info_debug("text forward\n");
	return_val_if_fail(NULL != thiz->text_parse, -1);
	int byteoff = txShow_getSenStartByte(&thiz->text_show);
	ret = text_next5Sentence(thiz->text_parse, byteoff);
	if(ret < 0){
		info_err("forward end\n");
		ret = txMain_jumpNextError(thiz);
		return ret;
	}
	txMain_getTextInfo(thiz);
	txMain_showScreen();
	thiz->fast_cnt ++;
	snprintf(tipstr, 256, "%d %s", thiz->fast_cnt*5,_("sentences"));
	txMain_playPrompt(BOOK_PROMPT, tipstr, txMain_fastForwardCb);
	return 0;
}

static int txMain_endStop(struct TextBook *thiz)
{
	txMain_stopAudio(thiz);
	thiz->playmode = MODE_END_STOP;
	txMain_showStateIcon(thiz->playmode);
	return 0;
}

static int txMain_enterKeyPro(struct TextBook *thiz)
{
	info_debug("mode=%d\n", thiz->playmode);
	if(thiz->playmode == MODE_PLAYING)
	{
		info_debug("!!book pause\n");
		txMain_pauseText(thiz);
		txMain_playPrompt(CANCEL_BEEP, NULL, NULL);
	}
	else if(thiz->playmode == MODE_PAUSE)
	{
		info_debug("!!book resume\n");
		txMain_playPrompt(NORMAL_BEEP, NULL, txMain_resumeText);
	}
	else if(thiz->playmode == MODE_STOP)
	{
		info_debug("!!book play\n");
		txMain_playText(thiz);
	}
	else if(thiz->playmode == MODE_END_STOP)
	{
		info_debug("!!book replay\n");
		txMain_jumpHead(thiz);
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
static int txMain_setNaviFunc(struct TextBook *thiz)
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

	total = txMain_headingLevels(thiz);
	mtol = total + JU_MAX_FUNC;
	thiz->prev_ju_func = app_malloc(
		sizeof(Ju_Func)*mtol);
	thiz->next_ju_func = app_malloc(
		sizeof(Ju_Func)*mtol);
	
	thiz->func_name = app_malloc(
		sizeof(char*)*mtol);
	thiz->hlevel = app_malloc(
		sizeof(int)*mtol);
	
	if(thiz->text_parse->funcs & JU_HEADING_FUNC){
		i = 1;
		while(i <= total){
			thiz->prev_ju_func[thiz->func_count] = txMain_prevHeading;
			thiz->next_ju_func[thiz->func_count] = txMain_nextHeading;
			thiz->hlevel[thiz->func_count] = i;
			thiz->func_name[thiz->func_count] = __("heading");
			thiz->func_count++;
			i++;
		}
	}

	if(thiz->text_parse->funcs & JU_SCREEN_FUNC){
		thiz->prev_ju_func[thiz->func_count] = txMain_prevScreen;
		thiz->next_ju_func[thiz->func_count] = txMain_nextScreen;
		thiz->func_name[thiz->func_count] = __("screen");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->text_parse->funcs & JU_PARA_FUNC){
		thiz->prev_ju_func[thiz->func_count] = txMain_prevParagraph;
		thiz->next_ju_func[thiz->func_count] = txMain_nextParagraph;
		thiz->func_name[thiz->func_count] = __("paragraph");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}

	if(thiz->text_parse->funcs & JU_SEN_FUNC){
		thiz->prev_ju_func[thiz->func_count] = txMain_prevSentence;
		thiz->next_ju_func[thiz->func_count] = txMain_nextSentence;
		thiz->func_name[thiz->func_count] = __("sentence");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	if(thiz->text_parse->funcs & JU_BOOKMARK_FUNC){
		thiz->prev_ju_func[thiz->func_count] = txMain_prevMark;
		thiz->next_ju_func[thiz->func_count] = txMain_nextMark;
		thiz->func_name[thiz->func_count] = __("bookmark");
		thiz->hlevel[thiz->func_count] = 0;
		thiz->func_count++;
	}
	
	return 0;
}

static void txMain_setAudioMode(struct TextBook *thiz)
{
	book_setAudioMode(0);
}

static int txMain_deleteMarkFiles(char *markfile)
{
	char cmd[PATH_MAX+10];
	if(NULL==markfile || 0==strlen(markfile)){
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", markfile);
	system(cmd); 
	return 0;
}

static int txMain_closeCurBook(struct TextBook *thiz, int bDelFile)
{
	char markfile[PATH_MAX];
	
	markfile[0]=0;
	if(thiz->text_parse){
		char *file = mark_getPath(thiz->text_parse->mark);
		if(NULL!=file){
			strlcpy(markfile, file, PATH_MAX);
		}
		text_close(thiz->text_parse);
		info_debug("book_close end\n");
		thiz->text_parse = 0;
	}
	
	if(bDelFile){
		char cmd[PATH_MAX+10];
		txMain_deleteMarkFiles(markfile);
		snprintf(cmd,sizeof(cmd),"rm -rf \"%s\"", thiz->textfile);
		system(cmd);
	}

	return 0;
}

static int txMain_openEndCb(struct TextBook *thiz)
{
	info_debug("txMain_openEndCb\n");
	if(TEXT_OPEN_OK == thiz->open_state)
	{
		info_debug("open ok\n");
		txMain_playText(thiz);
	}
	else if(TEXT_OPEN_ERR== thiz->open_state)
	{
		info_debug("open err\n");
		txMain_playPrompt(BOOK_OPEN_ERR, 0, 0);
		thiz->exit_type = EXIT_NORMAL;
		NeuxClearModalActive();
	}
	else if(TEXT_OPEN_ERR_NOT_ENOUGH_WORK_AREA== thiz->open_state)
	{
		info_debug("open err\n");
		txMain_playPrompt(BOOK_OPEN_ERR_NOT_ENOUGH_WORK_AREA, 0, 0);
		thiz->exit_type = EXIT_NORMAL;
		NeuxClearModalActive();
	}
	else if(TEXT_OPEN_ERR_CONVERT_CHARSET== thiz->open_state)
	{
		info_debug("open err\n");
		txMain_playPrompt(BOOK_OPEN_ERR_CONVERT_CHARSET, 0, 0);
		thiz->exit_type = EXIT_NORMAL;
		NeuxClearModalActive();
	}
	return 0;
}

static void txMain_initAudio(struct TextBook *thiz)
{
	thiz->book_tts = booktts;
	txMain_setBookSpeed();
	txMain_setBookEqualizer();
	return ;
}


static const char * const html_epub[] = {
	"htm",
	"html",
	"epub",
	NULL
};

static int isTheFile(const char * name, const char * const * p)
{
    char* ext;

	ext = PlextalkGetFileExtension(name);
	if (NULL == ext)
		return 0;

	while (*p) {
		if (!strcasecmp(ext, *p))
			return 1;
		p++;
	}

	return 0;
}

//open 
static int txMain_openNewBook(struct TextBook *thiz)
{
	int ret = -1; 

	if(thiz->text_parse){
		text_close(thiz->text_parse);
		thiz->text_parse = NULL;
	}

	if(0 == thiz->textfile[0]){
		return -1;
	}
	
	//info_debug("open new file:%s\n", thiz->textfile);
	thiz->open_state = TEXT_OPENING;
	txMain_playPrompt(BOOK_TITLE_OPEN, NULL, txMain_openEndCb);

	//1.is text book
	ret = PlextalkIsBookFile(thiz->textfile);
	if(!ret){
		info_err("not text book\n");
		thiz->open_state = TEXT_OPEN_ERR;
		goto fail;
	}
//============================================
	if(isTheFile(thiz->textfile, html_epub))
	{
		PlexResult result = ConvertHtmlEpubToText(thiz->textfile, thiz->genfile, PATH_MAX);
		if(PLEX_OK!=result)
		{
			if(PLEX_ERR_NOT_ENOUGH_WORK_AREA == result){
				thiz->open_state = TEXT_OPEN_ERR_NOT_ENOUGH_WORK_AREA;
			}else if(PLEX_ERR_CONVERT_CHARSET == result){
				thiz->open_state = TEXT_OPEN_ERR_CONVERT_CHARSET;
			}else{
				thiz->open_state = TEXT_OPEN_ERR;
			}
			goto fail;
		}
	}else{
		strlcpy(thiz->genfile, thiz->textfile, PATH_MAX);
	}
//============================================
	//2.open book and initial something
	thiz->text_parse = text_open(thiz->textfile, thiz->genfile); 
	if(NULL == thiz->text_parse){
		thiz->open_state = TEXT_OPEN_ERR;
		goto fail;
	}

	//3.set book navigation
	txMain_setNaviFunc(thiz);
	
	//5.save file for backup
	book_set_backup_path(thiz->textfile);
	
	//6.set file come form:sdcard or usb or storage
	txMain_showMediaIcon(thiz->textfile);

	//7.change titlebar icon
	txMain_showCateIcon(thiz);
	
	//8.change supplement display
	//book_change_supplement(thiz);

	//9.save andio mode, adjust volumn
	txMain_setAudioMode(thiz);
	
	//10.save the current file, for open again
	book_set_last_path(thiz->textfile);
	
	//set title bar
	txMain_showTitleBar(thiz);
	
	info_debug("first screen\n");
	txMain_firstScreen(thiz, 0);
	info_debug("first screen end\n");

	txMain_initAudio(thiz);
	
	thiz->open_state = TEXT_OPEN_OK;
	return 0;
fail:
	return -1;
}

static void txMain_destroy(struct TextBook *thiz)
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

		if(thiz->book_tts){
			tts_obj_stop(thiz->book_tts);
		}

		if(thiz->text_parse){
			text_close(thiz->text_parse);
			thiz->text_parse = 0;
		}

		txMain_ungrabKey(thiz);
		app_free(thiz);
		thiz = NULL;
	}
}


static struct TextBook *txMain_create(void)
{
	struct TextBook *thiz;

	thiz = (struct TextBook *)app_malloc(sizeof(struct TextBook));
	if(!thiz){
		info_err("malloc struct TextBook\n");
		return NULL;
	}
	memset(thiz, 0, sizeof(struct TextBook));
	gbook = thiz;
	return thiz;
}

static int txMain_setPrevUnit(struct TextBook *thiz)
{
	int paused;
	char tipstr[256];
	PromptCallback callback = NULL;
		
	return_val_if_fail(NULL != thiz->text_parse, -1);
	
	if(MODE_NULL == thiz->playmode){
		DBGMSG("MODE:%d\n", thiz->playmode);
		return -1;
	}else if(MODE_PLAYING == thiz->playmode){
		paused = txMain_pauseText(thiz);
		paused = 1;
	}else {
		paused = 0;
		callback = thiz->prompt_callback;
	}
	
	if(thiz->func_cur - 1 < 0){/*limit*/
		thiz->func_cur = thiz->func_count - 1;
	}
	else{
		thiz->func_cur--;
	}

	snprintf(tipstr, sizeof(tipstr), "%s", gettext(thiz->func_name[thiz->func_cur]));
	info_debug("unit=%s\n", tipstr);
	txMain_playPrompt(BOOK_PROMPT,tipstr,paused?txMain_resumeText:callback);
	return 0;

}

static int txMain_setNextUnit(struct TextBook*thiz)
{
	int paused;
	char tipstr[256];
	PromptCallback callback = NULL;

	return_val_if_fail(NULL != thiz->text_parse, -1);
	if(MODE_NULL == thiz->playmode){
		DBGMSG("MODE:%d\n", thiz->playmode);
		return -1;
	}else if(MODE_PLAYING == thiz->playmode){
		paused = txMain_pauseText(thiz);
		paused = 1;
	}else {
		paused = 0;
		callback = thiz->prompt_callback;
	}
	
	if(thiz->func_cur + 1 >= thiz->func_count){
		thiz->func_cur = 0;
	}
	else{
		thiz->func_cur++;
	}

	snprintf(tipstr, sizeof(tipstr), "%s", gettext(thiz->func_name[thiz->func_cur]));
	
	info_debug("unit=%s\n", tipstr);
	txMain_playPrompt(BOOK_PROMPT,tipstr,paused?txMain_resumeText:callback);
	return 0;

}

static int txMain_prevUnit(struct TextBook *thiz)
{
	return_val_if_fail(thiz != NULL, -1);
	return_val_if_fail(thiz ->text_parse!= NULL, -1);

	txMain_stopPrompt();
	txMain_stopAudio(thiz);
	thiz->prev_ju_func[thiz->func_cur](thiz, 1);
	return 0;
}


static int txMain_nextUnit(struct TextBook *thiz)
{
	return_val_if_fail(thiz != NULL, -1);
	return_val_if_fail(thiz ->text_parse!= NULL, -1);
	
	txMain_stopPrompt();
	txMain_stopAudio(thiz);
	thiz->next_ju_func[thiz->func_cur](thiz, 1);
	return 0;
}

static int txMain_infomation(struct TextBook *thiz)
{
	#define BUF_LEN  256
	#define TEXT_NUM 4
	
	int i, text_num;
	char (*p)[BUF_LEN] = NULL;
	char *info_text[TEXT_NUM];
	if(thiz->info_module){
		info_debug("!!!!!!rein\n");
		return 0;
	}
	//pause
	info_debug("info\n");
	txMain_msgPause();
	thiz->info_module = 1;
	thiz->info_pause= 0;
	
	NhelperTitleBarSetContent(_("Information")); 
	
	/* Create Info id */										 
	void *info_id = info_create(); 

	text_num = 0;
	if(NULL != thiz->text_parse){
		p = app_malloc(BUF_LEN*sizeof(char)*TEXT_NUM);
		for(i=0; i< TEXT_NUM; i++){
			info_text[i] = p+i;
		}

		txInfo_PercentElapseInfo(thiz->text_parse, info_text[text_num++], BUF_LEN, (MODE_END_STOP==thiz->playmode));
		txInfo_BookmarkNumberInfo(thiz->text_parse, info_text[text_num++], BUF_LEN);
		txInfo_TittleNameInfo(thiz->text_parse, info_text[text_num++], BUF_LEN);
		txInfo_CreationDateInfo(thiz->text_parse, info_text[text_num++], BUF_LEN);
	}

	info_start (info_id, info_text, text_num, &tip_is_running);
	
	info_destroy (info_id);
	
	if(NULL != p){
		app_free(p);
		p= NULL;
	}
	//resume
	thiz->info_module = 0;
	thiz->info_pause= 0;
	txMain_msgResume();

	NeuxFormShow(thiz->text_show.form, TRUE);
	NeuxWidgetFocus(thiz->text_show.form);
	
	#undef BUF_LEN
	#undef TEXT_NUM
	return 1;
}

#if 0
static void txMain_volumeKey(struct TextBook *thiz, int key)
{
	int limit = 0;
	int cur_vol = book_getCurVolumn();
	if(cur_vol == last_vol ){
		if(cur_vol >= PLEXTALK_SOUND_VOLUME_MAX 
			&& MWKEY_VOLUME_UP == key){
			limit = 1;
		} 
	}
	DBGMSG("volumn:%d\n",cur_vol);
	if(MODE_PLAYING == thiz->playmode){
		if(limit){
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
		}else{
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		}
	}else{
		if(limit){
			txMain_playPrompt(LIMIT_BEEP, 0 ,thiz->prompt_callback);
		}else{
			char volstr[50];
			DBGMSG("volumn22\n");
			snprintf(volstr, sizeof(volstr), "%s %d",_("Volume"), cur_vol);
			txMain_playPrompt(BOOK_VOLUMN,volstr ,thiz->prompt_callback);
		}
	}
	last_vol = cur_vol;
}
#endif
void txMain_keyTimerOut(void)
{
	struct TextBook*thiz = gbook;
	return_if_fail(thiz != NULL);
	if(thiz->select_book){
		return;
	}

	info_debug("keymask=0x%x\n", thiz->keymask);
	thiz->longcount++;
	switch(thiz->keymask){
	case MWKEY_LEFT:
		if(1 == thiz->longcount)
		{	//first in, after for callback--fast_forward_callback
			txMain_stopPrompt();
			txMain_stopAudio(thiz);
			txMain_fastRewind(thiz);
		} 
		break;
	case MWKEY_RIGHT:
		if(1 == thiz->longcount)
		{	//first in, after for callback--fast_forward_callback
			txMain_stopPrompt();
			txMain_stopAudio(thiz);
			txMain_fastForward(thiz);
		}
		break;
	}
}

int txMain_keyUpHandle(KEY__ key)
{
	int iret = 1;
	struct TextBook*thiz = gbook;
	return_val_if_fail(thiz != NULL, -1);
	
	/* select file or text pause or info moudle cant reponse key*/
	if(thiz->select_book || thiz->info_module || thiz->text_pause){
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
			txMain_nextUnit(thiz);
		}
		thiz->longcount = 0;
		break;
	case MWKEY_LEFT:
		if(thiz->longcount <= 0){
			//short press
			txMain_prevUnit(thiz);
		}
		thiz->longcount = 0;
		break;
//	case MWKEY_INFO:
//		if(thiz->longcount <= 0){
//			txMain_infomation(thiz);
//		}
//		thiz->longcount = 0;
//		break;
	default:
		break;
	}
	return iret;
}


int txMain_keyDownHandle(KEY__ key)
{
	int iret = 1;
	struct TextBook* thiz = gbook;
	return_val_if_fail(thiz != NULL, -1);

	/* select file or text pause or info moudle cant reponse key*/
	if(thiz->select_book || thiz->info_module || thiz->text_pause){
		return -1;
	}
	if (NeuxAppGetKeyLock(0)) {
		info_debug("key lock\n");
		if(MODE_PLAYING == thiz->playmode){
			txMain_pauseText(thiz);
			txMain_playPrompt(BOOK_KEYLOCK,0,txMain_resumeText);
		}else{
			txMain_playPrompt(BOOK_KEYLOCK,0,thiz->prompt_callback);
		}
		return 1;
	}

	switch(key.ch){
	case MWKEY_ENTER:
		{
			txMain_enterKeyPro(thiz);
		}
		break;
	case MWKEY_INFO:
		txMain_infomation(thiz);
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
//		txMain_volumeKey(thiz, key.ch);
//		break;

	case MWKEY_UP:
		{
			txMain_setPrevUnit(thiz);
		}
		break;
	case MWKEY_DOWN:
		{
			txMain_setNextUnit(thiz);
		}
		break;

	case VKEY_BOOK:
		{
			txMain_stopPrompt();
			txMain_stopText(thiz);
			//
			txMain_closeCurBook(thiz, 0);
			//exit, select new file
			thiz->exit_type = EXIT_NORMAL;
			NeuxClearModalActive();
			//txMain_select_new_file(thiz);
			//txMain_open_new_book(thiz);
		}
		break;
	default:
		break;
	}

	return iret;
}

void txMain_audioCheck(struct TextBook *thiz)
{
	return_if_fail(NULL != thiz->text_parse);

	if(thiz->book_tts && thiz->tts_play && !tts_obj_isruning(thiz->book_tts)){
		thiz->tts_play = 0;
		DBGMSG("play end\n");
		int ret = txShow_nextSentence(&thiz->text_show);
		if(ret >= 0){
			txMain_showScreen();
			txMain_playAudio(thiz);
			DBGMSG("ret\n");
			return;
		}

		int byte = txShow_getSenEndByte(&thiz->text_show);
		ret = text_nextText(thiz->text_parse, byte);
		if(ret < 0){
			DBGMSG("book end\n");
			txMain_endStop(thiz);
			CoolShmReadLock(g_config_lock);
			int repeat = g_config->setting.book.repeat;
			CoolShmReadUnlock(g_config_lock);
			DBGMSG("repeat:%d\n",repeat);
			DBGMSG("thiz->setting.book.repeat:%d\n",thiz->setting.book.repeat);
			if(repeat){
				txMain_playPrompt(BOOK_END,0,txMain_jumpHead);
			}else{
				txMain_playPrompt(BOOK_END,0,0);
			}
		}
		else{
			DBGMSG("start\n");
			txMain_getTextInfo(thiz);
			txMain_showScreen();
			txMain_playAudio(thiz);
		}
		DBGMSG("ret1\n");
	}
}

static void txMain_audioTimerOut(void)
{
	struct TextBook*thiz = gbook;
	//info_debug("timer out\n");
	return_if_fail(thiz != NULL);
	if(thiz->select_book){
		return;
	}

	txMain_promptCheck(thiz);
	txMain_audioCheck(thiz);
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	struct TextBook* thiz = gbook;
	
	int msgId = msg->msg.msg.msgId;
	DBGLOG("app msg %d .\n", msgId);
	if(PLEXTALK_APP_MSG_ID != msgId){
		return;
	}
	app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;
	switch (app->type)
	{
	/*设置相应的state，可能是暂停或者恢复*/
	case APPRQST_SET_STATE:
		{
			app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
			info_debug("set state:%d\n", rqst->pause);

			/* Special handler for info */
			if (rqst->spec_info) {

				printf("Specical msg for info module!\n");
				
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
				txMain_msgPause();
			}else{
				txMain_msgResume();
			}
		}
		break;

	/*menu 有bookmark相关的设置*/
	case APPRQST_BOOKMARK:
		{
			app_bookmark_rqst_t *rqst = (app_bookmark_rqst_t*)msg->msg.msg.msgTxt;
			if(APP_BOOKMARK_SET == rqst->op)
			{
				txMain_setMark(thiz);
			}
			else if(APP_BOOKMARK_DELETE == rqst->op)
			{
				txMain_delCurMark(thiz);
			}
			else if(APP_BOOKMARK_DELETE_ALL== rqst->op)
			{
				txMain_delAllMark(thiz);
			}
		}
		break;
		
	case APPRQST_DELETE_CONTENT:
		{
			txMain_delCurTitle();
		}
		break;
		
	case APPRQST_SET_SPEED:
		{
			return_if_fail(thiz != NULL);
			app_speed_rqst_t *rqst = (app_speed_rqst_t*)app;
			thiz->setting.book.speed = rqst->speed;
			txMain_setBookSpeed();
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
			txMain_setBookEqualizer();
		}
		break;
		
	case APPRQST_SET_FONTSIZE:
		{	//plextalk_lcd
			//widget_prop_t wprop;
			return_if_fail(thiz != NULL);
			//change font size
			info_debug("change font size\n");

			NeuxWidgetSetFont(thiz->text_show.form, sys_font_name, sys_font_size);
			thiz->text_show.show_gc = NeuxWidgetGC(thiz->text_show.form);
			txShow_Init(&thiz->text_show, sys_font_size);
			//redispplay
			txMain_reCalShowScr(thiz);
			//replay text tts
			txMain_replayScrText(thiz);
		}
		break;
	case APPRQST_SET_THEME:
		{	//plextalk_lcd
			return_if_fail(thiz != NULL);
			
			widget_prop_t wprop;
			info_debug("change theme\n");
			
			app_theme_rqst_t *rqst = (app_theme_rqst_t *)app;
			thiz->setting.lcd.theme = rqst->index;
			
			NeuxWidgetGetProp(thiz->text_show.form, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(thiz->text_show.form, &wprop);
			
			GrSetGCForeground(thiz->text_show.show_gc, theme_cache.foreground);
			GrSetGCBackground(thiz->text_show.show_gc, theme_cache.background);
			txMain_showScreen();
		}
		break;
	case APPRQST_SET_LANGUAGE:
		{
			info_debug("set language\n");
			/* Update book tts mode */
			txMain_showScreen();
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
			txMain_setBookSpeed();
			txMain_setBookEqualizer();
			//font size
			NeuxWidgetSetFont(thiz->text_show.form, sys_font_name, sys_font_size);
			thiz->text_show.show_gc = NeuxWidgetGC(thiz->text_show.form);
			txShow_Init(&thiz->text_show, sys_font_size);
			//theme
			NeuxWidgetGetProp(thiz->text_show.form, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(thiz->text_show.form, &wprop);
			GrSetGCForeground(thiz->text_show.show_gc, theme_cache.foreground);
			GrSetGCBackground(thiz->text_show.show_gc, theme_cache.background);
			
			/* Update book tts mode */

			//redisplay and replay audio
			txMain_reCalShowScr(thiz);
			//replay text tts
			txMain_replayScrText(thiz);
		}
		break;
	}
	
}

static void OnWindowKeydown(WID__ wid,KEY__ key)
{
    DBGLOG("OnWindowKeydown\n");
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		info_debug("help running\n");
		return;
	}

	txMain_keyDownHandle(key);
}

static void OnWindowKeyup(WID__ wid,KEY__ key)
{
    DBGLOG("OnWindowKeyup\n");
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		info_debug("help running\n");
		return;
	}
	txMain_keyUpHandle(key);
}

static void OnWindowRedraw(int clrBg)
{
	struct TextBook*thiz = gbook;
	GR_WINDOW_ID wid;
	GR_GC_ID gc;

	return_if_fail(thiz != NULL);
	if(thiz->select_book){
		return;
	}
	
	wid = thiz->text_show.wid;
	gc = thiz->text_show.show_gc;

	if(wid != GrGetFocus()){
		DBGMSG("NOT FOCUS\n");
		//return;
	}
	
	if(clrBg){
		NxSetGCForeground(gc, theme_cache.background);
		GrFillRect(wid, gc, 0, 0, thiz->text_show.w, thiz->text_show.h);
		NxSetGCForeground(gc, theme_cache.foreground);
	}
	
	if(NULL == thiz->text_parse){	//NOT OPEN BOOK
		char *pstr = _("BOOK");
		int w,h,b;
		int x,y;
		
		NeuxWidgetSetFont(thiz->text_show.form, sys_font_name, 24);
		gc = NeuxWidgetGC(thiz->text_show.form);
		NxSetGCForeground(gc, theme_cache.foreground);
		NxSetGCBackground(gc, theme_cache.background);
		
		NxGetGCTextSize(gc, pstr, -1, MWTF_UTF8, &w, &h, &b);
		x = (thiz->text_show.w - w)/2;
		y = (thiz->text_show.h - 24)/2;
		GrText(wid, gc, x, y, pstr, -1, MWTF_UTF8 | MWTF_TOP);

		NeuxWidgetSetFont(thiz->text_show.form, sys_font_name, sys_font_size);
		thiz->text_show.show_gc = NeuxWidgetGC(thiz->text_show.form);
	}else{
		txShow_showScreen(&thiz->text_show);
	}
}

static void OnWindowExposure(WID__ wid)
{
	DBGLOG("------exposure-------\n");
	OnWindowRedraw(0);
}

static void OnWindowGetFocus(WID__ wid)
{
	struct TextBook* thiz = gbook;
	return_if_fail(thiz != NULL);
	DBGLOG("-------windown focus %d.\n",wid);
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BOOK_TEXT);
	txMain_showMediaIcon(thiz->textfile);
	txMain_showStateIcon(thiz->playmode);
	txMain_showTitleBar(thiz);
}

static void OnWindowDestroy(WID__ wid)
{
    DBGLOG("-------window destroy %d.\n",wid);
	book_del_backup();
	plextalk_suspend_unlock();
	txMain_destroy(gbook);
	gbook = NULL;
}


static void OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	DBGMSG("Hotplug device=%d index=%d action=%d\n",device,index,action);
	struct TextBook* thiz = gbook;
	char *path;
	return_if_fail(thiz != NULL);
	
	if(thiz->select_book || thiz->info_module || thiz->text_pause){
		DBGLOG("return\n");
		return ;
	}

	if(TEXT_OPEN_ERR == thiz->open_state)
	{
		return;
	}
	
	if(0 == strlen(thiz->textfile))
	{
		return;
	}
	path = thiz->textfile;
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		break;

	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			if(StringStartWith(path, MEDIA_SDCRD))
			{
				info_debug("remove sdcard\n");
				txMain_stopText(thiz);
				thiz->exit_type = EXIT_MEDIA_REMOVE;
				NeuxClearModalActive();
			}
			break;
		case MWHOTPLUG_DEV_SCSI_DISK:
			if(StringStartWith(path, MEDIA_USB))
			{
				info_debug("remove usb\n");
				txMain_stopText(thiz);
				thiz->exit_type = EXIT_MEDIA_REMOVE;
				NeuxClearModalActive();
			}
			break;
		case MWHOTPLUG_DEV_SCSI_CDROM:
cdrom_remove:
			if(StringStartWith(path, MEDIA_CDROM))
			{
				info_debug("remove cdrom\n");
				txMain_stopText(thiz);
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
			int ret = CoolCdromMediaPresent();
			if (ret <= 0)
				goto cdrom_remove;
		}
		break;
	}
}

static void txMain_grabKey (struct TextBook*thiz)
{
}

static void txMain_ungrabKey (struct TextBook*thiz)
{
}


/* Function creates the main form of the application. */
static void txMain_CreateMainForm (struct TextBook*thiz)
{
	FORM  *formMain;
//	TIMER *timer;
    widget_prop_t wprop;

	txShow_Init(&thiz->text_show, sys_font_size);
	
	/*创建控件*/
    formMain = NeuxFormNew(thiz->text_show.x, thiz->text_show.y, thiz->text_show.w, thiz->text_show.h);
	thiz->text_show.form = formMain;
	thiz->text_show.wid = NeuxWidgetWID(formMain);
	
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
	thiz->text_show.show_gc = NeuxWidgetGC(formMain);
	//info_debug("thiz->gc:%d\n",thiz->text_show.gc);
	
	//create timer for check audio
	thiz->audiotimer = NeuxTimerNew(formMain, 300 ,-1);
	NeuxTimerSetCallback(thiz->audiotimer, txMain_audioTimerOut);
	
	//create timer for hold key 
	thiz->keytimer = NeuxTimerNew(formMain, 1000, -1);
	NeuxTimerSetCallback(thiz->keytimer, txMain_keyTimerOut);
	TimerDisable(thiz->keytimer);
	thiz->timerEnable = 0;

	/* 显示主窗口 */
	NeuxFormShow(formMain, TRUE);
	NeuxWidgetFocus(formMain);
	NeuxWidgetRedraw(formMain);
	txMain_grabKey(thiz);
	return ;
}


int text_main(char *file, int first, MsgFunc* func_ptr)
{
	int endOfModal;
	struct TextBook *thiz;

	if(NULL==file || 0 == strlen(file) ){
		return -1;
	}

	thiz = txMain_create();
	if(NULL == thiz){
		info_debug("malloc err\n");
		//FATAL(fmt,arg...);
		return 1;
	}
	//copy file
	memset(thiz->textfile, 0, sizeof(thiz->textfile));
	strlcpy(thiz->textfile, file, sizeof(thiz->textfile));

	//copy system setting
	CoolShmReadLock(g_config_lock);	
	memcpy(&thiz->setting, &g_config->setting, sizeof(thiz->setting));
	CoolShmReadUnlock(g_config_lock);
	
	/* Update book tts mode */

	plextalk_suspend_lock();
	
	txMain_CreateMainForm(thiz);

//	book_initKeyLock();
	
	*func_ptr = onAppMsg;
	//NeuxAppSetCallback(CB_MSG, onAppMsg);
	
	if(first)
	{	//first in ,must play bookmode
		txMain_playPrompt(BOOK_MODE_OPEN, 0, txMain_openNewBook);
	}
	else
	{
		txMain_openNewBook(thiz);
	}
	
	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	info_debug("modal exit\n");
	if(EXIT_MEDIA_REMOVE == thiz->exit_type){
		strlcpy(file, PLEXTALK_MOUNT_ROOT_STR, PATH_MAX);
	}

	*func_ptr = NULL;
	NeuxFormDestroy(&thiz->text_show.form);
	info_debug("text main exit\n");
	return 1;
}


