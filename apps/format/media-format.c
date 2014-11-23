/*
 *	Copyright(C) 2006 Neuros Technology International LLC.
 *				 <www.neurostechnology.com>
 *
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, version 2 of the License.
 *
 *
 *	This program is distributed in the hope that, in addition to its
 *	original purpose to support Neuros hardware, it will be useful
 *	otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Filedialog gui routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-08-29 NW
 *
 */

#define OSD_DBG_MSG
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include "nc-err.h"

#include "widgets.h"
#include "desktop-ipc.h"

#include "plextalk-config.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include <microwin/device.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "format_tts.h"
#include "storage.h"
#include "application-ipc.h"
#include "nxutils.h"
#include "format_pthread.h"

#define _(S)			gettext(S)

#define PTITLE_X	0
#define PTITLE_Y	35
#define PTITLE_H	20	

//#define PLEXTALK_SDA_RO 		"/sys/class/block/sda/ro"
#define PLEXTALK_MMC_RO 		"/sys/class/block/mmcblk1/ro"

#define PLEXTALK_DEV_PATH		"/sys/class/block"


#define FORMAT_DISK_SDCARDP1	"/dev/mmcblk1p1"
#define FORMAT_DISK_SDCARD 		"/dev/mmcblk1"

//#define FORMAT_DISK_USBP1		"/dev/sda1"
#define FORMAT_DISK_USB 		"/dev/sd"

#define FORMAT_DIST_INNER   	"/dev/mmcblk0p2"


#define FORMAT_BLOCK_PATH		"/sys/class/block/"
#define FORMAT_BLOCK_SDCARD1	FORMAT_BLOCK_PATH "mmcblk1p1"
//#define FORMAT_BLOCK_USB1 		FORMAT_BLOCK_PATH "sda1"



#define TIMER_PERIOD  500//2000

#define FORMAT_MEDIA_NONE		0
#define FORMAT_MEDIA_INNER		1
#define FORMAT_MEDIA_SDCARD		2
#define FORMAT_MEDIA_USB		3

#define SAVED_FILE_PATH  "/tmp/.plextalk/"

#define FORMAT_MEDIA_REMOVED		1
#define FORMAT_MEDIA_ERROR			0
#define FORMAT_MEDIA_READONLY		2
#define FORMAT_MEDIA_WRITEERROR		3

static FORM *  formMenu;
#define widget formMenu

#define FORMAT_MEDIA_PATH_SIZE		125

struct format_nano {

	LABEL *pTitle;
	TIMER *Timer;
	char curMedia[FORMAT_MEDIA_PATH_SIZE];//the media which will to be format 
	int isInnerMemory;
	int mediaType;
	int isFormatStart;

	int* is_running;
	int isSettingExist;
	int firstIn;
};

static struct format_nano g_format;
static int g_tts_isrunning;
static TIMER *key_timer;
static int long_pressed_key = -1;

static void
format_media_removed(int isremoved);
static int check_media_exist (const char* mediapath);
static void
format_success_exit();
static void
format_onHotplug(WID__ wid, int device, int index, int action);



#if 0
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

static int format_isMediaReadOnly(const char *path)
{
	char *p;
	char mediaPath[100];
	if(!path)
		return 0;

	DBGMSG("format_isMediaReadOnly path:%s\n",path);
	memset(mediaPath,0x00,100);

	p = strstr(path,"/mmc");
	if(p)
	{
		snprintf(mediaPath,100,"%s%s",PLEXTALK_MOUNT_ROOT_STR,p);
	}
	else
	{
		snprintf(mediaPath,100,"%s%s",PLEXTALK_MOUNT_ROOT_STR,path+4);// "/dev"
	}
	DBGMSG("format_isMediaReadOnly media:%s\n",mediaPath);
	
	int ret = CoolStorageEnumerate(storage_enum ,mediaPath);
	if(1 == ret){
		return readonly;
	}else{
		return 0;
	}
}
#else
static int format_isMediaReadOnly(const char *path)
{
	int ret = 0;
	char tpath[256];
	char *pmedia;

	if(!path)
		return 0;
	DBGMSG("format_isMediaReadOnly path:%s\n",path);

	if(StringStartWith(path,"/dev/mmc")) {
		sysfs_read_integer(PLEXTALK_MMC_RO, &ret);
	} else if(StringStartWith(path,"/dev/sd")) {
		memset(tpath,0x00,256);

		pmedia = path+strlen(path)-1;
		
		while(*pmedia && *pmedia!='/')
		{
			pmedia--;
		}
		DBGMSG("format_isMediaReadOnly2 :%s\n",pmedia);

		snprintf(tpath, 256, "%s%s%s", PLEXTALK_DEV_PATH, pmedia,"/ro");
		sysfs_read_integer(tpath, &ret);
	}
	DBGMSG("media is readonly:%d\n",ret);
	return ret;
}
#endif

//判断媒体类型
//FORMAT_MEDIA_SDCARD
//FORMAT_MEDIA_MMCBLK
//FORMAT_MEDIA_INNER
static int format_check_mediatype(char *media)
{
	if(!media){
		DBGMSG("Ouch,the parameter is null\n");
		return FORMAT_MEDIA_NONE;
	}
	
	if(!strncmp(media,FORMAT_DISK_SDCARD,strlen(FORMAT_DISK_SDCARD))) {
		DBGMSG("lucky,it is the SD Card!\n");
		return FORMAT_MEDIA_SDCARD;
	} else if(!strncmp(media,FORMAT_DISK_USB,strlen(FORMAT_DISK_USB))) {
		DBGMSG("lucky,it is the USB!\n");
		return FORMAT_MEDIA_USB;
	} else if(!strncmp(media,FORMAT_DIST_INNER,strlen(FORMAT_DIST_INNER))) {
		DBGMSG("lucky,it is the Internal Memory!\n");
		return FORMAT_MEDIA_INNER;
	}
	return FORMAT_MEDIA_NONE;
}

#if 0
//调整mmcblk1的路径，如果SD卡为有分区的卡那么调整为/dev/mmcblk1p1
//返回值 : 1 有修改；0:没修改
static int format_adjust_mmcblk(char *media,int size)
{
	struct stat info;

	if(!media){
		DBGMSG("Ouch,the parameter is null\n");
		return 0;
	}

	DBGMSG("the media before:%s\n",media);
	if(!stat(FORMAT_BLOCK_SDCARD1, &info)) {
		memset(media,0x00,size);
		strcpy(media,FORMAT_DISK_SDCARDP1);
		return 1;
	}
	return 0;
}
//调整USB的路径，如果USB为有分区的卡那么调整为/dev/sdx1
//返回值 : 1 有修改；0:没修改
static int format_adjust_sda(char *media,int size)
{
	struct stat info;

	if(!media){
		DBGMSG("Ouch,the parameter is null\n");
		return 0;
	}

	DBGMSG("the media before:%s\n",media);
	if(!stat(FORMAT_BLOCK_USB1, &info)) {
		memset(media,0x00,size);
		strcpy(media,FORMAT_DISK_USBP1);
		return 1;
	}
	return 0;
}
#endif

static void format_start_process()
{
#if 1
		g_format.isFormatStart = 0;
		if(!check_media_exist(g_format.curMedia))
		{
			DBGMSG("the media is removed \n");
			format_media_removed(FORMAT_MEDIA_REMOVED);
		}
		else if(NeuxStopAllOtherApp(TRUE))
		{
			DBGMSG("the NeuxStopAllOtherApp error \n");
			format_media_removed(FORMAT_MEDIA_WRITEERROR);
		}
		else if((!g_format.isInnerMemory) && format_isMediaReadOnly(g_format.curMedia))
		{
			DBGMSG("the media:%s is read only\n",g_format.curMedia);
			format_media_removed(FORMAT_MEDIA_READONLY);
			
		}
		else
		{
			start_format(g_format.curMedia);
			g_format.isFormatStart = 1;
			//format_success_exit();
		}
#endif	

}



static void
format_save_setting(struct format_nano* nano)
{
#if USE_UDISK_PLEXTAL_SETTING

	if(!nano->isInnerMemory)
		return;
	DBGMSG("format_save_setting!");
	
	DIR *dir = opendir(PLEXTALK_SETTING_DIR);
	if(dir == NULL)
	{
		DBGMSG("system set file not exist!!\n");
		nano->isSettingExist = 0;
	}
	else
	{
		closedir(dir);
		DBGMSG("system set file exist\n"); 
		{
			char cmd[1024];
			snprintf(cmd,1024, "cp -a %s %s", PLEXTALK_SETTING_DIR,"/tmp/");
			DBGMSG("cmd = %s\n", cmd);
			system(cmd);
			nano->isSettingExist = 1;
		}	
	}
#endif	
}

void
format_save_setting_outer()
{
	format_save_setting(&g_format);
}


static void
format_restore_setting(int isInnerMemory,int isSettingExist)
{
#if USE_UDISK_PLEXTAL_SETTING

	if(isInnerMemory && isSettingExist)
	{
		DBGMSG("format_restore_setting!");
#if 0		
		if (mkdir(PLEXTALK_SETTING_DIR, 0755) == -1) 
		{
			DBGMSG("create setting folder failure!");
			return ;			
		}
#endif		
		char cmd[1024];
		snprintf(cmd,1024, "mv %s %s", "/tmp/.plextalk","/media/mmcblk0p2/");
		DBGMSG("cmd = %s\n", cmd);
		system(cmd);
	}
#endif	
}
void
format_restore_setting_outer()
{
	format_restore_setting(g_format.isInnerMemory,g_format.isSettingExist);
}

static void
format_initApp(struct format_nano* nano)
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
	plextalk_update_lang(g_config->setting.lang.lang, "format");
	//CoolShmReadUnlock(g_config_lock);

	plextalk_update_sys_font(getenv("LANG"));

	g_tts_isrunning = 0;
	nano->is_running = &g_tts_isrunning;
	format_tts_init(nano->is_running);
	format_tts(TTS_ENTER_FORMAT);
	nano->firstIn = 1;

	//format_prepare();

	nano->isSettingExist = 0;
	nano->isFormatStart = 0;

	nano->mediaType = FORMAT_MEDIA_NONE;
	memset(nano->curMedia,0x00,FORMAT_MEDIA_PATH_SIZE);

}

static void
format_CreateLabels (FORM *widget,struct format_nano* nano)	
{
	widget_prop_t wprop;
	label_prop_t lprop;

	int h = sys_font_size+6;
	int y = (MAINWIN_HEIGHT - h)/2;
	
	nano->pTitle= NeuxLabelNew(widget,
						0, y, MAINWIN_WIDTH, h,_("Please wait"));
	
	NeuxWidgetGetProp(nano->pTitle, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->pTitle, &wprop);
	
	NeuxLabelGetProp(nano->pTitle, &lprop);
	lprop.align = LA_CENTER;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->pTitle, &lprop);
	NeuxWidgetSetFont(nano->pTitle, sys_font_name, sys_font_size);
	NeuxWidgetShow(nano->pTitle, TRUE);

}

static void format_OnGetFocus(WID__ wid)
{
	DBGLOG("On Window Get Focus\n");
	NhelperTitleBarSetContent(_("Media format"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);

}

static void
format_OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();

	plextalk_global_config_close();
}
static void
format_OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);

	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		DBGLOG("help is running\n");
		return;
	}
	if(NeuxAppGetKeyLock(0))
	{
		DBGMSG("the key is locked!!!\n");
		format_tts(TTS_FORMAT_LOCK);
		return;
	}

	if(format_get_tts_end_flag() == TTS_END_STATE_EXIT || format_get_tts_end_flag() == TTS_END_STATE_SUCCESS)
	{
		DBGMSG("I am in exit state,OK,if you are in a hurry,I will close format now\n");
		//wait_format_finish();
		format_set_tts_end_flag(TTS_END_STATE_NONE);
		NxAppStop();
		return;
	}

	if(g_format.firstIn) {
		g_format.firstIn = 0;
		voice_prompt_abort();
		DBGMSG("I got here the first time,so I will start format happy\n");
		format_start_process();
		if(!g_format.isFormatStart)
			return;
	}

	
	switch (key.ch) {
	case MWKEY_UP:
	case MWKEY_DOWN:
	case MWKEY_LEFT:
	case MWKEY_RIGHT:
	case MWKEY_ENTER:
	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		format_tts(TTS_FORMAT_WAIT_BEEP);

		break;
		
	case MWKEY_MENU:
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;
		
	case MWKEY_INFO:
		format_tts(TTS_FORMAT_WAIT);
		break;
	}

	
	
}
static void format_OnWindowKeyup(WID__ wid, KEY__ key)
{
	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		return;
	}

	if (long_pressed_key != key.ch)
		return;

	long_pressed_key = -1;
	if(key_timer)
		NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch)
	{	
	case MWKEY_MENU:
		format_tts(TTS_FORMAT_MENU_WAIT);
		break;
	default:
		break;
	}
}


static void 
format_OnKeyTtimer()
{
	static int count = 0;
	
	if(g_format.firstIn) {

		if(!(*g_format.is_running)) {
			g_format.firstIn = 0;
			DBGMSG("I got here the first time,so I will start format happy\n");
			format_start_process();
		}

		return;
	}

	if(g_format.isFormatStart) {
		if(format_pthread_exist()) {
		DBGMSG("Be careful,the Format Thread is still running\n");

		if((*g_format.is_running)){
			count = 0;
		}else{
			count ++;
			if( 0 == count%3){
				format_tts(TTS_FORMAT_WAIT_BEEP_NO_ABORT);
			}
		}	

		} else if(format_get_tts_end_flag() != TTS_END_STATE_SUCCESS){
			DBGMSG("OK,all conditon is already.Let's sing the success song\n");
			NeuxTimerSetEnabled(g_format.Timer, GR_FALSE);
			format_success_exit();
			
		}
	}

}

static void
format_CreateTimer(FORM *widget,struct format_nano* nano)
{
	nano->Timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->Timer, format_OnKeyTtimer);
	DBGLOG("format_CreateTimer nano->Timer:%d",nano->Timer);

}

static void OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}


/* Function creates the main form of the application. */
void
format_CreateFormMain (struct format_nano* nano)
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, format_OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 format_OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  format_OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_FOCUS_IN, format_OnGetFocus);
	NeuxFormSetCallback(widget, CB_HOTPLUG,  format_onHotplug);
	
	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(widget, &wprop);

	NeuxWidgetSetFont(widget, sys_font_name, sys_font_size);

	//LABEL用于显示字符串
	format_CreateLabels(widget,nano);
	//创建定时器用于计时和显示
	format_CreateTimer(widget,nano);
	key_timer = NeuxTimerNewWithCallback(widget, 0, 0, OnKeyTimer);

	//NhelperTitleBarSetContent(_("Input Number"));

	NeuxFormShow(widget, TRUE);
	NeuxWidgetFocus(widget);
	NeuxWidgetShow(nano->pTitle, TRUE);

	

}


//parameter: 1:the media is removed 
//                0:stop the app fail
//                2:the media is readonly
static void
format_media_removed(int type)
{			
	//prompt user the error 	
	if(type == FORMAT_MEDIA_REMOVED)
	{
		stop_format();
		format_tts(TTS_REMOVE_ERROR);
	}
	else if(type == FORMAT_MEDIA_READONLY)
	{
		stop_format();
		format_tts(TTS_READONLY);
	}
	else if(type == FORMAT_MEDIA_WRITEERROR)
	{
		stop_format();
		format_tts(TTS_WRITEERROR);
	}
	else
	{
		format_tts(TTS_ERROR);	
	}
	//NeuxLabelSetText(g_format.pTitle, _("Format error!"));
	//NeuxWidgetShow(g_format.pTitle, TRUE);
	//format_CreateEndTimer(widget,&g_format);

}

static void
format_success_exit()
{				
	format_tts(TTS_SUCCESS);
	//NeuxLabelSetText(g_format.pTitle, _("Format success"));
	//NeuxWidgetShow(g_format.pTitle, TRUE);
	//format_CreateEndTimer(widget,&g_format);

}



static void
format_onHotplug(WID__ wid, int device, int index, int action)
{
	DBGLOG("onHotplug device: %d ,index=%d,action=%d\n", device,index,action);

	if ((device == MWHOTPLUG_DEV_MMCBLK) 
	&& (action == MWHOTPLUG_ACTION_REMOVE)
	&& !g_format.isInnerMemory)
	{
		if(g_format.mediaType == FORMAT_MEDIA_SDCARD)
		{
			format_media_removed(1);
		}
	}
	else if ((device == MWHOTPLUG_DEV_SCSI_DISK) 
	&& (action == MWHOTPLUG_ACTION_REMOVE)
	&& !g_format.isInnerMemory)
	{
		if(g_format.mediaType == FORMAT_MEDIA_USB)
		{
			format_media_removed(1);
		}
	}
}

/* it will return 1 if media exist, or it will return 0 */
static int check_media_exist (const char* mediapath)
{
	struct stat64 st;	
	char path[256];

	if(!mediapath)
		return 0;

	char *pmedia = mediapath+strlen(mediapath)-1;
	
	while(*pmedia && *pmedia!='/')
	{
		pmedia--;
	}
	DBGMSG("check_media_exist3 :%s\n",pmedia);
	
	memset(path,0x00,256);
	
	snprintf(path, 256, "%s%s", PLEXTALK_DEV_PATH, pmedia);
	
	return !stat64(path, &st);
}

static void format_onAppMsg(const char * src, neux_msg_t * msg)
{
	DBGLOG("app msg %d .\n", msg->msg.msg.msgId);
	
	switch (msg->msg.msg.msgId)
	{	
		case PLEXTALK_APP_MSG_ID:
		{
			app_rqst_t *rqst = msg->msg.msg.msgTxt;
			DBGLOG("onAppMsg PLEXTALK_APP_MSG_ID : %d\n",rqst->type);
			switch (rqst->type)
			{
				case APPRQST_LOW_POWER://低电要关机了
					DBGMSG("Yes,I have received the lowpower message,but I will ingnore it\n");
				break;
			}
		}
		break;
			
	}
}

int
main(int argc,char **argv)
{
	DBGMSG("In media-format mode!\n");
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}
	
	//signal(SIGVPEND, cal_tip_voice_sig);


	format_initApp(&g_format);
	DBGMSG("application initialized\n");

	if(argc > 1)
	{
		//g_format.curMedia = argv[1];
		strcpy(g_format.curMedia,argv[1]);
		
		if(strlen(g_format.curMedia) <=0 )
		{
			return -1;
		}
		else
		{
			g_format.isInnerMemory = 0;
			
			if(!strcmp(g_format.curMedia,FORMAT_DIST_INNER))
			{
				g_format.isInnerMemory = 1;
			}
		}
	}
	else//no media
	{
		return -1;
	}
	DBGMSG("media:%s\n",g_format.curMedia);
	g_format.mediaType = format_check_mediatype(g_format.curMedia);

#if 0	//format_menu_init已经做了
	if(g_format.mediaType == FORMAT_MEDIA_SDCARD) {
		format_adjust_mmcblk(g_format.curMedia,FORMAT_MEDIA_PATH_SIZE);
	} else if(g_format.mediaType == FORMAT_MEDIA_USB) {
		format_adjust_sda(g_format.curMedia,FORMAT_MEDIA_PATH_SIZE);
	}
#endif
	
	DBGMSG("the media:%s\n",g_format.curMedia);
	DBGMSG("the isInnerMemory :%d\n",g_format.isInnerMemory);
	//界面禁止应用切换
	plextalk_schedule_lock();
	//界面只关屏，不休眠
	plextalk_suspend_lock();
	plextalk_sleep_timer_lock();
		
	//NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_CALC);


	format_CreateFormMain(&g_format);

	NeuxAppSetCallback(CB_MSG,	 format_onAppMsg);
	//NeuxWMAppSetCallback(CB_HOTPLUG, format_onHotplug);
	//NeuxAppSetCallback(CB_SW_ON,	Rec_onSWOn);
	//NeuxAppSetCallback(CB_SW_OFF, 	Rec_onSWOff);

	//this function must be run after create main form ,it need set schedule time
	DBGMSG("mainform shown\n");

	// start application, events starts to feed in.
	NeuxAppStart();

	return 0;
}



