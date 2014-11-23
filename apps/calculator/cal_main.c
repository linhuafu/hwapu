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
#include "cal_stat.h"
#define  DEBUG_PRINT 1

#include "cal_tts.h"

#include "cal_box.h"
#include "cal_stat.h"

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


#define DEBUG_PRINT 1
#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif

static FORM *  formMain;
#define widget formMain


#define _(S)	gettext(S)

#define CAL_WINDOW_WIDTH	160
#define CAL_WINDOW_HEIGHT   96

#define LINE_DRAW_SIZE	13//19				//this is only 15 element to show in a line

#define PTITLE_X	0
#define PTITLE_Y	35
#define PTITLE_H	20	

#define TIMER_PERIOD	700//500				//for blink
#define TIMER_LONG_PERIOD 1000

static struct cal_nano g_nano;
static int g_tts_isrunning;
struct plextalk_language_all all_langs;
static int key_locked;

/* For Key long_press judgement */
struct long_press {

	TIMER* lid;
	int l_flag;
	int pressed;
};


static void 
cal_OnKeyTtimer();
static void 
cal_OnLongKeyTtimer();



struct cal_nano {

	LABEL *pTitle;
	TIMER *Timer;
	int wid;
	int pix_wid;
	int pix_gc;
	int font_id;

	int fgcolor;
	int bgcolor;

	int keylocked;

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

	unsigned char pText[CAL_STR_MAX_SIZE];
	int lineStart[CAL_STR_MAX_LINES];
	int lineLen[CAL_STR_MAX_LINES];
	int totalline;
};

#define KEY_DOWN_STAT	0x01
#define KEY_UP_STAT		0x00


int 
fix_ret_bits (char* str, int size)
{
//	info_debug("before fix ret = %s\n", str);
	
	char* p = strrchr(str, '.');

	/*actually, this won't happend*/
	if (!p)
		return 0;		

	/*if (length > 12)*/
	/*This condtion always be ture, because i invoke sprintf("%.12f") out side*/

//由check_ret_bits内的最大最小数控制位数
	memset (str + 13, 0, size - 13);

	int end = 12;
	
	while (end > 0) {
		
		if (str[end] == '0') {
			str[end] = 0;
			end--;
		} else 
			break;
	}

	int len = strlen(str);
	/*this fixed for 5.00 etc, wipe of '.'*/
	if(str[len - 1] == '.')
		str[len - 1] = 0;
	
	return 0;
}

#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))


static int cal_grab_table[] = { 

		MWKEY_MENU,
		VKEY_BOOK,
		VKEY_MUSIC,
		VKEY_RECORD,
		VKEY_RADIO,
};

static void cal_grab_key ()
{
#if 0
	int i;
	
	for(i = 0; i < ARRAY_SIZE(cal_grab_table); i++)
		NeuxWidgetGrabKey(widget, cal_grab_table[i], NX_GRAB_HOTKEY_EXCLUSIVE);
#endif	
}


static void cal_ungrab_key ()
{
#if 0
	int i;

	for(i = 0; i < ARRAY_SIZE(cal_grab_table); i++)
		NeuxWidgetUngrabKey(widget, cal_grab_table[i]);
#endif	
}



static void
cal_draw_error (struct cal_nano* nano,int wid,int gc)
{
#if 1
	/*wheater draw a "E" in result or need to load picture?*/
	NxSetGCForeground(gc,nano->fgcolor);

	GrText(wid, gc, 
				1, 85-15, "E", -1, MWTF_UTF8|MWTF_TOP);
#endif
	
}

/*进行字节拷贝，转成unicode，将 / 换成数学除号,将*号换成数学乘号
转换只适用于ascii,返回转化后的字节长
数学除号只能这样输出:

GrText(nano->cal_pix_wid, nano->gc, 5, 30, "\x5f\x00", len, GR_TFUC16LE);或
GrText(nano->cal_pix_wid, nano->gc, 5, 30, "\x00\x5f", len, GR_TFUC16BE);


*/
static int
my_copystr(unsigned char *dstbuf,char *srcbuf,int maxsize)
{
	int i;
	int j = 0;
	
	if((dstbuf == NULL) || (srcbuf == NULL))
	{
		return 0;
	}
	
	
	for(i=0;i<LINE_DRAW_SIZE&&(j<maxsize-1);i++)
	{
		if(srcbuf[i] == '\0')
		{
			break;
		}
		if( srcbuf[i] == '/')
		{
			dstbuf[j++] = 0xf7;//计算器可输入的数字具有特殊性，不会超过0x80
		}
		else if(srcbuf[i] == '*')
		{
			dstbuf[j++] = 0xd7;//乘
		}
		else if(srcbuf[i] == '@')//negative
		{
			dstbuf[j++] = 0x2d;
		}
		else
		{
			dstbuf[j++] = srcbuf[i];
		}
		
		dstbuf[j++] = '\0';
	}

	return j;
}

void
cal_window_redraw(struct cal_nano* nano)
{
	//GrClearWindow(nano->wid, GR_TRUE);
	int text_gc = NeuxWidgetGC(widget);
	NxSetGCForeground(text_gc,nano->bgcolor);
	GrClearWindow(nano->wid, GR_FALSE); 
	
	NxSetGCForeground(text_gc,nano->fgcolor);
	
	//GrClearWindow(nano->wid, GR_FALSE);
	redraw_cal_str(nano->wid,nano);
	
}



void
draw_cal_str (struct cal_nano* nano)
{
	cal_window_redraw(nano);
}


void
redraw_cal_str (int wid,struct cal_nano* nano)
{
#if 1
	int len,i;
	unsigned char buf[LINE_DRAW_SIZE*2+2];
	char copy_express[128];
	int text_gc = NeuxWidgetGC(widget);


	NxSetGCForeground(nano->pix_gc,nano->bgcolor);
	GrFillRect(nano->pix_wid, nano->pix_gc,
			0, 0, CAL_WINDOW_WIDTH, CAL_WINDOW_HEIGHT);
	
	NxSetGCForeground(nano->pix_gc,nano->fgcolor);

	/* copy the calculation express here */
	memset(copy_express, 0, 128);
	memcpy(copy_express, express->cal_str, 128);

	if (nano->mode) { //blink number!
		if (nano->blink) {		//显示
			nano->blink = 0;
		} else {				//不显示
			int size = strlen(copy_express);
			copy_express[size - 1] = 0;
			nano->blink = 1;
		}
	} else {	//blink cursor
		if (nano->blink) {
			nano->blink = 0;
		} else {
			int size = strlen(copy_express);
			copy_express[size] = '_';
			nano->blink = 1;
		}
	}
	
	int slen = strlen(copy_express);
	int line_no = slen / LINE_DRAW_SIZE;
	
	memset(buf, 0, LINE_DRAW_SIZE*2+2);
	len = my_copystr(buf,copy_express,LINE_DRAW_SIZE*2+2);

	GrText (nano->pix_wid, nano->pix_gc, 1, 15-15, buf, len, MWTF_UC16LE|MWTF_TOP);

	if (line_no > 0) {
		memset(buf, 0, LINE_DRAW_SIZE*2+2);
		len = my_copystr(buf,copy_express+LINE_DRAW_SIZE,LINE_DRAW_SIZE*2+2);
		GrText (nano->pix_wid, nano->pix_gc, 1, 30-15, buf, len, MWTF_UC16LE|MWTF_TOP);
	}

	if (line_no > 1) {
		memset(buf, 0, LINE_DRAW_SIZE*2+2);

		len = my_copystr(buf,copy_express + 2 * LINE_DRAW_SIZE,LINE_DRAW_SIZE*2+2);
		GrText (nano->pix_wid, nano->pix_gc, 1, 45-15, buf, len, MWTF_UC16LE|MWTF_TOP);
	}

	if (line_no > 2) {
		memset(buf, 0, LINE_DRAW_SIZE*2+2);
		len = my_copystr(buf,copy_express+ 3 * LINE_DRAW_SIZE,LINE_DRAW_SIZE*2+2);	
		GrText (nano->pix_wid, nano->pix_gc, 1, 60-15, buf, len, MWTF_UC16LE|MWTF_TOP);
	}
	
	if (line_no >= 3) {
		info_debug("I won't handler this compution!\n");
		//this should be an error beep and do clear?
		//内部状态机已经实现了不能大于3
	}

	if (express->show_res_flag) {

		if (express->error_flag) {
			info_debug("Error of calcultion occurs!\n");
			cal_draw_error(nano,nano->pix_wid,text_gc);
		} else {
			memset(express->res_buf, 0, 32);
			snprintf(express->res_buf, 32, "%.12f", express->resault);
			fix_ret_bits(express->res_buf, 32);
			if(!strcmp(express->res_buf,"-0"))
			{
				express->res_buf[0] = '0';
				express->res_buf[1] = 0;
			}

			GrText (nano->pix_wid, nano->pix_gc, 1, 85-15, express->res_buf, -1, MWTF_UTF8|MWTF_TOP);

			if (express->res_tts_flag) {			
				cal_tts(TTS_RESULT, express->res_buf);			//tts here,maybe not proper
				express->res_tts_flag = 0;		//不应该自己关掉
			}
		}
	}

	NxSetGCForeground(text_gc,nano->fgcolor);
	GrCopyArea(wid, text_gc, 
					0, 0, CAL_WINDOW_WIDTH, CAL_WINDOW_HEIGHT, nano->pix_wid, 0, 0, 0);
#else
	int len,i;
	//unsigned char buf[CAL_STR_MAX_SIZE];
	char copy_express[CAL_STR_MAX_SIZE];
	int x,y;
	int line_height = 16;
	int text_gc = NeuxWidgetGC(widget);

	NxSetGCForeground(nano->pix_gc,nano->bgcolor);
	GrFillRect(nano->pix_wid, nano->pix_gc,
			0, 0, CAL_WINDOW_WIDTH, CAL_WINDOW_HEIGHT);

	NxSetGCForeground(nano->pix_gc,nano->fgcolor);

	/* copy the calculation express here */
	memset(copy_express, 0, CAL_STR_MAX_SIZE);
	memcpy(copy_express, express->cal_str, CAL_STR_MAX_SIZE);

	if (nano->mode) { //blink number!
		if (nano->blink) {		//显示
			nano->blink = 0;
		} else {				//不显示
			int size = strlen(copy_express);
			copy_express[size - 1] = 0;
			nano->blink = 1;
		}
	} else {	//blink cursor
		if (nano->blink) {
			nano->blink = 0;
		} else {
			int size = strlen(copy_express);
			copy_express[size] = '_';
			nano->blink = 1;
		}
	}

	memset(nano->pText, 0, CAL_STR_MAX_SIZE);
	len = my_copystr(nano->pText,copy_express,CAL_STR_MAX_SIZE);

	nano->totalline = utf16GetTextExWordBreakLines(nano->pix_gc, CAL_WINDOW_WIDTH, NULL
		, nano->pText, len,CAL_STR_MAX_LINES, nano->lineStart,nano->lineLen, NULL);	
	
	y = 0;
	x = 0;
	for(i = 0; i < nano->totalline; i++){
		GrText(nano->pix_wid, nano->pix_gc, x, y, nano->pText+nano->lineStart[i], nano->lineLen[i], MWTF_UC16LE|MWTF_TOP);
		y += line_height ;
	}
	

	if (express->show_res_flag) {

		if (express->error_flag) {
			info_debug("Error of calcultion occurs!\n");
			cal_draw_error(nano,nano->pix_wid,text_gc);
		} else {
			memset(express->res_buf, 0, 32);
			snprintf(express->res_buf, 32, "%.12f", express->resault);
			fix_ret_bits(express->res_buf, 32);
			if(!strcmp(express->res_buf,"-0"))
			{
				express->res_buf[0] = '0';
				express->res_buf[1] = 0;
			}

			GrText (nano->pix_wid, nano->pix_gc, 1, 85-15, express->res_buf, -1, MWTF_UTF8|MWTF_TOP);

			if (express->res_tts_flag) {			
				cal_tts(TTS_RESULT, express->res_buf);			//tts here,maybe not proper
				express->res_tts_flag = 0;		//不应该自己关掉
			}
		}
	}

	NxSetGCForeground(text_gc,nano->fgcolor);
	GrCopyArea(wid, text_gc, 
					0, 0, CAL_WINDOW_WIDTH, CAL_WINDOW_HEIGHT, nano->pix_wid, 0, 0, 0);

#endif	
}




/*unhandler for sequential press right key*/
void
cal_key_right_handler (struct cal_nano* nano)
{
	cal_str_git();
	
	if (cal_result_state())
		//scroll_text_set_text(nano->title_text, _("Result"));
		NhelperTitleBarSetContent(_("Result"));
		
	else
		//scroll_text_set_text(nano->title_text, _("Input Number"));
		NhelperTitleBarSetContent(_("Input Number"));
	
	nano->mode = 0;
	draw_cal_str(nano);
}


void 
cal_key_left_handler (struct cal_nano* nano)
{
	cal_str_back();	

	if (cal_result_state())
		//scroll_text_set_text(nano->title_text, _("Result"));
		NhelperTitleBarSetContent(_("Result"));
		
	else
		//scroll_text_set_text(nano->title_text, _("Input Number"));
		NhelperTitleBarSetContent(_("Input Number"));

	
	nano->mode = 0;
	draw_cal_str(nano);
}


void 
cal_key_up_handler (struct cal_nano* nano)
{
	DBGLOG("---------------Up Key \n");
	cal_str_update(ROLL_UP);	
	nano->mode = 1;
	draw_cal_str(nano);
}


void
cal_key_down_handler (struct cal_nano* nano)
{
	cal_str_update(ROLL_DOWN);	
	nano->mode = 1;
	draw_cal_str(nano);
}


void
cal_key_play_stop_handler (struct cal_nano* nano)
{
	//is this cal_str good for
	if (!*(express->cal_str)) 
		cal_tts(TTS_NOINPUT, NULL);
	else {
		if (express->show_res_flag) 
			cal_tts(TTS_RESULT, express->res_buf);
		else
		{
			DBGMSG("current cal: express->cal_str:%s  !!\n",express->cal_str);
			char *buf = get_all_calculations(express->cal_str);
			//cal_tts(TTS_PRENUM, get_prev_op_num(express->cal_str));
			cal_tts(TTS_RESULT, get_all_calculations(express->cal_str));
			free_calculations(buf);
			
		}
			
	}
}


#define MAX_INFO_ITEM	4

void
cal_key_info_handler (struct cal_nano* nano)
{
	info_debug("cal_key_info_handler\n");

#if 1
	cal_ungrab_key(nano);

	//scroll_text_set_text(nano->title_text, _("Information"));
	NhelperTitleBarSetContent(_("Information"));

	/* Fill info item */
	int info_item_num = 0;
	char* cal_info[MAX_INFO_ITEM];

	/* input value item */
	char input_info[128];

	int cur_stat = cal_result_state();
	char* res = get_prev_op_num(express->cal_str);

	/* result state */
	if (cur_stat)	
		snprintf(input_info, 128, "%s%s", _("Result:"), express->res_buf);
	else {	
		/* no input */
		if (!res)	
			snprintf(input_info, 128, "%s", _("No input"));
		else
			snprintf(input_info, 128, "%s%s", _("current input number:"), res);
	}
	cal_info[info_item_num] = input_info;
	info_item_num ++;

	nano->ininfo = 1;
	
	/* Start info TTS */
	nano->info_id = info_create();
	info_start (nano->info_id, cal_info, info_item_num, nano->is_running);
	info_destroy (nano->info_id);

	nano->ininfo = 0;

	if (cur_stat)
		//scroll_text_set_text(nano->title_text, _("Result"));
		NhelperTitleBarSetContent(_("Result"));
	else
		//scroll_text_set_text(nano->title_text, _("Input Number"));
		NhelperTitleBarSetContent(_("Input Number"));

	cal_grab_key(nano);
#endif	

}


static void
event_headphone_hander (void)
{
	info_debug("event for headphone\n");
}


static void
cal_timer_handler(struct cal_nano* nano)
{
	//info_debug("cal_timer_handler\n");

	/* if not first logo, I do flush to string */
	if (!(nano->first_logo))
		draw_cal_str(nano);
}


static void
menu_key_shortpress_handler(struct cal_nano* nano)
{
	//GrDestroyTimer(nano->menu_key.lid);
#if 0
	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();
	
	TimerDelete(nano->menu_key.lid);
	nano->menu_key.l_flag = 0;
	cal_ungrab_key(nano);
	//scroll_text_destroy(nano->title_text);
	//GrDestroyWindow(nano->cal_wid);
	cal_tts_destroy();
	//efont_destroy(nano->efont);

	/* Flush the calculator stat and express */
	cal_str_init();
#endif

	cal_tts(TTS_EXIT_CAL,NULL);

	//NeuxAppStop();
}


static void 
menu_key_longpress_handler(struct cal_nano* nano)
{
	DBGMSG("Handler for long press.\n");
	//GrDestroyTimer(nano->menu_key.lid);
	TimerDelete(nano->menu_key.lid);
	nano->menu_key.l_flag = 0;

	/* Ungrab key before in libhelp */
	cal_ungrab_key(nano);

	/* Destroy timer before in libhelp */
	//GrDestroyTimer(nano->tid);
	TimerDelete(nano->Timer);

	//help(nano->is_running);

	/* Recover timer for calculator mode */
	nano->Timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->Timer, cal_OnKeyTtimer);
	
	/* Grab key again when leave libhelp */
	cal_grab_key(nano);
}


static void 
cal_OnKeyTtimer()
{
	cal_timer_handler(&g_nano);
}
static void 
cal_OnLongKeyTtimer()
{
	menu_key_longpress_handler(&g_nano);
}



static void
cal_gettext_init (void)
{

}


static void
cal_theme_init (struct cal_nano* nano)
{
#if 0
	int theme_type = sys_get_global_skin();
	char* theme_dir = sys_get_theme_prefix(theme_type); 										
	//need to free the buffer

	nano->fg_color = sys_get_cur_theme_fgcolor(theme_type);
	nano->bg_color = sys_get_cur_theme_bgcolor(theme_type);
	//thiz->select_color = sys_get_cur_theme_selectcolor(thiz->skin);

	char img_path[128];
	memset(img_path, 0, 128);

	nano->cal_wid = GrNewWindowEx(GR_WM_PROPS_NOAUTOMOVE|GR_WM_PROPS_NODECORATE
								  |GR_WM_PROPS_APPWINDOW, NULL, GR_ROOT_WINDOW_ID,
								  0, 32, 160, 96, nano->bg_color);
	
	nano->cal_pix_wid = GrNewPixmap(160, 96, NULL);

#if ARCH_MIPS

	GrSelectEvents(nano->cal_wid, GR_EVENT_MASK_EXPOSURE
						|GR_EVENT_MASK_KEY_DOWN|GR_EVENT_MASK_KEY_UP
						|GR_EVENT_MASK_HOTPLUG|GR_EVENT_MASK_SW_ON
						|GR_EVENT_MASK_SW_OFF |GR_EVENT_MASK_TIMER);
#else

	GrSelectEvents(nano->cal_wid, GR_EVENT_MASK_EXPOSURE
							|GR_EVENT_MASK_KEY_DOWN|GR_EVENT_MASK_KEY_UP);

#endif

	nano->efont = efont_create(TTF_STSONG, 16);
	nano->gc    = efont_get_gc(nano->efont);
	nano->title_text   = scroll_text_create(0, 20, 160, 16);
	scroll_text_set_text(nano->title_text, _("Input Number"));

	GrSetGCForeground(nano->gc, nano->fg_color);
	GrSetGCBackground(nano->gc, nano->bg_color);

	/*Create gc for exposure handler*/
	nano->ex_gc = GrNewGC();
	GrSetGCForeground(nano->ex_gc, nano->bg_color);
	
	snprintf(img_path, 128, "%s%s", theme_dir, "/calculator/cal_back.png");
	nano->image = GrLoadImageFromFile(img_path, 0);
#endif	
}


int 
cal_ui_init (struct cal_nano* nano)
{

#if 0
	if (GrOpen() < 0) {
		info_debug("GrOpen error!\n");
		return -1;
	}
	
	cal_gettext_init();
	
	cal_theme_init(nano);

	GrMapWindow(nano->cal_wid);
	
	//GrDrawImageToFit(nano->cal_pix_wid, nano->gc,0, 0, 160, 92, nano->image);
	GrSetGCForeground(nano->gc, nano->bg_color);
	GrFillRect(nano->cal_pix_wid, nano->gc,
		0, 0, 160, 96);
	GrSetGCForeground(nano->gc, nano->fg_color);
	
	//GrFillRect(nano->cal_pix_wid, nano->gc,  0, 0, 160, 96);
	GrText(nano->cal_pix_wid, nano->gc, 0, 35, _("CALCULATOR"), -1, nano->efont->flag);
	GrCopyArea(nano->cal_wid, nano->gc, 0, 0, 160, 96, nano->cal_pix_wid, 0, 0, 0);
	nano->first_logo = 1;

	cal_tts_init(nano->is_running);
	cal_tts(TTS_ENTER_CAL, NULL);
	
	//nano->keylocked = keylock_locked();					//return 1 means key locked
	
	//nano->tid = GrCreateTimer(nano->cal_wid, TIMER_PERIOD);

	/*init menu longpress struct*/
	nano->menu_key.l_flag = 0;
	nano->menu_key.pressed = 0;

	/* Init key state as key up */
	nano->key_stat = KEY_UP_STAT;

	/*设定为可以sleep*/	
	//sys_set_suspend_action(ACTION_SLEEP);
#endif

	return 0;
	
}


void 
cal_ui_main_loop (struct cal_nano* nano)
{
#if 0
	GR_EVENT event;
	
	cal_grab_key(nano);

	while (1) {
		GrGetNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				info_debug("Gr event exposure!\n");
				//GrFillRect(nano->cal_wid, nano->gc,  0, 0, 160, 96);
				break;

			case GR_EVENT_TYPE_TIMER:
				if (event.timer.tid == nano->tid)
					cal_timer_handler(nano);
				else if (event.timer.tid == nano->menu_key.lid)
					menu_key_longpress_handler(nano);
				break;
			
#if ARCH_MIPS	
			case GR_EVENT_TYPE_SW_OFF: {
				info_debug("event sw_off.\n");
				if (event.sw.sw == MWSW_HP_INSERT) {
					event_headphone_hander();
				} else if (event.sw.sw == MWSW_KBD_LOCK) {
					nano->keylocked = 0;
				}
					
			}
			break;	
							
			case GR_EVENT_TYPE_SW_ON: {	
				info_debug("event sw on.\n");
				if (event.sw.sw == MWSW_HP_INSERT) {
					info_debug("mwsw hp insert.\n");
					event_headphone_hander();
				} else if (event.sw.sw == MWSW_KBD_LOCK) {
					nano->keylocked = 1;
				}
			}
			break;
#endif
			case GR_EVENT_TYPE_KEY_UP:
				info_debug("Key up handler!\n");

				/*如果是短按键处理，退出计算器*/
				if (nano->menu_key.l_flag) {
					menu_key_shortpress_handler(nano);
					return;
				} else
					break;
				
			case GR_EVENT_TYPE_KEY_DOWN:
				info_debug("Key down handler!\n");
				
				/*一旦有按键按下，就认为计算器已经开始输入*/
				if (nano->first_logo)
					nano->first_logo = 0;
				
				if (nano->keylocked) {
					cal_tts(TTS_ERRBEEP, NULL);
					break;
				}
				switch (event.keystroke.ch) {
						
					case PKEY_RIGHT:
						cal_key_right_handler(nano);
						break;
							
					case PKEY_LEFT:
						cal_key_left_handler(nano);
						break;
	
					case PKEY_UP:						
						cal_key_up_handler(nano);
						break;

					case PKEY_PLAY_STOP:
						cal_key_play_stop_handler(nano);
						break;
				
					case PKEY_DOWN:
						cal_key_down_handler(nano);
						break;

					case PKEY_MENU:					
						nano->menu_key.lid = GrCreateTimer(nano->cal_wid, 1000);		//1s
						nano->menu_key.l_flag = 1;
						break;
				
					case PKEY_INFO:
						cal_key_info_handler(nano);
						break;
						
					case PKEY_MUSIC:
					case PKEY_BOOK:
					case PKEY_RADIO:
					case PKEY_RECORD:
						cal_tts(TTS_ERRBEEP, NULL);
						break;
				}
		}
	}
#endif	
}

#if 1
//new tts add function!
void cal_tip_voice_sig(int sig)
{
	printf("calculator recieve signal of TTS end!\n");
	//cal_tts_set_stopped();
	g_nano.is_running = 0;
}
#endif



static void
cal_CreateTimer(FORM *widget,struct cal_nano* nano)
{
	nano->Timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->Timer, cal_OnKeyTtimer);
	DBGLOG("Rec_CreateTimer nano->rec_timer:%d",nano->Timer);

}

static void
cal_CreateLongTimer(FORM *widget,struct cal_nano* nano)
{
	nano->menu_key.lid= NeuxTimerNew(widget, TIMER_LONG_PERIOD, -1);
	NeuxTimerSetCallback(nano->menu_key.lid, cal_OnLongKeyTtimer);
	DBGLOG("cal_CreateLongTimer nano->rec_timer:%d",nano->menu_key.lid);

}



static void
cal_OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);
	 //help在运行时，不响应任何按键(因为help按键grap key时用的NX_GRAB_HOTKEY)
	 if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP) || g_nano.ininfo)
	 {
	  DBGLOG("help is running or is in info !!\n");
	  return;
	 }

	 if(cal_get_exit_flag())
	 {
	 	DBGLOG("I am in exit status,OK,if you are in a hurry,I will close help now\n");
		NxAppStop();
	 	return;
	 }
	 if (NeuxAppGetKeyLock(0)) {

		 cal_tts(TTS_KEYLOCK,NULL);
		 return;
	 }

	 
	/*一旦有按键按下，就认为计算器已经开始输入*/
	if (g_nano.first_logo)
	{
		g_nano.first_logo = 0;
		if(NeuxWidgetIsVisible(g_nano.pTitle))
		{
			//NeuxWidgetDestroy(g_nano.pTitle);
			NeuxWidgetShow(g_nano.pTitle,FALSE);
		}
	}

	switch (key.ch) {
			
		case MWKEY_RIGHT:
			cal_key_right_handler(&g_nano);
			break;
				
		case MWKEY_LEFT:
			cal_key_left_handler(&g_nano);
			break;
	
		case MWKEY_UP:						
			cal_key_up_handler(&g_nano);
			break;
	
		case MWKEY_ENTER:
			cal_key_play_stop_handler(&g_nano);
			break;
	
		case MWKEY_DOWN:
			cal_key_down_handler(&g_nano);
			break;
	
		case MWKEY_MENU: 	
			cal_CreateLongTimer(widget,&g_nano);
			//nano->menu_key.lid = GrCreateTimer(g_nano.cal_wid, );		//1s
			g_nano.menu_key.l_flag = 1;
			break;
	
		case MWKEY_INFO:
			cal_key_info_handler(&g_nano);
			break;
			
		case VKEY_MUSIC:
		case VKEY_BOOK:
		case VKEY_RADIO:
		case VKEY_RECORD:
			cal_tts(TTS_ERRBEEP, NULL);
			break;
	}


}

static void
cal_OnWindowKeyup(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeyup %d.\n",key.ch);


	/*如果是短按键处理，退出计算器*/
	if (g_nano.menu_key.l_flag) {
		menu_key_shortpress_handler(&g_nano);
	}
}

static void
cal_OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);

	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();
	
	TimerDelete(g_nano.menu_key.lid);
	g_nano.menu_key.l_flag = 0;
	cal_ungrab_key();
	//scroll_text_destroy(nano->title_text);
	//GrDestroyWindow(nano->cal_wid);
	cal_tts_destroy();
	//efont_destroy(nano->efont);
	
	/* Flush the calculator stat and express */
	cal_str_init();
	

	
	plextalk_global_config_close();

	GrDestroyWindow(g_nano.pix_wid);
	GrDestroyGC(g_nano.pix_gc);
#if 0
	TimerDelete(g_nano.Timer);
	NeuxWidgetDestroy(g_nano.pTitle);
	NeuxFormDestroy(widget);
	widget = NULL;
#endif	
}

static void
cal_OnWindowExposure(WID__ wid)
{
	DBGLOG("---------------cal_OnWindowExposure %d.\n",wid);
	if (!g_nano.first_logo)
		redraw_cal_str(wid,&g_nano);
	
}

static void cal_OnGetFocus(WID__ wid)
{
	DBGLOG("On Window Get Focus\n");
	NhelperTitleBarSetContent(_("Input Number"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_CALC);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);

}



static void cal_onAppMsg(const char * src, neux_msg_t * msg)
{
	DBGLOG("app msg %d .\n", msg->msg.msg.msgId);

	switch (msg->msg.msg.msgId)
	{
	case PLEXTALK_TIMER_MSG_ID:
		break;	
#if 0		
	case PLEXTALK_LANG_MSG_ID:
		//NeuxWidgetShow(g_nano.pTitle, TRUE);
		//NhelperTitleBarSetContent(_("Mic recording"));
		break;
	case PLEXTALK_TTS_MSG_ID:
		//rec_init_tts_prop();
		break;
	case PLEXTALK_GUIDE_MSG_ID:
		//rec_init_tts_prop();
		break;
	case PLEXTALK_THEME_ID:
		{
			
			widget_prop_t wprop;
			struct rec_nano* nano;
			nano = &s_nano;
			
			NeuxThemeInit(g_config->setting.lcd.theme);
			NeuxWidgetGetProp(widget, &wprop);
			wprop.bgcolor = theme_cache.background;
			wprop.fgcolor = theme_cache.foreground;
			NeuxWidgetSetProp(widget, &wprop);
	
			
			NeuxWidgetGetProp(nano->filename_label, &wprop);
			wprop.bgcolor = theme_cache.background;
			wprop.fgcolor = theme_cache.foreground;
			NeuxWidgetSetProp(nano->filename_label, &wprop);

			NeuxWidgetGetProp(nano->inputsource_label, &wprop);
			wprop.bgcolor = theme_cache.background;
			wprop.fgcolor = theme_cache.foreground;
			NeuxWidgetSetProp(nano->inputsource_label, &wprop);

			NeuxWidgetGetProp(nano->rectime_label, &wprop);
			wprop.bgcolor = theme_cache.background;
			wprop.fgcolor = theme_cache.foreground;
			NeuxWidgetSetProp(nano->rectime_label, &wprop);

			NeuxWidgetGetProp(nano->freshtime_label, &wprop);
			wprop.bgcolor = theme_cache.background;
			wprop.fgcolor = theme_cache.foreground;
			NeuxWidgetSetProp(nano->freshtime_label, &wprop);
			
			NeuxWidgetShow(s_nano.filename_label, TRUE);
			NeuxWidgetShow(s_nano.inputsource_label, TRUE);
			NeuxWidgetShow(s_nano.rectime_label, TRUE);
			NeuxWidgetShow(s_nano.freshtime_label, TRUE);
			NhelperTitleBarSetContent(_("Mic recording"));
			NeuxFormShow(widget, TRUE);
			
		}

		break;
#endif		
	case PLEXTALK_FILELIST_MSG_ID:
		//file_list_mode = !*((int*)msg->msg.msg.msgTxt);
		break;
	case PLEXTALK_BOOK_CONTENT_MSG_ID:
		//book_is_text = !*((int*)msg->msg.msg.msgTxt);
		break;
	case PLEXTALK_APP_MSG_ID:
		{
			app_rqst_t *rqst = msg->msg.msg.msgTxt;
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
			}
		}
		break;
		
	}
}



static void
cal_CreateLabels (FORM *widget,struct cal_nano* nano)	
{
	widget_prop_t wprop;
	label_prop_t lprop;

	int h,y;

	h = 24+6;
	y = (MAINWIN_HEIGHT - h)/2;
	
	nano->pTitle= NeuxLabelNew(widget,
						MAINWIN_LEFT,y,MAINWIN_WIDTH, h,_("CALCULATOR"));
	
	NeuxWidgetGetProp(nano->pTitle, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->pTitle, &wprop);
	
	NeuxLabelGetProp(nano->pTitle, &lprop);
	lprop.align = LA_CENTER;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->pTitle, &lprop);
	NeuxWidgetSetFont(nano->pTitle, sys_font_name, 24);
	NeuxWidgetShow(nano->pTitle, TRUE);

}


/* Function creates the main form of the application. */
void
cal_CreateFormMain (struct cal_nano* nano)
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, cal_OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 cal_OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  cal_OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_EXPOSURE,  cal_OnWindowExposure);
	NeuxFormSetCallback(widget, CB_FOCUS_IN, cal_OnGetFocus);

	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(widget, &wprop);

	NeuxWidgetSetFont(widget, sys_font_name, 16);

	//LABEL用于显示字符串
	cal_CreateLabels(widget,nano);
	//创建定时器用于计时和显示
	cal_CreateTimer(widget,nano);
	

	cal_grab_key();

	//NhelperTitleBarSetContent(_("Input Number"));

	NeuxFormShow(widget, TRUE);

	nano->wid = NeuxWidgetWID(widget);

	nano->pix_wid = GrNewPixmap(CAL_WINDOW_WIDTH, CAL_WINDOW_HEIGHT, NULL);
	nano->pix_gc = GrNewGC();
	nano->font_id = GrCreateFont((GR_CHAR*)sys_font_name, /*sys_font_size*/16, NULL);
	GrSetGCUseBackground(nano->pix_gc ,GR_FALSE);
	GrSetGCForeground(nano->pix_gc,nano->fgcolor);
	GrSetGCBackground(nano->pix_gc,nano->bgcolor);
	GrSetFontAttr(nano->font_id , GR_TFKERNING, 0);
	GrSetGCFont(nano->pix_gc, nano->font_id);

	NeuxWidgetFocus(widget);

	//int fontid = NxGetFontID("msyh.ttf", 16);
	//GrSetGCFont (nano->pix_gc, fontid);

}

static void cal_onSWOn(int sw)
{
	DBGLOG("------- cal_onSWOn	--------\n");

	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 1;
		break;
	case SW_HP_INSERT:
		DBGLOG("------- cal_onSWOn	--------\n");
		//event_headphone_hander(&s_nano);
		break;
	}
}

static void cal_onSWOff(int sw)
{
	DBGLOG("------- cal_onSWOff --------\n");

	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 0;
		break;
	case SW_HP_INSERT:
		DBGLOG("------- cal_onSWOff --------\n");
		//event_headphone_hander(&s_nano);
		break;
	}
}
static void
cal_read_keylock()
{
	char buf[16];
	int res;
	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0")){
	key_locked = 1;
	}else{
	key_locked = 0;
	}
	DBGMSG("keylock:%d\n",key_locked);
}



void DesktopScreenSaver(GR_BOOL activate)
{
}


static void
cal_initApp(struct cal_nano* nano)
{
	int res;

	/* global config */
	//plextalk_global_config_init();
	plextalk_global_config_open();

	/* theme */
	//CoolShmWriteLock(g_config_lock);
	NeuxThemeInit(g_config->setting.lcd.theme);
	//CoolShmWriteUnlock(g_config_lock);
#if 0	/* language */
	//plextalk_get_all_lang("plextalk.xml", &all_langs);

	//CoolShmReadLock(g_config_lock);
	res = plextalk_find_lang(&all_langs, g_config->setting.lang.lang);
	//CoolShmReadUnlock(g_config_lock);
	if (res < 0) {
		WARNLOG("Current setted language is not in languages list.\n");
		//CoolShmWriteLock(g_config_lock);
		strlcpy(g_config->setting.lang.lang, all_langs.langs[0].lang, PLEXTALK_LANG_MAX);
		//CoolShmWriteUnlock(g_config_lock);
	}
#endif
	//CoolShmReadLock(g_config_lock);
	plextalk_update_lang(g_config->setting.lang.lang, "calculator");
	//CoolShmReadUnlock(g_config_lock);

	plextalk_update_sys_font(getenv("LANG"));

	g_tts_isrunning = 0;
	nano->is_running = &g_tts_isrunning;
	cal_tts_init(nano->is_running);
	cal_tts(TTS_ENTER_CAL, NULL);

	/*init menu longpress struct*/
	nano->menu_key.l_flag = 0;
	nano->menu_key.pressed = 0;

	/* Init key state as key up */
	nano->key_stat = KEY_UP_STAT;

	nano->fgcolor = theme_cache.foreground;
	nano->bgcolor = theme_cache.background;

	nano->first_logo = 1;
	nano->ininfo = 0;

	//cal_read_keylock();

}


int
main(int argc,char **argv)
{
	info_debug("In calculator mode!\n");

		// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}
	
	
	signal(SIGVPEND, cal_tip_voice_sig);


	//cal_ui_main_loop(&g_nano);

	//info_debug("Out calculator mode!\n");
	//return 0;

	cal_initApp(&g_nano);
	DBGMSG("application initialized\n");

	//cal界面禁止应用切换
	plextalk_schedule_lock();
	//cal界面只关屏，不休眠
	plextalk_suspend_lock();
	plextalk_sleep_timer_lock();
		
	//NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_CALC);


	cal_CreateFormMain(&g_nano);

	NeuxAppSetCallback(CB_MSG,	 cal_onAppMsg);
	//NeuxWMAppSetCallback(CB_HOTPLUG, onHotplug);
	//NeuxAppSetCallback(CB_SW_ON,	cal_onSWOn);
	//NeuxAppSetCallback(CB_SW_OFF,  	cal_onSWOff);

	//this function must be run after create main form ,it need set schedule time
	DBGMSG("mainform shown\n");

	// start application, events starts to feed in.
	NeuxAppStart();

	return 0;
}

