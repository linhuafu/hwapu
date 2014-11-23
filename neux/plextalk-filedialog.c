/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#include <linux/unistd.h>
#include <linux/inotify.h>
#include <errno.h>
#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "file-helper.h"
#include "plextalk-config.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include <microwin/device.h>
#include "libinfo.h"
#include "application-ipc.h"
#include "desktop-ipc.h"
#include "neux.h"



// form handle globally available.
static FORM * form;
static WIDGET *flbox1;
static WIDGET *timer1;

static void (*origAppMsg)(const char * src, neux_msg_t * msg);
static int (*funcValidate)(const char * path);


static int filtertype;
static int indexItem;
static int endOfModal = -1;
static int iMsgReturn = MSGBX_BTN_CANCEL;
static char strbuf[PATH_MAX];

//static int refreshmode;
//static char refresh_media[PATH_MAX];

static int inotify_fd = -1;
static int inotify_wd = -1;

//km
static int *file_tts_running = NULL;
static int isininfo = 0;

static void
GetSelectedFiles(char ***selectedfiles, int *selectedcount)
{
	int i;
	int *selectedarray;

	*selectedfiles = NULL;
	*selectedcount = NeuxListboxGetMultiSelectedCount(flbox1);
	if( *selectedcount == 0 )
	{
		*selectedcount = 1;
		*selectedfiles = (char**)malloc(sizeof(char*)*(*selectedcount));
		(*selectedfiles)[0] = strdup(strbuf);
	}
	else
	{
		selectedarray = malloc(sizeof(int)*(*selectedcount));
		if( NeuxListboxGetMultiSelectedData(flbox1,selectedarray,*selectedcount) == *selectedcount )
		{
			*selectedfiles = (char**)malloc(sizeof(char*)*(*selectedcount));
			for( i=0; i<*selectedcount; i++ )
				(*selectedfiles)[i] = strdup(NeuxFileListboxGetFullNameFromIndex(flbox1,selectedarray[i]));
		}
		free(selectedarray);
	}
}

static void
UpdateMediaIcon(const char *path)
{
	if (StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "/mmcblk0"))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
	else if (StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "/mmcblk"))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	else if (StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "/sd") ||
             StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "/sr") ||
             StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "/cdda"))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
	else
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
}

static void
UpdateTitle(const char *path)
{
	char* ptr;
	char* title;

	ptr = path + strlen(PLEXTALK_MOUNT_ROOT_STR);
	if (*ptr == '\0') {
		title = _("Media");
	} else {
		/* *ptr must be '/' */
		ptr++;

		title = strchr(ptr, '/');
		if (title == NULL) {
			if (StringStartWith(ptr, "mmcblk0"))
				title = _("Internal Memory");
			else if (StringStartWith(ptr, "mmcblk"))
				title = _("SD Card");
			else if (StringStartWith(ptr, "sd"))
				title = _("USB Memory");
			else if (StringStartWith(ptr, "sr"))
				title = _("CDROM");
			else if (!strcmp(ptr, "cdda"))
				title = _("CDDA");
			else
				title = ptr;
		} else {
			title = strrchr(ptr, '/') + 1;
		}
	}

	NhelperTitleBarSetContent(title);
}

static void
doVoicePrompt(const char* lead)
{
	char *curr = NeuxFileListboxGetCurrName(flbox1);

	if (lead != NULL)
		voice_prompt_music(0, &vprompt_cfg, lead);

	if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
		if (StringStartWith(curr, "mmcblk0"))
			curr = _("Internal Memory");
		else if (StringStartWith(curr, "mmcblk"))
			curr = _("SD Card");
		else if (StringStartWith(curr, "sd"))
			curr = _("USB Memory");
		else if (StringStartWith(curr, "sr"))
			curr = _("CDROM");
		else if (!strcmp(curr, "cdda"))
			curr = _("CDDA");
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, curr);
	} else {
		int len = strlen(curr);
		int lang = VoicePromptDetectLanguage(curr, len);
		voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					lang, plextalk_get_tts_role(lang), curr, len);
	}
}

//km start
static void
doLimitLayerPrompt(const char* lead)
{
	char *curr = _("the current folder is 8th hierarchy, can not move to the lower folder.");

	if (lead != NULL)
		voice_prompt_music(0, &vprompt_cfg, lead);

	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, curr);
}

static int
isFileListMaxDepth(FILELISTBOX * flbox)
{
	int cnt = 0;
	char *curr = NeuxFileListboxGetFullName(flbox1);
	if(PlextalkIsDirectory(curr))
	{
		while(*curr)
		{
			if(*curr++ == '/')
				cnt++;
		}
	}

	return (cnt >= 10);//appk isFileListboxMaxDepth too
}



static void
doFileInfomation(FILELISTBOX * flbox)
{
	char *info_str,*info1,*info2;
	void* info_id;
	int bytes = PATH_MAX+256;
	int types;
	int info_item_num = 0;
	char* file_info[10];
	char* ptr;
	char* title;
	int is_empty =  (NeuxFileListboxGetEntCount(flbox) < 1);

	if(is_empty)
	{
		/* Start info */
		info_id = info_create();
		info_start (info_id, file_info, info_item_num, file_tts_running);
		info_destroy (info_id);
		return;
	}
	

	info_str = (char*)malloc(2*bytes+2);//because of the info's limits,two info lines must malloc two buffer
	memset(info_str,0x00,2*bytes+2);
	info1 = info_str;
	info2 = info_str+bytes;

	types = NeuxFileListboxGetCurrType(flbox);
	ptr = NeuxFileListboxGetCurrName(flbox);

	if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
		if (StringStartWith(ptr, "mmcblk0"))
			ptr = _("Internal Memory");
		else if (StringStartWith(ptr, "mmcblk1"))
			ptr = _("SD Card");
		else if (StringStartWith(ptr, "sda"))
			ptr = _("USB Memory");
		else if (!strcmp(ptr, "sr0"))
			ptr = _("CDROM");
		else if (!strcmp(ptr, "cdda"))
			ptr = _("CDDA");
	}

	//Name of the selected file or folder.
	//modified by km start
		//Name of the selected file or folder.
		if(types == DT_DIR)
		{
			strlcpy(info1,_("The current selected directory is:"),bytes);
		}
		else
		{
			strlcpy(info1,_("The current selected file is:"),bytes);
		}	
		strlcat(info1, ptr, bytes);
	
		//The name of the parent folder of the selected file or folder.
		//if(types == DT_DIR)
		{
			strlcpy(info2,_("The stored directory is:"),bytes);
		}
		//else
		//{
		//	strlcpy(info2,_("The name of the parent folder of the selected file:"),bytes);
		//}
		//modified by km end


	ptr = strbuf + strlen(PLEXTALK_MOUNT_ROOT_STR);
	if (*ptr == '\0') {
		title = _("Media");
	} else {
		/* *ptr must be '/' */
		ptr++;

		title = strchr(ptr, '/');
		if (title == NULL) {
			if (StringStartWith(ptr, "mmcblk0"))
				title = _("Internal Memory");
			else if (StringStartWith(ptr, "mmcblk1"))
				title = _("SD Card");
			else if (StringStartWith(ptr, "sda"))
				title = _("USB Memory");
			else if (!strcmp(ptr, "sr0"))
				title = _("CDROM");
			else if (!strcmp(ptr, "cdda"))
				title = _("CDDA");
			else
				title = ptr;
		} else {
			title = strrchr(ptr, '/') + 1;
		}
	}
	
	strlcat(info2,title,bytes);

	file_info[info_item_num] = info1;
	info_item_num ++;
	file_info[info_item_num] = info2;
	info_item_num ++;

	/* Start info */
	info_id = info_create();
	info_start (info_id, file_info, info_item_num, file_tts_running);
	info_destroy (info_id);

	free(info_str);

}
//km end

//km start
static void
OnFlboxKeydown(WID__ wid, KEY__ key)
{

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP) || isininfo)//added by km
		return;

	voice_prompt_abort();

	if (NeuxAppGetKeyLock(0)) {

		 DBGMSG("FileDialog the key is locked!\n");
		 
		 voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		 voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Keylock"));

		 return;
	 }
	switch (key.ch)
	{
	case MWKEY_ENTER:
		if (filtertype == FL_TYPE_MUSIC || filtertype == FL_TYPE_BOOK)
			goto _confirm;
		if (strcmp(strbuf, NeuxFileListboxGetPath(flbox1))) {			
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR))
				UpdateMediaIcon(NeuxFileListboxGetPath(flbox1));
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			UpdateTitle(strbuf);
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);//play the error voice,not the empty voice
				indexItem = 0;
			} else {
				indexItem = NeuxListboxSelectedItem(flbox1);
				doVoicePrompt(VPROMPT_AUDIO_PU);
			}

		} else {
_confirm:
			if (filtertype == FL_TYPE_BOOK && NeuxFileListBoxIsFolder(flbox1)) {//add by appk, the book only the right key can enter the folder
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);//play the error voice,not the empty voice
				return;
			}
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);//play the error voice,not the empty voice
				return;
			}else if(isFileListMaxDepth(flbox1)) {
				doLimitLayerPrompt(VPROMPT_AUDIO_BU);
				return;
			}
			if (funcValidate != NULL && funcValidate(NeuxFileListboxGetFullName(flbox1)) == 0) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
				return;
			}
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Enter."));
			strlcpy(strbuf, NeuxFileListboxGetFullName(flbox1), sizeof(strbuf));
			iMsgReturn = MSGBX_BTN_YES;
			endOfModal = 1;

		}
		break;

	case MWKEY_RIGHT://only the enter key can confirm the selected file
		if (strcmp(strbuf, NeuxFileListboxGetPath(flbox1))) {
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR))
				UpdateMediaIcon(NeuxFileListboxGetPath(flbox1));
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			UpdateTitle(strbuf);
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);//lhf
				indexItem = 0;
			} else {
				indexItem = NeuxListboxSelectedItem(flbox1);
				doVoicePrompt(VPROMPT_AUDIO_PU);
			}
		}else {
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
				return;
			}else if(isFileListMaxDepth(flbox1)) {
				doLimitLayerPrompt(VPROMPT_AUDIO_BU);
				return;
			}
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Cannot move the directory"));
			return;
		}
		break;

		
	case MWKEY_LEFT:
		if( !strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR) ) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Cannot move the media"));
		}
		else {
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			if( !strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR) )
				UpdateMediaIcon(NeuxFileListboxGetPath(flbox1));
			UpdateTitle(strbuf);
			indexItem = NeuxListboxSelectedItem(flbox1);
			doVoicePrompt(VPROMPT_AUDIO_PU);
		}
		break;
	case MWKEY_UP:
	case MWKEY_DOWN:
		if (indexItem == NeuxListboxSelectedItem(flbox1)) {
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
				return;
			} else
				doVoicePrompt(VPROMPT_AUDIO_KON);
		} else {
			indexItem = NeuxListboxSelectedItem(flbox1);
			doVoicePrompt(VPROMPT_AUDIO_PU);
		}
		break;
	case VKEY_BOOK:
		if (filtertype == FL_TYPE_BOOK) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			iMsgReturn = MSGBX_BTN_CANCEL;
			endOfModal = 1;
		}
//		else
//			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	case VKEY_MUSIC:
		if (filtertype == FL_TYPE_MUSIC) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			iMsgReturn = MSGBX_BTN_CANCEL;
			endOfModal = 1;
		}
//		else
//			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
//	default:
//		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
//		break;
	case MWKEY_INFO:
		NhelperTitleBarSetContent(_("Information"));//set title
		voice_prompt_abort();
		isininfo = 1;//added
		doFileInfomation(flbox1);//display information
		isininfo = 0;
		UpdateTitle(strbuf);//resum title
		break;


	}
}
//km end


#define WAIT_MEDIA_READY_TIME	2000
static void
OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	if (device != MWHOTPLUG_DEV_SCSI_CDROM || filtertype != FL_TYPE_MUSIC)
		return;

	/* special handle for CDDA */
	if (action == MWHOTPLUG_ACTION_CHANGE || action == MWHOTPLUG_ACTION_REMOVE)
		NeuxTimerSetControl(timer1, WAIT_MEDIA_READY_TIME, 1);
}

static void
OnWindowTimer(WID__ wid)
{
	int ret = CoolCDDAGetTrackCount();
	char media[PATH_MAX];

	if (ret > 0) {
		if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
			/* redraw listbox when we in root dir */
			strlcpy(media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
			strlcat(media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
			NeuxFileListboxSetPath(flbox1, media, filtertype);
			return;
		}
	} else {
		if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
			if (!strcmp(NeuxFileListboxGetCurrName(flbox1), "cdda"))
				NeuxFileListboxSetPath(flbox1, PLEXTALK_MOUNT_ROOT_STR, filtertype);
			else {
				strlcpy(media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
				strlcat(media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
				NeuxFileListboxSetPath(flbox1, media, filtertype);
				return;
			}
		} else if (StringStartWith(strbuf, PLEXTALK_MOUNT_ROOT_STR "/cdda")) {
			NeuxFileListboxSetPath(flbox1, PLEXTALK_MOUNT_ROOT_STR, filtertype);
			strlcpy(strbuf, PLEXTALK_MOUNT_ROOT_STR, sizeof(strbuf));
			UpdateMediaIcon(strbuf);
			UpdateTitle(strbuf);
		}
		indexItem = NeuxListboxSelectedItem(flbox1);
		voice_prompt_abort();
		doVoicePrompt(NULL);
	}
}


//zzx --start--
static void
MediaMountRootChange(void *dummy)
{
#define MAX_BUF_SIZE 1024
	char media[PATH_MAX];
	char buffer[MAX_BUF_SIZE];
	struct inotify_event * event;
	int offset = 0;
	int len, tmp_len;

	while ((len = read(inotify_fd, buffer + offset, MAX_BUF_SIZE)) > 0) {
		len += offset;

		event = (struct inotify_event *)buffer;
		while (((char *)event - buffer + sizeof(struct inotify_event)) <= len) {
			if ((char *)event - buffer + sizeof(struct inotify_event) + event->len > len) {
				offset = len - ((char *)event - buffer);
				memmove(buffer, event, offset);
				break;
			}
			if (event->mask & IN_CREATE) {
				if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
					/* redraw listbox when we in root dir */
					strlcpy(media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
					strlcat(media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
					NeuxFileListboxSetPath(flbox1, media, filtertype);
				}
			} else {
				if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
					/* redraw listbox when we in root dir */
					if (!strcmp(NeuxFileListboxGetCurrName(flbox1), event->name))
						NeuxFileListboxSetPath(flbox1, PLEXTALK_MOUNT_ROOT_STR, filtertype);
					else {
						strlcpy(media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
						strlcat(media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
						NeuxFileListboxSetPath(flbox1, media, filtertype);
						goto next;
					}
				} else {
					if (!strncmp(strbuf + strlen(PLEXTALK_MOUNT_ROOT_STR "/"), event->name, strlen(event->name)) &&
					    (strbuf[strlen(PLEXTALK_MOUNT_ROOT_STR "/") + strlen(event->name)] == '\0' ||
					     strbuf[strlen(PLEXTALK_MOUNT_ROOT_STR "/") + strlen(event->name)] == '/')) {
						NeuxFileListboxSetPath(flbox1, PLEXTALK_MOUNT_ROOT_STR, filtertype);
						strlcpy(strbuf, PLEXTALK_MOUNT_ROOT_STR, sizeof(strbuf));
						UpdateMediaIcon(strbuf);
						UpdateTitle(strbuf);
					} else
						goto next;
				}
				indexItem = NeuxListboxSelectedItem(flbox1);
				voice_prompt_abort();
				doVoicePrompt(NULL);
			}

next:
			event = (char *)event + sizeof(struct inotify_event) + event->len;
		}
	}
}

static int
MediaMountRootMonitorOn(void)
{
	inotify_fd = inotify_init1(IN_NONBLOCK);
	if (inotify_fd < 0) {
		DBGMSG("Fail to initialize inotify.\n");
		return -1;
	}

	inotify_wd = inotify_add_watch(inotify_fd, PLEXTALK_MOUNT_ROOT_STR, IN_CREATE | IN_DELETE);
	if (inotify_wd < 0) {
		DBGMSG("Can't add watch for %s.\n", PLEXTALK_MOUNT_ROOT_STR);
		close(inotify_fd);
		inotify_fd = -1;
		return -1;
	}

	NeuxAppFDSourceRegister(inotify_fd, NULL, MediaMountRootChange, NULL);
	NeuxAppFDSourceActivate(inotify_fd, 1, 0);

	return 0;
}


static void
MediaMountRootMonitorOff(void)
{
	if (inotify_fd < 0)
		return;

	NeuxAppFDSourceActivate(inotify_fd, 0, 0);
	NeuxAppFDSourceUnregister(inotify_fd);
	close(inotify_fd);
	inotify_fd = -1;
}
//zzx ---end---

static const char*
GetFatherDir(const char* f_path)
{
	static char father_dir[512];
	char dir_path[512];
	
	memset(dir_path, 0, 512);
	snprintf(dir_path, 512, "%s", f_path);
	
	memset(father_dir, 0, 512);
	/*there are two '/' in album dir_path*/
	int size = strlen(dir_path);
	
	if (*(dir_path + size - 1) == '/') 
	{
		*(dir_path + size - 1) = 0;
	}
	char* p = strrchr(dir_path, '/');
	int copy_size = p - dir_path;
	strncpy(father_dir, dir_path, copy_size);
	
	return father_dir;
}


static void
ReCheckFileDialog ()
{
	char *cur_fpath = NeuxFileListboxGetFullNameSafe(flbox1);		//for a bug
	
	if (PlextalkIsFileExist(cur_fpath)) {	
		NeuxFileListboxSetPath(flbox1, cur_fpath, filtertype);
	} else {
		char *p_dir = cur_fpath;
		do {
			p_dir = GetFatherDir(p_dir);
			if (!p_dir)
				p_dir = PLEXTALK_MOUNT_ROOT_STR;
		} while(!PlextalkIsFileExist(p_dir));

		NeuxFileListboxSetPath(flbox1, p_dir, filtertype);
	} 

	strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
	UpdateTitle(strbuf);
}


static void
OnWindowGetFocus(WID__ wid)
{
	DBGMSG("Get Focus\n");
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_SELECT);

//km start	
	if(filtertype == FL_TYPE_BOOK){
		NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BOOK);
	}else if(filtertype == FL_TYPE_MUSIC){
		NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MUSIC);
	}

	if(strlen(strbuf) <= 0){
		strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
	}
	DBGMSG("Get Focus strbuf:%s\n",strbuf);
	UpdateMediaIcon(strbuf);
	UpdateTitle(strbuf);
}
//km end

static void
OnWindowDestroy(WID__ wid)
{
	DBGMSG("---------------window destroy %d.\n",wid);
}


static void
hookAppMsg(const char * src, neux_msg_t * msg)
{
	
	DBGMSG("hookAppMsg enter!! \n");
	if (origAppMsg != NULL)
		origAppMsg(src, msg);

	if (msg->msg.msg.msgId == PLEXTALK_APP_MSG_ID &&
		flbox1 != NULL) {
		app_rqst_t *rqst = msg->msg.msg.msgTxt;
		widget_prop_t wprop;
		listbox_prop_t lsprop;	//add by lhf
		DBGMSG("hookAppMsg rqst->type:%d \n",rqst->type);

		switch (rqst->type) {
		case APPRQST_RESYNC_SETTINGS:
		case APPRQST_SET_FONTSIZE:
			NeuxWidgetSetFont(form, sys_font_name, sys_font_size);//app
			NeuxWidgetSetFont(flbox1, sys_font_name, sys_font_size);
			NxListboxRecalcPitems(flbox1);
			if (rqst->type == APPRQST_SET_FONTSIZE)
				break;
			/* fall through */
		case APPRQST_SET_THEME:
			NeuxFileListboxUpdateIcons();
			NeuxWidgetGetProp(flbox1, &wprop);
			wprop.fgcolor = theme_cache.foreground;
			wprop.bgcolor = theme_cache.background;
			NeuxWidgetSetProp(flbox1, &wprop);

			NeuxListboxGetProp(flbox1, &lsprop);	//added by lhf
			lsprop.sbar.fgcolor = theme_cache.foreground;
			lsprop.sbar.bordercolor = theme_cache.foreground;
			lsprop.sbar.bgcolor = theme_cache.background;
			lsprop.selectmarkcolor = theme_cache.selected;
			lsprop.selectedcolor = theme_cache.selected;
			lsprop.selectedfgcolor = theme_cache.foreground;
			NeuxListboxSetProp(flbox1, &lsprop);//add by lhf
			break;
		case APPRQST_SET_LANGUAGE:
			NeuxWidgetSetFont(form, sys_font_name, sys_font_size);
			NeuxWidgetSetFont(flbox1, sys_font_name, sys_font_size);
			break;
		case APPRQST_SET_STATE://added by lhf
			{
				app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
				DBGMSG("set state:%d\n", rqst->pause);

				if (!rqst->pause)	
					ReCheckFileDialog();			//recover时需要重新check一下 

				if(!isininfo){
					return;
				}
				if(rqst->pause){
					info_pause();
				}else{
					info_resume();
				}
			}//add by lhf end
			break;

		default:
			return;
		}

		DBGMSG("hookAppMsg indexItem:%d \n",indexItem);
		NxNeuxListboxSetIndex(flbox1, indexItem);
	}
}


static void
CreateFormFileDialog(const char* path, int type, GR_BOOL multisel)
{
	listbox_prop_t lsprop;
	widget_prop_t wprop;

	form = NeuxFormNew( MAINWIN_LEFT,
						MAINWIN_TOP,
						MAINWIN_WIDTH,
						MAINWIN_HEIGHT);

	NeuxFormSetCallback(form, CB_KEY_DOWN, OnFlboxKeydown);
	NeuxFormSetCallback(form, CB_FOCUS_IN, OnWindowGetFocus);
	NeuxFormSetCallback(form, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(form, CB_HOTPLUG,  OnWindowHotplug);

	timer1 = NeuxTimerNewWithCallback(form, 0, 0, OnWindowTimer);

	flbox1 = NeuxFileListboxNew(form, 0,  0, MAINWIN_WIDTH, MAINWIN_HEIGHT,
							path, type, multisel);

	NeuxFileListboxSetCallback(flbox1, CB_KEY_DOWN, OnFlboxKeydown);

	NeuxWidgetSetFont(flbox1, sys_font_name, sys_font_size);

	NeuxWidgetGetProp(flbox1, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(flbox1, &wprop);

	NeuxWidgetShow(flbox1, TRUE);

	NeuxWidgetShow(form, TRUE);
	NxSetWidgetFocus(form);
}

/*
  NeuxFileDialogOpen is modal, thus NeuxFileDialogOpen will not return before any button
  is clicked.
 */

static int CheckCDDAPath(const char *path)
{
	int track_count;
	int track_number;
	char *end;

	track_count = CoolCDDAGetTrackCount();
	if (track_count < 0)
		return 0;

	if (path[strlen(PLEXTALK_MOUNT_ROOT_STR "/cdda")] == '\0')
		return 1;

	if (path[strlen(PLEXTALK_MOUNT_ROOT_STR "/cdda")] != '/')
		return 0;

	if (path[strlen(PLEXTALK_MOUNT_ROOT_STR "/cdda/")] == '\0')
		return 1;

	if (!StringStartWith(path + strlen(PLEXTALK_MOUNT_ROOT_STR "/cdda/"), "Track "))
		return 0;

	track_number = strtol(path + strlen(PLEXTALK_MOUNT_ROOT_STR "/cdda/Track "),
						&end, 10);
	if (track_number > track_count || *end != '\0')
		return 0;

	return 1;
}

/**
 * Function creates NeuxFileDialogOpen widget.
 *
 * @param title
 *      NeuxFileDialogOpen title if existed.
 * @param filtertype
 * 		Open type, FL_TYPE_BOOK or FL_TYPE_MUSIC
 * @param filterfun
 * 		filter callback function
 * @param path
 *      it is default path and filename
 * @param selectedfiles
 *      return selected files array, you must call
 *      NeuxFileDialogFreeSelections to free selectedfiles
 * @param selectedcount
 *      return selected files count
 * @return
 *      button clicked.
 *
 */
int NeuxFileDialogOpenEx(const char *path, int _filtertype, GR_BOOL multisel,
						char ***selectedfiles, int *selectedcount,
						void (*onMsg)(const char * src, /*neux_msg_t*/void * msg),
						int *tts_running,	//km
						int (*validate)(const char *path))
{
	if (form != NULL)
		return -1;

	if (MediaMountRootMonitorOn() < 0)		//zzx
		return -1;
	
	if (path != NULL) {
		if (!StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR) ||
			(path[strlen(PLEXTALK_MOUNT_ROOT_STR)] != '\0' &&
			 path[strlen(PLEXTALK_MOUNT_ROOT_STR)] != '/'))
			return -1;
	
		if (StringStartWith(path + strlen(PLEXTALK_MOUNT_ROOT_STR), "/cdda")) {
			if (!CheckCDDAPath(path))
				path = PLEXTALK_MOUNT_ROOT_STR;
		} else if (path[strlen(path) - 1] == '/') {
			if (!PlextalkIsDirectory(path))
				path = PLEXTALK_MOUNT_ROOT_STR;
		} else {
			if (!PlextalkIsFileExist(path))
				path = PLEXTALK_MOUNT_ROOT_STR;
		}
	} else
		path = PLEXTALK_MOUNT_ROOT_STR;


	file_tts_running = tts_running;		//km

	iMsgReturn = MSGBX_BTN_CANCEL;
	filtertype = _filtertype;

	funcValidate = validate;
	origAppMsg = onMsg;
	NeuxWMAppSetCallback(CB_MSG, hookAppMsg);

	CreateFormFileDialog(path, _filtertype, multisel);

	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_SELECT);

	strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
	UpdateMediaIcon(strbuf);
	UpdateTitle(strbuf);
	indexItem = NeuxListboxSelectedItem(flbox1);
	doVoicePrompt(NULL);

	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	if (iMsgReturn == MSGBX_BTN_YES)
		GetSelectedFiles(selectedfiles, selectedcount);

	NeuxFormDestroy(&form);
	MediaMountRootMonitorOff();	//zzx
	form = NULL;		//km
	NeuxWMAppSetCallback(CB_MSG, onMsg);
	return iMsgReturn;
}

void NeuxFileDialogFreeSelections(char ***selectedfiles, int selectedcount)
{
	int i;

	if( *selectedfiles == NULL )
		return;

	for( i=0; i<selectedcount; i++ )
		free((*selectedfiles)[i]);
	free(*selectedfiles);
	*selectedfiles = NULL;
}


//km start
void NeuxRefreshFileDialog()
{
	listbox_prop_t lsprop;
	widget_prop_t wprop;

	DBGMSG("NeuxRefreshFileDialog\n");
	if(form && flbox1)
	{

		CoolShmReadLock(g_config_lock);
		NeuxThemeInit(g_config->setting.lcd.theme);
		plextalk_update_sys_font(g_config->setting.lang.lang);
		plextalk_update_tts(g_config->setting.lang.lang);
		if(filtertype == FL_TYPE_BOOK){
			plextalk_update_lang(g_config->setting.lang.lang, PLEXTALK_IPC_NAME_BOOK);
		}else if(filtertype == FL_TYPE_MUSIC){
			plextalk_update_lang(g_config->setting.lang.lang, PLEXTALK_IPC_NAME_MUSIC);
		}
		CoolShmReadUnlock(g_config_lock);
		
		NeuxWidgetSetFont(form, sys_font_name, sys_font_size);
		NeuxWidgetSetFont(flbox1, sys_font_name, sys_font_size);

		//DBGMSG("NeuxRefreshFileDialog theme_cache:%d,%d\n",theme_cache.foreground,theme_cache.background);
		//DBGMSG("NeuxRefreshFileDialog sys_font_size:%d,%s\n",sys_font_size,sys_font_name);
		NeuxWidgetGetProp(form, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(form, &wprop);
		
		NeuxWidgetGetProp(flbox1, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(flbox1, &wprop);	

		NeuxWidgetShow(form, TRUE);
		NxSetWidgetFocus(form);

		NeuxFileListboxUpdateIcons();
		NxListboxRecalcPitems(flbox1);
		NeuxWidgetRedraw(flbox1);
		NeuxWidgetShow(flbox1, TRUE);
		//NxNeuxListboxSetIndex(flbox1, indexItem);
		DBGMSG("NeuxRefreshFileDialog 2 \n");

		//NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_SELECT);
		//strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
		//UpdateMediaIcon(strbuf);
		//UpdateTitle(strbuf);
		DBGMSG("NeuxRefreshFileDialog strbuf:%s \n",strbuf);
		

	}
}
//km end

