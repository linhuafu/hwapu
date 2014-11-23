#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <nano-X.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
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
#include "nw-menu.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "nxutils.h"
#include "key-value-pair.h"
#include "mulitlist.h"
#include "xml-helper.h"
#include "file-helper.h"
#include "device.h"
#include "nw-menu.h"
#include "libvprompt.h"

#define DISK_INNER			PLEXTALK_MOUNT_ROOT_STR"/mmcblk0p2"
#define DISK_SDCRD			PLEXTALK_MOUNT_ROOT_STR"/mmcblk1"
#define DISK_USB			PLEXTALK_MOUNT_ROOT_STR"/sd"
#define DISK_CDROM			PLEXTALK_MOUNT_ROOT_STR"/sr"
#define DISK_CDDA			PLEXTALK_MOUNT_ROOT_STR"/cdda"

#define TIMER_PERIOD       500


struct BackUpInfo{
	FORM* form;
	TIMER *timer;
	int timerEnable;
	char *tts_str;
	
	struct MultilineList *list;
	int   ok;
	int target_media;
	int mode;
	char source[PATH_MAX];
	char dest[PATH_MAX];
};


extern int voice_prompt_end;

struct BackUpInfo backupinfo;
static int backup_endOfModal = -1;

static int mediaEnum(struct mntent *mount_entry, void* user_data)
{
	char **ptr = user_data;

	if (StringStartWith(mount_entry->mnt_fsname, *ptr)) {
		*ptr = strdup(mount_entry->mnt_dir);
		return 1;
	}

	return 0;
}

static int getDeviceMountDir(char *device, char *mntdir, int len)
{
	char *p;
	int ret;
	ret = CoolStorageEnumerate(mediaEnum, &device);
	DBGMSG("ret:%d\n",ret);
	if (ret == 1) {
		DBGMSG("device:%s\n",device);
		p = strstr(device, PLEXTALK_MOUNT_ROOT_STR);
		if(p){
			strlcpy(mntdir,p,len);
		}else{
			strlcpy(mntdir,device,len);
		}
		DBGMSG("mntdir:%s\n",mntdir);
		free(device);
		return 1;
	}
	return 0;
}

static int getSdcardMountDir(char *mntdir, int len)
{
	return getDeviceMountDir("/dev/mmcblk1", mntdir, len);
}

static int getUsbMountDir(char *mntdir, int len)
{
	return getDeviceMountDir("/dev/sd", mntdir, len);
}

static char * get_date_folder(char *str, int len)
{
	time_t resultp;
	struct tm *tp;
	time(&resultp);
	tp = localtime (&resultp);
	strftime(str, len, "%Y%m%d%H%M%S", tp);
	return str;
}

int backup_media_menu_init(void *arg)
{
	DBGMSG("curr_app = %s\n", curr_app);
	char path[PATH_MAX];
	int ret;

	if (!strcmp(curr_app, PLEXTALK_IPC_NAME_RADIO)) {
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,
				_("Cannot start the backup mode because of the radio mode."));
		return -1;
	}
	
	if(!strcmp(curr_app, PLEXTALK_IPC_NAME_RECORD)) {
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,
				_("Cannot start the backup mode because of the record mode."));
		return -1;
	}
	
	snprintf(path, PATH_MAX, "/tmp/.%s", curr_app);
	if (!PlextalkIsFileExist(path))
	{
		goto err;
	}

	ret = CoolGetCharValue(path, "backup", "backup_path", backupinfo.source, PATH_MAX);
	if (ret != COOL_INI_OK)
		goto err;
	
	if (StringStartWith(backupinfo.source, DISK_INNER)) {
		backup_media_menu_items[0] = _("SD Card");
		backup_media_menu_items[1] = _("USB Memory");
	} else if (StringStartWith(backupinfo.source, DISK_SDCRD)) {
		backup_media_menu_items[0] = _("Internal Memory");
		backup_media_menu_items[1] = _("USB Memory");
	} else if (StringStartWith(backupinfo.source, DISK_USB) ||
	           StringStartWith(backupinfo.source, DISK_CDROM) ||
	           StringStartWith(backupinfo.source, DISK_CDDA)) {
		backup_media_menu_items[0] = _("Internal Memory");
		backup_media_menu_items[1] = _("SD Card");
	} else {
		goto err;
	}
	//voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
	return 0;
err:
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role,_("No title in backup source media."));
	return -1;
}

static void
OnMsgBoxKeyDown(WID__ wid, KEY__ key)
{
//	WIDGET *w = (WIDGET *)NeuxGetWidgetFromWID(wid);
//	msgbox_context_t *cntx = NeuxWidgetGetData(w);

	switch (key.ch) {
	case MWKEY_UP:
	case MWKEY_DOWN:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	}
}


static void OnBackupKeydown(WID__ wid, KEY__ key)
{
	printf("OnInfoKeydown key.ch = %d\n", key.ch);
	char *curtext = NULL;
	
	if (menu_exit) {
		voice_prompt_abort();
		NeuxAppStop();
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
			if(backupinfo.target_media && backupinfo.tts_str){
				voice_prompt_string(1, &vprompt_cfg,
					ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
					backupinfo.tts_str, strlen(backupinfo.tts_str));
			}
			voice_prompt_end = 0;
		}
		return;
	}

	switch (key.ch)
	{
	case MWKEY_DOWN:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(backupinfo.target_media)
			{
				if(!backupinfo.timerEnable){
					TimerEnable(backupinfo.timer);
					backupinfo.timerEnable = 1;
				}
				mullistNextItem(backupinfo.list);
				curtext = getItemContent(backupinfo.list);
				DBGMSG("curtext = %s\n", curtext);
				voice_prompt_string(1, &vprompt_cfg,
					ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
					curtext, strlen(curtext));
				backupinfo.tts_str = curtext;
			}
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_UP:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(backupinfo.target_media)
			{
				if(!backupinfo.timerEnable){
					TimerEnable(backupinfo.timer);
					backupinfo.timerEnable = 1;
				}
				mullistPrevItem(backupinfo.list);
				curtext = getItemContent(backupinfo.list);
				DBGMSG("curtext = %s\n", curtext);
				voice_prompt_string(1, &vprompt_cfg,
					ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
					curtext, strlen(curtext));
				backupinfo.tts_str = curtext;
			}
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_ENTER:
	case MWKEY_RIGHT:
		DBGMSG("confirm, ok!\n");
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			if(backupinfo.target_media){
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, gettext("Enter"));
			}
			voice_prompt_end = 0;
			backupinfo.ok = 1;
			backup_endOfModal = 1;
		}
		break;
//	case MWKEY_MENU:
	case MWKEY_LEFT:
		DBGMSG("confirm, cancel!\n");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext("Cancel."));
		voice_prompt_end = 0;
		backupinfo.ok = 0;
		backup_endOfModal = 1;
		break;
	}
}

static void OnBackUpTimer(WID__ wid)
{
	char *curtext = NULL;
	if (menu_exit) {
		return;
	}
	if(!backupinfo.target_media){
		return;
	}
	
	if(!voice_prompt_end){
		return;
	}

	int end;
	DBGMSG("tips end\n");
	curtext = getMoreContent(backupinfo.list);
	if(!curtext){
		mullistNextItem(backupinfo.list);
		if(mullistIsLastItem(backupinfo.list)){
			DBGMSG("read end\n");
			TimerDisable(backupinfo.timer);
			backupinfo.timerEnable = 0;
			backupinfo.tts_str = NULL;
			return;
		}
		curtext = getItemContent(backupinfo.list);
	}
	DBGMSG("curtext = %s\n", curtext);
	voice_prompt_string(1, &vprompt_cfg,
		ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
		curtext, strlen(curtext));
	backupinfo.tts_str = curtext;
	voice_prompt_end = 0;
	
}


static void OnBackupRedraw(WID__ wid)
{
	if(backupinfo.list)
		multiline_list_show(backupinfo.list);
}

static void OnBackupGetFocus(WID__ wid)
{
	NhelperTitleBarSetContent(_("Backup Menu"));
}

static void OnBackupDestroy(WID__ wid)
{

}

static void HotplugRemove()
{
	if(backupinfo.target_media)
	{		
		backupinfo.ok = 0;
		backup_endOfModal = 1;
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("The media is removed.Removing media while accessing could destroy data on the card."));
		voice_prompt_end = 0;
	}
}

static void onBackUpHotplug(WID__ wid, int device, int index, int action)
{
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		break;
	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			{
				DBGMSG("remove sdcard\n");
				if(StringStartWith(backupinfo.dest, DISK_SDCRD) 
					|| StringStartWith(backupinfo.source, DISK_SDCRD))
				{
					HotplugRemove();
				}
			}
			break;
		case MWHOTPLUG_DEV_SCSI_DISK:
			{
				DBGMSG("remove usb\n");
				if(StringStartWith(backupinfo.dest, DISK_USB) 
					|| StringStartWith(backupinfo.source, DISK_USB))
				{
					HotplugRemove();
				}
			}
			break;
		case MWHOTPLUG_DEV_SCSI_CDROM:
cdrom_remove:	
			{
				DBGMSG("remove cdrom\n");
				if(StringStartWith(backupinfo.source, DISK_CDROM) 
					|| StringStartWith(backupinfo.source, DISK_CDDA))
				{
					HotplugRemove();
				}
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

static void CreateBackUpForm(void)
{
    widget_prop_t wprop;

	/* Create a New window */
	backupinfo.form = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	/* Create timer */
   	backupinfo.timer = NeuxTimerNew(backupinfo.form, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(backupinfo.timer, OnBackUpTimer);
	TimerEnable(backupinfo.timer);
	backupinfo.timerEnable = 1;
	
	backupinfo.list = multiline_list_create(backupinfo.form ,NeuxWidgetWID(backupinfo.form), 0, 0, MAINWIN_WIDTH, MAINWIN_HEIGHT, 
		theme_cache.foreground, theme_cache.background);

	multiline_list_set_loop(backupinfo.list);
	
	/* Set callback for window events */	
	NeuxFormSetCallback(backupinfo.form, CB_KEY_DOWN, OnBackupKeydown);
	NeuxFormSetCallback(backupinfo.form, CB_EXPOSURE, OnBackupRedraw);
	NeuxFormSetCallback(backupinfo.form, CB_HOTPLUG, onBackUpHotplug);
	NeuxFormSetCallback(backupinfo.form, CB_DESTROY,  OnBackupDestroy);
	NeuxFormSetCallback(backupinfo.form, CB_FOCUS_IN, OnBackupGetFocus);

	/* Set proproty(color)*/
	NeuxWidgetGetProp(backupinfo.form, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(backupinfo.form, &wprop);

}

//由路径名获取文件名
static void GetFileName(const char* filepath, char * outfilename)
{
	int nLen, i = 0;
	char* p1;
	
	if(0 == *filepath)
	{
		*outfilename = 0 ;
		return;
	}
	
	nLen = strlen(filepath) - 1;
	p1 = filepath + nLen - 1;
	i = 1;
	while('/' != *p1)
	{
		if(i > nLen)
			break;
		p1--;
		i++;
	}
	
	if(NULL != p1)
	{
		strcpy(outfilename, p1 + 1);
	}
	else
	{
		*outfilename = 0 ;	
	}
}

int do_backup(void *arg)
{
	int ok = 0;
	int index = (int) arg;
	char source_path[512];
	char dest_path[512];
	char *curtext;
	char *destdir = "";	
	char *media_tip= "";
	int iscdda = 0;
	int track_num = 0;

	DBGMSG("in\n");
	backupinfo.target_media = 0;
	memset(backupinfo.dest, 0x00, sizeof(backupinfo.dest));
	DBGMSG("backupinfo.source=%s\n",backupinfo.source);
	if (!strcmp(backup_media_menu_items[index], _("SD Card"))) {
		if(getSdcardMountDir(backupinfo.dest, sizeof(backupinfo.dest))){
			backupinfo.target_media = 1;
			destdir = _("SD Card");
		}else{
			media_tip = _("please insert SD card");
		}
	}
	else if (!strcmp(backup_media_menu_items[index], _("USB Memory"))) {
		if(getUsbMountDir(backupinfo.dest, sizeof(backupinfo.dest))){
			backupinfo.target_media = 1;
			destdir = _("USB Memory");
		}else{
			media_tip = _("please insert USB memory");
		}
	}
	else /*if(!strcmp(backup_media_menu_items[index], _("Internal Memory")))*/
	{
		strlcpy(backupinfo.dest, DISK_INNER, sizeof(backupinfo.dest));
		backupinfo.target_media = 1;
		destdir = _("Internal Memory");
	}
	DBGMSG("backupinfo.dest=%s\n",backupinfo.dest);
	
	CreateBackUpForm();

	if(backupinfo.target_media)
	{
		char dstfile[256];
		char tmp[256];
		char *sdir;
		char sfile[256];
		int len;
		char *pstart;
		
		memset(sfile, 0x00, sizeof(sfile));
		if (StringStartWith(backupinfo.source, DISK_INNER)) {
			sdir = _("Internal Memory");
		} else if (StringStartWith(backupinfo.source, DISK_SDCRD)) {
			sdir = _("SD Card");
		} else if (StringStartWith(backupinfo.source, DISK_USB) ||
		           StringStartWith(backupinfo.source, DISK_CDROM)) {
		    sdir = _("USB Memory");
		} else if(StringStartWith(backupinfo.source, DISK_CDDA)){
			sdir = _("CD-DA");
			iscdda = 1;
		} else {
			sdir = "";
		}
		
		pstart = strchrnul(backupinfo.source + strlen(PLEXTALK_MOUNT_ROOT_STR "/"), '/');
		strlcpy(sfile, pstart, sizeof(sfile));
		DBGMSG("sfile=%s\n",sfile);
		
		memset(dstfile, 0x00, sizeof(dstfile));
		memset(tmp, 0x00, sizeof(tmp));
		
		if(iscdda)
		{
			if(0 == strcmp(backupinfo.source, DISK_CDDA))
			{
				strlcpy(dstfile, "Track/", sizeof(dstfile));
			}
			else{
				char buf[256];
				memset(buf, 0x00, sizeof(buf));
				strlcpy(buf, backupinfo.source + strlen("/media/cdda/Track "), sizeof(buf));
				track_num = atoi(buf);
				snprintf(dstfile, sizeof(dstfile),"%s%02d%s", "Track", track_num, ".mp3");
			}
				
		}
		else {
			if(PlextalkIsDirectory(backupinfo.source))
			{
				if(backupinfo.source[strlen(backupinfo.source)-1] !='/'){
					strlcat(backupinfo.source, "/", sizeof(backupinfo.source));
				}
			}
			//PlextalkGetFilenameFromPath
			GetFileName(backupinfo.source, dstfile);
		}

		char datefolder[50];
		get_date_folder(datefolder, sizeof(datefolder));
		if(!strcmp(curr_app, PLEXTALK_IPC_NAME_MUSIC)){
			snprintf(tmp, sizeof(tmp),"%s%s/%s", "/MusicBackup/", datefolder, dstfile);
			backupinfo.mode = 0;
		}
		else{
			snprintf(tmp, sizeof(tmp),"%s%s/%s", "/BookBackup/", datefolder, dstfile);
			backupinfo.mode = 0;
		}
		strlcat(backupinfo.dest, tmp, sizeof(backupinfo.dest));
		DBGMSG("backupinfo.dest=%s\n",backupinfo.dest);
		
		snprintf(source_path, sizeof(source_path), "%s%s %s", _("source:"), sdir, sfile);		
		snprintf(dest_path, sizeof(dest_path), "%s%s %s", _("destination:"), destdir, tmp);
		DBGMSG("source_path = %s dest_path = %s\n", source_path, dest_path);
		multiline_list_add_item(backupinfo.list, source_path);
		multiline_list_add_item(backupinfo.list, dest_path);
	}
	else{
		multiline_list_add_item(backupinfo.list, media_tip);
	}
	
	/* Show window */
    NeuxFormShow(backupinfo.form, TRUE);
	NeuxWidgetFocus(backupinfo.form);

	initCurrentItem(backupinfo.list);
	curtext = getItemContent(backupinfo.list);
	if(!backupinfo.target_media){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
	}
	voice_prompt_string(1, &vprompt_cfg,ivTTS_CODEPAGE_UTF8, tts_lang,tts_role,
		curtext, strlen(curtext));
	backupinfo.tts_str = curtext;
	voice_prompt_end = 0;
	
	backup_endOfModal = 0;
	NeuxDoModal(&backup_endOfModal, NULL);

	multiline_list_destroy(backupinfo.list);
	NeuxFormDestroy(&backupinfo.form);

	if(backupinfo.ok)
	{
		if(backupinfo.target_media)
		{
			int ret = 0;
			ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL,
				1, _("You are about to backup the selected title. Are you sure?"));				
			if(ret == MSGBX_BTN_OK)
			{	
				char backup[1024];
				char buf_source[PATH_MAX];

				memset(backup, 0x00, 1024); 
				memset(buf_source, 0x00, PATH_MAX);

				if(iscdda)
				{
					if(strstr(backupinfo.source, "/media/cdda/Track "))
					{
						sprintf(buf_source, "%s%d", "/media/cdda/Track", track_num);
					}
					else{
						sprintf(buf_source, "%s/", backupinfo.source);
					}
				} 
				else{
					strcpy(buf_source, backupinfo.source);
				}
				
				sprintf(backup, "%s %d \"%s\" \"%s\"", PLEXTALK_BINS_DIR PLEXTALK_IPC_NAME_BACKUP, backupinfo.mode, buf_source, backupinfo.dest);
				DBGMSG("backup = %s \n", backup);
				NeuxWMAppRun(backup);
				wait_focus_out = 1;
				MenuExit(0);
				return 0;
			}
		}
	}
	return -1;
}


struct SysInfo{
	FORM* form;
	TIMER* timer;
	int timerEnable;
	
	struct MultilineList *list;
	int   ok;
	char voicbuf[256];
};

struct SysInfo sysinfo;
static int sysInfo_endOfModal = -1;

static void filterSpeakChar(char *ptr)
{
	if( isdigit(ptr[0]) && '-'==ptr[1] ){
		ptr[1] = 0x20;
	}
}

//获取得到指定盘的信息
static int GetDirInfo(const char *path, char *info, char *showbuf, int buflen)
{
 	float totalnumsize;
	float  freenumsize;
	unsigned long long Totaldisk; 
    unsigned long long freedisk; 
	struct statfs diskinfo;

	statfs (path, &diskinfo);

    Totaldisk = (long long)diskinfo.f_blocks * (long long)diskinfo.f_bsize;
	freedisk = (long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize;

	totalnumsize = (float)Totaldisk/(long long)(1024*1024*1000);
	freenumsize = (float)freedisk/(long long)(1024*1024*1000);

	snprintf(showbuf, buflen,"%s: %.2f%s,%s: %.2f%s", info, freenumsize+0.005, "GB", _(" Total"), totalnumsize+0.005,"GB");

	return 0;
	
}

static int GetVerSnInfo(char *str, int len)
{
	int ret = 0;
	xmlDocPtr doc;
	xmlChar *buf = NULL;
	
	doc = xml_open((const xmlChar*)PLEXTALK_CONFIG_DIR "version.xml");
	if(doc){
		buf = xml_get(doc, (const xmlChar*)"/version/software");
		if(buf){
			ret = 1;
			strlcpy(str, (const char*)buf, len);
			xmlFree(buf);
		}
		xml_close(doc);
	}
	else{
		DBGMSG("read fail\n");
	}
	return ret;
}


static void OnSysInfoKeydown(WID__ wid, KEY__ key)
{
	char *curtext = NULL;
	printf("OnInfoKeydown key.ch = %d\n", key.ch);

	if (menu_exit) {
		voice_prompt_abort();
		NeuxAppStop();
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
			if(strlen(sysinfo.voicbuf)){
				voice_prompt_string(1, &vprompt_cfg,
					ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
					sysinfo.voicbuf, strlen(sysinfo.voicbuf));
			}
			voice_prompt_end = 0;
		}
		return;
	}

	switch (key.ch)
	{
	case MWKEY_DOWN:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			
			memset(sysinfo.voicbuf, 0x00, sizeof(sysinfo.voicbuf));
			mullistNextItem(sysinfo.list);
			curtext = getItemContent(sysinfo.list);
			strlcpy(sysinfo.voicbuf, curtext, sizeof(sysinfo.voicbuf));
			filterSpeakChar(sysinfo.voicbuf);
			DBGMSG("sysinfo.voicbuf = %s", sysinfo.voicbuf);
			voice_prompt_string(1, &vprompt_cfg,
				ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
				sysinfo.voicbuf, strlen(sysinfo.voicbuf));
			if(!sysinfo.timerEnable){
				TimerEnable(sysinfo.timer);
				sysinfo.timerEnable = 1;
			}
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_UP:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			memset(sysinfo.voicbuf, 0x00, sizeof(sysinfo.voicbuf));
			mullistPrevItem(sysinfo.list);
			curtext = getItemContent(sysinfo.list);
			strlcpy(sysinfo.voicbuf, curtext, sizeof(sysinfo.voicbuf));
			filterSpeakChar(sysinfo.voicbuf);

			DBGMSG("sysinfo.voicbuf = %s", sysinfo.voicbuf);
			voice_prompt_string(1, &vprompt_cfg,
				ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
				sysinfo.voicbuf, strlen(sysinfo.voicbuf));
			if(!sysinfo.timerEnable){
				TimerEnable(sysinfo.timer);
				sysinfo.timerEnable = 1;
			}
			voice_prompt_end = 0;
		}
		break;
	case MWKEY_ENTER:
	case MWKEY_RIGHT:
		DBGMSG("confirm, ok!");
//		sysinfo.ok = 1;
//		sysInfo_endOfModal = 1;
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext("Enter"));
		MenuExit(0);
		break;
	case MWKEY_LEFT:
		DBGMSG("confirm, cancel!");
		sysinfo.ok = 0;
		sysInfo_endOfModal = 1;
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext("Cancel."));
		break;
	}
}

static void OnSysInfoTimer(WID__ wid)
{
	char *curtext = NULL;
	if(menu_exit){
		return;
	}
	if(!voice_prompt_end){
		return;
	}
	
	int end;
	DBGMSG("tips end\n");
	curtext = getMoreContent(sysinfo.list);
	if(!curtext){
		mullistNextItem(sysinfo.list);
		if(mullistIsLastItem(sysinfo.list)){
			DBGMSG("read end\n");
			TimerDisable(sysinfo.timer);
			sysinfo.timerEnable = 0;
			memset(sysinfo.voicbuf, 0x00, sizeof(sysinfo.voicbuf));
			return;
		}
		curtext = getItemContent(sysinfo.list);
	}
	
	memset(sysinfo.voicbuf, 0x00, sizeof(sysinfo.voicbuf));
	strlcpy(sysinfo.voicbuf, curtext, sizeof(sysinfo.voicbuf));
	filterSpeakChar(sysinfo.voicbuf);

	DBGMSG("sysinfo.voicbuf = %s", sysinfo.voicbuf);
	voice_prompt_string(1, &vprompt_cfg,
		ivTTS_CODEPAGE_UTF8,tts_lang,tts_role,
		sysinfo.voicbuf, strlen(sysinfo.voicbuf));
	voice_prompt_end = 0;
}


static void OnSysInfoRedraw(WID__ wid)
{
	if(sysinfo.list)
		multiline_list_show(sysinfo.list);
}

static void OnSysInfoGetFocus(WID__ wid)
{
	NhelperTitleBarSetContent(_("System properties"));
}

static void OnSysInfoDestroy(WID__ wid)
{

}

static void CreateSystemForm(void)
{
    widget_prop_t wprop;

	/* Create a New window */
	sysinfo.form = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	sysinfo.list = multiline_list_create(sysinfo.form, NeuxWidgetWID(sysinfo.form), 0, 0, MAINWIN_WIDTH, MAINWIN_HEIGHT, 
		theme_cache.foreground, theme_cache.background);

	multiline_list_set_loop(sysinfo.list);

	/* Create timer */
   	sysinfo.timer = NeuxTimerNew(sysinfo.form, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(sysinfo.timer, OnSysInfoTimer);
	TimerEnable(sysinfo.timer);
	sysinfo.timerEnable = 1;
	/* Set callback for window events */	
	NeuxFormSetCallback(sysinfo.form, CB_KEY_DOWN, OnSysInfoKeydown);
	NeuxFormSetCallback(sysinfo.form, CB_EXPOSURE, OnSysInfoRedraw);
	NeuxFormSetCallback(sysinfo.form, CB_DESTROY,  OnSysInfoDestroy);
	NeuxFormSetCallback(sysinfo.form, CB_FOCUS_IN, OnSysInfoGetFocus);

	/* Set proproty(color)*/
	NeuxWidgetGetProp(sysinfo.form, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(sysinfo.form, &wprop);

}


//modify by ztz --start---

#define UPDATE_PATH "/dev/mmcblk0"
#define SERIAL_NUM_LEN 12
#define SERIAL_NUM_DEFAULT "514188000000"

const char* 
GetSnInfo (void)
{
	int mmcblk0_fd;
	int ret;
	static char serial_str1[SERIAL_NUM_LEN + 1];
	static char serial_str2[SERIAL_NUM_LEN + 1];

	int serial_no_offset1 = 9*1024*1024; //序列号地址
	int serial_no_offset2 = 9*1024*1024+512*1024;//备份的序列号地址

	mmcblk0_fd =  open(UPDATE_PATH ,O_RDONLY);
	if (mmcblk0_fd < 0) {
		DBGMSG("open %s err!!!\n", UPDATE_PATH);
		goto open_err;
	}

	ret = lseek(mmcblk0_fd, serial_no_offset1, SEEK_SET);
	if (ret < 0) {
		DBGMSG("lseek mmcblk0 to %d err!!!\n", serial_no_offset1);
		goto op_err;
	}
	
	ret = read(mmcblk0_fd, serial_str1, SERIAL_NUM_LEN);
	if (ret != SERIAL_NUM_LEN) {
		DBGMSG("read mmcblk0 to seral_str1 err!!!\n");
		goto op_err;
	}

	ret = lseek(mmcblk0_fd, serial_no_offset2, SEEK_SET);
	if (ret < 0) {
		DBGMSG("lseek mmcblk0 to %d err!!!\n", serial_no_offset2);
		goto op_err;
	}

	ret = read(mmcblk0_fd, serial_str2, SERIAL_NUM_LEN);
	if (ret != SERIAL_NUM_LEN) {
		DBGMSG("read mmcblk0 to seral_str2 err!!!\n");
		goto op_err;
	}
	
	serial_str1[SERIAL_NUM_LEN] = '\0';
	serial_str2[SERIAL_NUM_LEN] = '\0';

	DBGMSG("serial_str1[SERIAL_NUM_LEN] : %s\n",serial_str1);
	DBGMSG("serial_str2[SERIAL_NUM_LEN] : %s\n",serial_str2);

	if (strncmp(serial_str1, serial_str2, SERIAL_NUM_LEN) || strncmp(serial_str1, "514188", 6)) {
		DBGMSG("WARNING! serial1 not equal to serial2");
		goto op_err;
	}

	close(mmcblk0_fd);
	return serial_str1;
	
op_err:
	close(mmcblk0_fd);
open_err:
	return SERIAL_NUM_DEFAULT;
}

//modify by ztz	---- end ---

int do_system_properties(void *arg)
{
	char  versionbuf[256];
	char  xuliehaobuf[256];
	char  InnerShowbuf[256];
	char  SDbuf[256];
	char  USBbuf[256];
	char  mntdir[50];
	char  verstr[20];
	char *curtext = NULL;

	memset(versionbuf, 0x00, sizeof(versionbuf));
	if( GetVerSnInfo(verstr, sizeof(verstr)))
	{
		snprintf(versionbuf, sizeof(versionbuf),"%s %s", _("1- Software version:"), verstr);
	}
	else
	{
		snprintf(versionbuf, sizeof(versionbuf),"%s", _("1- Software version: no"));
	}
	
    memset(xuliehaobuf, 0x00, sizeof(xuliehaobuf));
	curtext = GetSnInfo();
	if(curtext)
	{
		snprintf(xuliehaobuf, sizeof(xuliehaobuf),"%s %s", _("2- Serial number:"), curtext);
	}
	else
	{
		snprintf(xuliehaobuf, sizeof(xuliehaobuf),"%s", _("2- Serial number: no"));
	}

// modify by ztz	 ---end----
	memset(InnerShowbuf, 0x00, sizeof(InnerShowbuf));
	GetDirInfo(DISK_INNER, _("3- Internal memory: Free"), InnerShowbuf, sizeof(InnerShowbuf));

	memset(SDbuf,0,sizeof(SDbuf));
	int ret = getSdcardMountDir(mntdir, sizeof(mntdir));
	DBGMSG("ret = %d", ret);
	if(ret){
	   GetDirInfo(mntdir, _("4- SD card: Free"), SDbuf, sizeof(SDbuf));
	}else{
	  snprintf(SDbuf,sizeof(SDbuf),"%s",_("4- SD card: No SD card"));
	}

	DBGMSG("SDbuf = %s", SDbuf);
	
	memset(USBbuf,0,sizeof(USBbuf));
	ret = getUsbMountDir(mntdir, sizeof(mntdir));
	DBGMSG("ret = %d", ret);
	if(ret){
	   GetDirInfo(mntdir, _("5- USB memory: Free"), USBbuf, sizeof(USBbuf));
	}else{
	  snprintf(USBbuf, sizeof(USBbuf), "%s", _("5- USB memory: No USB memory"));
	}
	DBGMSG("USBbuf = %s", USBbuf);

	CreateSystemForm();

	multiline_list_add_item(sysinfo.list, versionbuf);
	multiline_list_add_item(sysinfo.list,xuliehaobuf);
	multiline_list_add_item(sysinfo.list,InnerShowbuf);
	multiline_list_add_item(sysinfo.list,SDbuf);
	multiline_list_add_item(sysinfo.list,USBbuf);
	
	/* Show window */
    NeuxFormShow(sysinfo.form, TRUE);
	NeuxWidgetFocus(sysinfo.form);

	memset(sysinfo.voicbuf, 0x00, sizeof(sysinfo.voicbuf));

	initCurrentItem(sysinfo.list);
	curtext = getItemContent(sysinfo.list);
	
	strlcpy(sysinfo.voicbuf, curtext, sizeof(sysinfo.voicbuf));
	filterSpeakChar(sysinfo.voicbuf);

	DBGMSG("sysinfo.voicbuf = %s", sysinfo.voicbuf);
	voice_prompt_string(1, &vprompt_cfg,
		ivTTS_CODEPAGE_UTF8, tts_lang,tts_role,
		sysinfo.voicbuf, strlen(sysinfo.voicbuf));
	voice_prompt_end = 0;

	sysInfo_endOfModal = 0;
	NeuxDoModal(&sysInfo_endOfModal, NULL);
	
	multiline_list_destroy(sysinfo.list);
	NeuxFormDestroy(&sysinfo.form);
	
	if(sysinfo.ok){
		//MenuExit(0);
		return 0;
	}
	else{
		NhelperTitleBarSetContent(_("General menu"));
		return -1;
	}
}


