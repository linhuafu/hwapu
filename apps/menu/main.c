#include <nano-X.h>

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
#include "nw-menu.h"
#include "plextalk-keys.h"

char* curr_app;
struct plextalk_setting setting;
struct voice_prompt_cfg vprompt_cfg;

static int signal_fd;
int voice_prompt_end;
int info_tts_running;//km

extern int menu_exit;
extern int wait_focus_out;
extern int focus_out;

void MenuVoicePromptHook(void);

static void onDestroy(WID__ wid)
{
    DBGLOG("---------------window destroy %d.\n",wid);
    NeuxAppFDSourceUnregister(signal_fd);
    if (curr_app)
    	free(curr_app);
    voice_prompt_uninit();
    plextalk_schedule_unlock();
    plextalk_sleep_timer_unlock();

	CoolShmWriteLock(g_config_lock);
	g_config->menu_is_running = 0;
	DBGMSG("set g_config->menu_is_running to 0!\n");
	CoolShmWriteUnlock(g_config_lock);

   	plextalk_global_config_close();
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	app_rqst_t *rqst;

	if (msg->msg.msg.msgId != PLEXTALK_APP_MSG_ID)
		return;

	rqst = msg->msg.msg.msgTxt;
	switch (rqst->type) {
	case APPRQST_GUIDE_VOL_CHANGE:
		vprompt_cfg.volume = rqst->guide_vol.volume;
		break;
//lhf
	case APPRQST_BOOKMARK_RESULT:
		bookmark_result(rqst);
		break;
	case APPRQST_DELETE_RESULT:
		delete_result(rqst);
		break;
	default:
		break;
//lhf
	}
}

static void
signalRead(void *dummy)
{
	struct signalfd_siginfo fdsi;
	ssize_t ret;

	ret = read(signal_fd, &fdsi, sizeof(fdsi));
	if (ret < 0) {
		WARNLOG("Read signal fd faild.\n");
		return;
	}

	DBGMSG("Voice prompt %d has terminated.\n", fdsi.ssi_int);

	voice_prompt_end = 1;
	info_tts_running = 0;//km

	MenuVoicePromptHook();
}

static void initApp(void)
{
	int res;

	/* global config */
	plextalk_global_config_open();

	plextalk_schedule_lock();
	plextalk_sleep_timer_lock();

	/* theme */
	CoolShmReadLock(g_config_lock);
	memcpy(&setting, &g_config->setting, sizeof(setting));
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed  = g_config->setting.guide.speed;
	CoolShmReadUnlock(g_config_lock);

	NeuxThemeInit(setting.lcd.theme);
	plextalk_update_lang(setting.lang.lang, PLEXTALK_IPC_NAME_MENU);

	plextalk_update_sys_font(setting.lang.lang);
	plextalk_update_tts(setting.lang.lang);

	voice_prompt_init();

	voice_prompt_abort();
	//voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);	// deleted by ztz
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
						 _("Main Menu"));
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
						 _("Use Up or Down key to select item. Press PlayStop, Right key to confirm or Left key to cancel."));

	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
	NhelperTitleBarSetContent(_("Main Menu"));
}

int
main(int argc, char **argv)
{
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	NeuxWMAppSetCallback(CB_DESTROY, onDestroy);
	NeuxWMAppSetCallback(CB_MSG,     onAppMsg);

	if (argc >= 2)
		curr_app = strdup(argv[1]);
	else
		curr_app = strdup("");
	initApp();

	signal_fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(signal_fd, NULL, signalRead, NULL);
	NxAppSourceActivate(signal_fd, 1, 0);

	ShowFormMenu();

	// start the framework the last thing
	// events starts to feed in, this API never returns.
	NeuxAppStart();

	return 0;
}
