#include <microwin/nano-X.h>
#include <microwin/device.h>

#define OSD_DBG_MSG
#include "nc-err.h"

#include "amixer.h"
#include "application.h"
#include "neux.h"
#include "widgets.h"
#include "desktop-ipc.h"
#include "application-ipc.h"
#include "plextalk-i18n.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "sysfs-helper.h"
#include "plextalk-keys.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"

struct voice_prompt_cfg vprompt_cfg;
static char* curr_app;

static int signal_fd;
static int do_exit;

//for BUG("can't recieve tts'end signal")
static int wait_tts_end;
static int wait_tts_count;



//debug..
#define PC_DBG(fmt, arg...) do { \
	printf("[PC_CONNECTOR-->%s, %d]:\n" fmt, __func__, __LINE__, ##arg); \
} while(0)

static void onDestroy(WID__ wid)
{
	PC_DBG("step!\n");

    DBGLOG("---------------window destroy %d.\n",wid);
	int theme;
	CoolShmReadLock(g_config_lock);
	theme = g_config->setting.lcd.theme;
	CoolShmReadUnlock(g_config_lock);
	NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP, theme, 1);
	
	if (curr_app)
    	free(curr_app);
    voice_prompt_uninit();
    plextalk_schedule_unlock();
    plextalk_sleep_timer_unlock();
	plextalk_set_hotplug_voice_disable(0);		//enable desktop's hotplug voice
   	plextalk_global_config_close();
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	app_rqst_t *rqst;

//debug
	PC_DBG("step!\n");

//	printf("OnAppMsg, src = %s\n", src);

	if (msg->msg.msg.msgId != PLEXTALK_APP_MSG_ID)
		return;

	rqst = msg->msg.msg.msgTxt;
	switch (rqst->type) {
	case APPRQST_GUIDE_VOL_CHANGE:
		vprompt_cfg.volume = rqst->guide_vol.volume;
		break;
	}
	
	PC_DBG("step!\n");
}

static void
OnMsgBoxKeyDown(WID__ wid, KEY__ key)
{
//	WIDGET *w = (WIDGET *)NeuxGetWidgetFromWID(wid);
//	msgbox_context_t *cntx = NeuxWidgetGetData(w);

	switch (key.ch) {
	case MWKEY_RIGHT:
	case MWKEY_ENTER:
		if (curr_app != NULL && strcmp(curr_app, PLEXTALK_IPC_NAME_DESKTOP) != 0) {
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Canceled the current processing."));
			NeuxStopAllOtherApp(TRUE);
		}
		break;
	case MWKEY_LEFT:
	case MWKEY_POWER:
	case MWKEY_VOLUME_UP:
	case MWKEY_VOLUME_DOWN:
		break;
	default:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	}

	PC_DBG("step!\n");
}

static void
OnMsgBoxHotplug(WID__ wid, int device, int index, int action)
{
	WIDGET *w;
	msgbox_context_t *cntx;

	PC_DBG("step!\n");

	if (device != MWHOTPLUG_DEV_UDC || action != MWHOTPLUG_ACTION_OFFLINE)
		return;

	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_USB_OFF);
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Disconnected from PC."));
	PC_DBG("step!\n");

	w = (WIDGET *)NeuxGetWidgetFromWID(wid);
	PC_DBG("step!\n");

	cntx = NeuxWidgetGetData(w);
	cntx->msgReturn = MSGBX_BTN_CANCEL;
	cntx->endOfModal = 1;

	PC_DBG("step!\n");
}

static void
OnWindwowKeyDown(WID__ wid, KEY__ key)
{

	//debug
	PC_DBG("window key down, do_exit = %d\n", do_exit);

	if (do_exit)
		return;

	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
		}
		return;
	}

	switch (key.ch) {
	case MWKEY_POWER:
	case MWKEY_VOLUME_UP:
	case MWKEY_VOLUME_DOWN:
		break;
	default:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Connected to PC."));
		break;
	}
}

static void
OnWindowHotplug(WID__ wid, int device, int index, int action)
{
		//debug
	printf("onWindowHotplug!!\n");
	PC_DBG("OnwindowHotPlug device = %d, action = %d\n");

	if (device != MWHOTPLUG_DEV_UDC || action != MWHOTPLUG_ACTION_OFFLINE)
		return;

	CoolRunCommand("/bin/udc_massstorage.sh", "stop", NULL);

	//debug
	PC_DBG("do vprompt disconnect from pc!\n");

	wait_tts_end = 1;		//timer start to wait tts end!!
	wait_tts_count = 0;
	
	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_USB_OFF);
	voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Disconnected from PC."));
	do_exit = 1;
}


/* on timer 1s */
static void
OnTimer(WID__ wid)
{
	printf("pc-connector on timer!\n");

	if (wait_tts_end) {
		printf("pc-connector wait tts end!!\n");
		wait_tts_count ++;

		if (wait_tts_count > 10) {	//5s
			printf("pc-connector wait time out!, force stop app!!!\n");
			NeuxAppStop();
		}
	}
}


static void
ShowConnectorForm()
{
	FORM* widget;
	TIMER* timer;	//create timer for force exit when count out!

	PC_DBG("step!\n");
		
	widget = NeuxFormNew(0, 0,
						NeuxScreenWidth(), NeuxScreenHeight());
	NeuxWidgetSetBgPixmap(widget, PLEXTALK_IMAGES_DIR "usb.png");

	NeuxFormSetCallback(widget, CB_KEY_DOWN, OnWindwowKeyDown);
	NeuxFormSetCallback(widget, CB_HOTPLUG, OnWindowHotplug);
	PC_DBG("step!\n");

   	timer = NeuxTimerNew(widget, 1000, -1);		// 1s
	NeuxTimerSetCallback(timer, OnTimer);
	TimerEnable(timer);
	PC_DBG("step!\n");

	NeuxWidgetShow(widget, TRUE);
	NxSetWidgetFocus(widget);
	PC_DBG("step!\n");

	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Connected to PC."));

	//debug
	PC_DBG("before run udc_masstorage.sh\n");
	
	CoolRunCommand("/bin/udc_massstorage.sh", "start", NULL);
}

static void
SignalRead(void *dummy)
{

//debug
	printf("Signal read!!!\n");

	struct signalfd_siginfo fdsi;
	ssize_t ret;

	ret = read(signal_fd, &fdsi, sizeof(fdsi));
	if (ret < 0) {
		WARNLOG("Read signal fd faild.\n");
//		return;
	}

	PC_DBG("Voice prompt %d has terminated.\n", fdsi.ssi_int);

	NeuxAppStop();
}

//ztz	----start--

static FORM* tmp_form = NULL;

static void
CreateTmpForm (void)
{
	widget_prop_t wprop;

	tmp_form = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxWidgetGetProp(tmp_form, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(tmp_form, &wprop);

	NeuxFormShow(tmp_form, TRUE);
}

static void
DestroyTmpForm (void)
{
	NeuxFormDestroy(&tmp_form);
}

//ztz	----end---


static void initApp(void)
{
	/* global config */
	PC_DBG("open config!\n");
	plextalk_global_config_open();

	plextalk_schedule_lock();
	plextalk_sleep_timer_lock();

	PC_DBG("init theme and others!\n");
	/* theme */
	CoolShmReadLock(g_config_lock);
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed  = g_config->setting.guide.speed;
	NeuxThemeInit(g_config->setting.lcd.theme);
	plextalk_update_lang(g_config->setting.lang.lang, PLEXTALK_IPC_NAME_PC_CONNECTOR);
	plextalk_update_sys_font(g_config->setting.lang.lang);
	plextalk_update_tts(g_config->setting.lang.lang);
	CoolShmReadUnlock(g_config_lock);

	PC_DBG("pc-connecotr prompt init!\n");
	voice_prompt_init();

	/* Do not abort VBUS(hotplug) TTS voice & disable desktop_hotplug voice */
	plextalk_set_hotplug_voice_disable(1);
//	voice_prompt_abort();		
//	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_USB_ON);

	PC_DBG("pc-connecotr prompt set Icon!\n");
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
	NhelperTitleBarSetContent(_("PC Connection"));
}

int
main(int argc, char **argv)
{
	int udc_online;
	int ret;

	PC_DBG("pc-connector start!!!\n");


	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	if (argc >= 2)
		curr_app = strdup(argv[1]);

	PC_DBG("pc-connecotr before init app!!\n");
	
	initApp();

	PC_DBG("pc-connecotr after init app!!\n");

	NeuxWMAppSetCallback(CB_DESTROY, onDestroy);
	NeuxWMAppSetCallback(CB_MSG,     onAppMsg);

	PC_DBG("pc-connector reisgter signal read!\n");
	signal_fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(signal_fd, NULL, SignalRead, NULL);
	NxAppSourceActivate(signal_fd, 1, 0);

	PC_DBG("setp!\n");
	sysfs_read_integer(PLEXTALK_UDC_ONLINE, &udc_online);
	if (!udc_online) {
			PC_DBG("setp!\n");

		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_USB_OFF);
		voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Disconnected from PC."));
		do_exit = 1;
			PC_DBG("setp!\n");

	} else {
		PC_DBG("setp!\n");

		CreateTmpForm();
			PC_DBG("setp!\n");

		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, OnMsgBoxHotplug,
				0, _("Connect to PC as a Mass Storage device Mode. Are you Sure?"));
		PC_DBG("setp!\n");

		if (ret == MSGBX_BTN_OK) {
			PC_DBG("setp!\n");

			ShowConnectorForm();
		} else {
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			do_exit = 1;
		}

		PC_DBG("setp!\n");

		DestroyTmpForm();
		PC_DBG("setp!\n");

	}

	// start the framework the last thing
	// events starts to feed in, this API never returns.
	NeuxAppStart();

	return 0;
}
