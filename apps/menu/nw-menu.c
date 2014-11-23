#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <nano-X.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
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
#include "menu-backup.h"
#include "menu-defines.h"


static FORM *  formMenu;
#define widget formMenu
static WIDGET* listbox1;

static struct {
	struct menu* menu;
	int          topItem;
	int          actItem;
} menu_stack[MENU_MAX_DEPTH];
static int menu_sp;

//added by ztz
extern struct plextalk_setting setting;

struct menu* menu;
static int actItem;
static int in_info;//added by app
//static int topItem;

int menu_exit;
int wait_focus_out;
int focus_out;
int wait_app_response;//added by lhf
//added by app start

extern int voice_prompt_end;
int timer_nr;
struct plextalk_timer timer;
//added by app end
static struct plextalk_language_all all_langs;

// 10 maybe enough
static storage_info_t storages[10];

static void* value_base = &setting;

//lhf start
static TIMER *lock_timer;

void appResponseLock(void)
{
	if(!wait_app_response){
		wait_app_response = 1;
		NeuxTimerSetControl(lock_timer, 10000, 1);
		plextalk_schedule_lock();
		DBGMSG("response lock\n");
	}
}

void appResponseUnlock(void)
{
	if(wait_app_response){
		wait_app_response = 0;
		plextalk_schedule_unlock();
		DBGMSG("response unlock\n");
	}
}
//lhf end
void MenuVoicePromptHook(void)
{
	if (menu_exit) {
		if (!wait_focus_out || focus_out) {
			menu = NULL;
			NeuxAppStop();
		}
	}
}

void MenuExit(int sync_setting_file)
{
	if (sync_setting_file) {
		PlextalkCheckAndCreateDirectory(PLEXTALK_SETTING_DIR);
		plextalk_setting_write_xml(PLEXTALK_SETTING_DIR "setting.xml");
	}
	
	menu_exit = 1;

	if (!sync_setting_file ){//add by lhf
		if (menu == &guide_volume_menu ) {
			NhelperMenuGuideVol(0);
			CoolShmReadLock(g_config_lock);
			vprompt_cfg.volume = g_config->volume.guide;
			CoolShmReadUnlock(g_config_lock);
			NhelperGuideVolumeChange(vprompt_cfg.volume);	
		} else if (menu == &guide_speed_menu) {
			CoolShmReadLock(g_config_lock);
			vprompt_cfg.speed  = g_config->setting.guide.speed;
			CoolShmReadUnlock(g_config_lock);
		} else if (menu == &theme_menu ){
			if (setting.lcd.theme != (int)menu->actions[actItem].data) {
				widget_prop_t wprop;
				listbox_prop_t lsprop;

				NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP, setting.lcd.theme, 1);	//ztz
				
				NeuxThemeInit(setting.lcd.theme);
				wprop.fgcolor = theme_cache.foreground;
				wprop.bgcolor = theme_cache.background;
				NeuxWidgetSetProp(listbox1, &wprop);

				NeuxListboxGetProp(listbox1, &lsprop);
				lsprop.sbar.fgcolor = theme_cache.foreground;
				lsprop.sbar.bordercolor = theme_cache.foreground;
				lsprop.sbar.bgcolor = theme_cache.background;
				lsprop.selectmarkcolor = theme_cache.selected;
				lsprop.selectedcolor = theme_cache.selected;
				lsprop.selectedfgcolor = theme_cache.foreground;
				NeuxListboxSetProp(listbox1, &lsprop);
			}
		}
	}//lhf end
	
	if (menu == &timer_menu ||
		(menu_sp > 1 && menu_stack[1].menu == &timer_menu)) {
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Close the timer setting menu"));
	}
	//<xlm backupclose tts>....start....
	else if(menu == &backup_menu || (menu_sp > 1 && menu_stack[1].menu == &backup_menu))
	{
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Close the backup mode"));
	}
	//<xlm backupclose tts>....end....
	else {
		GR_WINDOW_ID focus = GrGetFocus();

		if (menu == &general_menu && actItem == MENU_GENERAL_SYSTEM_DATETIME_ID &&
	        NeuxWidgetWID(formMenu) != focus &&
	        NeuxWidgetWID(listbox1) != focus) {
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Close set the system date and time menu"));	
		} else {
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Close the menu"));
		}
	}
	voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
}

static void
doVoicePrompt(const char* lead)
{
	char *curr = gettext(menu->items[actItem]);

	if (lead != NULL)
		voice_prompt_music(0, &vprompt_cfg, lead);

	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, curr);
}
//lhf start
static void menuSetExit(int sync_setting_file)
{
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Set."));
	MenuExit(sync_setting_file);
}
//lhf end
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
//lhf start
int delete_result(app_delete_result* rqst)
{
	DBGMSG("result:%d\n",rqst->result);
	
	appResponseUnlock();
	
	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		tts_lang, tts_role,_("Enter."));
	
	switch(rqst->result){
		case APP_DELETE_OK:
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Finished."));
			break;
			
		case APP_DELETE_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Failed to delete."));
			break;
			
		case APP_DELETE_READ_ONLY_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Read only media."));
			break;
			
		case APP_DELETE_MEDIA_LOCKED_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("The media is locked."));
			break;
		default:
			break;
	}
	MenuExit(0);
	return 1;
}
//lhf end


//lhf start 
int book_jump_page(void *arg)
{
	int op = (int) arg;
	int ret;
	char path[PATH_MAX];
	char title[PATH_MAX];
	
	if (0 != strcmp(curr_app, PLEXTALK_IPC_NAME_BOOK)){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Title of the Book category is not slelected"));
		return -1;
	}
	
	snprintf(path, PATH_MAX, "/tmp/.%s", curr_app);
	ret = CoolGetCharValue(path, "backup", "backup_path", title, PATH_MAX);
	if(COOL_INI_OK != ret){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Title of the Book category is not slelected"));
		return -1;
	}

	if(!PlextalkIsDsiayBook(title)){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("DAISY contents is not selected."));
		return -1;
	}

	NeuxWMAppRun(PLEXTALK_BINS_DIR PLEXTALK_IPC_NAME_JUMP_PAGE);

	wait_focus_out = 1;

	MenuExit(0);
	return 0;
}

int book_delete_operation(void *arg)
{
	int op = (int) arg;
	int ret;
	char path[PATH_MAX];
	char title[PATH_MAX];
	
	if (0 != strcmp(curr_app, PLEXTALK_IPC_NAME_BOOK)){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Title of the Book category is not slelected"));
		return -1;
	}

	snprintf(path, PATH_MAX, "/tmp/.%s", curr_app);
	ret = CoolGetCharValue(path, "backup", "backup_path", title, PATH_MAX);
	if(COOL_INI_OK != ret){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Title of the Book category is not slelected"));
		return -1;
	}
	
	ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
			_("You are about to delete the selected title. Are you Sure?"));
	if (ret != MSGBX_BTN_OK)
		return -1;
	
	NhelperDeleteOp(PLEXTALK_IPC_NAME_BOOK, op);
	
	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
	appResponseLock();
	return 0;
}
//lhf end
int book_set_speed(void *arg)
{
	int speed = (int) arg;
	int change;

	change = setting.book.speed != speed;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.book.speed = speed;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetSpeed(PLEXTALK_IPC_NAME_BOOK, speed);
	}

	menuSetExit(change);//lhf
	return 0;
}

int book_set_repeat(void *arg)
{
	int repeat = (int) arg;
	int change;

	change = setting.book.repeat != repeat;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.book.repeat = repeat;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetRepeat(PLEXTALK_IPC_NAME_BOOK, repeat);
	}

	menuSetExit(change);//lhf
	return 0;
}
//lhf start
int music_delete_operation(void *arg)
{
	int op = (int) arg;
	int ret;
	char path[PATH_MAX];
	char title[PATH_MAX];

	if (0 != strcmp(curr_app, PLEXTALK_IPC_NAME_MUSIC)) {
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Content of the Music category is not slelected"));
		return -1;
	}

	snprintf(path, PATH_MAX, "/tmp/.%s", curr_app);
	ret = CoolGetCharValue(path, "backup", "backup_path", title, PATH_MAX);
	if(COOL_INI_OK != ret){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Content of the Music category is not slelected"));
		return -1;	
	}

	ret = MSGBX_BTN_CANCEL;
	switch (op) {
	case APP_DELETE_TRACK:
		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to delete the selected track. Are you sure?"));
		break;
	case APP_DELETE_ALBUM:
		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to delete the selected album. Are you sure?"));
		break;
	}
	if (ret != MSGBX_BTN_OK)
		return -1;

	NhelperDeleteOp(PLEXTALK_IPC_NAME_MUSIC, op);
	
	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);

	appResponseLock();
	return 0;
}//lhf end

int music_set_speed(void *arg)
{
	int speed = (int) arg;
	int change;

	change = setting.music.speed != speed;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.music.speed = speed;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetSpeed(PLEXTALK_IPC_NAME_MUSIC, speed);
	}

	menuSetExit(change);//lhf
	return 0;
}

int music_set_repeat(void *arg)
{
	int repeat = (int) arg;
	int change;

	change = setting.music.repeat != repeat;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.music.repeat = repeat;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetRepeat(PLEXTALK_IPC_NAME_MUSIC, repeat);
	}

	menuSetExit(change);//lhf
	return 0;
}
//lhf start
int bookmark_result(app_bookmark_result* rqst)
{
	DBGMSG("result:%d\n",rqst->result);
	
	appResponseUnlock();
	
	voice_prompt_abort();
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		tts_lang, tts_role,_("Enter."));
	
	switch(rqst->result){
		case APP_BOOKMARK_DEL_OK:
		case APP_BOOKMARK_SET_OK:
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Finished."));
			break;
			
		case APP_BOOKMARK_EXIST_ERR:
			{
			char tipstr[256];
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			snprintf(tipstr, sizeof(tipstr), "%s %d %s", _("Bookmark"), rqst->addtion,  _("is already set at this position."));
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, tipstr);
			}
			break;
				
		case APP_BOOKMARK_UPPER_LIMIT_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("The bookmark number reached the upper limit.Delete the unused bookmark."));
			break;
			
		case APP_BOOKMARK_SET_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Write error.It may not access the media correctly."));
			break;
		
		case APP_BOOKMARK_NOT_EXIST_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Bookmark does not exist at this position."));
			break;
			
		case APP_BOOKMARK_DEL_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Failed to delete."));
			break;
			
		case APP_BOOKMARK_READ_ONLY_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("Read only media."));
			break;
			
		case APP_BOOKMARK_MEDIA_LOCKED_ERR:
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role,_("The media is locked."));
			break;
		default:
			break;
	}
	MenuExit(0);
	return 1;
}
//lhf end
//lhf start
int bookmark_validate(void *arg)
{
	int ret;
	char path[PATH_MAX];
	char title[PATH_MAX];
	
	if (0!=strcmp(curr_app, PLEXTALK_IPC_NAME_BOOK) &&
		0!=strcmp(curr_app, PLEXTALK_IPC_NAME_MUSIC)){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Cannot set a bookmark because the content is not selected."));
		return -2;
	}
	
	snprintf(path, PATH_MAX, "/tmp/.%s", curr_app);
	ret = CoolGetCharValue(path, "backup", "backup_path", title, PATH_MAX);
	if(COOL_INI_OK != ret){
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
							_("Cannot set a bookmark because the content is not selected."));
		return -1;
	}

	return 0;
}
//lhf end
//lhf start
int bookmark_operation(void *arg)
{
	int op = (int) arg;
	int ret = MSGBX_BTN_CANCEL;
	char filename[PATH_MAX];
	int bookmark_cnt, bookmark_nr;
	int bmkret,bmkret2;//lhf

	snprintf(filename, PATH_MAX, "/tmp/.%s", curr_app);

	switch (op) {
	case APP_BOOKMARK_SET:
		bookmark_cnt = 0;
		bmkret = CoolGetIntValue(filename, "bookmark", "count", &bookmark_cnt);
		if (bmkret != COOL_INI_OK) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role,
					_("Cannot set a bookmark because the content is not selected."));
			return -1;
		}

		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to set bookmark %d. Are you sure?"),
				bookmark_cnt + 1);
		break;

	case APP_BOOKMARK_DELETE:
		bmkret = CoolGetIntValue(filename, "bookmark", "current", &bookmark_nr);
		bmkret2 = CoolGetIntValue(filename, "bookmark", "count", &bookmark_cnt);
		if (COOL_INI_OK != bmkret || bookmark_nr <= 0 || 
			COOL_INI_OK != bmkret2 || bookmark_cnt <= 0) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role,
					_("Bookmark does not exist at this position."));
			MenuExit(0);
			return 0;
		}

		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to delete the bookmark %d. Are you sure?"),
				bookmark_nr);
		break;

	case APP_BOOKMARK_DELETE_ALL:
		bookmark_cnt = 0;
		bmkret = CoolGetIntValue(filename, "bookmark", "count", &bookmark_cnt);
		if (COOL_INI_OK != bmkret || bookmark_cnt <= 0 ) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role,
					_("Bookmark does not exist at this position."));
			MenuExit(0);
			return 0;
		}

		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to clear all the bookmarks on the current content. Are you sure?"));
		break;
	}

	if (ret != MSGBX_BTN_OK)
		return -1;

	NhelperBookmarkOp(curr_app, op);
	appResponseLock();
	return 0;
}
//lhf end

int radio_delete_menu_validate(void*arg)
{
	if (!strcmp(curr_app, PLEXTALK_IPC_NAME_RADIO))
		return 0;

	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
	voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
						_("Radio function is not slected."));
	return -1;
}

int radio_delete_operation(void *arg)
{
	int op = (int) arg;
	int ret = MSGBX_BTN_CANCEL;
	int current_chn;
	char freq[128];
	int freq_int, freq_frac;
	int chnret;

	switch (op) {
	case APP_DELETE_CURRENT_CHANNEL:
		chnret = CoolGetIntValue("/tmp/." PLEXTALK_IPC_NAME_RADIO, "chan", "index", &current_chn);
		if (chnret == COOL_INI_OK)
			chnret = CoolGetCharValue("/tmp/." PLEXTALK_IPC_NAME_RADIO, "chan", "freq", freq, sizeof(freq));
		if (chnret != COOL_INI_OK) {
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role,
					_("Preset channel is not selected."));
			return -1;
		}

		freq_frac = 0;
		sscanf(freq, "%d.%d", &freq_int, &freq_frac);
		if (freq_frac != 0)
			ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
					_("You are about to delete preset channel number %d %d.%d Mhz. Are you Sure?"),
					current_chn, freq_int, freq_frac);
		else
			ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
					_("You are about to delete preset channel number %d %d Mhz. Are you Sure?"),
					current_chn, freq_int);
		break;

	case APP_DELETE_ALL_CHANNELS:
		ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
				_("You are about to delete all preset channels. Are you Sure?"));
		break;
	}
	if (ret != MSGBX_BTN_OK)
		return -1;

	NhelperDeleteOp(PLEXTALK_IPC_NAME_RADIO, op);
	appResponseLock();//lhf
	//MenuExit(0);
	return 0;
}

int radio_set_output(void *arg)
{
	int output = (int) arg;
	int change;

	change = setting.radio.output != output;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.radio.output = output;
		CoolShmWriteUnlock(g_config_lock);

//		NhelperSetRadioOutput(PLEXTALK_IPC_NAME_RADIO, output);
		NhelperSetRadioOutput(PLEXTALK_IPC_NAME_DESKTOP, output);
	}

	menuSetExit(change);//lhf
	return 0;
}

int recording_set_target(void *arg)
{
	int saveto = (int) arg;
	int change;

	change = setting.record.saveto != saveto;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.record.saveto = saveto;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetRecordTarget(PLEXTALK_IPC_NAME_RECORD, saveto);
		NhelperSetRecordTarget(PLEXTALK_IPC_NAME_RADIO, saveto);
	}

	menuSetExit(change);//lhf
	return 0;
}

static int backup_media_enum(struct mntent *mount_entry, void* user_data)
{
	char **ptr = user_data;

	if (StringStartWith(mount_entry->mnt_fsname, *ptr)) {
		*ptr = strdup(mount_entry->mnt_dir);
		return 1;
	}

	return 0;
}


int timer_set_active(void *arg)
{
	timer_nr = (int) arg;
	memcpy(&timer, &setting.timer[timer_nr], sizeof(timer));
// lhf
	if(!timer.enabled){ 
		timer.hour = -1;
		timer.minute = -1;
	}else{
		if(PLEXTALK_TIMER_COUNTDOWN == timer.type){
			timer.hour = -1;
			timer.minute = -1;
		}
	}
// lhf
	//timer.enabled = 1;   //remove by lhf
	value_base = &timer;

	return 0;
}

int timer_disable(void *arg)
{
	int change;

	timer.enabled = 0;
	if (confirm_timer_setting() < 0) {
		return -1;
	}

	change = setting.timer[timer_nr].enabled;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.timer[timer_nr].enabled = 0;
		CoolShmWriteUnlock(g_config_lock);

		NhelperReschduleTimer(timer_nr);
	}

	MenuExit(change);
	return 0;
}

int timer_set_func(void *arg)
{
	timer.enabled = 1;	//add by lhf
	timer.function = (int) arg;
	return 0;
}

int timer_set_sound(void *arg)
{
	timer.sound = (int) arg;
	return 0;
}

int timer_set_type(void *arg)
{
	timer.type = (int) arg;
	return 0;
}

int timer_set_clocktime(void *arg)
{
	timer.type = (int) arg;
	return set_timer_clocktime(0);
}

int timer_set_repeat(void *arg)
{
	int change;

	timer.repeat = (int) arg;
	if (timer.repeat == PLEXTALK_ALARM_REPEAT_WEEKLY) {
		if (set_timer_weekday(0) < 0) {
			return -1;
		}
	}
retry:
	if (confirm_timer_setting() < 0) {
		if (timer.repeat != PLEXTALK_ALARM_REPEAT_WEEKLY) {
			return -1;
		}
		if (set_timer_weekday(1) < 0) {
			return -1;
		}
		goto retry;
	}

	if (timer.enabled &&
		timer.function == PLEXTALK_TIMER_ALARM &&
		timer.type == PLEXTALK_TIMER_CLOCKTIME &&
		timer.repeat == PLEXTALK_ALARM_REPEAT_WEEKLY &&
		!(timer.weekday & 0x7f)) {
		/* alarm's repeat setting is not validity */
		timer.enabled = 0;
	}

	change = memcmp(&setting.timer[timer_nr], &timer, sizeof(timer)) != 0;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		memcpy(&g_config->setting.timer[timer_nr], &timer, sizeof(timer));
		CoolShmWriteUnlock(g_config_lock);

		NhelperReschduleTimer(timer_nr);
	}

	MenuExit(change);
	return 0;
}

int timer_set_countdown(void *arg)
{
	int change;

	timer.elapse = (int) arg;
	if (confirm_timer_setting() < 0) {
		return -1;
	}

	change = memcmp(&setting.timer[timer_nr], &timer, sizeof(timer)) != 0;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		memcpy(&g_config->setting.timer[timer_nr], &timer, sizeof(timer));
		CoolShmWriteUnlock(g_config_lock);

		NhelperReschduleTimer(timer_nr);
	}

	MenuExit(change);
	return 0;
}


//wgp
void  GetLanguage_Menu_Item()
{

	int i;
	

	

	printf("Enter  GetLanguage_Menu_Item Fun_aaaaaaaaaaaaaaa!\n");
	
	if(g_config->langsetting==0)
	{//0: English,Simplified Chinese,Traditional Chinese (Default: Simplified Chinese)
		
		printf("Enter  GetLanguage_Menu_Item Fun___111!\n");
//		all_langs.lang_nr=4;
		all_langs.lang_nr=3;

		all_langs.langs = malloc(sizeof(*all_langs.langs ) * all_langs.lang_nr);

		printf("Enter  GetLanguage_Menu_Item Fun___aaaaaaaaaaa!\n");
		strcpy(all_langs.langs[0].name,"English");
		strcpy(all_langs.langs[1].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xae\x80\xe4\xbd\x93");  //中文简体(UTF8)
		strcpy(all_langs.langs[2].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xb9\x81\xe4\xbd\x93");  //中文繁体(UTF8)
//		strcpy(all_langs.langs[3].name,"\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80\x20");//印度语(UTF8)

		strcpy(all_langs.langs[0].lang,"en_US");
		strcpy(all_langs.langs[1].lang,"zh_CN");
		strcpy(all_langs.langs[2].lang,"zh_TW");
//		strcpy(all_langs.langs[3].lang,"hi_IN");
		
		
	}
	else  if(g_config->langsetting==1)
	{//  1: English,Hindi (Default: Hindi)

		printf("Enter  GetLanguage_Menu_Item Fun___2222!\n");
		all_langs.lang_nr=2;

		all_langs.langs = malloc(sizeof(*all_langs.langs ) * all_langs.lang_nr);
		strcpy(all_langs.langs[0].name,"English");
		strcpy(all_langs.langs[1].name,"\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80\x20");//印度语(UTF8)

		strcpy(all_langs.langs[0].lang,"en_US");
		strcpy(all_langs.langs[1].lang,"hi_IN");
		
	}
	else if(g_config->langsetting==2)
	{// 2: English,Japanese (Default: Japanese)

//old defines	
#if 0
		printf("Enter  GetLanguage_Menu_Item Fun___3333333!\n");
		all_langs.lang_nr=2;

		all_langs.langs = malloc(sizeof(*all_langs.langs ) * all_langs.lang_nr);
		strcpy(all_langs.langs[0].name,"English");
		strcpy(all_langs.langs[1].name,"Japanese");//印度语(UTF8)

		strcpy(all_langs.langs[0].lang,"en_US");
		strcpy(all_langs.langs[1].lang,"jp_AN");
#endif
		all_langs.lang_nr=4;
		all_langs.langs = malloc(sizeof(*all_langs.langs ) * all_langs.lang_nr);

		strcpy(all_langs.langs[0].name,"English");
		strcpy(all_langs.langs[1].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xae\x80\xe4\xbd\x93");  //中文简体(UTF8)
		strcpy(all_langs.langs[2].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xb9\x81\xe4\xbd\x93");  //中文繁体(UTF8)
		strcpy(all_langs.langs[3].name,"\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80\x20");//印度语(UTF8)

		strcpy(all_langs.langs[0].lang,"en_US");
		strcpy(all_langs.langs[1].lang,"zh_CN");
		strcpy(all_langs.langs[2].lang,"zh_TW");
		strcpy(all_langs.langs[3].lang,"hi_IN");
	}
	else 
	{//0: English,Simplified Chinese,Traditional Chinese (Default: Simplified Chinese)


	
		printf("Enter  GetLanguage_Menu_Item Fun___444444444!\n");
		all_langs.lang_nr=4;
		all_langs.langs = malloc(sizeof(*all_langs.langs ) * all_langs.lang_nr);
		
		strcpy(all_langs.langs[0].name,"English");
		strcpy(all_langs.langs[1].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xae\x80\xe4\xbd\x93");  //中文简体(UTF8)
		strcpy(all_langs.langs[2].name,"\xE4\xB8\xAD\xE6\x96\x87\xe7\xb9\x81\xe4\xbd\x93");  //中文繁体(UTF8)
		strcpy(all_langs.langs[3].name,"\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80\x20");//印度语(UTF8)

		strcpy(all_langs.langs[0].lang,"en_US");
		strcpy(all_langs.langs[1].lang,"zh_CN");
		strcpy(all_langs.langs[2].lang,"zh_TW");
		strcpy(all_langs.langs[3].lang,"hi_IN");
	}

	printf("Enter  GetLanguage_Menu_Item Fun___end!\n");
}
//wgp

//extern  struct menu tts_menu;

 extern  const  char * tts_menu_items[];
extern   const char * tts_menu_items_1[] ;


extern  const struct  menu_action tts_menu_actions[];
extern const  struct   menu_action tts_menu_actions_1[];
 



int  tts_menu_init(void *arg)
{


	printf("Enter  tts_menu_init!\n");
	

	if(tts_menu.items==NULL)
	{

		printf("Enter  tts_menu_init_1111111111111111!\n");
		
		if(g_config->tts_setting==1)
		{//去掉 Standard Chinese  and Cantonese

			printf("Enter  tts_menu_init_222222222222222222!\n");
		
			tts_menu.items_nr		= 1;

			tts_menu.items			= tts_menu_items_1;

			tts_menu.actions		= tts_menu_actions_1;
			g_config->setting.tts.chinese = 0;

		}
		else 
		{
			printf("Enter  tts_menu_init_33333333333333333!\n");
			
			tts_menu.items_nr		= 2;

			tts_menu.items			= tts_menu_items;

			tts_menu.actions		= tts_menu_actions;

		
		}

		
			

	}



	return 0;
}

int language_menu_init(void *arg)
{

//wgp
	if (language_menu.items == NULL)
	{
		int i;

		if (all_langs.langs == NULL)
		{

			#if 0
			plextalk_get_all_lang(PLEXTALK_CONFIG_DIR "plextalk.xml", &all_langs);
			#else
			GetLanguage_Menu_Item();
			
			#endif

		}

		language_menu.items_nr = all_langs.lang_nr;
		language_menu.items    = malloc(sizeof(const char*) * all_langs.lang_nr);
		language_menu.actions  = calloc(all_langs.lang_nr, sizeof(struct menu_action));

		for (i = 0; i < all_langs.lang_nr; i++) 
		{
			language_menu.items[i]            = all_langs.langs[i].name;
			language_menu.actions[i].callback = set_language;
			language_menu.actions[i].data     = all_langs.langs[i].lang;
		}
	}
//wgp

	return 0;

}

static int language_menu_destroy(void)
{
	if (language_menu.items) {
		free(language_menu.items);
		language_menu.items = NULL;
	}
	if (language_menu.actions) {
		free(language_menu.actions);
		language_menu.actions = NULL;
	}
	language_menu.items_nr = 0;

	if (all_langs.langs) {
		free(all_langs.langs);
		all_langs.langs = NULL;
	}
	all_langs.lang_nr = 0;
}

int do_set_time(void *arg)
{// app
	if (sys_set_data_time() != 0)
		return -1;

	NhelperStatusBarReflushTime();
	
	menuSetExit(0);
	return 0;
//app
}

int do_calculator(void *arg)
{
	NeuxWMAppRun(PLEXTALK_BINS_DIR PLEXTALK_IPC_NAME_CALCULATOR);

	wait_focus_out = 1;

	MenuExit(0);
	return 0;
}

int do_file_management(void *arg)
{
	NeuxWMAppRun(PLEXTALK_BINS_DIR PLEXTALK_IPC_NAME_FILE_MANAGEMENT);

	wait_focus_out = 1;

	MenuExit(0);
	return 0;
}

static int format_media_enum(struct mntent *mount_entry, void* user_data)
{
	char **ptr = user_data;

	if (StringStartWith(mount_entry->mnt_fsname, *ptr)) {
		*ptr = strdup(mount_entry->mnt_fsname);
		return 1;
	}

	return 0;
}

int format_menu_init(void *arg)
{
#if 0//modified by km start
	int n = 1;
	char *device;
	int ret;

	device = "/dev/mmcblk1";
	ret = CoolStorageEnumerate(format_media_enum, &device);
	if (ret == 1) {
		format_menu_items[n] = "SD Card";
		format_menu_actions[n].data = device;
		n++;
	}

	device = "/dev/sda";
	ret = CoolStorageEnumerate(format_media_enum, &device);
	if (ret == 1) {
		format_menu_items[n] = "USB Memory";
		format_menu_actions[n].data = device;
		n++;
	}

	format_menu.items_nr = n;
	return 0;
#else
	menu_format_list_init(&format_menu,FORMAT_MENU_ITEM_NUM-2);
#endif//modified by km end	
}


static void format_menu_destroy(void)
{
	int n;

	for (n = 1; n < format_menu.items_nr; n++) {
		if (format_menu_actions[n].data) {
			DBGMSG("free %d:%p,%s\n",n,format_menu_actions[n].data,format_menu_actions[n].data);
			free(format_menu_actions[n].data);
			format_menu_actions[n].data = NULL;
		}
		if (format_menu.items[n]) {//appk
			DBGMSG("free %d:%p,%s\n",n,format_menu.items[n],format_menu.items[n]);
			free(format_menu.items[n]);
			format_menu.items[n] = NULL;
		}//appk
	}

	format_menu.items_nr = 1;
}

// << ztz: begin: 加入刷新menu的
static void refresh_menu(void)
{
 	widget_prop_t wprop;
	listbox_prop_t lsprop;

	CoolShmReadLock(g_config_lock);
	memcpy(&setting, &g_config->setting, sizeof(setting));
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed  = g_config->setting.guide.speed;
	CoolShmReadUnlock(g_config_lock);

	NeuxThemeInit(setting.lcd.theme);
	plextalk_update_lang(setting.lang.lang, PLEXTALK_IPC_NAME_MENU);

	plextalk_update_sys_font(setting.lang.lang);
	plextalk_update_tts(setting.lang.lang);

	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;

	NeuxWidgetSetProp(listbox1, &wprop);

    NeuxListboxGetProp(listbox1, &lsprop);
	lsprop.sbar.fgcolor = theme_cache.foreground;
	lsprop.sbar.bordercolor = theme_cache.foreground;
	lsprop.sbar.bgcolor = theme_cache.background;
	lsprop.selectmarkcolor = theme_cache.selected;
	lsprop.selectedcolor = theme_cache.selected;
	lsprop.selectedfgcolor = theme_cache.foreground;
    NeuxListboxSetProp(listbox1, &lsprop);

	NeuxWidgetSetFont(listbox1, sys_font_name, sys_font_size);
	NxListboxRecalcPitems(listbox1);
	NeuxListboxSetIndex(listbox1, actItem);
}
//<< end


//ztz  start
static char* DeviceConfigLangStr[] = {
	"zh_CN",
	"hi_IN",
	"en_US",			//to be considered...
};

void
DeviceSettingFixLang (void)
{
	CoolShmWriteLock(g_config_lock);
	strlcpy(g_config->setting.lang.lang, DeviceConfigLangStr[g_config->langsetting], 
			sizeof(g_config->setting.lang.lang));
	CoolShmWriteUnlock(g_config_lock);
}
//ztz	end


int do_restore_settings(void *arg)
{
	int ret;

	ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
			_("You are about to initialize all settings to default. Are you sure?"));
	if (ret != MSGBX_BTN_OK)
		return -1;

	plextalk_setting_read_xml(PLEXTALK_CONFIG_DIR "setting.xml");
	plextalk_volume_read_xml(PLEXTALK_CONFIG_DIR "volume.xml");

	Get_Device_Setting_Xml();
	plextalk_time_system_init();		//add by lhf

	DeviceSettingFixLang();			//ztz
	
	NhelperRsyncSettings();
	refresh_menu();	//added by ztz

	menuSetExit(1);//lhf
	return 0;
}

int guide_set_volume(void *arg)
{
	int vol = (int) arg;
	int change;


	CoolShmWriteLock(g_config_lock);
	change = g_config->volume.guide != vol;
	g_config->volume.guide = vol;
	CoolShmWriteUnlock(g_config_lock);

	if (change)
		NhelperGuideVolumeChange(vol);
	NhelperMenuGuideVol(0);

	menuSetExit(change);//lhf
	return 0;
}

int guide_set_speed(void *arg)
{
	int speed = (int) arg;
	int change;

	change = setting.guide.speed != speed;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.guide.speed = speed;
		CoolShmWriteUnlock(g_config_lock);

		NhelperGuideSpeedChange(speed);
	}

	menuSetExit(change);//lhf
	return 0;
}

int book_set_audio_equalizer(void *arg)
{
	int equ = (int) arg;
	int change;

	change = setting.book.audio_equalizer != equ;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.book.audio_equalizer = equ;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetEqu(PLEXTALK_IPC_NAME_BOOK, equ, 1);
	}

	menuSetExit(change);//lhf
	return 0;
}

int book_set_text_equalizer(void *arg)
{
	int equ = (int) arg;
	int change;

	change = setting.book.text_equalizer != equ;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.book.text_equalizer = equ;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetEqu(PLEXTALK_IPC_NAME_BOOK, equ, 0);
	}

	menuSetExit(change);//lhf
	return 0;
}

int music_set_equalizer(void *arg)
{
	int equ = (int) arg;
	int change;

	change = setting.music.equalizer != equ;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.music.equalizer = equ;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetEqu(PLEXTALK_IPC_NAME_MUSIC, equ, 0);
	}

	menuSetExit(change);//lhf
	return 0;
}

int set_screensaver_timeout(void *arg)
{
	int timeout = (int) arg;
	int change;

	change = setting.lcd.screensaver != timeout;
	DBGMSG("timeout:%d\n",timeout);
	if (change) {
		if (timeout == PLEXTALK_SCREENSAVER_ACTIVENOW) {
			sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 4);
			sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 4);
			//sysfs_write_integer(PLEXTALK_LCD_BLANK, 1);
 		} else if (setting.lcd.screensaver == PLEXTALK_SCREENSAVER_ACTIVENOW) {
			sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 0);
			sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 0);
			//sysfs_write_integer(PLEXTALK_LCD_BLANK, 0);
		}

		CoolShmWriteLock(g_config_lock);
		g_config->setting.lcd.screensaver = timeout;
		CoolShmWriteUnlock(g_config_lock);

#if 0//自动关机有问题，先关掉

		//appk start
		DBGMSG("timeout:%d,offtimer:%d,elapse:%d\n",timeout,plextalk_get_offtimer_disable(),g_config->setting.timer[2].elapse);
		if(timeout == PLEXTALK_SCREENSAVER_DEACTIVE && !plextalk_get_offtimer_disable()){
			NeuxWMAppSetScreenSaverTime(60*g_config->setting.timer[2].elapse);
		} else 
#endif		
		{
			NeuxWMAppSetScreenSaverTime(plextalk_screensaver_timeout[timeout]);
		}
		//appk end

		
	}

	menuSetExit(change);//lhf
	return 0;
}

int set_font_size(void *arg)
{
	int size = (int) arg;
	int change;

	change = setting.lcd.fontsize != size;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.lcd.fontsize = size;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetFontsize(size);
	}

	menuSetExit(change);//lhf
	return 0;
}

int set_theme(void *arg)
{
	int theme = (int) arg;
	int change;

	change = setting.lcd.theme != theme;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.lcd.theme = theme;
		CoolShmWriteUnlock(g_config_lock);
		NhelperBroadcastSetTheme(PLEXTALK_IPC_NAME_DESKTOP, theme);
		NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP, theme, 1);		//added by ztz
	}

	menuSetExit(change);//lhf
	return 0;
}

int set_brightness(void *arg)
{
	int brightness = (int) arg;
	int change;

	change = setting.lcd.backlight != brightness;
	if (change) {
		CoolShmWriteLock(g_config_lock);
		g_config->setting.lcd.backlight = brightness;
		CoolShmWriteUnlock(g_config_lock);
	}

	menuSetExit(change);//lhf
	return 0;
}

int set_language(void *arg)
{
	const char* lang = (const char*) arg;
	int change;

	change = strcmp(setting.lang.lang, lang) != 0;

	if (change) {
		CoolShmWriteLock(g_config_lock);
		strlcpy(g_config->setting.lang.lang, lang, PLEXTALK_LANG_MAX);
		CoolShmWriteUnlock(g_config_lock);
		NhelperSetLanguage(lang);
	}

	menuSetExit(change);//lhf
	return 0;
}


int tts_set_voice_species(void *arg)
{
	int female = (int) arg;
	int change;

	change = setting.tts.voice_species != female;
	
	if (change)
	{
		CoolShmWriteLock(g_config_lock);
		g_config->setting.tts.voice_species = female;
		CoolShmWriteUnlock(g_config_lock);

		tts_role = plextalk_get_tts_role(tts_lang);
		NhelperSetTTSVoiceSpecies(female);
	}

	menuSetExit(change);//lhf
	return 0;
}

int tts_set_chinese_standard(void *arg)
{
	int contonese = (int) arg;
	int change;

	change = setting.tts.chinese != contonese;
	
	printf("Enter  tts_set_voice_species fun=%d !\n",contonese);

	
	if (change) 
	{
		CoolShmWriteLock(g_config_lock);
		g_config->setting.tts.chinese = contonese;
		CoolShmWriteUnlock(g_config_lock);

		NhelperSetTTSChinese(contonese);
	}

	menuSetExit(change);//lhf
	return 0;
}

int do_format(void *arg)
{
	char format_cmd[PATH_MAX];
	int ret;

	ret = NeuxMessageboxNew(OnMsgBoxKeyDown, NULL, 0,
			_("You are about to format %s. Are you sure?"),
			gettext(menu->items[actItem]));
	if (ret != MSGBX_BTN_OK)
		return -1;

	snprintf(format_cmd, PATH_MAX,
			PLEXTALK_BINS_DIR PLEXTALK_IPC_NAME_FORMAT " %s",
			/*menu->actions[actItem].data*/arg);
	NeuxWMAppRun(format_cmd);

	wait_focus_out = 1;

	MenuExit(0);
	return 0;
}

int find_fontsize_index(void *arg)
{
	int size = *(int*)arg;
	int i;

	for (i = 0; i < font_size_menu.items_nr; i++) {
		if (size == (int)font_size_menu.actions[i].data)
			return i;
	}

	return 0;
}

int find_language_index(void *arg)
{
	char *lang = (char*) arg;
	int i;

	for (i = 0; i < language_menu.items_nr; i++) {
		if (!strcmp(lang, language_menu.actions[i].data))
			return i;
	}

	return 0;
}

static int
getItem(WID__ sender, int index, char **text)
{
	if (index == -1)
		return menu->items_nr;

	if (menu == &language_menu)
		*text = menu->items[index];
	else
		*text = gettext(menu->items[index]);

	return index - NeuxListboxTopItem(listbox1);
}

static void push_menu(void)
{
	listbox_prop_t lsprop;

	menu_stack[menu_sp].menu    = menu;
	menu_stack[menu_sp].topItem = NeuxListboxTopItem(listbox1);
	menu_stack[menu_sp].actItem = actItem;
	menu_sp++;

	menu = menu->actions[actItem].submenu;

	NeuxListboxGetProp(listbox1, &lsprop);
	lsprop.bWrapAround = menu->wrap_around;
	NeuxListboxSetProp(listbox1, &lsprop);

	if (!menu->has_default) {
//		topItem = 0;
		actItem = 0;
	} else {
		if (menu->find_index != NULL) {
			actItem = menu->find_index(menu->value_offset + value_base);
		} else {
			if (menu == &guide_volume_menu)
				actItem = menu->index_offset + vprompt_cfg.volume;
			else
				actItem = menu->index_offset + *(int*)(menu->value_offset + value_base);
			if (actItem < 0)
				actItem = -actItem;
			if (actItem >= menu->items_nr)
				actItem = menu->items_nr - 1;
		}

//		if (actItem < NeuxListboxGetPageItems(listbox1))
//			topItem = 0;
//		else
//			topItem = actItem - (NeuxListboxGetPageItems(listbox1) - 1);
	}

//	NeuxListboxSetItem(listbox1, actItem - topItem, topItem);
	NeuxListboxSetIndex(listbox1, actItem);
	NhelperTitleBarSetContent(gettext(menu->caption));

	if (menu->voice_prompt) {
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext(menu->voice_prompt));
	}

	doVoicePrompt(NULL);
}

static void pop_menu(void)
{
	listbox_prop_t lsprop;
	int topItem;

	--menu_sp;

	menu    = menu_stack[menu_sp].menu;
	actItem = menu_stack[menu_sp].actItem;
	topItem = menu_stack[menu_sp].topItem;

	NeuxListboxGetProp(listbox1, &lsprop);
	lsprop.bWrapAround = menu->wrap_around;
	NeuxListboxSetProp(listbox1, &lsprop);

	NeuxListboxSetItem(listbox1, actItem - topItem, topItem);
	NhelperTitleBarSetContent(gettext(menu->caption));

	if (menu->voice_prompt) {
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext(menu->voice_prompt));
	}

	doVoicePrompt(NULL);
}

static TIMER *key_timer;
static int long_pressed_key = -1;
static int in_info;

static void
OnListboxKeydown(WID__ wid,KEY__ key)
{
	static int iKeyProing = 0;//lhf
	widget_prop_t wprop;
	listbox_prop_t lsprop;

	if (menu_exit) {
		if (!wait_focus_out) {
			voice_prompt_abort();
			menu = NULL;
			NeuxAppStop();
		}
		return;
	}

	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Keylock"));
		}
		return;
	}
//lhf
	if(wait_app_response){
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_WAIT_BEEP);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Please wait."));
//lhf
		return;
	}

	switch (key.ch) {
	case MWKEY_UP:
	case MWKEY_DOWN:
		voice_prompt_abort();
		if (actItem == NeuxListboxSelectedItem(listbox1)) {
			doVoicePrompt(VPROMPT_AUDIO_KON);
		} else {
			actItem = NeuxListboxSelectedItem(listbox1);
			if (menu == &language_menu)
				plextalk_update_tts(menu->actions[actItem].data);
			else if (menu == &guide_volume_menu){
				vprompt_cfg.volume = (int)menu->actions[actItem].data;
				NhelperGuideVolumeChange(vprompt_cfg.volume); //add by lhf
			}
			else if (menu == &guide_speed_menu)//lhf
				vprompt_cfg.speed = (int)menu->actions[actItem].data;//lhf
			doVoicePrompt(VPROMPT_AUDIO_PU);

			/* brightness or theme preview */
			if (menu == &brightness_menu) {
				sysfs_write_integer(PLEXTALK_LCD_BRIGHTNESS,
							plextalk_lcd_brightless[(int)menu->actions[actItem].data]);
			} else if (menu == &theme_menu) {
				int topItem = NeuxListboxTopItem(listbox1);
				NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP,
							(int)menu->actions[actItem].data, 0);		//ztz

				NeuxThemeInit((int)menu->actions[actItem].data);
				wprop.fgcolor = theme_cache.foreground;
				wprop.bgcolor = theme_cache.background;
				NeuxWidgetSetProp(listbox1, &wprop);

				NeuxListboxGetProp(listbox1, &lsprop);
				lsprop.sbar.fgcolor = theme_cache.foreground;
				lsprop.sbar.bordercolor = theme_cache.foreground;
				lsprop.sbar.bgcolor = theme_cache.background;
				lsprop.selectmarkcolor = theme_cache.selected;
				lsprop.selectedcolor = theme_cache.selected;
				lsprop.selectedfgcolor = theme_cache.foreground;
				NeuxListboxSetProp(listbox1, &lsprop);

				NeuxListboxSetItem(listbox1, actItem - topItem, topItem);
			}
		}
		break;

	case MWKEY_LEFT:
		voice_prompt_abort();
		if (menu_sp == 0) {
			doVoicePrompt(VPROMPT_AUDIO_KON);
		} else {
			if (menu == &guide_speed_menu) {//lhf start
				CoolShmReadLock(g_config_lock);
				vprompt_cfg.speed  = g_config->setting.guide.speed;
				CoolShmReadUnlock(g_config_lock);
			} else if (menu == &guide_volume_menu) {
				NhelperMenuGuideVol(0);
				
				CoolShmReadLock(g_config_lock);
				vprompt_cfg.volume = g_config->volume.guide;
				CoolShmReadUnlock(g_config_lock);
				NhelperGuideVolumeChange(vprompt_cfg.volume);	
			}//lhf end
			
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Cancel"));

			if (menu == &language_menu)
				plextalk_update_tts(getenv("LANG"));
			else if (menu == &timer_menu) {
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
							tts_lang, tts_role, _("Close the timer setting menu"));
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
				value_base = &setting;
				NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);
			} else if (menu == &brightness_menu) {
				/* restore brightless setting */
				if (setting.lcd.backlight != (int)menu->actions[actItem].data)
					sysfs_write_integer(PLEXTALK_LCD_BRIGHTNESS,
								plextalk_lcd_brightless[setting.lcd.backlight]);
			} else if (menu == &theme_menu) {
				/* restore theme setting */
				if (setting.lcd.theme != (int)menu->actions[actItem].data) {
					NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP, setting.lcd.theme, 1); //ztz
					NeuxThemeInit(setting.lcd.theme);
					wprop.fgcolor = theme_cache.foreground;
					wprop.bgcolor = theme_cache.background;
					NeuxWidgetSetProp(listbox1, &wprop);

					NeuxListboxGetProp(listbox1, &lsprop);
					lsprop.sbar.fgcolor = theme_cache.foreground;
					lsprop.sbar.bordercolor = theme_cache.foreground;
					lsprop.sbar.bgcolor = theme_cache.background;
					lsprop.selectmarkcolor = theme_cache.selected;
					lsprop.selectedcolor = theme_cache.selected;
					lsprop.selectedfgcolor = theme_cache.foreground;
					NeuxListboxSetProp(listbox1, &lsprop);
				}
			} else if (menu == &timer_repeat_menu) {
				if (set_timer_clocktime(1) == 0) {
					if (menu->voice_prompt) {
						voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
									tts_lang, tts_role, gettext(menu->voice_prompt));
					}
					doVoicePrompt(NULL);
					return;
				}
			} else if (menu == &format_menu)
				format_menu_destroy();
			else if (menu == &backup_menu)
				NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MENU);

			pop_menu();
		}
		break;

	case MWKEY_RIGHT:
	case MWKEY_ENTER:
		if(iKeyProing){//lhf
			DBGMSG("**************key double in\n");
			return;
		}
		iKeyProing = 1;//lhf
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, _("Enter"));
		if (menu->actions[actItem].callback != NULL) {
			// add by xlm
			int ret = menu->actions[actItem].callback(menu->actions[actItem].data);
			if (ret == -1) {
				if (menu->voice_prompt) {
					voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
								tts_lang, tts_role, gettext(menu->voice_prompt));
				}
				doVoicePrompt(NULL);
				iKeyProing = 0;//lhf
				return;
			}
			else if(ret == -2)
			{
				MenuExit(0);
				iKeyProing = 0;//lhf
				return;
			}
			//add by xlm end

		}
		if (menu->actions[actItem].submenu != NULL) {
			push_menu();

			if (menu == &guide_volume_menu)
				NhelperMenuGuideVol(1);
			else if (menu == &timer_menu)
				NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_TIMER);
			else if (menu == &backup_menu)
				NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_BACKUP);
		}
		iKeyProing = 0;//lhf
		break;

	case MWKEY_MENU:
		if (!key.hotkey)
			return;
		if (in_info || NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
			return;
		long_pressed_key = MWKEY_MENU;
    	NeuxTimerSetControl(key_timer, 500, 1);
		break;

	case MWKEY_INFO:
		if (!key.hotkey)
			return;
		if (in_info || NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
			return;
		if (menu == &general_menu && actItem == MENU_GENERAL_SYSTEM_DATETIME_ID) {
			GR_WINDOW_ID focus = GrGetFocus();
	        if (NeuxWidgetWID(formMenu) != focus &&
	       		 NeuxWidgetWID(listbox1) != focus)
	        	return;
		}
// km 
		in_info = 1;
		do_Infomation(gettext(menu->caption),gettext(menu->items[actItem]));
		in_info = 0;
// km
		break;

	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		if (!key.hotkey)
			return;
	if (in_info || NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
			return;
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		break;
	}
}

static void
OnListboxKeyup(WID__ wid,KEY__ key)
{
	if (!key.hotkey)
		return;
	if (long_pressed_key != key.ch)
		return;

	long_pressed_key = -1;
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch) {
	case MWKEY_MENU:
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		MenuExit(0);
		break;
	}
}

static void
OnKeyTimer(WID__ wid)
{
	/* long pressed menu and info key handled by desktop */
	long_pressed_key = -1;
}

//lhf
static void
OnLockTimer(WID__ wid)
{
	DBGMSG("lock timerout\n");
	if(wait_app_response){
		appResponseUnlock();
		MenuExit(0);
	}
}
//lhf
static void
OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);

	if (menu == &brightness_menu) {
		/* restore brightless setting */
		if (setting.lcd.backlight != (int)menu->actions[actItem].data)
			sysfs_write_integer(PLEXTALK_LCD_BRIGHTNESS,
				plextalk_lcd_brightless[setting.lcd.backlight]);
	} else if (menu == &theme_menu) {
		/* restore theme setting */
		if (setting.lcd.theme != (int)menu->actions[actItem].data) {
			NhelperSetTheme(PLEXTALK_IPC_NAME_DESKTOP, setting.lcd.theme, 1);	//added by ztz
		}
	} else if (menu == &guide_volume_menu)
		NhelperMenuGuideVol(0);
	
	language_menu_destroy();
	format_menu_destroy();

	widget = NULL;
}

//ztz
static void 
OnWindowInFocus (WID__ wid)
{
	NhelperTitleBarSetContent(gettext(menu->caption));
}
//ztz

static void
OnWindowLoseFocus(WID__ wid)
{
	if (!menu_exit)
		return;
	if (voice_prompt_end) {
		menu = NULL;
		NeuxAppStop();
	}
	focus_out = 1;
}

void
ShowFormMenu(void)
{
 	widget_prop_t wprop;
	listbox_prop_t lsprop;

	widget = NeuxFormNew( MAINWIN_LEFT,
	                      MAINWIN_TOP,
	                      MAINWIN_WIDTH,
	                      MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, OnListboxKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,   OnListboxKeyup);
//	NeuxFormSetCallback(widget, CB_EXPOSURE, OnWindowRedraw);
	NeuxFormSetCallback(widget, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_FOCUS_OUT, OnWindowLoseFocus);
	NeuxFormSetCallback(widget, CB_FOCUS_IN, OnWindowInFocus);	//ztz

	listbox1 = NeuxListboxNew(widget, 0,  0, MAINWIN_WIDTH, MAINWIN_HEIGHT,
							getItem, NULL, NULL);

	NeuxListboxSetCallback(listbox1, CB_KEY_DOWN, OnListboxKeydown);
	NeuxListboxSetCallback(listbox1, CB_KEY_UP,   OnListboxKeyup);

    NeuxWidgetGrabKey(widget, VKEY_BOOK,   NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_MUSIC,  NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_RADIO,  NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, VKEY_RECORD, NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, MWKEY_MENU,  NX_GRAB_HOTKEY);
    NeuxWidgetGrabKey(widget, MWKEY_INFO,  NX_GRAB_HOTKEY);

//	NeuxWidgetGetProp(listbox1, &wprop);
//	wprop.bgcolor = theme_cache.foreground;
//	wprop.fgcolor = theme_cache.background;
//	NeuxWidgetSetProp(listbox1, &wprop);

    NeuxListboxGetProp(listbox1, &lsprop);
    lsprop.useicon = FALSE;
    lsprop.ismultiselec = FALSE;
    lsprop.bWrapAround = TRUE;
    lsprop.bScroll = TRUE;
    lsprop.bScrollBidi = FALSE;
    lsprop.scroll_interval = 500;
    lsprop.scroll_endingscrollcnt = -1;
	lsprop.sbar.fgcolor = theme_cache.foreground;
	lsprop.sbar.bordercolor = theme_cache.foreground;
	lsprop.sbar.bgcolor = theme_cache.background;
	lsprop.sbar.width = SBAR_WIDTH;
    lsprop.itemmargin.left = 0;
    lsprop.itemmargin.right = 0;
    lsprop.itemmargin.top = 0;
    lsprop.itemmargin.bottom = 0;
    lsprop.textmargin.left = 2;
    lsprop.textmargin.right = 2;
    lsprop.textmargin.top = 0;
    lsprop.textmargin.bottom = 0;
    NeuxListboxSetProp(listbox1, &lsprop);

	NeuxWidgetSetFont(listbox1, /*sys_font_name*/"arialuni.ttf", sys_font_size);
	NxListboxRecalcPitems(listbox1);

	menu = &main_menu;
	if (!strcmp(curr_app, PLEXTALK_IPC_NAME_BOOK)) {
		actItem = 0;
	} else if (!strcmp(curr_app, PLEXTALK_IPC_NAME_MUSIC)) {
		actItem = 1;
	} else if (!strcmp(curr_app, PLEXTALK_IPC_NAME_RADIO)) {
		actItem = 3;
	} else if (!strcmp(curr_app, PLEXTALK_IPC_NAME_RECORD)) {
		actItem = 4;
	} else {
		actItem = 0;
	}
//	if (actItem < NeuxListboxGetPageItems(listbox1)) {
//		topItem = 0;
//	} else {
//		topItem = actItem - (NeuxListboxGetPageItems(listbox1) - 1);
//	}
//	NeuxListboxSetItem(listbox1, actItem - topItem, topItem);
	NeuxListboxSetIndex(listbox1, actItem);

    NeuxWidgetShow(listbox1, TRUE);

   	key_timer = NeuxTimerNew(widget, 500, 0);
	NeuxTimerSetCallback(key_timer, OnKeyTimer);
	//lhf
	lock_timer = NeuxTimerNew(widget, 10000, 0);
	NeuxTimerSetCallback(lock_timer, OnLockTimer);
	//lhf
	NeuxFormShow(widget, TRUE);
	NeuxWidgetFocus(widget);
}
