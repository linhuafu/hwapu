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

#define OSD_DBG_MSG
#include "nc-err.h"
//#include "nx-widgets.h"
#include <stdlib.h>
#include <string.h>
#include "widgets.h"
#include "file-helper.h"
#include "Desktop-ipc.h"
#include "plextalk-config.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include <microwin/device.h>
#include "nxutils.h"
#include "Storage.h"
#include "application.h"
#include "application-ipc.h"
#include "neux.h"
#include "libinfo.h"
#include "file_main.h"
//extern int isKeyLock(void);
//extern void initKeyLock(void);
#define MEDIA_INNER				PLEXTALK_MOUNT_ROOT_STR"/mmcblk0p2"
#define MEDIA_SDCRD				PLEXTALK_MOUNT_ROOT_STR"/mmcblk1"
#define MEDIA_USB				PLEXTALK_MOUNT_ROOT_STR"/sd"
#define MEDIA_CDROM				PLEXTALK_MOUNT_ROOT_STR"/sr"
#define MEDIA_CDDA				PLEXTALK_MOUNT_ROOT_STR"/cdda"

static void (*origAppMsg)(const char * src, neux_msg_t * msg);

// form handle globally available.
static FORM * form;
static WIDGET *flbox1;
static WIDGET *timer1;
static TIMER *key_timer;
static int long_pressed_key = -1;

static int refreshmode;
static char refresh_media[PATH_MAX];

static int filtertype;
static int indexItem;
static int dirIsEmpty = 0;	//endOfModal的地方必须赋值
static int endOfModal = -1;
static int iMsgReturn = MSGBX_BTN_CANCEL;
static char strbuf[PATH_MAX];
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
	if (StringStartWith(path, MEDIA_INNER))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
	else if (StringStartWith(path, MEDIA_SDCRD))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	else if (StringStartWith(path, MEDIA_USB) ||
             StringStartWith(path, MEDIA_CDROM) ||
             StringStartWith(path, MEDIA_CDDA))
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
	else
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);	
}

static char *getMediaStr(char *ptr)
{
	char *title;
	if (StringStartWith(ptr, "mmcblk0p2"))
		title = _("Internal Memory");
	else if (StringStartWith(ptr, "mmcblk1"))
		title = _("SD Card");
	else if (StringStartWith(ptr, "sd"))
		title = _("USB Memory");
	else if (StringStartWith(ptr, "sr"))
		title = _("CDROM");
	else if (StringStartWith(ptr, "cdda"))
		title = _("CDDA");
	else
		title = ptr;
	return title;
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
			title = getMediaStr(ptr);
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
		curr = getMediaStr(curr);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, curr);
	} else {
		int len = strlen(curr);
		int lang = VoicePromptDetectLanguage(curr, len);
		//DBGMSG("lang:%d\n",lang);
		voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					lang, plextalk_get_tts_role(lang), curr, len);
	}
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
	

	info_str = (char*)malloc(2*bytes);//because of the info's limits,two info lines must malloc two buffer
	memset(info_str,0x00,2*bytes);
	info1 = info_str;
	info2 = info_str+bytes;

	types = NeuxFileListboxGetCurrType(flbox);
	ptr = NeuxFileListboxGetCurrName(flbox);

	if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
		ptr = getMediaStr(ptr);
	}
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
			title = getMediaStr(ptr);
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

static void
doLimitLayerPrompt(const char* lead)
{
	char *curr = _("the current folder is 8th hierarchy, can not move to the lower folder.");

	voice_prompt_abort();
	
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

	return (cnt >= 10);
}



static void
OnFlboxKeydown(WID__ wid, KEY__ key)
{
	if(checkFileExit()){
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP) || isininfo)
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Keylock"));
		return;
	}

	switch (key.ch)
	{
	case MWKEY_ENTER:
		{
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);			
			if (NeuxFileListboxGetEntCount(flbox1) < 1){
				strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
				dirIsEmpty = 1;
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Read error.It may not access the media correctly."));				
				return;
			}else{
				strlcpy(strbuf, NeuxFileListboxGetFullName(flbox1), sizeof(strbuf));
				dirIsEmpty = 0;
			}
			iMsgReturn = MSGBX_BTN_YES;
			endOfModal = 1;
			break;
		}
		/* fall through */
	case MWKEY_RIGHT:
		voice_prompt_abort();
		if (strcmp(strbuf, NeuxFileListboxGetPath(flbox1))) {
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR))
				UpdateMediaIcon(NeuxFileListboxGetPath(flbox1));
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			UpdateTitle(strbuf);
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
				indexItem = 0;
			} else {
				indexItem = NeuxListboxSelectedItem(flbox1);
				voice_prompt_abort();
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
			
			/*if (NeuxFileListboxGetEntCount(flbox1) < 1) {*/
			//voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Cannot move the directory."));
			//}
		}
		break;
	case MWKEY_LEFT:
		if( !strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR) ){
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Cannot move the media."));
		}
		else {
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			if( !strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR) )
				UpdateMediaIcon(NeuxFileListboxGetPath(flbox1));
			UpdateTitle(strbuf);
			indexItem = NeuxListboxSelectedItem(flbox1);
			voice_prompt_abort();
			doVoicePrompt(VPROMPT_AUDIO_PU);
		}
		break;
	case MWKEY_UP:
	case MWKEY_DOWN:
		if (indexItem == NeuxListboxSelectedItem(flbox1)) {
			if (NeuxFileListboxGetEntCount(flbox1) < 1) {
				voice_prompt_abort();
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
				return;
			} else{
				voice_prompt_abort();
				doVoicePrompt(VPROMPT_AUDIO_KON);
			}
		} else {
			indexItem = NeuxListboxSelectedItem(flbox1);
			voice_prompt_abort();
			doVoicePrompt(VPROMPT_AUDIO_PU);
		}
		break;
	case MWKEY_INFO:
		{
			NhelperTitleBarSetContent(_("Information"));//set title
			voice_prompt_abort();
			isininfo = 1;
			doFileInfomation(flbox1);//display information
			isininfo = 0;
			UpdateTitle(strbuf);//resum title
			NeuxWidgetShow(flbox1, TRUE);//show
			NeuxWidgetShow(form, TRUE);
			NeuxWidgetFocus(form);
			//initKeyLock();
		}
		break;
	case MWKEY_MENU:
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;
		
	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RADIO:
	case VKEY_RECORD:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
		
	default:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	}
}

static void
OnFlboxKeyup(WID__ wid, KEY__ key)
{
	if(checkFileExit()){
		return;
	}

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
	case MWKEY_MENU:
		FileManagementExit();
		break;
#if 0		
	case MWKEY_INFO:
		{
			NhelperTitleBarSetContent(_("Information"));//set title
			voice_prompt_abort();
			doFileInfomation(flbox1);//display information
			UpdateTitle(strbuf);//resum title
			NeuxWidgetShow(flbox1, TRUE);//show
			NeuxWidgetShow(form, TRUE);
			NeuxWidgetFocus(form);
			initKeyLock();
		}
		break;
#endif
	default:
		break;
	}
}

extern char *strchrnul (const char *__s, int __c);

static void
OnWindowHotplug(WID__ wid, int device, int index, int action)
{
	DBGMSG("Hotplug device=%d action=%d\n",device,action);
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		if (device != MWHOTPLUG_DEV_SCSI_CDROM &&
			!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
			refreshmode = 0;
			strlcpy(refresh_media, PLEXTALK_MOUNT_ROOT_STR, PATH_MAX);
			NeuxTimerSetControl(timer1, 2000, 1);
		}
		break;

	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
				refreshmode = !StringStartWith(NeuxFileListboxGetCurrName(flbox1), "mmcblk1")
								? 0 : 1;
				strlcpy(refresh_media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
				strlcat(refresh_media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
			} else if (StringStartWith(strbuf, PLEXTALK_MOUNT_ROOT_STR "/mmcblk1")){
				refreshmode = 1;
				strlcpy(refresh_media, strbuf,
						strchrnul(strbuf + strlen(PLEXTALK_MOUNT_ROOT_STR "/"), '/') - strbuf + 1);
			} else{
				return;
			}
			break;
		case MWHOTPLUG_DEV_SCSI_DISK:
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
				refreshmode = !StringStartWith(NeuxFileListboxGetCurrName(flbox1), "sd")
								? 0 : 1;
				strlcpy(refresh_media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
				strlcat(refresh_media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
			} else if (StringStartWith(strbuf, PLEXTALK_MOUNT_ROOT_STR "/sd")){
				refreshmode = 1;
				strlcpy(refresh_media, strbuf,
						strchrnul(strbuf + strlen(PLEXTALK_MOUNT_ROOT_STR "/"), '/') - strbuf + 1);
			} else{
				return;
			}
			break;
		case MWHOTPLUG_DEV_SCSI_CDROM:
cdrom_remove:
			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
				refreshmode = (!StringStartWith(NeuxFileListboxGetCurrName(flbox1), "sr") &&
				               strcmp(NeuxFileListboxGetCurrName(flbox1), "cdda") != 0)
								? 0 : 1;
				strlcpy(refresh_media, PLEXTALK_MOUNT_ROOT_STR "/", PATH_MAX);
				strlcat(refresh_media, NeuxFileListboxGetCurrName(flbox1), PATH_MAX);
			} else if (StringStartWith(strbuf, PLEXTALK_MOUNT_ROOT_STR "/sr") ||
				       StringStartWith(strbuf, PLEXTALK_MOUNT_ROOT_STR "/cdda"))
			{
				refreshmode = 1;
				strlcpy(refresh_media, strbuf,
						strchrnul(strbuf + strlen(PLEXTALK_MOUNT_ROOT_STR "/"), '/') - strbuf + 1);
			} else{
				return;
			}
			break;
		default:
			return;
		}
		DBGMSG("refresh_media:%s\n",refresh_media);
		if(refreshmode){
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("The media is removed.Removing media while accessing could destroy data on the card."));
		}
		NeuxTimerSetControl(timer1, 2000, 1);
		break;

	case MWHOTPLUG_ACTION_CHANGE:
		if (device == MWHOTPLUG_DEV_SCSI_CDROM)
		{
			int ret = CoolCdromMediaPresent();
			if (ret <= 0)
				goto cdrom_remove;

			if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
				refreshmode = 0;
				strlcpy(refresh_media, PLEXTALK_MOUNT_ROOT_STR, PATH_MAX);
				NeuxTimerSetControl(timer1, 2000, 1);
			}
		}
		break;
	}
}

static void
OnWindowTimer(WID__ wid)
{
	if (!refreshmode) {
		if (strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR) != 0)
			return;
		NeuxFileListboxSetPath(flbox1, refresh_media, filtertype);
	} else {
		if (StringStartWith(strbuf, refresh_media)) {
			NeuxFileListboxSetPath(flbox1, PLEXTALK_MOUNT_ROOT_STR, filtertype);
			strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
			UpdateMediaIcon(strbuf);
			UpdateTitle(strbuf);
		} else if (!strcmp(strbuf, PLEXTALK_MOUNT_ROOT_STR)) {
			NeuxFileListboxSetPath(flbox1, refresh_media, filtertype);
		} else
			return;
		indexItem = NeuxListboxSelectedItem(flbox1);
		//voice_prompt_abort();
		doVoicePrompt(NULL);
	}
}

static void
OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}

static void
OnWindowGetFocus(WID__ wid)
{
	DBGMSG("Get Focus\n");
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_SELECT);
	if(strlen(strbuf) <= 0){
		strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
	}
	DBGMSG("Get Focus strbuf:%s\n",strbuf);
	UpdateMediaIcon(strbuf);
	UpdateTitle(strbuf);	
}

static void
OnWindowDestroy(WID__ wid)
{
	DBGMSG("---------------window destroy %d.\n",wid);
}

static void
CreateFormFileDialog(const char* path, int type, GR_BOOL multisel)
{
//	listbox_prop_t lsprop;
	widget_prop_t wprop;

	form = NeuxFormNew( MAINWIN_LEFT,
						MAINWIN_TOP,
						MAINWIN_WIDTH,
						MAINWIN_HEIGHT);

	NeuxFormSetCallback(form, CB_KEY_DOWN, OnFlboxKeydown);
	NeuxFormSetCallback(form, CB_KEY_UP, OnFlboxKeyup);
	NeuxFormSetCallback(form, CB_FOCUS_IN, OnWindowGetFocus);
	NeuxFormSetCallback(form, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(form, CB_HOTPLUG,  OnWindowHotplug);

	timer1 = NeuxTimerNewWithCallback(form, 0, 0, OnWindowTimer);
   	key_timer = NeuxTimerNewWithCallback(form, 0, 0, OnKeyTimer);

	flbox1 = NeuxFileListboxNew(form, 0,  0, MAINWIN_WIDTH, MAINWIN_HEIGHT,
							path, type, multisel);

	NeuxFileListboxSetCallback(flbox1, CB_KEY_DOWN, OnFlboxKeydown);
	NeuxFileListboxSetCallback(flbox1, CB_KEY_UP, OnFlboxKeyup);
	
	NeuxWidgetSetFont(flbox1, sys_font_name, sys_font_size);

	NeuxWidgetGetProp(flbox1, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(flbox1, &wprop);

	NeuxWidgetShow(flbox1, TRUE);

	NeuxWidgetShow(form, TRUE);
	NeuxWidgetFocus(form);
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
	case APPRQST_SET_STATE:
		{
			app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
			DBGMSG("set state:%d\n", rqst->pause);
			if(!isininfo){
				return;
			}
			if(rqst->pause){
				info_pause();
			}else{
				info_resume();
			}
		}
		break;
	default:
		break;
	}
}

/*
  NeuxFileDialogOpen is modal, thus NeuxFileDialogOpen will not return before any button
  is clicked.
 */

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
int FileDialogOpen(const char *path, int _filtertype, GR_BOOL multisel, char ***selectedfiles, int *selectedcount,int* tts_running)
{
	if (form != NULL)
		return -1;

	if (path == NULL ||
		(path[strlen(path) - 1] == '/' ? !PlextalkIsDirectory(path) : !PlextalkIsFileExist(path)))
		path = PLEXTALK_MOUNT_ROOT_STR;
	else {
		if (!StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR) ||
		    (path[strlen(PLEXTALK_MOUNT_ROOT_STR)] != '\0' &&
		     path[strlen(PLEXTALK_MOUNT_ROOT_STR)] != '/'))
			//return -1;
			path = PLEXTALK_MOUNT_ROOT_STR;
	}

	file_tts_running = tts_running;

	iMsgReturn = MSGBX_BTN_CANCEL;
	filtertype = _filtertype;

	CreateFormFileDialog(path, _filtertype, multisel);

	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_SELECT);

	strlcpy(strbuf, NeuxFileListboxGetPath(flbox1), sizeof(strbuf));
	UpdateMediaIcon(strbuf);
	UpdateTitle(strbuf);
	indexItem = NeuxListboxSelectedItem(flbox1);
	doVoicePrompt(NULL);
	
	strlcpy(refresh_media, strbuf, PATH_MAX);
	
	origAppMsg = NeuxAppGetCallback(CB_MSG);
	NeuxAppSetCallback(CB_MSG, onAppMsg);
	endOfModal = 0;
	dirIsEmpty = 0;
	NeuxDoModal(&endOfModal, NULL);
	if (iMsgReturn == MSGBX_BTN_YES)
		GetSelectedFiles(selectedfiles, selectedcount);

	NeuxFormDestroy(&form);
	form = NULL;
	NeuxAppSetCallback(CB_MSG, origAppMsg);
	if(dirIsEmpty){
		return -1;
	}else{
		return 0;
	}
}

void FileDialogFreeSelections(char ***selectedfiles, int selectedcount)
{
	int i;

	if( *selectedfiles == NULL )
		return;

	for( i=0; i<selectedcount; i++ )
		free((*selectedfiles)[i]);
	free(*selectedfiles);
	*selectedfiles = NULL;
}

