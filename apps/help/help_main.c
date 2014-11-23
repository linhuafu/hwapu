#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <microwin/device.h>
#include <stdio.h>
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "help_tts.h"

#include "libvprompt.h"

#include "plextalk-config.h"
#include "Amixer.h"
#include "plextalk-statusbar.h"
#include "plextalk-keys.h"
#include "desktop-ipc.h"
#include "plextalk-theme.h"
#include "application-ipc.h"
#include "plextalk-ui.h"
#include "Sysfs-helper.h"

#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif

#define KEYLOCK  "/sys/devices/platform/gpio-keys/on_switches"

#define HEADPHONE_JACK1 	"/sys/devices/platform/jz-codec/headphone_jack"

#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))

#define LONG_KEY_TIMES	1000

static FORM *  formMain;
#define widget formMain

#if 0
/* For Key long_press judgement */
struct long_press {
	int l_flag;
	int pressed;
};
#endif

struct help_nano {
	TIMER *long_timer;
	int wid;
	int key_locked;
	int is_running;

	int short_flag;
	int key_val;
};

enum {

	OUTPUT_BYPASS = 0,
	OUTPUT_DAC,
	OUTPUT_SIDETONE,

	INPUT_MIC1,
	INPUT_LINEIN,

	MIC1_BOOST,
	CAPTURE,
	AGC_ON,
	
	SPEAKER_ON,
	SPEAKER_OFF,

	HEADPHONE_ON,
	HEADPHONE_OFF,

	VOLUME_SET,
	ANALOG_PATH_IGNORE_SUSPEND,
	
	
};


static struct help_nano g_help_nano;
//static struct long_press menu_key; 
struct plextalk_language_all all_langs;


/* do not show this window */
//static GR_WINDOW_ID h_wid;

/* Use for Key_lock */
static int key_locked;


static int grab_key_table[] = {
	MWKEY_POWER,
	VKEY_BOOK, 
	VKEY_MUSIC,
	VKEY_RADIO,
	MWKEY_ENTER,
	MWKEY_UP,
	MWKEY_DOWN,
	MWKEY_LEFT,
	MWKEY_RIGHT,
	MWKEY_VOLUME_UP,
	MWKEY_VOLUME_DOWN,
	VKEY_RECORD,
	MWKEY_MENU,
	MWKEY_INFO,
};

static void
help_menu_key_longpress_handler (void);



void DesktopScreenSaver(GR_BOOL activate)
{
}

static int
help_keyIsLocked()
{
	return g_help_nano.key_locked;
}

static void
help_grab_key()
{
	int i;

	for(i = 0; i < ARRAY_SIZE(grab_key_table); i++)
		NeuxWidgetGrabKey(widget, grab_key_table[i], NX_GRAB_HOTKEY);
}


static void
help_ungrab_key ()
{
	int i;
	for(i = 0; i < ARRAY_SIZE(grab_key_table); i++)
		NeuxWidgetUngrabKey(widget, grab_key_table[i]);
}

static void help_p_mixer_set (int choice, int val)
{
	char* pra[2];
	char buf[32];
	const char* info = NULL;

	int adjust_vol;

	int battery;
	
	switch (choice) {
		case ANALOG_PATH_IGNORE_SUSPEND:
			pra[0] = "\'Analog Path Ignore Suspend\'";
			if(!val){
				pra[1] = "off";
				info = "[pmixer]: Analog Path Ignore Suspend off!\n";
			}
			else{
				pra[1] = "on";
				info = "[pmixer]: Analog Path Ignore Suspend on!\n";
			}
			
			break;
		case OUTPUT_BYPASS:
			pra[0] = "\'Output Mux\'";
			pra[1] = "Bypass";
			info = "[pmixer]:output mux bypass!\n";
			break;

		case OUTPUT_DAC:
			pra[0] = "\'Output Mux\'";
			pra[1] = "DAC"; 
			info = "[pmixer]:output mux DAC!\n";
			break;

		case OUTPUT_SIDETONE:
			pra[0] = "\'Output Mux\'";
			pra[1] = "Sidetone1";
			info = "[pmixer]:output sidetone!\n";
			break;

		case INPUT_MIC1:
			pra[0] = "\'Input Mux\'";
			pra[1] = "Mic1";
			info = "[pimxer]:Input mux Mic1!\n";
			break;

		case INPUT_LINEIN:
			pra[0] = "\'Input Mux\'";
			pra[1] = "Line In";
			info = "[pmixer]:input linein!\n";
			break;

		case MIC1_BOOST:
			pra[0] = "\'Mic1 Boost\'";
			snprintf(buf, 32, "%d", val);
			pra[1] = buf;
			info = "[pmixer]:Mic1 boost set!\n";
			break;
			
		case CAPTURE:
			pra[0] = "\'Capture\'";
			snprintf(buf, 32, "%d", val);
			pra[1] = buf;
			info = "[pmixer]: set Capture!\n";
			break;

		case AGC_ON:
			pra[0] = "\'AGC\'";
			pra[1] = "on";
			info = "[pmixer]:AGC on!\n";
			break;

		case SPEAKER_ON:
			pra[0] = "Speaker";
			pra[1] = "on";
			info = "[pmixer]: Speaker on!\n";
			break;

		case SPEAKER_OFF:
			pra[0] = "Speaker";
			pra[1] = "off";
			info = "[pmixer]: Speaker off!\n";
			break;

		case HEADPHONE_ON:
			pra[0] = "Headphone";
			pra[1] = "on";
			info = "[pmixer]: Headphone on!\n";
			break;

		case HEADPHONE_OFF:
			pra[0] = "Headphone";
			pra[1] = "off";
			info = "[pmixer]: Headphone off!\n";
			break;

		case VOLUME_SET:
			break;

		default:
			info = "[pmixer]: No contorl!\n";
			break;
	}

	amixer_direct_sset_str(VOL_RAW, pra[0], pra[1]);
	
	//printf("%s", info);
}


static int 
headphone_on (void)
{
	int hp_online;
	
	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);
	
	info_debug("mic_headphone hp_online :%d",hp_online);
	
	return hp_online;

}


#if 0
int 
keylock_locked (void)
{
	char res;
	
	int fd = open(KEYLOCK, O_RDONLY);

	if (fd == -1) {
		printf("open key lock error!\n");
		return 0;
	}
	
	if ((read(fd, &res, 1)) != 1) {
		printf("read key_lock error!\n");
		close(fd);
		return 0;
	}

	close(fd);

	if (res == '0') {
		printf("[keypad]:locked\n");
		return 1;
	} else {
		printf("[keypad]:unlocked\n");
		return 0;
	}
}
#endif

#if 0
/* Init help function */
static void 
help_init (void)
{


   printf("Enter help_init fun!\n");
#if 0
	if (GrOpen() < 0) {
			printf("GrOpen error!\n");
			return ;
		}
#endif	
	int i;


	
	h_wid = GrNewWindowEx(GR_WM_PROPS_NOAUTOMOVE|GR_WM_PROPS_NODECORATE
								  |GR_WM_PROPS_APPWINDOW, NULL, GR_ROOT_WINDOW_ID,
								  0, 32, 160, 92, WHITE);
#if ARCH_MIPS
	GrSelectEvents(h_wid, GR_EVENT_MASK_EXPOSURE
			| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP
			| GR_EVENT_MASK_HOTPLUG | GR_EVENT_MASK_TIMER
			| GR_EVENT_MASK_SW_ON | GR_EVENT_MASK_SW_OFF);
#else
	GrSelectEvents(h_wid, GR_EVENT_MASK_EXPOSURE
			|GR_EVENT_MASK_KEY_DOWN|GR_EVENT_MASK_KEY_UP);

#endif
	
	for(i = 0; i < ARRAY_SIZE(grab_key_table); i++)
		GrGrabKey(h_wid,grab_key_table[i], GR_GRAB_HOTKEY_EXCLUSIVE);

	/* init menu_key */
	menu_key.l_flag = 0;
	menu_key.pressed = 0;

	key_locked = keylock_locked();
	if (key_locked)
		menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_LOCK_LOCK);
	else 
		menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_LOCK_UNLOCK);

	/*设定为可以sleep*/
	//system("echo 1 > /sys/power-helper/auto_suspend_action");
	sys_set_suspend_action(ACTION_SLEEP);

	/* init gettext */

	/* init tts */
	help_tts_init();

	printf("Enter TTS_BEGIN tts play  fun!\n");
	help_tts(TTS_BEGIN);
}
#endif

static void 
event_headphone_handler(void)
{
	if (headphone_on()) {
		help_p_mixer_set(SPEAKER_OFF, 0);
		help_p_mixer_set(HEADPHONE_ON, 0);
	} else {
		help_p_mixer_set(SPEAKER_ON, 0);
		help_p_mixer_set(HEADPHONE_OFF, 0);
	}	

}

static void 
help_OnKeyLongTtimer()
{
	help_menu_key_longpress_handler();
}


static void
help_CreateLongTimer(int keych)
{
	g_help_nano.long_timer = NeuxTimerNew(widget, LONG_KEY_TIMES, -1);
	NeuxTimerSetCallback(g_help_nano.long_timer, help_OnKeyLongTtimer);
	g_help_nano.short_flag = 1;
	g_help_nano.key_val= keych;
	DBGLOG("help_CreateLongTimer nano->rec_timer:%d",g_help_nano.long_timer);
}


static void
help_key_down_handler (KEY__ key)
{

	int key_val = key.ch;
	printf("Enter   help_key_down_handler,key_val=%d !\n",key_val); 

	if (NeuxAppGetKeyLock(0)) {
		{
			help_tts(TTS_LOCK); 
		}
		return;
	}
	

	if(help_get_tts_state() == TTS_END_STATE_EXIT)
	{
		DBGMSG("I am in exit status,OK,if you are in a hurry,I will close help now\n");
		help_tts_end_toDestroy();
		return;
	}

	
	switch (key_val) 
	{
		case MWKEY_POWER:
		//help_tts(TTS_POWER);
		help_CreateLongTimer(key_val);
		break;

		case VKEY_BOOK:
		help_tts(TTS_BOOK);
		break;
		
		case VKEY_MUSIC:
		help_tts(TTS_MUSIC);
		break;
		
		case VKEY_RADIO:		
		help_tts(TTS_RADIO);
		break;
		
		case MWKEY_ENTER:
		help_tts(TTS_PLAY_STOP);
		break;
		
		case MWKEY_UP:
		help_tts(TTS_UP);
		break;
		
		case MWKEY_DOWN:			
		help_tts(TTS_DOWN);
		break;

		case MWKEY_LEFT:
		help_tts(TTS_LEFT);
		break;
		
		case MWKEY_RIGHT:
		help_tts(TTS_RIGHT);
		break;
		
		case MWKEY_VOLUME_UP:
		help_tts(TTS_VOL_INC);
		break;
		
		case MWKEY_VOLUME_DOWN:
		help_tts(TTS_VOL_DEC);
		break;

		case VKEY_RECORD:
		help_tts(TTS_RECORD);
		break;

		case MWKEY_MENU:
		//menu_key.lid = GrCreateTimer(h_wid, 1000);		//1s
		//menu_key.l_flag = 1;
		//help_tts(TTS_MENU);
		help_CreateLongTimer(key_val);
		break;

		case MWKEY_INFO:
		help_tts(TTS_INFO);
		break;

		case SW_KBD_LOCK:
		//help_tts(TTS_LOCK);	
		break;
	}
}


static void
help_menu_key_longpress_handler (void)
{
	printf("Handler for long press!\n");
	TimerDelete(g_help_nano.long_timer);
	g_help_nano.short_flag = 0;

	if(MWKEY_MENU== g_help_nano.key_val){
		help_tts(TTS_EXIT);
	}else if(MWKEY_POWER== g_help_nano.key_val){
	
	}
	//wait_tts();

	//给desktop发送一个信号
	//menu_notify_cmd("desktop", 
	//	MN_NOTIFY_APP_HELP_EXIT);
	//exit(0);
}


static void
help_menu_key_shortpress_handler (void)
{
	printf("Handler for short press!\n");
	
	TimerDisable(g_help_nano.long_timer);
	g_help_nano.short_flag = 0;

	if(MWKEY_MENU== g_help_nano.key_val){
		help_tts(TTS_MENU);
	}else if(MWKEY_POWER== g_help_nano.key_val){
		help_tts(TTS_POWER);
	}
}


//其他事件要求退出的，直接退出

#if 0
static void
help_main_loop (void)
{	
	GR_EVENT event;

	while (1) 
	{
		GrGetNextEvent(&event);
		
		switch (event.type) 
		{
			printf("Help: have   what event:%d\n",event.type); 	
			
			case GR_EVENT_TYPE_EXPOSURE:
				printf("Gr event exposure!\n"); 			
			break;

			case GR_EVENT_TYPE_TIMER:
				menu_key_longpress_handler();
			break;
		
			case GR_EVENT_TYPE_KEY_DOWN:	

				printf("have help  key down event!\n"); 	
				
				if (key_locked)
				{
					help_tts(TTS_ERROR);
					
				}
				else
				{

				   help_key_down_handler(event.keystroke.ch);
				}
			break;

			case GR_EVENT_TYPE_KEY_UP:
				if (menu_key.l_flag)
					menu_key_shortpress_handler();
			break;
			
#if ARCH_MIPS			
			case GR_EVENT_TYPE_SW_OFF:
				if (event.sw.sw == MWSW_HP_INSERT) 
				{
				
						printf("mwsw hp remove.\n");

						event_headphone_handler();
				}
				else if (event.sw.sw == MWSW_KBD_LOCK) { 
					key_locked = 0;
					menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_LOCK_UNLOCK);
				}
					
			break;	
				
				
			case GR_EVENT_TYPE_SW_ON: {
				if (event.sw.sw == MWSW_HP_INSERT)
				{
				
						printf("mwsw hp insert.\n");
						event_headphone_handler();
				}
				else if (event.sw.sw == MWSW_KBD_LOCK) {
					key_locked = 1;
					menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_LOCK_LOCK);
				}
			}
			break;
#endif
		}
	}
}
#endif


void help_tts_end_toDestroy(void)
{
	DBGLOG("Record App Stop!!\n");
	help_set_tts_end_state(TTS_END_STATE_NONE);
	//plextalk_schedule_unlock();
	//plextalk_suspend_unlock();
	help_ungrab_key();
	help_tts_end();
	NeuxAppStop();
	
}



static void
help_OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);
	help_key_down_handler(key);

}

static void
help_OnWindowKeyup(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeyup %d.\n",key.ch);
		/*如果是短按键处理，当成MENU */
	if (g_help_nano.short_flag) {
		help_menu_key_shortpress_handler();
	}

}

static void
help_OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_global_config_close();

#if 0	
	TimerDelete(s_nano.rec_timer);
	NeuxFormDestroy(widget);	
	widget = NULL;
#endif	
}



static void
help_onSWOn(int sw)
{
	DBGLOG("help_onSWOn :%d.\n",sw);
	switch (sw) {
	case SW_KBD_LOCK:
		g_help_nano.key_locked = 1;
		//help_key_down_handler(SW_KBD_LOCK);
		break;
	case SW_HP_INSERT:
		event_headphone_handler();
		break;
	}
}

static void
help_onSWOff(int sw)
{
	DBGLOG("help_onSWOff :%d.\n",sw);
	switch (sw) {
	case SW_KBD_LOCK:
		g_help_nano.key_locked = 0;
		//help_key_down_handler(SW_KBD_LOCK);
		break;
	case SW_HP_INSERT:
		event_headphone_handler();
		break;
	}
}

static void
help_OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	switch (device) {
	case MWHOTPLUG_DEV_UDC:
		if (action == MWHOTPLUG_ACTION_ONLINE){
			DBGMSG("the USB is Inline!!\n");
			help_tts_end_toDestroy();
		}
		break;
	}
}


static void
help_read_keylock(struct help_nano* nano)
{
	char buf[16];
	int res;
	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0")){
		nano->key_locked = 1;
	}else{
		nano->key_locked = 0;
	}
	DBGMSG("keylock:%d\n",nano->key_locked);
}



static void
help_initApp(struct help_nano* nano)
{
	int res;

	/* global config */
	//plextalk_global_config_init();
	plextalk_global_config_open();

	/* theme */
	//CoolShmWriteLock(g_config_lock);
	//NeuxThemeInit(g_config->setting.lcd.theme);
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
	plextalk_update_lang(g_config->setting.lang.lang, "help");
	//CoolShmReadUnlock(g_config_lock);

	//plextalk_update_sys_font();

	//nano->is_running = 0;
	//rec_tts_init(&(nano->is_running));
	//rec_tts(TTS_USAGE);

	nano->key_val = -1;
	nano->short_flag = 0;
	nano->is_running = 0;
	help_tts_init();

	//help_read_keylock(nano);
	

}

static void
help_InitForm()
{
	help_grab_key();
	help_tts(TTS_BEGIN);
}


/* Function creates the main form of the application. */
void
help_CreateFormMain ()
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, help_OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 help_OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  help_OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_HOTPLUG,  help_OnWindowHotplug);

#if 0

	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(widget, &wprop);

	NeuxFormShow(widget, FALSE);

	//创建四个LABEL用于显示字符串
	Rec_CreateLabels(widget,nano,data);
	//创建定时器用于计时和显示
	Rec_CreateTimer(widget,nano);
#endif	

	help_InitForm();

}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	app_rqst_t *rqst;

	if (msg->msg.msg.msgId != PLEXTALK_APP_MSG_ID)
		return;

	rqst = msg->msg.msg.msgTxt;
	switch (rqst->type) 
	{
	case APPRQST_LOW_POWER://低电要关机了
		DBGMSG("Yes,I have received the lowpower message\n");
		help_tts_end_toDestroy();
		break;	

	}
}


void help_tip_voice_sig(int sig)
{
	printf("Help recieve signal for TTS stopped!\n");
	help_tts_set_stop();
}


int
main(int argc,char **argv)

{
	
	//signal(SIGVPEND, help_tip_voice_sig);
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	printf("Enter help main fun!\n");

	help_initApp(&g_help_nano);
	DBGMSG("application initialized\n");
	
	DBGMSG("NeuxAppCreate app create success!\n");
	//get input window id
	//g_help_nano.wid = NeuxAppMsgWID();
	DBGLOG("get wid success!\n");

	//help界面禁止应用切换
	plextalk_schedule_lock();
	//help界面只关屏，不休眠
	plextalk_suspend_lock();

	help_CreateFormMain();
	DBGMSG("create form success!\n");
	NeuxAppSetCallback(CB_MSG,	 onAppMsg);
	//NeuxAppSetCallback(CB_SW_ON,	help_onSWOn);
	//NeuxAppSetCallback(CB_SW_OFF,	help_onSWOff);
	
	//this function must be run after create main form ,it need set schedule time
	DBGMSG("mainform shown\n");
	
	// start application, events starts to feed in.
	NeuxAppStart();
	
	return 0;
}
