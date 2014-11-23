#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#include <gst/gst.h>
#include "book_prompt.h"
#include "tchar.h"
#include "widgets.h"
//#include "PlexDaisyAnalyzeAPI.h"
#include "application-ipc.h"
//#include "Nx-application.h"
#include "desktop-ipc.h"
#include "application.h"
#include "Plextalk-config.h"
#include "Plextalk-statusbar.h"
#include "Plextalk-i18n.h"
#include "Plextalk-keys.h"
#include "Plextalk-theme.h"
#include "Plextalk-titlebar.h"
#include "Plextalk-ui.h"
#include "File-helper.h"
#include "neux.h"
#include "Key-value-pair.h"
#include "daisy_main.h"
#include "libvprompt.h"
#include "text_main.h"
#include "book.h"
#include "storage.h"
#include "Sysfs-helper.h"

#include "dbmalloc.h"
//static int keylock = 0;
static int signal_fd;
static int isAudio = 0;

//int last_vol = -1;

struct TtsObj* booktts = NULL;

int tip_is_running;

static MsgFunc BookMsgFunc;

#define 	BOOK_INI      		PLEXTALK_SETTING_DIR"book.ini"
#define		SEC_HISTORY			"history"

#define		BOOK_BACKUP		 	"/tmp/.book"
#define		SEC_BACKUP			"backup"

static int readonly;
static int storage_enum(struct mntent *mount_entry, void* user_data)
{
	DBGMSG("mnt_dir=%s\n",mount_entry->mnt_dir);
	char *pStart = strstr(mount_entry->mnt_dir ,PLEXTALK_MOUNT_ROOT_STR);
	if(NULL != pStart){
		DBGMSG("pStart=%s\n",pStart);
		if(!strncmp(pStart, user_data, strlen(pStart))){
			readonly = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
			return 1;
		}
	}
	return 0;
}

int isMediaReadOnly(const char *path)
{
	int ret = CoolStorageEnumerate(storage_enum ,path);
	if(1 == ret){
		return readonly;
	}else{
		return 0;
	}
}

int book_isTtsPlaying(void)
{
	return tip_is_running;
}

#if 0
int book_isKeyLock(void)
{
	return keylock;
}
void book_initKeyLock(void)
{
	char buf[16];
	int res;
	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0")){
		keylock = 1;
	}else{
		keylock = 0;
	}
	DBGMSG("keylock:%d\n",keylock);
}
#endif

void book_setAudioMode(int audio)
{ 
	isAudio = audio;
	NhelperBookContent(audio);
//	last_vol = book_getCurVolumn();
}

int book_getCurVolumn(void)
{
	int hp_online;
	int volume;
	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);
	DBGMSG("hp_online:%d isAudio:%d\n",hp_online, isAudio);
	CoolShmReadLock(g_config_lock);
	if(isAudio){
		volume = g_config->volume.book_audio[hp_online];
	}else{
		volume = g_config->volume.book_text[hp_online];
	}
	CoolShmReadUnlock(g_config_lock);
	DBGMSG("volume:%d\n",volume);
	return volume;
}

//for backup and menu bookmark used
int book_set_backup_path(char *path)
{
	CoolSetCharValue(BOOK_BACKUP,SEC_BACKUP,"backup_path",path);
	return 0;
}

//for backup and menu bookmark used
int book_set_bookmark(int current, int count)
{
	//bookmark count
	char str[20];
	snprintf(str, 20, "%d", count);
	CoolSetCharValue(BOOK_BACKUP,"bookmark","count",str);
	//bookmark current
	snprintf(str, 20, "%d", current+1);
	CoolSetCharValue(BOOK_BACKUP,"bookmark","current",str);
	return 0;
}

//for jump page info used
int book_set_pageinfo(unsigned long ulCurPage, unsigned long ulMaxPage, unsigned long ulFrPage, unsigned long ulSpPage)
{
	char str[20];
	snprintf(str, 20, "%lu", ulCurPage);
	CoolSetCharValue(BOOK_BACKUP,"page","curpage",str);

	snprintf(str, 20, "%lu", ulMaxPage);
	CoolSetCharValue(BOOK_BACKUP,"page","maxpage",str);
	
	snprintf(str, 20, "%lu", ulFrPage);
	CoolSetCharValue(BOOK_BACKUP,"page","frpage",str);

	snprintf(str, 20, "%lu", ulSpPage);
	CoolSetCharValue(BOOK_BACKUP,"page","sppage",str);
	return 0;
}

//for jump page info used
int book_del_pageinfo()
{
	CoolDeleteSectionKey(BOOK_BACKUP,"page","curpage");
	CoolDeleteSectionKey(BOOK_BACKUP,"page","maxpage");
	CoolDeleteSectionKey(BOOK_BACKUP,"page","frpage");
	CoolDeleteSectionKey(BOOK_BACKUP,"page","sppage");
	return 0;
}

//for backup and menu bookmark used
int book_del_backup(void)
{
	CoolDeleteSectionKey(BOOK_BACKUP, SEC_BACKUP, "backup_path");
	CoolDeleteSectionKey(BOOK_BACKUP, "bookmark", "count");
	CoolDeleteSectionKey(BOOK_BACKUP, "bookmark", "current");
	book_del_pageinfo();
	return 0;
}

int book_set_last_path(char *path)
{
	CoolSetCharValue(BOOK_INI,SEC_HISTORY,"resume_path",path);
	return 0;
}

int book_get_last_path(char *path)
{
	CoolGetCharValue(BOOK_INI,SEC_HISTORY,"resume_path",path, PATH_MAX);
	
	if(!PlextalkIsFileExist(path)){
		info_debug("not exist=%s\n", path);
		return -1;
	}

	if(PlextalkIsBookFile(path) || PlextalkIsDsiayBook(path))
	{
		return 0;
	}
	return -1;
}
static void book_onAppMsg(const char * src, neux_msg_t * msg);

static int book_select_new_file(char *file)
{
	char **selectedfiles;
	int selectedcount;
	int i;
	int ret = -1;
	void *pTest = NULL;
	DBGMSG("select book\n");
	pTest = malloc(128);
	DBGMSG("malloc ok\n");
	selectedcount = 0;
	//if file is exist, go to last file, else goto root

	plextalk_suspend_unlock();
	NhelperFilelistMode(1);
	if(strlen(file))
	{
		NeuxFileDialogOpen(file, FL_TYPE_BOOK, GR_FALSE, &selectedfiles, &selectedcount,book_onAppMsg,&tip_is_running);
	}
	else
	{
		NeuxFileDialogOpen(PLEXTALK_MOUNT_ROOT_STR, FL_TYPE_BOOK, GR_FALSE, &selectedfiles, &selectedcount,book_onAppMsg,&tip_is_running);
	}
	NhelperFilelistMode(0);
	plextalk_suspend_lock();

	for (i = 0; i < selectedcount; i++){
		DBGMSG("%s (%d): %s\n", __func__, i, selectedfiles[i]);
	}

	ret = -1;
	if (selectedcount)
	{
		DBGMSG("had select file\n");
		ret = 0;
		strcpy(file, selectedfiles[0]);
		NeuxFileDialogFreeSelections(&selectedfiles, selectedcount);
	}
	DBGMSG("free now\n");
	if(pTest){
		free(pTest);
	}
	DBGMSG("select ret\n");
	return ret;
}


static void book_onAppMsg(const char * src, neux_msg_t * msg)
{
	int msgId = msg->msg.msg.msgId;
	if(PLEXTALK_APP_MSG_ID != msgId)
		return;

	app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;
	switch (app->type)
	{
	case APPRQST_GUIDE_VOL_CHANGE:
		{	
			app_volume_rqst_t* rqst = (app_volume_rqst_t*)app;
			vprompt_cfg.volume = rqst->volume;
		}
		break;

	case APPRQST_GUIDE_SPEED_CHANGE:
		{
			app_speed_rqst_t* rqst = (app_speed_rqst_t*)app;
			vprompt_cfg.speed = rqst->speed;
		}
		break;
//		case APPRQST_GUIDE_EQU_CHANGE:
//			{
//				app_equ_rqst_t* rqst = (app_equ_rqst_t*)app;
//				vprompt_cfg.equalizer = rqst->equ;
//			}
//			break;
		//tts lang, tts role
	case APPRQST_SET_TTS_VOICE_SPECIES:
	case APPRQST_SET_TTS_CHINESE_STANDARD:
		{
			plextalk_update_tts(getenv("LANG"));
		}
		break;

	case APPRQST_SET_FONTSIZE:
		{
			//CoolShmReadLock(g_config_lock);
			//plextalk_update_sys_font(g_config->setting.lang.lang);
			//CoolShmReadUnlock(g_config_lock);
			plextalk_update_sys_font(getenv("LANG"));
		}
		break;
		
	case APPRQST_SET_THEME:
		{	
			//CoolShmReadLock(g_config_lock); 
			//NeuxThemeInit(g_config->setting.lcd.theme);
			//CoolShmReadUnlock(g_config_lock);
			NeuxThemeInit(app->theme.index);
		}
		break;
		
	case APPRQST_SET_LANGUAGE:
		{
			//CoolShmReadLock(g_config_lock);
			//plextalk_update_lang(g_config->setting.lang.lang, "book");
			//CoolShmReadUnlock(g_config_lock);
			plextalk_update_lang(app->language.lang, "book");
			plextalk_update_sys_font(app->language.lang);

			plextalk_update_tts(getenv("LANG"));
			//plextalk_update_sys_font(getenv("LANG"));
		}
		break;

	case APPRQST_RESYNC_SETTINGS:
		{
			CoolShmReadLock(g_config_lock);
			
			NeuxThemeInit(g_config->setting.lcd.theme);
			plextalk_update_lang(g_config->setting.lang.lang, "book");
			plextalk_update_sys_font(g_config->setting.lang.lang);
			plextalk_update_tts(g_config->setting.lang.lang);

			CoolShmReadUnlock(g_config_lock);
		}
		break;
	}

	if (BookMsgFunc != NULL)
		BookMsgFunc(src, msg);
	
}


static void book_OnAppDestroy()
{
	info_debug("BookOnAppDestroy destroy!\n");
	tts_obj_destroy(booktts);
	plextalk_global_config_close();
	book_prompt_destroy();
	NeuxAppFDSourceUnregister(signal_fd);
}

#if 0
static void book_onSWOn(int sw)
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

static void book_onSWOff(int sw)
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

	tip_is_running = 0;

	DBGMSG("Voice prompt %d has terminated.\n", fdsi.ssi_int);
}


static int VoicePromptDoneOrKeyDown(GR_EVENT *ep)
{
	return ep->type == GR_EVENT_TYPE_FD_ACTIVITY ||
	       ep->type == GR_EVENT_TYPE_KEY_DOWN ||
	       ep->type == GR_EVENT_TYPE_SW_ON ||
	       ep->type == GR_EVENT_TYPE_SW_OFF;
}



int main(int argc, char **argv)
{
	char file[PATH_MAX];
	int firstIn;
	BookMsgFunc = NULL;

	if (NeuxAppCreate(argv[0]))
		FATAL("unable to create application.!\n");

	initDebugMalloc();

	plextalk_global_config_open();
	plextalk_update_sys_font(getenv("LANG"));	
	plextalk_update_tts(getenv("LANG"));

	CoolShmReadLock(g_config_lock);	

	NeuxThemeInit(g_config->setting.lcd.theme);
	plextalk_update_lang(g_config->setting.lang.lang, "book");

	CoolShmReadUnlock(g_config_lock);

	/* Init tts */
	tip_is_running = 0;
	book_prompt_init(&tip_is_running);
	signal_fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(signal_fd, NULL, signalRead, NULL);
	NxAppSourceActivate(signal_fd, 1, 0);

	NeuxAppSetCallback(CB_MSG, 	   book_onAppMsg);
	NeuxAppSetCallback(CB_DESTROY, book_OnAppDestroy);
	//NeuxAppSetCallback(CB_SW_ON,   book_onSWOn);
	//NeuxAppSetCallback(CB_SW_OFF,  book_onSWOff);

	PlextalkCheckAndCreateDirectory(PLEXTALK_SETTING_DIR);

	booktts = tts_obj_create();

	firstIn = 1;
	
	//get the last path
	memset(file, 0, sizeof(file));
	if(book_get_last_path(file)<0)
	{	//last path is no exit, select new
	
		book_prompt_play(BOOK_MODE_OPEN, NULL);
		NeuxWaitEvent(VoicePromptDoneOrKeyDown);
		firstIn = 0;
		
		memset(file, 0, sizeof(file));
		book_select_new_file(file);
	}

	book_setAudioMode(0);

	while(1)
	{
		if(PlextalkIsBookFile(file))
		{
			checkMallocStart();
			text_main(file, firstIn, &BookMsgFunc);
			checkMallocEnd();
		}
		else
		{
			checkMallocStart();
			daisy_main(file, firstIn, &BookMsgFunc);
			checkMallocEnd();
		}
		firstIn = 0;
		//select new file
		book_select_new_file(file);
	}
	return 0;
}

void DesktopScreenSaver(GR_BOOL activate)
{

}


