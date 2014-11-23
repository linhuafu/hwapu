#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <nano-X.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
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
#include "plextalk-theme.h"
#include "nxutils.h"
#include "libvprompt.h"
#include "sysfs-helper.h"
#include "plextalk-keys.h"
#include "file_menu.h"
#include "storage.h"
#include <microwin/device.h>


#define MEDIA_INNER				PLEXTALK_MOUNT_ROOT_STR"/mmcblk0p2"
#define MEDIA_SDCRD				PLEXTALK_MOUNT_ROOT_STR"/mmcblk1"
#define MEDIA_USB				PLEXTALK_MOUNT_ROOT_STR"/sd"
#define MEDIA_CDROM				PLEXTALK_MOUNT_ROOT_STR"/sr"

typedef enum{
	RET_OK,
	ERR_SRC_RM,
	ERR_DEST_RM,
	ERR_NO_SPACE,
	ERR_MAX_DEPTH,
	ERR_COPY_BREAK,
}PRO_RESULT;

typedef enum {
	MEDIA_NO,
	MEDIA_INTERNAL_MEM,
	MEDIA_SD_CARD,
	MEDIA_USB_MEM,
}MEDIA_TYPE;

typedef enum{
	CAL_SRC_SIZE,
	COPYING,
}PRO_STATE;

typedef struct {
	FORM	*form;
	LABEL	*item_lable;
	
	char	readBuf[1024];
	
	int		wait_cnt;
	storage_info_t storageInfo;
	
	OP_FILE *op_file;
	int		src_err;
	int		dest_err;
	PRO_RESULT	msgReturn;
	int		endOfModal;

	int		checkExist;
	int		alreadyExist;
	PRO_STATE pro_state;
	FILE	*size_fp;
	int		size_fd;
	
	FILE	*copy_fp;
	int		copy_fd;
	
	int		src_size;
	int		dest_free;

	int		src_depth;
	int		dest_depth;
	
	int		copying;
	int		copyProgress;
}FILE_PROC;

extern int voice_prompt_running;

static void (*origAppMsg)(const char * src, neux_msg_t * msg);

static FILE_PROC *fileProc = NULL;
static TIMER *key_timer;
static int long_pressed_key = -1;

void CreateProcForm(void);

static void
OnMsgBoxKeyDown(WID__ wid, KEY__ key)
{
	switch (key.ch) {
//	case MWKEY_MENU:
//		FileManagementExit();
//		break;
	default:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	}
}

static int isRootMedia(const char *path)
{
	if(!StringStartWith(path,PLEXTALK_MOUNT_ROOT_STR)){
		return 0;
	}
	
	char *p = strchr(path + strlen(PLEXTALK_MOUNT_ROOT_STR)+1, '/');
	if(NULL == p){
		DBGMSG("root media\n");
		return 1;
	}
	return 0;
}

static MEDIA_TYPE getMediaType(const char *path)
{
	MEDIA_TYPE media_type;
	if (StringStartWith(path, MEDIA_INNER))
		media_type= MEDIA_INTERNAL_MEM;
	else if (StringStartWith(path, MEDIA_SDCRD))
		media_type= MEDIA_SD_CARD;
	else if (StringStartWith(path, MEDIA_USB) ||
             StringStartWith(path, MEDIA_CDROM))
		media_type= MEDIA_USB_MEM;
	else
		media_type= MEDIA_NO;
	return media_type;
}

static int storage_enum(struct mntent *mount_entry, void* user_data)
{
	struct statfs s;
	DBGMSG("mnt_dir=%s\n",mount_entry->mnt_dir);
	char *pStart = strstr(mount_entry->mnt_dir ,PLEXTALK_MOUNT_ROOT_STR);
	if(NULL != pStart){
		DBGMSG("pStart=%s\n",pStart);
		if(!strncmp(pStart, user_data, strlen(pStart))){
			strlcpy(fileProc->storageInfo.device, mount_entry->mnt_fsname, sizeof(fileProc->storageInfo.device));
			strlcpy(fileProc->storageInfo.mount_point, mount_entry->mnt_dir, sizeof(fileProc->storageInfo.mount_point));
			fileProc->storageInfo.ro  = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
			
			statfs(fileProc->storageInfo.mount_point, &s);
			fileProc->storageInfo.total = (unsigned long long)s.f_blocks * (unsigned long long)s.f_bsize;
			fileProc->storageInfo.free  = (unsigned long long)s.f_bavail * (unsigned long long)s.f_bsize;
			return 1;
		}
	}
	return 0;
}


static void OnWindowKeydown(WID__ wid,KEY__ key)
{
	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Keylock"));
		voice_prompt_running = 1;
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
		voice_prompt_abort();
		voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
		voice_prompt_running = 1;
		break;
		
	case MWKEY_MENU:
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;
		
	case MWKEY_INFO:
		voice_prompt_abort();
		doVoicePrompt(NULL, _("File processing"));
		voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_running = 1;
		break;
	}
}

static void OnWindowKeyup(WID__ wid, KEY__ key)
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
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch)
	{
//	case MWKEY_INFO:
//		voice_prompt_abort();
//		doVoicePrompt(NULL, _("File processing"));
//		doVoicePrompt(VPROMPT_AUDIO_PU, NULL);
//		break;
		
	case MWKEY_MENU:
		voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("File processing,please wait."));
		voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
		voice_prompt_running = 1;
		break;
	default:
		break;
	}
}

static void OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	if(fileProc)
	{
		free(fileProc);
		fileProc = NULL;
	}
	plextalk_suspend_unlock();
}
static void OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	char removeMedia[PATH_MAX];
	OP_FILE *op_file;
	if(NULL == fileProc){
		return;
	}

	op_file = fileProc->op_file;
	memset(removeMedia, 0, sizeof(removeMedia));
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		break;

	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			strlcpy(removeMedia, MEDIA_SDCRD, PATH_MAX);
			break;
			
		case MWHOTPLUG_DEV_SCSI_DISK:
			strlcpy(removeMedia, MEDIA_USB, PATH_MAX);
			break;
			
		case MWHOTPLUG_DEV_SCSI_CDROM:
			strlcpy(removeMedia, MEDIA_CDROM, PATH_MAX);
			break;
			
		default:
			break;
		}
		break;
		
	case MWHOTPLUG_ACTION_CHANGE:
		if (device == MWHOTPLUG_DEV_SCSI_CDROM)
		{
			if ( CoolCdromMediaPresent() <=0 )
			{
				strlcpy(removeMedia, MEDIA_CDROM, PATH_MAX);
			}
		}
		break;
	}

	if( 0 == strlen(removeMedia)){
		return;
	}
	DBGMSG("media:%s rm\n",removeMedia);
	fileProc->src_err = 0;
	fileProc->dest_err = 0;
	if (StringStartWith(op_file->src_path, removeMedia)){
		fileProc->src_err = 1;
	}
	if (StringStartWith(op_file->dest_path, removeMedia)){
		fileProc->dest_err = 1;
	}

	if(fileProc->src_err || fileProc->dest_err){
		if(CAL_SRC_SIZE ==fileProc->pro_state){
			system("pkill du");	//stop copy
		}else if(COPYING ==fileProc->pro_state){
			system("pkill rsync");	//stop copy
		}
		voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("The media is removed.Removing media while accessing could destroy data on the card."));
		
		if(fileProc->src_err){
			doVoicePrompt(VPROMPT_AUDIO_BU, _("Read error.It may not access the media correctly."));
		}else{
			doVoicePrompt(VPROMPT_AUDIO_BU, _("Write error.It may not access the media correctly."));
		}
		voice_prompt_running = 1;
	}
}

static void onAppMsg(const char * src, neux_msg_t * msg)
{
	if( NULL !=origAppMsg){
		origAppMsg(src, msg);
	}
	
	if(PLEXTALK_APP_MSG_ID != msg->msg.msg.msgId){
		return;
	}

	app_rqst_t *rqst = (app_rqst_t*)msg->msg.msg.msgTxt;
	switch (rqst->type) {
	case APPRQST_LOW_POWER:
		{
			DBGMSG("LOW POWER\n");
			if(CAL_SRC_SIZE ==fileProc->pro_state){
				system("pkill du");
			}else if(COPYING ==fileProc->pro_state){
				system("pkill rsync");
			}
		}
		break;
	default:
		break;
	}
}

static void OnTimerOut(void)
{
	static int count = 0;
	char str[64];
	char *proStr[]=
	{
		".",
		"..",
		"...",
	};
	int progress = fileProc->wait_cnt%3;
	snprintf(str, sizeof(str), "%s%s", _("Please wait"),proStr[progress]);
	NeuxLabelSetText(fileProc->item_lable, str);
	fileProc->wait_cnt ++;

	if(voice_prompt_running){
		count = 0;
	}else{
		count ++;
		if( 0 == count%3){
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
			voice_prompt_running = 1;
		}
	}	
}

static void OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}

void CreateProcForm(void)
{
	widget_prop_t wprop;
	label_prop_t lprop;
	TIMER *timer;
	int y,h;
	fileProc->form = NeuxFormNew( MAINWIN_LEFT,
	                      MAINWIN_TOP,
	                      MAINWIN_WIDTH,
	                      MAINWIN_HEIGHT);

	NeuxFormSetCallback(fileProc->form, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(fileProc->form, CB_KEY_UP, OnWindowKeyup);
//	NeuxFormSetCallback(widget, CB_EXPOSURE, OnWindowRedraw);
	NeuxFormSetCallback(fileProc->form, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(fileProc->form, CB_HOTPLUG,  OnWindowHotplug);

	h = sys_font_size+6;
	y = (MAINWIN_HEIGHT - h)/2;
	
	fileProc->item_lable = NeuxLabelNew(fileProc->form,0,y,MAINWIN_WIDTH,h, _("Please wait"));

	timer = NeuxTimerNew(fileProc->form, 500, -1);
	NeuxTimerSetCallback(timer, OnTimerOut);
	fileProc->wait_cnt = 0;

	key_timer = NeuxTimerNewWithCallback(fileProc->form, 0, 0, OnKeyTimer);
	
	NeuxWidgetGetProp(fileProc->item_lable, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(fileProc->item_lable, &wprop);
	
	NeuxLabelGetProp(fileProc->item_lable, &lprop);
	lprop.align = LA_CENTER;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(fileProc->item_lable, &lprop);
	
	NeuxWidgetSetFont(fileProc->item_lable, sys_font_name, sys_font_size);
	
	NeuxWidgetShow(fileProc->item_lable, TRUE);
	
	NeuxFormShow(fileProc->form, TRUE);
	NeuxWidgetFocus(fileProc->form);
}

static int getFreeSizeKB(char *path){
	struct statfs diskinfo;
	int freesize;

	statfs (path, &diskinfo);
	freesize = ((long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize)/1024;
	return freesize;
}

//删除未完成拷贝的文件
static int removeUndoneFile(void)
{
	DBGMSG("removeUndoneFile\n");
	if(PlextalkIsDirectory(fileProc->op_file->src_path)){
		//拷贝的是目录
		DBGMSG("is dir!!\n");
	}else{
		//拷贝的是文件
		char *filename = PlextalkGetFilenameFromPath(fileProc->op_file->src_path);
		char cmd[PATH_MAX+20];
		
		snprintf(cmd,sizeof(cmd), "rm -rf \"%s/%s\"", fileProc->op_file->dest_path, filename);
		DBGMSG("cmd=%s\n",cmd);
		system(cmd);
	}
	return 1;
}

static void copy_fd_read(void *pdate)
{
	char *p;
	char *pread;
	if(NULL == fileProc || -1 == fileProc->copy_fd){
		return;
	}
	
	memset(fileProc->readBuf, 0, sizeof(fileProc->readBuf));
   	//ssize_t ret = read(fileProc->copy_fd, fileProc->readBuf, sizeof(fileProc->readBuf));
	pread = fgets(fileProc->readBuf, sizeof(fileProc->readBuf), fileProc->copy_fp);
	if( NULL != pread){
		printf("%s",fileProc->readBuf);
		p = strstr(fileProc->readBuf, "total size is");
		if(p){
			fileProc->copyProgress = 100;
		}
	}else{
		DBGMSG("read:end\n");

		fileProc->copying = 0;
		NeuxAppFDSourceActivate(fileProc->copy_fd, 0, 0);
		NeuxAppFDSourceUnregister(fileProc->copy_fd );
		pclose(fileProc->copy_fp);

		fileProc->endOfModal = 1;
		if(100==fileProc->copyProgress){
			system("sync");
			fileProc->msgReturn = RET_OK;
		}else if(fileProc->src_err){
			if(!fileProc->dest_err){
				removeUndoneFile();
			}
			fileProc->msgReturn = ERR_SRC_RM;
		}else if(fileProc->dest_err){
			fileProc->msgReturn = ERR_DEST_RM;
		}else{
			removeUndoneFile();
			fileProc->msgReturn = ERR_COPY_BREAK;
		}
	}
}

static int startCopyfile(void)
{
	char cmd[PATH_MAX*2+50];
	//拷贝整个磁盘
	if(isRootMedia(fileProc->op_file->src_path)){
		DBGMSG("copy whole media\n");
		strlcat(fileProc->op_file->src_path, "/", sizeof(fileProc->op_file->src_path));
	}
	
	snprintf(cmd, sizeof(cmd), "rsync -r -v --progress \"%s\" \"%s\"", fileProc->op_file->src_path, fileProc->op_file->dest_path);
	DBGMSG("cmd:%s\n",cmd);
	fileProc->copy_fp = popen(cmd, "r");
	fileProc->copy_fd = fileno(fileProc->copy_fp);
	DBGMSG("copy_fd=%d\n",fileProc->copy_fd);
	
	fcntl(fileProc->copy_fd, F_SETFL, O_NONBLOCK);

	NeuxAppFDSourceRegister(fileProc->copy_fd, NULL, copy_fd_read, NULL);
	NeuxAppFDSourceActivate(fileProc->copy_fd, 1, 0);
	fileProc->pro_state = COPYING;

	doVoicePrompt(NULL, _("File processing,please wait."));
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
	voice_prompt_running = 1;
	return 1;
}

#define COPY_MAX_DEPTH	9

static void size_fd_read(void *pdate)
{
	char *p;
	char *pread;

	if(NULL == fileProc || -1 == fileProc->size_fd){
		return;
	}
	
	memset(fileProc->readBuf, 0, sizeof(fileProc->readBuf));
	pread = fgets(fileProc->readBuf, sizeof(fileProc->readBuf), fileProc->size_fp);
   	//ssize_t ret = read(fileProc->size_fd, fileProc->readBuf, sizeof(fileProc->readBuf));
   	if( NULL != pread){
		DBGMSG("read:%s\n",fileProc->readBuf);
		
		p = strstr(fileProc->readBuf, "\x09/media");
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
			if(fileProc->src_depth < depth ){
				fileProc->src_depth = depth;
			}
			if(fileProc->checkExist && !fileProc->alreadyExist){
				char path[PATH_MAX];
				if(isRootMedia(p)){
					return;
				}
				char *filename = p + strlen(fileProc->op_file->src_path);
				if('/'== *filename){
					filename ++;
				}
				PlextalkCatFilename2Path(path, PATH_MAX, fileProc->op_file->dest_path, filename);
				DBGMSG("check:%s\n", path);
				if(PlextalkIsFileExist(path)){
					DBGMSG("already exist\n");
					fileProc->alreadyExist = 1;
				}
			}
			return;
		}

		p = strstr(fileProc->readBuf, "\x09total\n");
		if(p){
			fileProc->src_size = atol(fileProc->readBuf);
			DBGMSG("size:%d\n", fileProc->src_size);
			return;
		}
	}else{
		DBGMSG("read:end\n");
		NeuxAppFDSourceActivate(fileProc->size_fd, 0, 0);
		NeuxAppFDSourceUnregister(fileProc->size_fd );
		pclose(fileProc->size_fp);

		if(fileProc->src_err){
			fileProc->endOfModal = 1;
			fileProc->msgReturn = ERR_SRC_RM;
			return;
		}else if(fileProc->dest_err){
			fileProc->endOfModal = 1;
			fileProc->msgReturn = ERR_DEST_RM;
			return;
		}

		if(0==fileProc->src_size){
			DBGMSG("src_size:0\n");
			fileProc->endOfModal = 1;
			fileProc->msgReturn = ERR_COPY_BREAK;
			return;
		}

		fileProc->dest_free =  getFreeSizeKB(fileProc->op_file->dest_path);
		fileProc->dest_depth = PlextalkMediaPathDepth(fileProc->op_file->dest_path);
		fileProc->src_depth = fileProc->src_depth - PlextalkMediaPathDepth(fileProc->op_file->src_path);
		DBGMSG("free:%d depth:%d\n", fileProc->dest_free, fileProc->dest_depth);
		DBGMSG("srcdepth:%d\n",fileProc->src_depth);

		if(fileProc->checkExist && fileProc->alreadyExist){
			DBGMSG("already exist\n");
			int ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("The file name already exists.Overwrite?"));
			if (ret != MSGBX_BTN_OK){
				fileProc->endOfModal = 1;
				fileProc->msgReturn = ERR_COPY_BREAK;
				return;
			}
		}

		if(fileProc->src_size > fileProc->dest_free){
			DBGMSG("No enough space\n");
			fileProc->endOfModal = 1;
			fileProc->msgReturn = ERR_NO_SPACE;
			voice_prompt_abort();
			doVoicePrompt(VPROMPT_AUDIO_BU ,_("Not enough space on the target media."));
			voice_prompt_running = 1;
			return;
		}
		if((fileProc->dest_depth + fileProc->src_depth)>=COPY_MAX_DEPTH){
			DBGMSG("depth over\n");
			fileProc->endOfModal = 1;
			fileProc->msgReturn = ERR_MAX_DEPTH;
			voice_prompt_abort();
			doVoicePrompt(VPROMPT_AUDIO_BU ,_("Cannot paste to the directory.Maximum directory level is 8."));
			voice_prompt_running = 1;
			return;
		}
		startCopyfile();
	}
}

static int calSrcPathSize(void)
{
	char cmd[PATH_MAX+50];

	fileProc->checkExist = 0;
	fileProc->alreadyExist = 0;
	fileProc->src_size = 0;
	if(isRootMedia(fileProc->op_file->src_path)){
		DBGMSG("ROOT MEDIA\n");
		fileProc->checkExist = 1;
		fileProc->alreadyExist = 0;
	}
	snprintf(cmd, sizeof(cmd), "du -ck \"%s\"", fileProc->op_file->src_path);
	fileProc->size_fp = popen(cmd, "r");
	DBGMSG("fileProc->size_fp=%x\n", (unsigned int)fileProc->size_fp);
	fileProc->size_fd = fileno(fileProc->size_fp);
	DBGMSG("fileProc->size_fd=%x\n", fileProc->size_fd);
	fcntl(fileProc->size_fd, F_SETFL, O_NONBLOCK);
	NeuxAppFDSourceRegister(fileProc->size_fd, NULL, size_fd_read, NULL);
	NeuxAppFDSourceActivate(fileProc->size_fd, 1, 0);
	fileProc->pro_state = CAL_SRC_SIZE;
	return 1;
}


static PRO_RESULT copyProDialog(void)
{
	PRO_RESULT ret;
	plextalk_suspend_lock();
	//create wait form
	CreateProcForm();
	calSrcPathSize();
	fileProc->endOfModal = 0;
	fileProc->copyProgress = 0;
	fileProc->src_depth = 0;
	fileProc->dest_depth = 0;
	//voice_prompt_abort();
	origAppMsg = NeuxAppGetCallback(CB_MSG);
	NeuxAppSetCallback(CB_MSG, onAppMsg);

	NeuxDoModal(&fileProc->endOfModal, NULL);
	ret = fileProc->msgReturn;
	NeuxFormDestroy(&fileProc->form);
	DBGMSG("ret:%d\n",ret);
	NeuxAppSetCallback(CB_MSG, origAppMsg);
	return ret;
}

static int deleteFilePro(OP_FILE *op_file)
{
	char cmd[PATH_MAX+10];
//	char msg[256];
	int ret;
	char *filename;

	if(isRootMedia(op_file->src_path)){
		//voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("Failed to delete."));
		goto l_exit;
	}
	
	ret = CoolStorageEnumerate(storage_enum ,op_file->src_path);
	if(1==ret && fileProc->storageInfo.ro)
	{
		DBGMSG("read only\n");
		//voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("The media is locked."));
		goto l_exit;
	}

	//filename = PlextalkGetFilenameFromPath(op_file->src_path);
	//snprintf(msg,sizeof(msg), "%s %s.%s", _("You are about to delete"), filename, _("Are you sure?"));	
	if(PlextalkIsDirectory(op_file->src_path)){
		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("You are about to delete the selected directory.Are you sure?"));
	}else{
		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("You are about to delete the selected file.Are you sure?"));
	}
	if (ret != MSGBX_BTN_OK){
		goto l_exit;
	}

	//del
	snprintf(cmd,sizeof(cmd), "rm -rf \"%s\"", op_file->src_path);
	system(cmd);

	//voice_prompt_abort();
	doVoicePrompt(NULL ,_("Deleting completed."));
	PlextalkGetPathFromFullname(op_file->src_path, op_file->dest_path, PATH_MAX);
	op_file->op_mode = OP_NONE;
	memset(op_file->src_path, 0, sizeof(op_file->src_path));
	return 1;
l_exit:
	op_file->op_mode = OP_NONE;
	memset(op_file->src_path, 0, sizeof(op_file->src_path));
	memset(op_file->dest_path, 0, sizeof(op_file->dest_path));
	return 0;
}

static int cutFilePro(OP_FILE *op_file)
{
	char path[PATH_MAX];
	int ret;
	
	ret = CoolStorageEnumerate(storage_enum ,op_file->src_path);
	if(1==ret && fileProc->storageInfo.ro)
	{
		DBGMSG("read only\n");
		//voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("The media is locked."));
		goto l_exit;
	}
	
	ret = CoolStorageEnumerate(storage_enum ,op_file->dest_path);
	if(1==ret && fileProc->storageInfo.ro)
	{
		DBGMSG("read only\n");
		doVoicePrompt(VPROMPT_AUDIO_BU, _("The media is locked."));
		goto l_exit;
	}

	if(isRootMedia(op_file->src_path)){
		//voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, _("Failed to move."));
		goto l_exit;
	}

	if(!PlextalkIsFileExist(op_file->src_path))
	{	//src no exist
		DBGMSG("src:%s no exist\n", op_file->src_path);
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to move."));
		goto l_exit;
	}

	if(!PlextalkIsFileExist(op_file->dest_path))
	{	//dest no exist
		DBGMSG("dest:%s no exist\n", op_file->src_path);
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to move."));
		goto l_exit;
	}
	
	if(!PlextalkIsDirectory(op_file->dest_path))
	{	
		PlextalkGetPathFromFullname(op_file->dest_path, path, PATH_MAX);
		strcpy(op_file->dest_path, path);
		DBGMSG("dest:%s\n", op_file->dest_path);
	}

	PlextalkGetPathFromFullname(op_file->src_path, path, PATH_MAX);
	DBGMSG("src path:%s\n", path);
	if(0 == strcmp(path, op_file->dest_path)){
		//dest equ src
		DBGMSG("src equ dest\n");
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to move."));
		goto l_exit;
	}

	char *filename = PlextalkGetFilenameFromPath(op_file->src_path);
	PlextalkCatFilename2Path(path, PATH_MAX, op_file->dest_path, filename);
	DBGMSG("destfile:%s\n", path);
	if(PlextalkIsFileExist(path)){
		//char msg[256];
		//snprintf(msg,sizeof(msg), "%s %s", filename, _("already exists.Overwrite? "));
		if(PlextalkIsDirectory(op_file->src_path)){
			ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("The directory name already exists.Overwrite?"));
		}else{
			ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("The file name already exists.Overwrite?"));
		}
		
		if (ret != MSGBX_BTN_OK){
			goto l_exit;
		}
	}

	MEDIA_TYPE srctype = getMediaType(op_file->src_path);
	MEDIA_TYPE desttype = getMediaType(op_file->dest_path);
	if(srctype== desttype){
		//move only
		char cmd[2*PATH_MAX+10];
		DBGMSG("move\n");
		snprintf(cmd,sizeof(cmd), "mv \"%s\" \"%s\"", op_file->src_path, op_file->dest_path);
		system(cmd);

		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Moving completed."));
		goto l_exit;
	}
	
	PRO_RESULT proret = copyProDialog();
	if(RET_OK == proret){
		char cmd[PATH_MAX+10];
		DBGMSG("del src file\n");
		snprintf(cmd,sizeof(cmd), "rm -rf \"%s\"", op_file->src_path);
		system(cmd);
		voice_prompt_abort();
		doVoicePrompt(NULL ,_("Moving completed."));
		goto l_exit;
	}

	doVoicePrompt(NULL ,_("Failed to move."));
	if(ERR_DEST_RM == proret || ERR_SRC_RM == proret){
		op_file->op_mode = OP_NONE;
		strlcpy(op_file->dest_path, MEDIA_INNER,PATH_MAX);
		return 0;
	}
l_exit:
	op_file->op_mode = OP_NONE;
	memset(op_file->src_path, 0, sizeof(op_file->src_path));
	memset(op_file->dest_path, 0, sizeof(op_file->dest_path));
	return 0;
}

static int copyFilePro(OP_FILE *op_file)
{
	char path[PATH_MAX];
	int ret;
	
	ret = CoolStorageEnumerate(storage_enum ,op_file->dest_path);
	if(1==ret && fileProc->storageInfo.ro)
	{
		DBGMSG("read only\n");
		doVoicePrompt(VPROMPT_AUDIO_BU, _("The media is locked."));
		goto l_exit;
	}

	if(!PlextalkIsFileExist(op_file->src_path))
	{	//src no exist
		DBGMSG("src:%s no exist\n", op_file->src_path);
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to copy."));
		goto l_exit;
	}

	if(!PlextalkIsFileExist(op_file->dest_path))
	{	//dest no exist
		DBGMSG("dest:%s no exist\n", op_file->src_path);
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to copy."));
		goto l_exit;
	}
	
	if(!PlextalkIsDirectory(op_file->dest_path))
	{	
		PlextalkGetPathFromFullname(op_file->dest_path, path, PATH_MAX);
		strlcpy(op_file->dest_path, path, PATH_MAX);
		DBGMSG("dest:%s\n", op_file->dest_path);
	}

	if(isRootMedia(op_file->src_path)){
		strlcpy(path, op_file->src_path, PATH_MAX);
	}else{
		PlextalkGetPathFromFullname(op_file->src_path, path, PATH_MAX);
	}
	DBGMSG("src path:%s\n", path);
	if(0 == strcmp(path, op_file->dest_path)){
		//dest equ src
		DBGMSG("src equ dest\n");
		//voice_prompt_abort();
		doVoicePrompt(NULL ,_("Failed to copy."));
		goto l_exit;
	}
	
	if(!isRootMedia(op_file->src_path)){
		char *filename = PlextalkGetFilenameFromPath(op_file->src_path);
		PlextalkCatFilename2Path(path, PATH_MAX, op_file->dest_path, filename);
		DBGMSG("destfile:%s\n", path);
		if(PlextalkIsFileExist(path)){
			//char msg[256];
			//snprintf(msg,sizeof(msg), "%s %s", filename, _("already exists.Overwrite? "));
			//ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, msg);
			if(PlextalkIsDirectory(op_file->src_path)){
				ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("The directory name already exists.Overwrite?"));
			}else{
				ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 1, _("The file name already exists.Overwrite?"));
			}
			
			if (ret != MSGBX_BTN_OK){
				goto l_exit;
			}
		}
	}
	
	PRO_RESULT proRet = copyProDialog();
	if(RET_OK == proRet){
		voice_prompt_abort();
		doVoicePrompt(NULL ,_("Copying completed."));
		goto l_exit;
	}
	
	doVoicePrompt(NULL ,_("Failed to copy."));
	if(ERR_DEST_RM == proRet || ERR_SRC_RM == proRet){
		op_file->op_mode = OP_NONE;
		strlcpy(op_file->dest_path, MEDIA_INNER,PATH_MAX);
		return 0;
	}
l_exit:
	op_file->op_mode = OP_NONE;
	memset(op_file->src_path, 0, sizeof(op_file->src_path));
	memset(op_file->dest_path, 0, sizeof(op_file->dest_path));
	return 0;
}

int file_operate(OP_FILE *op_file)
{
	fileProc = (FILE_PROC*)malloc(sizeof(FILE_PROC));
	if(!fileProc){
		return 0;
	}
	memset(fileProc, 0, sizeof(FILE_PROC));
	fileProc->op_file = op_file;
	
	if(OP_DELETE == op_file->op_mode){	//delete
		deleteFilePro(op_file);
	}

	if(OP_CUT == op_file->op_mode)
	{
		cutFilePro(op_file);
	}
	
	if(OP_COPY == op_file->op_mode)
	{
		copyFilePro(op_file);
	}

	if(fileProc){
		free(fileProc);
		fileProc = NULL;
	}
	return 0;
}


