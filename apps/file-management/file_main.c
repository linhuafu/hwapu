#include <nano-X.h>
#include <limits.h>
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
#include "plextalk-config.h"
#include "plextalk-ui.h"
#include "libvprompt.h"
#include "sysfs-helper.h"
#include "plextalk-keys.h"
#include "file_menu.h"
#include "file_proc.h"
#include "file_dialog.h"
#include "storage.h"

OP_FILE *op_file;

static int signal_fd;
int voice_prompt_running = 0;

struct plextalk_setting setting;
struct voice_prompt_cfg vprompt_cfg;

static int file_exit = 0;
static void onAppMsg(const char * src, neux_msg_t * msg);

void FileManagementExit(void)
{
	file_exit = 1;
	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Cancel."));
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Close the file management mode."));
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);

}

int checkFileExit(void)
{
	if(file_exit){
		voice_prompt_abort();
		NeuxAppStop();
		return 1;
	}
	return 0;
}

static int file_explorer(char *file)
{
	char **selectedfiles;
	int selectedcount;
	int i;
	int ret = -1;

	DBGMSG("file:%s\n",file);

	selectedcount = 0;
	ret = FileDialogOpen(file, FL_TYPE_FILEMANAGEMENT, GR_FALSE, &selectedfiles, &selectedcount,&voice_prompt_running);

//	for (i = 0; i < selectedcount; i++)
//		DBGMSG("%d: %s\n", i, selectedfiles[i]);
	if (selectedcount){
		strcpy(file, selectedfiles[0]);
		DBGMSG("select path:%s\n", file);
		FileDialogFreeSelections(&selectedfiles, selectedcount);
	}
	DBGMSG("ret: %d\n",ret);
	return ret;
}

static void opFile_destroy()
{
	if(NULL!=op_file)
	{
		free(op_file);
		op_file = NULL;
	}
}

static OP_FILE* opFile_init()
{
	op_file = (OP_FILE*)malloc(sizeof(OP_FILE));
	if(NULL==op_file)
	{
		DBGMSG("malloc err");
		return 0;
	}
	memset(op_file, 0, sizeof(OP_FILE));
	op_file->op_mode = OP_NONE;
	return op_file;
}

static void onAppDestroy(WID__ wid)
{
    DBGLOG("-------app destroy %d.\n",wid);
	opFile_destroy();
    voice_prompt_uninit();
	
	NxAppSourceActivate(signal_fd, 0, 0);
	NeuxAppFDSourceUnregister(signal_fd);

	plextalk_schedule_unlock();
	plextalk_sleep_timer_unlock();
//	plextalk_set_auto_off_disable(0);
   	plextalk_global_config_close();
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	if(PLEXTALK_APP_MSG_ID != msg->msg.msg.msgId){
		return;
	}

	app_rqst_t *rqst = (app_rqst_t*)msg->msg.msg.msgTxt;
	switch (rqst->type) {
	case APPRQST_GUIDE_VOL_CHANGE:
		DBGMSG("guide volume\n");
		vprompt_cfg.volume = rqst->guide_vol.volume;
		break;
	default:
		break;
	}
}

#if 0
static void onSWOn(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		DBGMSG("key lock\n");
		keylock = 1;
		break;
	case SW_HP_INSERT:
		DBGMSG("hp in\n");
		break;
	}
}

static void onSWOff(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		DBGMSG("key unlock\n");
		keylock = 0;
		break;
	case SW_HP_INSERT:
		DBGMSG("hp out\n");
		break;
	}
}
#endif

static void signalRead(void *dummy)
{
	struct signalfd_siginfo fdsi;
	ssize_t ret;

	ret = read(signal_fd, &fdsi, sizeof(fdsi));
	if (ret < 0) {
		WARNLOG("Read signal fd faild.\n");
		return;
	}

	voice_prompt_running = 0;
	DBGMSG("Voice prompt %d has terminated.\n", fdsi.ssi_int);
	if(file_exit){
		NeuxAppStop();
	}
}

static void initApp(void)
{
//	int res;

	/* global config */
	plextalk_global_config_open();

	/* theme */
	CoolShmReadLock(g_config_lock);
	memcpy(&setting, &g_config->setting, sizeof(setting));
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed     = g_config->setting.guide.speed;
	//vprompt_cfg.equalizer = g_config->setting.guide.equalizer;
	CoolShmReadUnlock(g_config_lock);

	NeuxThemeInit(setting.lcd.theme);
	plextalk_update_lang(setting.lang.lang, "file-management");

	plextalk_update_sys_font(setting.lang.lang);
	plextalk_update_tts(setting.lang.lang);

	voice_prompt_init();

	voice_prompt_abort();
	doVoicePrompt(NULL, _("File management mode."));

	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
	NhelperTitleBarSetContent(_("File management"));

	NeuxAppSetCallback(CB_DESTROY, onAppDestroy);
	NeuxAppSetCallback(CB_MSG, onAppMsg);
	DBGMSG("onAppMsg:%x\n", onAppMsg);
//	NeuxAppSetCallback(CB_SW_ON,   onSWOn);
//	NeuxAppSetCallback(CB_SW_OFF,  onSWOff);

	plextalk_schedule_lock();
	plextalk_sleep_timer_lock();
//	plextalk_set_auto_off_disable(1);
}

int
main(int argc, char **argv)
{
	char file[PATH_MAX];
	int ret;
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	if(!opFile_init())
	{
		FATAL("unable to create opfile.!\n");
	}
	
	initApp();
	
//	initKeyLock();
	
	signal_fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(signal_fd, NULL, signalRead, NULL);
	NxAppSourceActivate(signal_fd, 1, 0);

	strcpy(file ,PLEXTALK_MOUNT_ROOT_STR);

	{	//´´½¨¿Õ´°Ìå
		FORM* form = NeuxFormNew( MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);
		NeuxWidgetShow(form, TRUE);
		NeuxWidgetRedraw(form);
	}
	
	while(1){
	
		ret = file_explorer(file);

		OP_MODE op_mode = op_select();
		if(OP_COPY == op_mode || OP_CUT == op_mode){//copy cut
			if(-1 == ret){//cur dir is empty
				doVoicePrompt(VPROMPT_AUDIO_BU ,NULL);
				continue;
			}
			
			op_file->op_mode = op_mode;
			strlcpy(op_file->src_path, file, sizeof(op_file->src_path));
			doVoicePrompt(NULL ,_("Select the target directory."));
			continue;
		}
		
		if(OP_DELETE == op_mode){
			if(-1 == ret){//cur dir is empty
				doVoicePrompt(VPROMPT_AUDIO_BU ,NULL);
				continue;
			}
			
			op_file->op_mode = op_mode;
			strlcpy(op_file->src_path, file, sizeof(op_file->src_path));
			file_operate(op_file);
			//update select file
			if(strlen(op_file->dest_path)){
				strlcpy(file, op_file->dest_path, sizeof(file));
			}
			continue;
		}
		
		if(OP_PASTE == op_mode){
			if(OP_COPY != op_file->op_mode && OP_CUT != op_file->op_mode)
			{
				doVoicePrompt(VPROMPT_AUDIO_BU ,_("The file or directory for paste is not selected."));
				continue;
			}
			strlcpy(op_file->dest_path, file, sizeof(op_file->dest_path));
			file_operate(op_file);
			//update select file
			if(strlen(op_file->dest_path)){
				strlcpy(file, op_file->dest_path, sizeof(file));
			}
			continue;
		}
	}
	return 0;
}

