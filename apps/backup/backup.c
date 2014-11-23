#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h> /*basename*/
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <time.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <gst/gst.h>
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <microwin/device.h>
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "libvprompt.h"
#include "Widgets.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "plextalk-config.h"
#include "application-ipc.h"
#include "plextalk-i18n.h"
#include "plextalk-keys.h"
#include "application.h"
#include "neux.h"
#include "backup_tts.h"
#include "backup.h"
#include "libinfo.h"
#include "device.h"
#include "libinfo.h"
#include "desktop-ipc.h"
#include "file-helper.h"
#include "nxutils.h"
#include "storage.h"

static struct backup_nano bk_nano;
pthread_t back_pid;
struct backup_nano *cdda_nano;

#define MAX_PATH_BYTES  1024
#define TIMER_BACKUP    500
#define TIMER_LONG_BACKUP    1000
#define LABEL_L          40
#define LABEL_T          40
#define LABEL_H          30

#define DISK_INNER			PLEXTALK_MOUNT_ROOT_STR"/mmcblk0p2"
#define DISK_SDCRD			PLEXTALK_MOUNT_ROOT_STR"/mmcblk1"
#define DISK_USB			PLEXTALK_MOUNT_ROOT_STR"/sd"
#define DISK_CDROM			PLEXTALK_MOUNT_ROOT_STR"/sr"
#define DISK_CDDA			PLEXTALK_MOUNT_ROOT_STR"/cdda"

#define MUSIC_TMP_PATH		DISK_INNER"/musictmp"

static int g_mode = 0;
static int g_end = 0;

extern int Backup_cdda(void* arg );
extern float Return_precnet();

static int readonly;
static int storage_enum(struct mntent *mount_entry, void* user_data)
{
	char *pStart = strstr(mount_entry->mnt_dir ,PLEXTALK_MOUNT_ROOT_STR);
	if(NULL != pStart){
		if(!strncmp(pStart, user_data, strlen(pStart))){
			readonly = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
			return 1;
		}
	}
	return 0;
}

static int isMediaReadOnly(const char *path)
{
	int ret = CoolStorageEnumerate(storage_enum ,path);
	if(1 == ret){
		return readonly;
	}else{
		return 0;
	}
}

static int getMediaPath(const char *path, char *out, int len)
{
	char *p = strchr(path + strlen(PLEXTALK_MOUNT_ROOT_STR)+1, '/');
	if(p){
		strlcpy(out, path, p - path +1);
	}else{
		strlcpy(out, path, len);
	}
	return 1;
}

static void backupRemoveDest(void)
{
	DBGMSG("\n");
	if(bk_nano.iscdda || !bk_nano.err_exit){
		return;
	}
	if(g_end){//copy complete
		return;
	}
	
	char cmd[PATH_MAX+20];
	char path[PATH_MAX];
	char *tmp;
	int len;
	
	memset(path, 0, sizeof(path));
	strlcpy(path, bk_nano.dest, PATH_MAX);

	len = strlen(path);
	if(len <= 0){
		return;
	}
	
	if('/'== path[len-1]){
		path[len-1] = 0;
	}

	tmp = strrchr(path, '/');
	if (tmp != NULL){
		*tmp = 0;
	}
	
	snprintf(cmd,sizeof(cmd), "rm -rf \"%s\"",path);
	DBGMSG("cmd:%s \n",cmd);
	if(system(cmd)!=0){
		DBGMSG("rm dest error!\n");
	}else{
		DBGMSG("rm dest ok\n");
	}
}

int backup_copy_progress(struct CopyInfo *thiz)
{
//	char *strinfo;
	ssize_t count;
//	size_t num;
	char *p, pread;
	float persent = 0.0;	

//	num = MAX_PATH_BYTES;
	memset(thiz->strinfo, 0, MAX_PATH_BYTES);

	//pread = fgets(thiz->strinfo, MAX_PATH_BYTES , thiz->fp);
	count = read(thiz->fd, thiz->strinfo, MAX_PATH_BYTES);
	//DBGMSG("count = %d\n", count);
	//DBGMSG("thiz->strinfo = %s\n", thiz->strinfo);
	
	if(count > 0){
		thiz->strinfo[count] = 0;
		DBGMSG("line:%s\n", thiz->strinfo);
		p = strrchr(thiz->strinfo, '%');
		if(p){
			struct statfs diskinfo;
			unsigned long long copysize, remainsize;
			
			statfs (bk_nano.dirpath, &diskinfo);

			remainsize = ((long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize);
 
			copysize = bk_nano.freedir - remainsize;

			DBGMSG("copysize = %lld \n", copysize);

			persent = (float)(copysize) / (float)(bk_nano.pathtotal) * 100.0;		
			
			thiz->prog_val = persent;
			DBGMSG("thiz->prog_val = %f \n", thiz->prog_val);
		}
		else{
			p = strstr(thiz->strinfo, "total size is");
			if(p)
			{
				persent = 100.0;
				thiz->prog_val = persent;
			}
			else{
				DBGMSG("no per num \n");
			}
		}

	}else if((count == -1) && (errno == EAGAIN)){
		DBGMSG("no data yet!\n");
	}else{
		DBGMSG("read empty\n");
		thiz->report = 0;
		NeuxAppFDSourceUnregister(thiz->fd);
		DBGMSG("GrUnregisterInput\n");
	}
	

	if(persent >= 100.0){
		thiz->report = 0;
		DBGMSG("100 persent\n");
		NeuxAppFDSourceUnregister(thiz->fd);
		DBGMSG("GrUnregisterInput\n");
	}
	
	return 0;
}

int backup_copy_close()
{	
	if(!bk_nano.thiz){
		return;
	}

	if(bk_nano.thiz->fp){
		pclose(bk_nano.thiz->fp);
		bk_nano.thiz->fp = NULL;
	}
	if(bk_nano.thiz->strinfo){
		free(bk_nano.thiz->strinfo);
		bk_nano.thiz->strinfo = NULL;
	}
	
	return 0;
}
int backup_real_copy(struct CopyInfo *thiz, char *src, char*dest)
{
	char cmd[PATH_MAX + 50];

	snprintf(cmd, sizeof(cmd), "rsync -r -v --progress \"%s\" \"%s\"", src, dest);
	DBGMSG("cmd:%s\n", cmd);
	thiz->fp = popen(cmd, "r");
	thiz->fd = fileno(thiz->fp);
	fcntl(thiz->fd, F_SETFL, O_NONBLOCK);

	thiz->strinfo = (char*)malloc(MAX_PATH_BYTES);
	thiz->prog_val = 0;
	thiz->report = 1;
	return 0;
}

static void mkdir_recursive2(char *path)
{
	char cmd[PATH_MAX+20];
	snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", path);
	DBGMSG("cmd = %s\n", cmd);
	system(cmd);
}


/* 回调函数 */
void backup_cp_read(void *pdata)
{
   if(bk_nano.thiz->report){
		DBGMSG("cp progress\n");
		backup_copy_progress(bk_nano.thiz);					
	}
}

int backup_copy(char *source, char*dest)
{
	char dir_dest[PATH_MAX];
	char *ptr;
//	char* pdata;   //private data


	strlcpy(dir_dest, dest, PATH_MAX);
	ptr = strrchr(dir_dest, '/');
	if(ptr){
		*ptr = 0;
	}
	
	mkdir_recursive2(dir_dest);
	
	backup_real_copy(bk_nano.thiz, source, dir_dest);
	/*copy progress show interface*/

	NeuxAppFDSourceRegister(bk_nano.thiz->fd, NULL, backup_cp_read, NULL);
	NeuxAppFDSourceActivate(bk_nano.thiz->fd, 1, 0);

	return 0;
}


static void signal_handler(int signum)
{
	DBGLOG("------- signal caught:[%d] --------\n", signum);
	if ((signum == SIGINT) || (signum == SIGTERM) || (signum == SIGQUIT))
	{
		DBGLOG("------- global config destroyed --------\n");
		NeuxAppStop();
		DBGLOG("------- NEUX stopped			--------\n");
		exit(0);
	}
}

static void signalInit (void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sa, NULL)){}
    if (sigaction(SIGTERM, &sa, NULL)){}
    if (sigaction(SIGQUIT, &sa, NULL)){}
}


/* 回调函数 */
void backup_fd_read(void *pdata)
{
   struct signalfd_siginfo fdsi;

   ssize_t ret = read(bk_nano.fd, &fdsi, sizeof(fdsi));

   if (ret != sizeof(fdsi))
     {
     		
   	}
	DBGMSG("backup_fd_read tts stop\n");
	backup_tts_set_stopped();	

}

void backup_reg_ttsSingal()
{
//	char* pdata;   //private data

	/* 得到fd */
	bk_nano.fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(bk_nano.fd, NULL, backup_fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(bk_nano.fd, 1, 0);
}

static void backupLowpowerPro()
{
	if(!bk_nano.iscdda)
	{
		char cmd[256];
		memset(cmd, 0x00, 256);
		if(bk_nano.getting_size)
		{
			NeuxAppFDSourceUnregister(bk_nano.path_fd);
			sprintf(cmd, "pkill du");
		}
		else{
			if(!g_end)
			{
				NeuxAppFDSourceUnregister(bk_nano.thiz->fd);
				sprintf(cmd, "pkill rsync");
			}
		}
		system(cmd);
		bk_nano.err_exit = 1;
	}
}

static void OnBackupAppMsg(const char * src, neux_msg_t * msg)
{
	DBGLOG("app msg %d .\n", msg->msg.msg.msgId);

	switch (msg->msg.msg.msgId)
	{
	case PLEXTALK_APP_MSG_ID:
		{
			app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;
			switch (app->type)
			{
			/*设置相应的state，可能是暂停或者恢复*/
			case APPRQST_SET_STATE:
				{		
					app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
					if(rqst->pause)
					{
						if(bk_nano.binfo)
						{
							info_pause();
						}

					}else{
						if(bk_nano.binfo)
						{
							info_resume();
						}
					}				
				}
				break;
			case APPRQST_LOW_POWER:
				DBGMSG("received lowpower message\n");
				voice_prompt_abort();
				backupLowpowerPro();
				backupRemoveDest();
				break;
			}
		}
		break;
	}
}

static void OnBackupAppDestroy()
{
//	plextalk_set_auto_off_disable(0);
	DBGMSG("OnBackupAppDestroy destroy!\n");
	close(bk_nano.fd);
}

static void backup_key_info_handler ()
{
	bk_nano.binfo = 1;
	
	NhelperTitleBarSetContent(_("Information"));
	TimerDisable(bk_nano.timer);	

	/* Create Info id */
	bk_nano.info_id = info_create();
	int* is_running = backup_get_ttsstate();

	/* Fill info item */
	char* backup_info[1];
	char present_info[128];
	snprintf(present_info, 128, "%s %f%% %s", _("Backup"), bk_nano.thiz->prog_val, _("completed."));	
	backup_info[0] = present_info;
	
	info_start (bk_nano.info_id, backup_info, 1, is_running);
	info_destroy (bk_nano.info_id);

	TimerEnable(bk_nano.timer);
	NhelperTitleBarSetContent(_("Backup"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BACKUP);
	switch(bk_nano.media)
	{
	case 0:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		break;
	case 1:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
		break;
	case 2:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
		break;
	}
	bk_nano.binfo = 0;
}

static void OnBackUpKeyUp(WID__ wid, KEY__ key)
{
	if(g_end || bk_nano.err_exit)
		return;	
}


static void OnBackUpKeydown(WID__ wid, KEY__ key)
{
	printf("OnBackUpKeydown key.ch = %d\n", key.ch);
	 if (NeuxAppGetKeyLock(0)) {
		  if (!key.hotkey) {
		   voice_prompt_abort();
		   voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		   voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		      tts_lang, tts_role, _("Keylock"));
		  }
		  return;
	  }
	 if(bk_nano.binfo)
	 	return;

	if(g_end || bk_nano.err_exit)
		return;
	
	switch (key.ch)
    {
		case MWKEY_INFO:	//next item
		{
			if(bk_nano.thiz->prog_val < 100.0)
			{
				bk_nano.bgsound = 0;
				backup_key_info_handler();
			}
		}
		break;
 		case MWKEY_UP:
		case MWKEY_DOWN:
		case MWKEY_ENTER:
		case MWKEY_RIGHT:
		case MWKEY_LEFT:
		case VKEY_BOOK:
		case VKEY_MUSIC:
		case VKEY_RECORD:
		case VKEY_RADIO:
		{
			bk_nano.bgsound = 0;
			backup_tts(TTS_ELEMENT, bk_nano.thiz->prog_val);
		}
		break;
		case MWKEY_MENU:
		{
			bk_nano.bgsound = 0;
			backup_tts(TTS_MEN_KEY, 0);
		}
		break;
	}
}

static void OnBackUpTimer(WID__ wid)
{
	//DBGMSG("onbackuptimer 112!\n");
	if(bk_nano.err_exit)
	{
		if(backup_tts_get_stopped())
		{
			plextalk_set_hotplug_voice_disable(0);
			DBGMSG("[in]test:--- enable desktop tts\n");
			TimerDisable(bk_nano.timer);
			NeuxAppStop();
		}
		return;
	}
	if(bk_nano.iscdda)
	{
		if(bk_nano.ret_pathsize)
		{
			cdda_nano=&bk_nano;
			bk_nano.ret_pathsize = 0;
			DBGMSG("run backup_cdda!\n");
			if (pthread_create(&back_pid, NULL, Backup_cdda, (void*)cdda_nano)) {
		    	DBGMSG("backup_cdda start pthread failure!\n");
				return;
			}else{
				DBGMSG("backup_cdda start pthread success!\n");
			}
			//Backup_cdda(bk_nano.source, bk_nano.dest);
		}
		else{
			DBGMSG("get prog_val !\n");
			int ret = return_backup_error();
			DBGMSG("return backup error ret:%d\n",ret);
			if(ret > 0)
			{
				switch(ret)
				{
				case CDDA_Remove:
					
					backup_tts(TTS_REMOVE_MEDIA_READ, 0);
					break;
				case SD_card_Remove:
					backup_tts(TTS_REMOVE_MEDIA_WRITE, 0);
					break;
				case Stroage_medium_Full:
					backup_tts(TTS_NOTMEM, 0);
					break;
				case Faile_to_backup:
					backup_tts(TTS_CDDA_WRITE_ERROR, 0);
					break;
				default:
					backup_tts(TTS_FAILBACKUP, 0);
					break;
				}
				bk_nano.err_exit = 1;
				return;
			}
			else{
				bk_nano.thiz->prog_val = Return_precnet();
				//DBGMSG("bk_nano.thiz->prog_val = %f \n",bk_nano.thiz->prog_val );
			}
		}
	}
	else{
		if(bk_nano.ret_pathsize)
		{
			DBGMSG("OnBackUpTimer \n");
			if(bk_nano.pathtotal < bk_nano.freedir)
			{
				if(bk_nano.path_fp){
					pclose(bk_nano.path_fp);
					bk_nano.path_fp = NULL;
				}
				DBGMSG("copy\n");
				backup_copy(bk_nano.source, bk_nano.dest);
			}
			else{
				DBGMSG("exit\n");
				backup_tts(TTS_NOTMEM, 0);
				bk_nano.err_exit = 1;
				return;
			}
			bk_nano.ret_pathsize = 0;
			bk_nano.getting_size = 0;
		}
	}
	
		
	if(bk_nano.thiz->prog_val < 100.0)
	{
		if(backup_tts_get_stopped() && bk_nano.bgsound == 0)
		{
			bk_nano.bgsound = 1;
		}
		if(backup_tts_get_stopped() && bk_nano.bgsound == 1){
			backup_tts(TTS_BG_MP3, 0);
		}
		//DBGMSG("bk_nano.thiz->prog_val = %f \n",bk_nano.thiz->prog_val );
		switch(bk_nano.wait_state)
		{
		case 0:
			NeuxLabelSetText(bk_nano.label, _("Waiting."));
			break;
		case 1:
			NeuxLabelSetText(bk_nano.label, _("Waiting.."));
			break;
		case 2:
			NeuxLabelSetText(bk_nano.label, _("Waiting..."));
			break;
		}
		if(bk_nano.wait_state < 2)
			bk_nano.wait_state++;
		else
			bk_nano.wait_state = 0;
	}
	else
	{
		//DBGMSG("bk_nano.thiz->prog_valaa = %f \n",bk_nano.thiz->prog_val );
		DBGMSG("bk_nano.thiz->prog_val >= 100 g_end = %d\n", g_end);
		if(g_end)
		{
			if(backup_tts_get_stopped())
			{
				TimerDisable(bk_nano.timer);
				NeuxAppStop();
				DBGMSG("NeuxAppStop()\n");
			}
		}
		else{
			system("sync"); 
			g_end= 1;
			backup_tts(TTS_COMPELTE, 0);			
		}
	}
}

static void OnBackUpGetFocus(WID__ wid)
{
	NhelperTitleBarSetContent(_("Backup"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BACKUP);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
	switch(bk_nano.media)
	{
	case 0:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		break;
	case 1:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
		break;
	case 2:
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
		break;
	}
	
	
}

static void OnBackUpDestory(WID__ wid)
{
	plextalk_set_hotplug_voice_disable(0);//可重复设置，确保退出时重新ENABLE	
	plextalk_suspend_unlock();
	plextalk_schedule_unlock();
	plextalk_sleep_timer_unlock();
	plextalk_global_config_close();
	DBGMSG("destroy\n");
	backup_tts_destroy();
	if(bk_nano.bmusic)
	{
		char cmd[1024];
		memset(cmd, 0x00, 1024);
		sprintf(cmd, "rm -r \"%s\"", bk_nano.source);
		system(cmd);
	}
	if(!bk_nano.iscdda)
	{
		backup_copy_close();
		backup_size_close();
	}

	if(bk_nano.thiz){
		free(bk_nano.thiz);
		bk_nano.thiz = NULL;
	}
	
}

static void HotplugRemove()
{
	if(!bk_nano.iscdda)
	{
		char cmd[256];
		memset(cmd, 0x00, 256);
		if(bk_nano.getting_size)
		{
			NeuxAppFDSourceUnregister(bk_nano.path_fd);
			sprintf(cmd, "pkill du");
		}
		else{
			if(!g_end)
			{
				NeuxAppFDSourceUnregister(bk_nano.thiz->fd);
				sprintf(cmd, "pkill rsync");
			}
		}
		system(cmd);

		if(bk_nano.remove_type == 1)
		{
			backup_tts(TTS_REMOVE_MEDIA_READ, 0);
		}
		else if(bk_nano.remove_type == 2)
		{
			backup_tts(TTS_REMOVE_MEDIA_WRITE, 0);
		}

		bk_nano.err_exit = 1;
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
				if(StringStartWith(bk_nano.dest,DISK_SDCRD))
					bk_nano.remove_type = 2;
				else if(StringStartWith(bk_nano.source, DISK_SDCRD))
					bk_nano.remove_type = 1;
			}
			break;
		case MWHOTPLUG_DEV_SCSI_DISK:
			{
				DBGMSG("remove usb\n");
				if(StringStartWith(bk_nano.dest,DISK_USB))
					bk_nano.remove_type = 2;
				else if(StringStartWith(bk_nano.source, DISK_USB))
					bk_nano.remove_type = 1;
			}
			break;
		case MWHOTPLUG_DEV_SCSI_CDROM:
cdrom_remove:
			{
				DBGMSG("remove cdrom\n");
				if(StringStartWith(bk_nano.source, DISK_CDROM) 
					|| StringStartWith(bk_nano.source, DISK_CDDA))
				{
					bk_nano.remove_type = 1;
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

	if(bk_nano.remove_type == 1 || bk_nano.remove_type == 2)
	{
		HotplugRemove();
	}
}


static void CreateBackUpFormMain ()
{
    widget_prop_t wprop;
	label_prop_t lbprop;

	/* Create a New window */
	bk_nano.formMain = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);
	
	/* Create timer */
   	bk_nano.timer = NeuxTimerNew(bk_nano.formMain, TIMER_BACKUP, -1);
	NeuxTimerSetCallback(bk_nano.timer, OnBackUpTimer);
	TimerEnable(bk_nano.timer);
	
	/* Set callback for window events */	
	NeuxFormSetCallback(bk_nano.formMain, CB_KEY_DOWN, OnBackUpKeydown);
	NeuxFormSetCallback(bk_nano.formMain, CB_KEY_UP,OnBackUpKeyUp);
	NeuxFormSetCallback(bk_nano.formMain, CB_FOCUS_IN, OnBackUpGetFocus);
	NeuxFormSetCallback(bk_nano.formMain, CB_DESTROY, OnBackUpDestory);
	NeuxFormSetCallback(bk_nano.formMain, CB_HOTPLUG, onBackUpHotplug);
	
	/* Set proproty(color)*/	
	NeuxWidgetGetProp(bk_nano.formMain, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(bk_nano.formMain, &wprop);

	/* label1 */
	bk_nano.label = NeuxLabelNew(bk_nano.formMain, LABEL_L, LABEL_T, MAINWIN_WIDTH, LABEL_H, _("Waiting."));

	NeuxWidgetGetProp(bk_nano.label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(bk_nano.label, &wprop);

	NeuxLabelGetProp(bk_nano.label, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_LEFT;
	lbprop.transparent = FALSE;//TRUE;
	NeuxLabelSetProp(bk_nano.label, &lbprop);

	NeuxWidgetSetFont(bk_nano.label, sys_font_name, 16);

	NeuxWidgetShow(bk_nano.formMain, TRUE);
	NeuxWidgetShow(bk_nano.label, TRUE);
	NeuxWidgetFocus(bk_nano.formMain);

	bk_nano.wait_state = 0;

}

int backup_size_close()
{
	NeuxAppFDSourceUnregister(bk_nano.path_fd);
	
	if(bk_nano.path_fp){
		pclose(bk_nano.path_fp);
		bk_nano.path_fp = NULL;
	}
	
	return 0;
}

void backup_pathsize_progress()
{
	char strinfo[MAX_PATH_BYTES];
//	ssize_t count;
	char *p;
	char *pread;
	int end = 0;
	long size = 0;

	memset(strinfo, 0, MAX_PATH_BYTES);

	pread = fgets(strinfo, sizeof(strinfo), bk_nano.path_fp);
//	count = read(bk_nano.path_fd, strinfo, 500);
//	DBGMSG("count = %d\n", count);

	if( NULL != pread){
		DBGMSG("read:%s\n",strinfo);
		
		p = strstr(strinfo, "\x09/media");
		if(p){
			int depth;
			p++;
			char *pEnd = p + strlen(p) -1;
			while(0x0d==*pEnd || 0x0a==*pEnd ){
				*pEnd = 0;
				pEnd --;
			}
			depth = PlextalkMediaPathDepth(p);
			DBGMSG("depth:%d\n", depth);
			if(bk_nano.src_depth < depth ){
				bk_nano.src_depth = depth;
			}
			return;
		}

		p = strstr(strinfo, "\x09total\n");
		if(p){
			size = atol(strinfo);
			DBGMSG("size:%ld\n", size);
			bk_nano.pathtotal =  size * 1024;
			bk_nano.ret_pathsize = 1;
			DBGMSG("size:%lld\n", bk_nano.pathtotal);
			bk_nano.path_report = 0;
			NeuxAppFDSourceUnregister(bk_nano.path_fd);
			return;
		}
	}else{
		DBGMSG("read:end\n");
//		bk_nano.path_report = 0;
		NeuxAppFDSourceUnregister(bk_nano.path_fd);

		bk_nano.dest_depth = PlextalkMediaPathDepth(bk_nano.dest);
		bk_nano.src_depth = bk_nano.src_depth - PlextalkMediaPathDepth(bk_nano.source);
		DBGMSG("depth:%d\n", bk_nano.dest_depth);
		DBGMSG("srcdepth:%d\n",bk_nano.src_depth);
		

		if((bk_nano.dest_depth + bk_nano.src_depth)>=8){
			DBGMSG("depth over\n");
			backup_tts(TTS_MAX_FILELEVEL, 0);
			backup_tts(TTS_FAILBACKUP, 0);
			bk_nano.err_exit = 1;
			return;
		}
	}
}


/* 回调函数 */
void backup_pathsize_read(void *pdata)
{
   if(bk_nano.path_report){
		DBGMSG("report progress\n");
		backup_pathsize_progress();
					
	}
}

void BackUp_Init(char *source, char*dest)
{
	/* Init TTS */
	memset(&bk_nano, 0x00, sizeof(struct backup_nano));
	bk_nano.thiz = (struct CopyInfo *)malloc(sizeof(struct CopyInfo));
	if(!bk_nano.thiz){
		bk_nano.err_exit = 1;
		return;
	}
	memset(bk_nano.thiz, 0x00, sizeof(struct CopyInfo));
	backup_reg_ttsSingal();
	plextalk_update_tts(g_config->setting.lang.lang);
	/* Init language */
	plextalk_update_lang(g_config->setting.lang.lang, "backup");
	backup_tts_init();
	
	bk_nano.source = source;
	bk_nano.dest = dest;

	if(StringStartWith(source, DISK_INNER)){
		bk_nano.media = 0;
	}else if(StringStartWith(source, DISK_SDCRD)){
		bk_nano.media = 1;
	}else{
		bk_nano.media = 2;
	}

	//只读判断
	if(isMediaReadOnly(bk_nano.dest))
	{
		backup_tts(TTS_MEDIA_LOCKED, 0);
		bk_nano.err_exit = 1;
		return;
	}

	if(StringStartWith(source, DISK_CDDA))
	{
		bk_nano.iscdda = 1;
		bk_nano.ret_pathsize = 1;
	}
	else{		
		struct statfs diskinfo;
		getMediaPath(bk_nano.dest, bk_nano.dirpath, sizeof(bk_nano.dirpath));
		DBGMSG("bk_nano.dirpath = %s\n", bk_nano.dirpath);
		statfs (bk_nano.dirpath, &diskinfo);
		bk_nano.freedir = (long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize;
		DBGMSG("bk_nano.freedir  = %lld\n", bk_nano.freedir);
		bk_nano.getting_size = 1;

		bk_nano.bmusic = g_mode;
		//计算得到目标文件路径总大小		
		char cmd[1024];
		if(bk_nano.bmusic)
		{
			DBGMSG("copy only mp3 wave\n");
			sprintf(cmd, "find \"%s\" -name *.mp3 -exec cp {} \"%s\" \;", bk_nano.source, MUSIC_TMP_PATH);
			system(cmd);
			sprintf(cmd, "find \"%s\" -name *.wav -exec cp {} \"%s\" \;", bk_nano.source, MUSIC_TMP_PATH);
			system(cmd);
			bk_nano.source = MUSIC_TMP_PATH;
		}

		snprintf(cmd, 1024, "du -ck  \"%s\"", bk_nano.source);
		DBGMSG("cmd:%s\n", cmd);
		bk_nano.path_fp = popen(cmd, "r");
		bk_nano.path_fd = fileno(bk_nano.path_fp);
		bk_nano.path_report = 1;

		NeuxAppFDSourceRegister(bk_nano.path_fd , NULL, backup_pathsize_read, NULL);
		NeuxAppFDSourceActivate(bk_nano.path_fd , 1, 0);
	}
	 
	CreateBackUpFormMain();

//	backup_tts(TTS_ENTER_BACKUP, 0);
	backup_tts(TTS_ENTER_BACKUP, 0);
	bk_nano.bgsound = 1;

}

#if 0
static void backup_onSWOn(int sw)
{
	if(sw == SW_KBD_LOCK)
	{
     		bk_nano.key_locked = 1;
		if(bk_nano.binfo)
		{
			info_setkeylock( bk_nano.key_locked);
		 }
	}
}

static void backup_onSWOff(int sw)
{
	if(sw == SW_KBD_LOCK)
	{
	      bk_nano.key_locked = 0;
		if(bk_nano.binfo)
		{
			info_setkeylock( bk_nano.key_locked);
		 }
	}
}
#endif

int main (int argc, char **argv)
{
	if (NeuxAppCreate(argv[0]))
		FATAL("unable to create application.!\n");

	char *source;
	char*dest;
	signalInit();

	plextalk_global_config_open();
	plextalk_set_hotplug_voice_disable(1);
	DBGMSG("[in]test:--- disable desktop tts\n");
	plextalk_update_sys_font(g_config->setting.lang.lang);
	//所有对g_config的引用都要加读写锁
	NeuxThemeInit(g_config->setting.lcd.theme);
	//for test
	g_mode = atoi(argv[1]);
	source = argv[2]; //"/media/mmcblk0p2/RadioRecording/";
	dest = argv[3]; //"/media/mmcblk0p2/libs/RadioRecording/";
	g_end = 0;

	DBGMSG("source = %s dest = %s\n", source, dest);

	plextalk_schedule_lock();
	plextalk_suspend_lock();
	plextalk_sleep_timer_lock();
//	plextalk_set_auto_off_disable(1);	//disable auto off
	
	BackUp_Init(source, dest);
	
	/* Set callback for app events */
	NeuxAppSetCallback(CB_MSG, OnBackupAppMsg);
//	NeuxWMAppSetCallback(CB_SW_ON,   backup_onSWOn);
//	NeuxWMAppSetCallback(CB_SW_OFF,  backup_onSWOff);
	NeuxAppSetCallback(CB_DESTROY, OnBackupAppDestroy);

	/* Main loop */
	NeuxAppStart();

	return 0;
}

