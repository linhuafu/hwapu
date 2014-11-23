#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <gst/gst.h>
#include <signal.h>

#include "jump_page_tts.h"

#include "libvprompt.h"

#include "plextalk-config.h"
#include "Amixer.h"
#include "plextalk-statusbar.h"
#include "plextalk-keys.h"
#include "desktop-ipc.h"
#include "plextalk-theme.h"
#include "application-ipc.h"
#include "plextalk-ui.h"
#include "nx-widgets.h"
#include "widgets.h"
//#include "events.h"
#include "application.h"
//#include "wm.h"
#include "common/tchar.h"
#include "daisy/PlexDaisyAnalyzeAPI.h"
#include "Key-value-pair.h"

#define OSD_DBG_MSG
#include "nc-err.h"

static FORM *  formMain;
#define widget formMain

#define _(S)	gettext(S)

#define PAGE_WINDOW_WIDTH	160
#define PAGE_WINDOW_HEIGHT   96

#define TIMER_PERIOD	700//500				//for blink
#define TIMER_LONG_PERIOD 1000

static struct page_nano g_nano;
static int g_tts_isrunning;
struct plextalk_language_all all_langs;

/* For Key long_press judgement */
struct long_press {
	TIMER* lid;
	int l_flag;
	int pressed;
};

static void 
page_OnKeyTtimer();
static void 
page_OnLongKeyTtimer();

enum roll {

	ROLL_UP = 0,

	ROLL_DOWN,
};

#define MAX_PAGEBIT		6

struct  PAGE_STR{

	char   displaytext[MAX_PAGEBIT+4];
	char   oktext[MAX_PAGEBIT+4];
	char   tempchar[2];
	int  offset;						//表示处理后的数字
	int index;						//字符选择的序号
	
};

struct page_nano {

	TIMER *Timer;
	TIMER *lock_timer;
	int wid;

	int fgcolor;
	int bgcolor;

	int blink;
	
	/*0 means blink cursor, 1 means blink num*/
	int mode;	

	/*for menu longpress judgement*/
	struct long_press menu_key; 

	int* is_running;

	int first_logo;		/*If user just logo in calculator module*/

	int key_stat;

	/* ID for infomation */
	void* info_id;
	int ininfo;

	int wait_app_response;
};

#define KEY_DOWN_STAT	0x01
#define KEY_UP_STAT		0x00

void appResponseLock(void)
{
	if(!g_nano.wait_app_response){
		g_nano.wait_app_response = 1;
		NeuxTimerSetControl(g_nano.lock_timer, 10000, 1);
		//plextalk_schedule_lock();
		DBGMSG("response lock\n");
	}
}

void appResponseUnlock(void)
{
	if(g_nano.wait_app_response){
		g_nano.wait_app_response = 0;
		//plextalk_schedule_unlock();
		DBGMSG("response unlock\n");
	}
}

//#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))

static struct PAGE_STR  mpage_str;
static char input_umber[]={'0','1','2','3','4','5','6','7','8','9','E','C'};

static void page_str_update (int roll)
{
	//DBGMSG("page_stat=%d,page_str.choice=%d,page_str.page_str_p=%d\n",page_stat,page_str.choice,page_str.page_str_p);
	int maxnum = ARRAY_SIZE(input_umber);
	char ttsstr[2];
	
	if (roll == ROLL_UP)
		mpage_str.index ++;
	else
		mpage_str.index --;

	if (mpage_str.index >= maxnum)
		mpage_str.index = 0;
	else if (mpage_str.index < 0)
		mpage_str.index = maxnum - 1;
	
	mpage_str.tempchar[0]=input_umber[mpage_str.index];     		//选择的数
	mpage_str.tempchar[1]=0;

	page_tts(TTS_ELEMENT, mpage_str.tempchar);

}

static void page_grab_key ()
{

}

static void page_ungrab_key ()
{

}

void
redraw_page_str (int wid,struct page_nano* nano)
{
	int len,i,size;
	char copy_express[20];
	int gc = NeuxWidgetGC(widget);
	//boolean clrBg = 1;
	
	if(wid != GrGetFocus()){
		DBGMSG("NOT FOCUS\n");
		//return;
	}

	//if(1)


	/* copy the calculation express here */
	memset(copy_express, 0, sizeof(copy_express));
	strlcpy(copy_express, mpage_str.oktext, sizeof(copy_express));
	size = strlen(copy_express);
	
	if (nano->blink) 
	{//不显示
		nano->blink = 0;
	}
	else 
	{//显示
		if(mpage_str.tempchar[0]=='_' || mpage_str.tempchar[0]==0)
		{
			copy_express[size]	='_';	
		}
		else
		{
			copy_express[size]	=mpage_str.tempchar[0];
		}
		nano->blink = 1;
	}
	len = strlen(copy_express);
	//DBGMSG("display str:%s\n",copy_express);
	GrText (nano->wid, gc, 1, 0, copy_express, len, MWTF_UTF8|MWTF_TOP);
	
}

void
draw_page_str (struct page_nano* nano)
{
	int gc = NeuxWidgetGC(widget);
	//CLEAR WINDOW
	NxSetGCForeground(gc, theme_cache.background);
	GrFillRect(nano->wid, gc, 0, 0, PAGE_WINDOW_WIDTH, PAGE_WINDOW_HEIGHT);
	NxSetGCForeground(gc, theme_cache.foreground);
	
	redraw_page_str(nano->wid,nano);
}

/*unhandler for sequential press right key*/
void
page_key_right_handler (struct page_nano* nano)
{
	int len=strlen(mpage_str.oktext);

	DBGMSG("page_key_right_handler  oktext=%s\n",mpage_str.oktext);

	if(mpage_str.tempchar[0]=='C')
	{
		page_tts(TTS_CLEARALL	,NULL); 		//清除所有的数据
		memset(&mpage_str,0x00,sizeof(mpage_str));
		mpage_str.tempchar[0]='_';
		mpage_str.index = -1;
	}
	else if(mpage_str.tempchar[0]=='E')
	{//跳到相应的页
		if(len<=0)
		{
			 page_tts(TTS_ERRBEEP,NULL);
			 return;
		}
		unsigned long page = strtoul(mpage_str.oktext, NULL, 0);
		DBGMSG("page=%lu\n",page);
		NhelperJumpPage(page);
		appResponseLock();
		return;
	}
	else if( '0'<= mpage_str.tempchar[0] && mpage_str.tempchar[0]<='9')
	{	
		if(len<6){//向后确认一个数，但最大的只有6位
			mpage_str.oktext[len]=mpage_str.tempchar[0];
			page_tts(TTS_INPUTSET,mpage_str.tempchar);	
			mpage_str.tempchar[0]='_';				//flash data;
			mpage_str.index = -1;
		}else{
			page_tts(TTS_MAXINPUTVALUE,NULL);
			return;
		}
	}
	else
	{
		page_tts(TTS_ERRBEEP,NULL);
		return;
	}

	nano->mode = 0;
	draw_page_str(nano);
}

void 
page_key_left_handler (struct page_nano* nano)
{
	int len=strlen(mpage_str.oktext);
	DBGMSG("page_key_left_handler  oktext=%s!\n",mpage_str.oktext);
	
	if(len<=0)
	{//没有数字的输入，只显示光标
		 page_tts(TTS_TOPFIGURE,NULL);
		 mpage_str.tempchar[0]='_';
		 mpage_str.index = -1;
		 return;
	}
	else
	{//向前删除一个数
		if('_'== mpage_str.tempchar[0] || 0 == mpage_str.tempchar[0] ){
			mpage_str.oktext[len-1]=0;
			len=strlen(mpage_str.oktext);
		}
		 
		if(len==0)
		{// 删除一个数字后，就没有数字
			page_tts(TTS_BACKCANCEL,NULL);	
		}
		else
		{
			page_tts(TTS_BACKCANCEL,mpage_str.oktext);				//向前输入一个数字				
		}
		mpage_str.tempchar[0]='_';
		mpage_str.index = -1;
	}

	nano->mode = 0;
	draw_page_str(nano);
}


void 
page_key_up_handler (struct page_nano* nano)
{
	DBGLOG("---------------Up Key \n");
	page_str_update(ROLL_UP);	
	nano->mode = 1;
	draw_page_str(nano);
}


void
page_key_down_handler (struct page_nano* nano)
{
	page_str_update(ROLL_DOWN);	
	nano->mode = 1;
	draw_page_str(nano);
}

static int getInputNumberStr(char *buf, int buflen)
{
	char *pstr=NULL;
	int len=strlen(mpage_str.oktext);
	
	memset(buf, 0, buflen);
	if(mpage_str.tempchar[0]=='E' || mpage_str.tempchar[0]=='C')
	{
		if(mpage_str.tempchar[0]=='E')
			pstr = _("Enter");
		else
			pstr = _("clear");
		memset(buf,0x00,buflen);
		snprintf(buf,buflen,"%s: %s",mpage_str.oktext,pstr);
		//page_tts(TTS_NUMBER,buf);
	}
	else if('0'<=mpage_str.tempchar[0] && mpage_str.tempchar[0]<='9')
	{
		strlcpy(buf,mpage_str.oktext, buflen);
		strlcat(buf,mpage_str.tempchar, buflen);
		//page_tts(TTS_NUMBER,tempstr);
	}else if(len<=0){
		return -1;
	}else{
		strlcpy(buf,mpage_str.oktext, buflen);
	}
	return 0;
}

void
page_key_play_stop_handler (struct page_nano* nano)
{
	DBGMSG("enter page_key_enter_handler!\n");
	char buf[128]={0};
	if(getInputNumberStr(buf, sizeof(buf)) < 0){
		page_tts(TTS_NOINPUT,NULL);
	}else{
		page_tts(TTS_NUMBER,buf);
	}
}

#define		BOOK_BACKUP		 	"/tmp/.book"
//参考 dsInfo_PagesInfo 函数
int getDaisyPagesInfo(char *pagetstr, int maxlen)
{
	unsigned long pulCurPage,pulMaxPage;
	unsigned long pulFrPage,pulSpPage;
	char *pageinfo = _("Page information");
	char *curpage = _("Current page");
	char numstr[10];
	int bmkret;

	strlcpy(pagetstr, pageinfo, maxlen);
	strlcat(pagetstr, "\n", maxlen);

	bmkret = CoolGetUnlongValue(BOOK_BACKUP, "page", "curpage", &pulCurPage);
	if (bmkret != COOL_INI_OK) {
		DBGMSG("get page error\n");
		strlcat(pagetstr, _("There is no page."), maxlen);
		return -1;
	}

	bmkret = CoolGetUnlongValue(BOOK_BACKUP, "page", "maxpage", &pulMaxPage);
	bmkret = CoolGetUnlongValue(BOOK_BACKUP, "page", "frpage", &pulFrPage);
	bmkret = CoolGetUnlongValue(BOOK_BACKUP, "page", "sppage", &pulSpPage);
	
	DBGMSG("pulCurPage=%lu\n",pulCurPage);
	DBGMSG("pulMaxPage=%lu\n",pulMaxPage);
	DBGMSG("pulFrPage=%lu\n",pulFrPage);
	DBGMSG("pulSpPage=%lu\n",pulSpPage);

	if(PLEX_DAISY_PAGE_NONE == pulCurPage){
		DBGMSG("none page\n");
		strlcat(pagetstr, _("Beginning of title."), maxlen);
	}else if(PLEX_DAISY_PAGE_FRONT == pulCurPage){
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, _("Front Page"), maxlen);
	}else if(PLEX_DAISY_PAGE_SPECIAL == pulCurPage){
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, _("Special Page"), maxlen);
	}else{
		snprintf(numstr, sizeof(numstr), "%s %lu", _("page"), pulCurPage);
		strlcat(pagetstr, curpage, maxlen);
		strlcat(pagetstr, ":", maxlen);
		strlcat(pagetstr, numstr, maxlen);
	}

	strlcat(pagetstr, "\n", maxlen);
	snprintf(numstr, sizeof(numstr), "%lu", pulMaxPage);
	strlcat(pagetstr, _("Maximum number of pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	strlcat(pagetstr, "\n", maxlen);
	
	snprintf(numstr, sizeof(numstr), "%lu", pulFrPage);
	strlcat(pagetstr, _("Total number of front pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	strlcat(pagetstr, "\n", maxlen);
	
	snprintf(numstr, sizeof(numstr), "%lu", pulSpPage);
	strlcat(pagetstr, _("Total number of special pages"), maxlen);
	strlcat(pagetstr, ":", maxlen);
	strlcat(pagetstr, numstr, maxlen);
	DBGMSG("page:%s\n",pagetstr);
	return 0;
}


#define MAX_INFO_ITEM	3

void
page_key_info_handler (struct page_nano* nano)
{
	DBGMSG("page_key_info_handler\n");

	if(1 == nano->ininfo){
		return;
	}
	//scroll_text_set_text(nano->title_text, _("Information"));
	NhelperTitleBarSetContent(_("Information"));

	/* Fill info item */
	int info_item_num = 0;
	char* info[MAX_INFO_ITEM];

	/* input value item */
	char input_info[128];
	/* no input */
	char tmpbuf[128];
	if (getInputNumberStr(tmpbuf, sizeof(tmpbuf)) < 0){
		snprintf(input_info, sizeof(input_info), "%s", _("No input"));
	}else{
		snprintf(input_info, sizeof(input_info), "%s%s", _("current input number:"),tmpbuf);
	}
	info[info_item_num] = input_info;
	info_item_num ++;

	/*daisy page info*/
	char page_info[256];
	getDaisyPagesInfo(page_info, sizeof(page_info));
	info[info_item_num] = page_info;
	info_item_num ++;

	nano->ininfo = 1;
	
	/* Start info TTS */
	nano->info_id = info_create();
	info_start (nano->info_id, info, info_item_num, nano->is_running);
	info_destroy (nano->info_id);

	nano->ininfo = 0;

	NhelperTitleBarSetContent(_("Book menu"));
}

static void
menu_key_shortpress_handler(struct page_nano* nano)
{
	//GrDestroyTimer(nano->menu_key.lid);
#if 0
	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();
	
	TimerDelete(nano->menu_key.lid);
	nano->menu_key.l_flag = 0;
	page_ungrab_key(nano);
	//scroll_text_destroy(nano->title_text);
	//GrDestroyWindow(nano->page_wid);
	page_tts_destroy();
	//efont_destroy(nano->efont);

	/* Flush the pageculator stat and express */
	page_str_init();
#endif

	page_tts(TTS_EXIT_JUMP_PAGE,NULL);

	//NeuxAppStop();
}


static void 
menu_key_longpress_handler(struct page_nano* nano)
{
	DBGMSG("Handler for long press.\n");
	//GrDestroyTimer(nano->menu_key.lid);
	TimerDelete(nano->menu_key.lid);
	nano->menu_key.l_flag = 0;

	/* Ungrab key before in libhelp */
	page_ungrab_key(nano);

	/* Destroy timer before in libhelp */
	//GrDestroyTimer(nano->tid);
	TimerDelete(nano->Timer);

	//help(nano->is_running);

	/* Recover timer for pageculator mode */
	nano->Timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->Timer, page_OnKeyTtimer);
	
	/* Grab key again when leave libhelp */
	page_grab_key(nano);
}


static void 
page_OnKeyTtimer()
{
	draw_page_str(&g_nano);
}
static void 
page_OnLongKeyTtimer()
{
	menu_key_longpress_handler(&g_nano);
}

static void
page_OnLockTimer(WID__ wid)
{
	DBGMSG("lock timerout\n");
	if(g_nano.wait_app_response){
		appResponseUnlock();
	}
}

static void
page_CreateTimer(FORM *widget,struct page_nano* nano)
{
	nano->Timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->Timer, page_OnKeyTtimer);
	DBGLOG("Rec_CreateTimer nano->rec_timer:%d",nano->Timer);

	nano->lock_timer = NeuxTimerNew(widget, 5000, 0);
	NeuxTimerSetCallback(nano->lock_timer, page_OnLockTimer);
}

static void
page_CreateLongTimer(FORM *widget,struct page_nano* nano)
{
	nano->menu_key.lid= NeuxTimerNew(widget, TIMER_LONG_PERIOD, -1);
	NeuxTimerSetCallback(nano->menu_key.lid, page_OnLongKeyTtimer);
	DBGLOG("page_CreateLongTimer nano->rec_timer:%d",nano->menu_key.lid);
}

static void
page_OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);
	 //help在运行时，不响应任何按键(因为help按键grap key时用的NX_GRAB_HOTKEY)
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP) || g_nano.ininfo)
	{
		DBGLOG("help is running or is in info !!\n");
		return;
	}

	if(page_get_exit_flag())
	{
		DBGLOG("I am in exit status,OK,if you are in a hurry,I will close help now\n");
		NxAppStop();
		return;
	}
	 
	if (NeuxAppGetKeyLock(0)) {
		page_tts(TTS_KEYLOCK,NULL);
		return;
	}

	if(g_nano.wait_app_response){
		page_tts(TTS_PLEASE_WAIT,NULL);
		return;
	}

	/*一旦有按键按下，就认为计算器已经开始输入*/
	if (g_nano.first_logo)
	{
		g_nano.first_logo = 0;
	}

	switch (key.ch) {
			
		case MWKEY_RIGHT:
			page_key_right_handler(&g_nano);
			break;
				
		case MWKEY_LEFT:
			page_key_left_handler(&g_nano);
			break;
	
		case MWKEY_UP:						
			page_key_up_handler(&g_nano);
			break;
	
		case MWKEY_ENTER:
			page_key_play_stop_handler(&g_nano);
			break;
	
		case MWKEY_DOWN:
			page_key_down_handler(&g_nano);
			break;
	
		case MWKEY_MENU: 	
			page_CreateLongTimer(widget,&g_nano);
			//nano->menu_key.lid = GrCreateTimer(g_nano.page_wid, );		//1s
			g_nano.menu_key.l_flag = 1;
			break;
	
		case MWKEY_INFO:
			page_key_info_handler(&g_nano);
			break;
			
		case VKEY_MUSIC:
		case VKEY_BOOK:
		case VKEY_RADIO:
		case VKEY_RECORD:
			page_tts(TTS_ERRBEEP, NULL);
			break;
	}


}

static void
page_OnWindowKeyup(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeyup %d.\n",key.ch);
	/*如果是短按键处理，退出计算器*/
	if (g_nano.menu_key.l_flag) {
		menu_key_shortpress_handler(&g_nano);
	}
}

static void
page_OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);

	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();
	
	TimerDelete(g_nano.menu_key.lid);
	g_nano.menu_key.l_flag = 0;
	page_ungrab_key();
	//scroll_text_destroy(nano->title_text);
	//GrDestroyWindow(nano->page_wid);
	page_tts_destroy();
	//efont_destroy(nano->efont);
	
	/* Flush the pageculator stat and express */
	//page_str_init();
	
	plextalk_global_config_close();

#if 0
	TimerDelete(g_nano.Timer);
	NeuxFormDestroy(widget);
	widget = NULL;
#endif	
}

static void
page_OnWindowExposure(WID__ wid)
{
	DBGLOG("---------------page_OnWindowExposure %d.\n",wid);
	//if (!g_nano.first_logo)
	redraw_page_str(wid,&g_nano);
}

static void page_OnGetFocus(WID__ wid)
{
	DBGLOG("On Window Get Focus\n");
	NhelperTitleBarSetContent(_("Book menu"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
}

static int jump_page_result(app_jump_page_result* rqst)
{
	DBGMSG("result:%d\n",rqst->result);
	
	appResponseUnlock();
	switch(rqst->result){
		case APP_JUMP_OK:
			page_tts(TTS_EXIT_JUMP_PAGE,NULL);
			break;
			
		case APP_JUMP_ERR:
			{
				page_tts(TTS_FAILED_MOVE_PAGE,NULL);
				memset(&mpage_str,0x00,sizeof(mpage_str));
				mpage_str.tempchar[0]='_';
			}
			break;
				
		case APP_JUMP_NOT_EXIST_PAGE:
			page_tts(TTS_THERE_NO_PAGE,NULL);
			break;
			
		default:
			break;
	}
	return 1;
}

static void page_onAppMsg(const char * src, neux_msg_t * msg)
{
	DBGLOG("app msg %d .\n", msg->msg.msg.msgId);
	app_rqst_t *rqst;

	if (msg->msg.msg.msgId != PLEXTALK_APP_MSG_ID)
		return;

	rqst = msg->msg.msg.msgTxt;
	DBGLOG("onAppMsg PLEXTALK_APP_MSG_ID : %d\n",rqst->type);
	switch (rqst->type)
	{
	case APPRQST_LOW_POWER://低电要关机了
		DBGMSG("Yes,I have received the lowpower message\n");
		NxAppStop();
		break;
	
	case APPRQST_SET_STATE:
		{
			app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
			if(g_nano.ininfo) {
				if(rqst->pause)
					info_pause();
				else
					info_resume();
			}
		}
		break;
	case APPRQST_JUMP_PAGE_RESULT:
		{
			app_jump_page_result *rqst = (app_jump_page_result*)msg->msg.msg.msgTxt;
			jump_page_result(rqst);
		}
		break;
	}
}

/* Function creates the main form of the application. */
void
page_CreateFormMain (struct page_nano* nano)
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, page_OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 page_OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  page_OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_EXPOSURE,  page_OnWindowExposure);
	NeuxFormSetCallback(widget, CB_FOCUS_IN, page_OnGetFocus);

	NeuxWidgetGetProp(widget, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(widget, &wprop);

	NeuxWidgetSetFont(widget, sys_font_name, 16);

	//LABEL用于显示字符串
	//page_CreateLabels(widget,nano);
	//创建定时器用于计时和显示
	page_CreateTimer(widget,nano);
	
	page_grab_key();

	//NhelperTitleBarSetContent(_("Book menu"));

	NeuxFormShow(widget, TRUE);

	nano->wid = NeuxWidgetWID(widget);

	NeuxWidgetFocus(widget);

	//int fontid = NxGetFontID("msyh.ttf", 16);
	NeuxWidgetRedraw(widget);

}

static void
page_initApp(struct page_nano* nano)
{
	int res;

	/* global config */
	//plextalk_global_config_init();
	plextalk_global_config_open();

	/* theme */
	//CoolShmWriteLock(g_config_lock);
	NeuxThemeInit(g_config->setting.lcd.theme);
	//CoolShmWriteUnlock(g_config_lock);

	//CoolShmReadLock(g_config_lock);
	plextalk_update_lang(g_config->setting.lang.lang, "jump-page");
	//CoolShmReadUnlock(g_config_lock);

	plextalk_update_sys_font(getenv("LANG"));

	g_tts_isrunning = 0;
	nano->is_running = &g_tts_isrunning;
	page_tts_init(nano->is_running);
	page_tts(TTS_ENTER_JUMP_PAGE, NULL);

	/*init menu longpress struct*/
	nano->menu_key.l_flag = 0;
	nano->menu_key.pressed = 0;

	/* Init key state as key up */
	nano->key_stat = KEY_UP_STAT;

	nano->fgcolor = theme_cache.foreground;
	nano->bgcolor = theme_cache.background;

	nano->first_logo = 1;
	nano->ininfo = 0;
}


int
main(int argc,char **argv)
{
	DBGMSG("In jump page mode!\n");

	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}
	page_initApp(&g_nano);
	DBGMSG("application initialized\n");

	//page界面禁止应用切换
	plextalk_schedule_lock();
	//page界面只关屏，不休眠
	plextalk_suspend_lock();
	plextalk_sleep_timer_lock();
		
	//NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_CALC);
	page_CreateFormMain(&g_nano);

	NeuxAppSetCallback(CB_MSG,	 page_onAppMsg);
	//NeuxWMAppSetCallback(CB_HOTPLUG, onHotplug);

	//this function must be run after create main form ,it need set schedule time
	DBGMSG("mainform shown\n");

	// start application, events starts to feed in.
	NeuxAppStart();

	return 0;
}

