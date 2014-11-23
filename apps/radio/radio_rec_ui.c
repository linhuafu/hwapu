#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include "radio_rec_extern.h"
#include "radio_type.h"
#include "radio_tts.h"
#include "radio_rec_draw.h"
#include "radio_rec_ui.h"
#include "radio_play.h"
#include "radio_rec_create.h"
#include "led_ctrl.h"
#include "plextalk-config.h"
#include "widgets.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "plextalk-statusbar.h"
#include "amixer.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "application.h"
#include "desktop-ipc.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "nx-widgets.h"
#include "radio_rec_newui.h"
#include <microwin/device.h>


#define _(S)			gettext(S)
/* Timer for Recorder UI flush */
#define TIMER_PERIOD  500
#define REC_TIMER_LKEY 1000


#define SEC_IN_MILLISECONDS 	1000

struct _rec_data data;

extern struct radio_flags global_flags;
extern struct radio_nano nano;

static int flag = 0;



#if 0
#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))

static int radio_rec_grab_table[] = {
	
		MWKEY_ENTER,
		MWKEY_RIGHT, 
		MWKEY_LEFT, 
		MWKEY_UP,
		MWKEY_DOWN,
		MWKEY_INFO,

		VKEY_BOOK, 
		VKEY_MUSIC, 
		VKEY_RADIO,

		VKEY_RECORD, 
};


void
radio_rec_grab_key ()
{	
	int i;
	
	for(i = 0; i < ARRAY_SIZE(radio_rec_grab_table); i++)
		NeuxWidgetGrabKey(nano.formRecMain, radio_rec_grab_table[i],  NX_GRAB_HOTKEY_EXCLUSIVE);
}


void
radio_rec_ungrab_key ()
{
	int i;
	
	for(i = 0; i < ARRAY_SIZE(radio_rec_grab_table); i++)
		NeuxWidgetUngrabKey(nano.formRecMain, radio_rec_grab_table[i]);
}
#endif


static void
rec_replace_time(char *pDest,char *pSrc,int size)
{
	int len,i;
	if(!pSrc || !pDest)
		return;
	len = strlen(pDest);
	if(len < size)
		return;
	
	for(i=0;i<size;i++) {
		*(pDest + i) = *pSrc;
		pSrc++;
	}
}

//是否需要热插拔事件声音
static int
rec_need_hotplug_prompt(struct _rec_data* data)
{
	if(data->rec_stat == REC_RECORDING && !data->rec_media) {
		data->ishotplugvoicedisable = 1;
		DBGMSG("disable hotplug prompt\n");
		return 0;
	}

	data->ishotplugvoicedisable = 0;
	DBGMSG("enable hotplug prompt\n");
	return 1;
}
//是否需要SD热插拔事件声音
static int
rec_need_SD_hotplug_prompt(struct _rec_data* data)
{
	if((data->rec_stat == REC_RECORDING || data->rec_stat == REC_PAUSE) && !data->rec_media) {
		data->isSDhotplugvoicedisable = 1;
		DBGMSG("disable SD hotplug prompt\n");
		return 0;
	}

	data->isSDhotplugvoicedisable = 0;
	DBGMSG("enable SD hotplug prompt\n");
	return 1;
}



/* function for signal gmenu set recorder media*/
int 
radio_rec_change_media ()
{	
	int media;

	CoolShmReadLock(g_config_lock);
	media = g_config->setting.record.saveto;
	CoolShmReadUnlock(g_config_lock);
#if REC_USE_PREPARE	
	if ((!media) && (!check_sd_exist())) {
		//这里也应该给出语言提示吧
		radio_tts(RADIO_TTS_NO_SDCARD, 0);
		DBGMSG("No SD card , set to inner memory.\n");
		DBGMSG("No SD card , set to inner memory.\n");
		/* set with inner memory */
		CoolShmWriteLock(g_config_lock);
		g_config->setting.record.saveto = 1;
		CoolShmWriteUnlock(g_config_lock);
		data.rec_media = 1;

		return 0;
	}
#endif	

	if (media != data.rec_media) {
		/*这里应当给出语言提示吧*/
		if (media)
		{
			NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		}
		else{
			NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
		}

#if REC_USE_PREPARE	
		radio_recorder_exit ();
		check_record_file(data.rec_filepath);	

		/* Reinit the record data */
		data.rec_filename = radio_rec_get_filename();
		data.rec_media    = media;
		data.rec_filepath = get_store_path(data.rec_filename, data.rec_media);
		data.rec_stat	  = REC_PERPARE;

		if(!data.rec_filepath) {
			radio_tts(RADIO_REC_WRITE_ERR, 0);
			return -1;
		}

		nano.remain_time = radio_rec_get_remaintime(media);
		nano.freespace = radio_rec_get_freespace(media);
	
		radio_recorder_prepare(data.rec_filepath);
		led_ctrl(LED_ORANGE, 1);
#endif
		data.rec_media    = media;
		nano.record_recover = 0;//回到准备状态，因此不能再恢复

		//标题要重新显示
#if USE_LABEL_CONTROL		
		if(nano.filename_label && data.rec_filename)
			NeuxLabelSetText(nano.filename_label, data.rec_filename);
		NeuxLabelSetText(nano.remaintime_label, radio_rec_time_to_string(nano.remain_time));
#else

#if REC_USE_PREPARE
#else
		nano.remain_time = radio_rec_get_fake_remaintime(data.rec_media);
#endif

		rec_replace_time(nano.textShow.pRecTimeValue,
			radio_rec_time_to_string(nano.cur_time),nano.textShow.RecTimeValueLen);
		if(data.rec_filename)
			rec_replace_time(nano.textShow.pFilename,
				data.rec_filename,nano.textShow.FileNameLen);
		rec_replace_time(nano.textShow.pRemainTimeValue,
			radio_rec_time_to_string(nano.remain_time),nano.textShow.RemainTimeValueLen);
	
		Radio_Rec_showScreen();
#endif
	}
	return 0;
}


/* Flush the Record UI Window */
static void 
radio_rec_timer_handler (struct _rec_data* data)
{
#if USE_LABEL_CONTROL
	NeuxLabelSetText(nano.freshtime_label, radio_rec_time_to_string(nano.cur_time));
	if(nano.remain_time > 0)//当remain_time为0时，TTS还在发音，定时还在会使remain_time为负
		NeuxLabelSetText(nano.remaintime_label, radio_rec_time_to_string(nano.remain_time));
#else
	rec_replace_time(nano.textShow.pRecTimeValue,
		radio_rec_time_to_string(nano.cur_time),nano.textShow.RecTimeValueLen);

	if(nano.remain_time < 0) {
		nano.remain_time = 0;
	}
	rec_replace_time(nano.textShow.pRemainTimeValue,
		radio_rec_time_to_string(nano.remain_time),nano.textShow.RemainTimeValueLen);

	Radio_Rec_showScreen();

#endif
}


/* Clean up & recover Radio Window */
static void 
radio_rec_ui_uninit (struct _rec_data* data)
{
	radio_recorder_exit();	

	/* Set LED green on */
	led_ctrl(LED_GREEN, 0);
	
	radio_tts(RECORD_EXIT, 0);
	
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RADIO);

	/* empty file will be removed */	
	check_record_file(data->rec_filepath);


}


/* handler for sd card removed */
static void 
hotplug_sd_remove_handler ()
{
	/*这里可能需要语言提示*/
	if (!data.rec_media) {
		DBGMSG("SD_CARD removed, set to inner memory.\n");

		if(data.rec_stat == REC_RECORDING || data.rec_stat == REC_PAUSE) {
			DBGMSG("sd removed force exit!!\n");
			radio_tts(RADIO_REC_SD_REMOVE, 0);
			nano.rec_err = 1;
			return;
		}
				
		/* set with inner memory */
		data.rec_media = REC_STORE_INTER_MEM;

		/* clear up the record data */
		radio_recorder_exit ();
		check_record_file(data.rec_filepath);
		radio_tts(RADIO_REC_SD_REMOVE, 0);
	
		/* Reinit the record data */
		data.rec_filename = radio_rec_get_filename();
		data.rec_media    = REC_STORE_INTER_MEM;
		data.rec_filepath = get_store_path(data.rec_filename, data.rec_media);
		data.rec_stat	  = REC_PERPARE;

		/* Restart the recorder pipeline */
		radio_recorder_prepare(data.rec_filepath);
		
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		nano.remain_time = radio_rec_get_remaintime(REC_STORE_INTER_MEM);
		nano.freespace = radio_rec_get_freespace(REC_STORE_INTER_MEM);
		led_ctrl(LED_ORANGE, 1);

//		plextalk_schedule_unlock();
//		plextalk_sleep_timer_unlock();
//		NeuxWidgetDestroy(&nano.formRecMain);
//		radio_rec_exit();
		
	} else  {
		DBGMSG("Record media inner memory.\n");
		DBGMSG("No need to handler for sd_card removed\n");
	}
}


/* Play/Stop key will close the record mode */
static void 
radio_rec_key_play_stop_handler (struct _rec_data* data)
{
	DBGLOG("radio_rec_key_play_stop_handler\n");
	radio_rec_ui_uninit(data);
}

static void radio_rec_err_stop()
{
	radio_recorder_exit();
	/* Set LED green on */
	led_ctrl(LED_GREEN, 0);
	radio_tts(RECORD_EXIT_ERROR, 0);
	NeuxWidgetDestroy(&nano.formRecMain);
	radio_rec_exit();
}


#define PLEXTALK_SDA_RO 		"/sys/class/block/sda/ro"
#define PLEXTALK_MMC_RO 		"/sys/class/block/mmcblk1/ro"

static int rec_isMediaReadOnly(const char *path)
{
	int ret = 0;

	if(!path)
		return 0;
	DBGMSG("radio rec_isMediaReadOnly path:%s\n",path);

	if(strstr(path,"/mmc")) {
		sysfs_read_integer(PLEXTALK_MMC_RO, &ret);
	}
	DBGMSG("media is readonly:%d\n",ret);
	return ret;
}



int rec_process_before_recording(struct radio_nano* nano,struct _rec_data* data)
{
	// 1.先判断目标介质是否存在
	if ((!data->rec_media) && (!check_sd_exist())) {
		radio_tts(RADIO_TTS_NO_SDCARD,0);
		nano->rec_err = 1;
		DBGMSG("No SD card\n");
		return -1;
	}
	// 2.再判断目标介质是否可写
	if((!data->rec_media) && rec_isMediaReadOnly("/mmcblk")) {
		radio_tts(RADIO_REC_SD_READONLY,0);
		nano->rec_err = 1;
		DBGMSG("Read only.\n");
		return -1;
	}
	// 3.再往目标介质写文件夹
	data->rec_filepath = get_store_path(data->rec_filename, data->rec_media);
	if(!data->rec_filepath) {
		radio_tts(RADIO_REC_WRITE_ERR,0);
		nano->rec_err = 1;
		DBGLOG("media write error!\n");
		return -1;
	}
#if 0//可以放到准备界面去创建，因为可以在开始录音前设置路径	
	// 4.再初始化录音线程
	if (radio_recorder_prepare(data->rec_filepath) == -1) {
		DBGLOG("mic_rec_prepare error!\n");
		radio_tts(RADIO_REC_WRITE_ERR,0);
		nano->rec_err = 1;
		return -1;
	}
#endif	

	// 5.重新初始可录时间并刷新显示
	nano->remain_time = radio_rec_get_remaintime(data->rec_media);
	nano->freespace = radio_rec_get_freespace(data->rec_media);
	rec_replace_time(nano->textShow.pRecTimeValue,
		radio_rec_time_to_string(nano->cur_time),nano->textShow.RecTimeValueLen);
	if(nano->remain_time < 0) {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			radio_rec_time_to_string(0),nano->textShow.RemainTimeValueLen);
	} else {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			radio_rec_time_to_string(nano->remain_time),nano->textShow.RemainTimeValueLen);
	}
	Radio_Rec_showScreen();	

	// 6.再判断是否够容量写
	if(nano->remain_time <= 0){
		radio_tts(RADIO_REC_NOSPACE,0);
		nano->rec_err = 1;
		DBGLOG("not enough!\n");
		return -1;
	}
	return 0;
	
}





static void 
radio_rec_key_record_handler (struct _rec_data* data)
{
	DBGMSG("key record handler!\n");
	
	int res;

	switch (radio_recorder_stat()) {
		case PREPARE:
#if REC_USE_PREPARE
#else
			res = rec_process_before_recording(&nano,data);
			if(res == -1){
				return;
			}
			NhelperRecordingActive(1);
#endif			
			//fall through
		case PAUSE:
			res = radio_recorder_recording(data->rec_filepath,(data->rec_stat==REC_PERPARE)?1:0);
			if (res == -1) {
				radio_tts(CHANGE_STATE_ERROR, 0);
//				printf("[fatal]:error of set radio rec recording.\n");
				return;
			}
			data->rec_stat = REC_RECORDING;

			if(rec_need_hotplug_prompt(data)) {
				plextalk_set_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_hotplug_voice_disable(TRUE);
			}
			if(rec_need_SD_hotplug_prompt(data)) {
				plextalk_set_SD_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_SD_hotplug_voice_disable(TRUE);
			}
			
			led_ctrl(LED_ORANGE, 0);
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
			radio_tts(CHANGE_STATE_RECORDING, 0);
			break;

		case RECORDING:
			res = radio_recorder_pause();
			if (res == -1) {
				radio_tts(CHANGE_STATE_ERROR, 0);
//				printf("[fatal]:error of set radio rec recording.\n");
				return;
			}
			data->rec_stat = REC_PAUSE;
			if(rec_need_hotplug_prompt(data)) {
				plextalk_set_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_hotplug_voice_disable(TRUE);
			}
			if(rec_need_SD_hotplug_prompt(data)) {
				plextalk_set_SD_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_SD_hotplug_voice_disable(TRUE);
			}
			led_ctrl(LED_ORANGE, 1);
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
			radio_tts(CHANGE_STATE_PAUSE, 0);
			break;	
	}
}



/*Handler for radio rec volume adjust*/
extern struct radio_flags global_flags;

static void
fill_rec_stat_info (char* buf, int size)
{
	char* info = NULL;

	switch (radio_recorder_stat()) {
		case PREPARE:
			info = _("Preparing the recording");
			break;

		case REC_PAUSE:
			info = _("pause recording");
			break;
			
		case RECORDING:
			info = _("recording");
			break;

		default:
			DBGMSG("[Fatal ERROR]: not a correct state\n");
			info = _("Unkown");
			break;
	}
	snprintf(buf, size, "%s", info);

}


static void
fill_rec_elapse_info (char* buf, int size,int time)
{
	int elapse = time;//radio_recorder_time();

	int hour, mins, sec;
	
	sec = elapse % 60;
	elapse /= 60;
	mins = elapse % 60;
	elapse /= 60;
	//hour = elapse % 24;
	hour = elapse % 100;
#if 0	
	snprintf(buf, size, "%s:\n%d%s%d%s%d%s", _("elapsed recording"),
		hour, _("hour"), mins, _("minutes"), sec, _("seconds"));
#else
	snprintf(buf, size, "%s:\n%d %s %d %s", _("Recording time past"),
		hour, _("hour"), mins, _("minutes"));

#endif
}


static void
fill_rec_config_info (char* buf, int size, struct _rec_data* data)
{		
	char* media ;

	if (data->rec_media == 1)
		media = _("Internal memory");
	else
		media = _("SD card");
	
	snprintf(buf, size, "%s\n%s\n%s", _("Recording media:"), media,
		_("Recording source: Radio"));
}


#define MAX_INFO_ITEM	10

/*information for radio recorder*/
static void
radio_rec_key_info_handler(struct _rec_data* data)
{
	DBGMSG("radio rec info group.\n");
	TimerDisable(nano.rec_timer);
						
	/* Radio recorder Ungrab */
//	radio_rec_ungrab_key();

	radio_tts_abort();
	NhelperTitleBarSetContent(_("Information"));

#if 0
	if(road_flag == OUT_PUT_BYPASS)
	{
		amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);
		road_flag = OUT_PUT_DAC;
	}
#endif

	amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);

	int info_item_num = 0;

	/* Fill info item */
	char* radio_rec_info[MAX_INFO_ITEM];

	/* rec state info item */
	char rec_stat_info[128];
	fill_rec_stat_info(rec_stat_info, 128);
	radio_rec_info[info_item_num] = rec_stat_info;
	info_item_num ++;

	if (radio_recorder_stat() != PREPARE) {
		/* elapse info item */
		char rec_elapse_info[128];
		fill_rec_elapse_info(rec_elapse_info, 128,nano.cur_time);
		radio_rec_info[info_item_num] = rec_elapse_info;
		info_item_num ++;
	}

	/* configure info item */
	char rec_config_info[128];
	fill_rec_config_info(rec_config_info, 128, data);
	radio_rec_info[info_item_num] = rec_config_info;
	info_item_num ++;

	/* Start info */
	int* is_running = radio_tts_isrunning();

	nano.binfo = 1;
	nano.info_id = info_create();
	info_start (nano.info_id, radio_rec_info, 
		info_item_num,is_running);
	
	info_destroy (nano.info_id);
	
	nano.binfo = 0;
	/* Radio recorder Grab again */
	//NeuxWidgetFocus(nano.formRecMain);
//	radio_rec_grab_key();
//去掉bypass切换
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_BYPASS);
	road_flag = OUT_PUT_BYPASS;
	NhelperTitleBarSetContent(_("Recording Radio"));

	TimerEnable(nano.rec_timer);

}

#if 0
void
rec_event_headphone_handler ()
{
	//暂时对radio里面的record,耳机插拔不做处理
	//等下来处理这里的情况
		//检查是否有耳机插入，有耳机插入(模式为耳机模式，用耳机输出，否则外放输出)，无耳机插入则用外放输出
	if (check_headphone()) {
		int new_out = g_config->setting.radio.output;
		if(new_out == OUTPUT_HEADPHONE)
		{
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_OFF);
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_ON);
			global_flags.out_put = OUTPUT_HEADPHONE;
		}
		else
		{
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
			global_flags.out_put = OUTPUT_SPEAKER;	
		}
	} else {
		amixer_direct_sset_str(VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
		amixer_direct_sset_str(VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
		global_flags.out_put = OUTPUT_SPEAKER;
	}
}

/* handler for sig menu exit handler */
static void sig_rec_menu_exit_handler (struct radio_nano* nano)
{			
	DBGMSG("radio rec menu exit.\n");
	global_flags.menu_flag = 0;
	
	radio_rec_grab_key();
	NhelperTitleBarSetContent(_("Radio Recording"));
	if(nano->sig_menu_exit)
		nano->sig_menu_exit = 0;

	if (nano->record_recover)
	{
		radio_recorder_recording();
		nano->record_recover = 0;
		data.rec_stat = REC_RECORDING;
	}
}

static void sig_rec_change_media_handler (struct radio_nano* nano)
{			
	DBGMSG("Radio recorder change media!\n");

	radio_rec_change_media();
	nano->sig_chg_mea = 0;
	sig_rec_menu_exit_handler(nano);
}


/* handler for change the system language */
static void sig_rec_change_language_handler (struct radio_nano* nano)
{			
	DBGMSG("sig_change_language_handler\n");

	sys_set_cur_lang_env("radio");	

	nano->set_language= 0;
	sig_rec_menu_exit_handler(nano);
}

/* handler for change the system volume */
static void sig_rec_change_guid_volume_handler(struct radio_nano* nano)
{
	radio_tts_change_volume();
	nano->set_guid_volume = 0;
	sig_rec_menu_exit_handler(nano);
}

/* handler for change the system speed */
static void sig_rec_change_guid_speed_handler(struct radio_nano* nano)
{
	nano->set_guid_speed = 0;
	sig_rec_menu_exit_handler(nano);
}

/* handler for change the system fontsize */
static void sig_rec_menu_font_size_handler(struct radio_nano* nano)
{
	nano->set_font_size= 0;
	sig_rec_menu_exit_handler(nano);
}

/* handler for change the system tts */
static void sig_rec_menu_set_tts_handler(struct radio_nano* nano)
{
	nano->set_tts = 0;
	sig_rec_menu_exit_handler(nano);
}

/* handler for change the system backlight */
static void sig_rec_menu_set_backlight_handler(struct radio_nano* nano)
{
	nano->set_backlight = 0;
	sig_rec_menu_exit_handler(nano);
}


/* handler for change the system language */
static void sig_rec_change_font_color_handler (struct radio_nano* nano)
{			
	DBGMSG("sig_change_font_color_handler\n");

	int theme_type  = sys_get_global_skin();

	nano->fg_color = sys_get_cur_theme_fgcolor(theme_type);
	nano->bg_color = sys_get_cur_theme_bgcolor(theme_type);

	GrSetGCForeground(nano->ex_gc, nano->bg_color);

	/*set main gc*/
	GrSetGCForeground(nano->gc, nano->fg_color);
	GrSetGCBackground(nano->gc, nano->bg_color);

	nano->set_font_color = 0;
	sig_rec_menu_exit_handler(nano);
}



/*这个check是因为外面的signal_check不会执行，外面不会来time_out
 *所以这里执行一个signal_check
 *对于radio_rec，只需要判断一gemnu，help是不需要各个去判断的
 */
static void radio_rec_check_signal (struct radio_nano* nano)
{
	if (nano->sig_menu_exit){
		sig_rec_menu_exit_handler(nano);
	}
	else if(nano->sig_chg_mea){
		sig_rec_change_media_handler(nano);
	}
	else if(nano->set_guid_volume){
		sig_rec_change_guid_volume_handler(nano);
	}
	else if(nano->set_guid_speed){
		sig_rec_change_guid_speed_handler(nano);
	}
	else if(nano->set_font_size){
		sig_rec_menu_font_size_handler(nano);
	}
	else if(nano->set_font_color){
		sig_rec_change_font_color_handler(nano);
	}
	else if(nano->set_language){
		sig_rec_change_language_handler(nano);
	}
	else if(nano->set_backlight){
		sig_rec_menu_set_backlight_handler(nano);
	}
	else if(nano->set_tts){
		sig_rec_menu_set_tts_handler(nano);
	}
}
#endif

static void 
radio_rec_init ()
{
	/* Init the record data */
	
	
	/* media :1, means inner memory, 0, means sd card */
	CoolShmReadLock(g_config_lock);
	data.rec_media = g_config->setting.record.saveto;
	CoolShmReadUnlock(g_config_lock);

#if REC_USE_PREPARE//准备界面 不更改其他录音介质，等到开始录音时再提示后退出	
	if ((!data.rec_media) && (!check_sd_exist())) {
		DBGMSG("No SD card , set to inner memory.\n");
		/* set with inner memory */
		data.rec_media = 1;
	}
#endif	

	data.rec_filename = radio_rec_get_filename();
	data.rec_stat	  = REC_PERPARE;
	nano.cur_time = 0;
	nano.freespace = 0;


	/* Set monitor ON as defaults */
	if (radio_is_muted())
	radio_set_mute(false, 0);

	NhelperTitleBarSetContent(_("Radio recording"));
	/* This TTS can be interrupted */
	radio_tts(RECORD_USAGE, 0);
	/* Set LED orange & blink */
	led_ctrl(LED_ORANGE, 1);

	/* src icon:  radio */
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RADIO);
	
	if (data.rec_media)
	{
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
	}
	else{
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	}
	nano.remain_time = radio_rec_get_remaintime(data.rec_media);
	nano.freespace = radio_rec_get_freespace(data.rec_media);

	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK); 

#if REC_USE_PREPARE
	data.rec_filepath = get_store_path(data.rec_filename, data.rec_media);
	/* Create pipeline for radio_recorder */
	radio_recorder_prepare(data.rec_filepath);	
#else
	nano.remain_time = radio_rec_get_fake_remaintime(data.rec_media);
	radio_recorder_prepare(data.rec_filepath);	
#endif	
}



static void radio_rec_createlabels (struct _rec_data* data,struct radio_nano* pnano)	
{
#ifdef USE_LABEL_CONTROL

	widget_prop_t wprop;
	label_prop_t lprop;

	nano.filename_label= NeuxLabelNew(nano.formRecMain, MAINWIN_LEFT, FM_LABEL_T, MAINWIN_WIDTH, RADIO_LABEL_H,_(""));
	
	NeuxWidgetGetProp(nano.filename_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.filename_label, &wprop);
	
	NeuxLabelGetProp(nano.filename_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano.filename_label, &lprop);
	NeuxWidgetSetFont(nano.filename_label, sys_font_name, 16);
	NeuxWidgetShow(nano.filename_label, FALSE);

	nano.rectime_label= NeuxLabelNew(nano.formRecMain,MAINWIN_LEFT, FM_LABEL_T+RADIO_LABEL_H, MAINWIN_WIDTH, RADIO_LABEL_H,_("Recording Time:"));
	
	NeuxWidgetGetProp(nano.rectime_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.rectime_label, &wprop);
	
	NeuxLabelGetProp(nano.rectime_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano.rectime_label, &lprop);
	NeuxLabelSetText(nano.rectime_label, _("Recording Time:"));
	NeuxWidgetSetFont(nano.rectime_label, sys_font_name, 16);
	NeuxWidgetShow(nano.rectime_label, FALSE);

	nano.freshtime_label= NeuxLabelNew(nano.formRecMain,MAINWIN_LEFT, FM_LABEL_T+2*RADIO_LABEL_H, MAINWIN_WIDTH, RADIO_LABEL_H,"00:00:00");
	
	NeuxWidgetGetProp(nano.freshtime_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.freshtime_label, &wprop);
	
	NeuxLabelGetProp(nano.freshtime_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano.freshtime_label, &lprop);
	NeuxWidgetSetFont(nano.freshtime_label, sys_font_name, 16);
	NeuxWidgetShow(nano.freshtime_label, FALSE);

	nano.remain_label= NeuxLabelNew(nano.formRecMain,MAINWIN_LEFT, FM_LABEL_T+3*RADIO_LABEL_H, MAINWIN_WIDTH, RADIO_LABEL_H,_("Remaining Time:"));
	
	NeuxWidgetGetProp(nano.remain_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.remain_label, &wprop);
	
	NeuxLabelGetProp(nano.remain_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano.remain_label, &lprop);
	NeuxWidgetSetFont(nano.remain_label, sys_font_name, 16);
	NeuxWidgetShow(nano.remain_label, FALSE);


	nano.remaintime_label = NeuxLabelNew(nano.formRecMain,MAINWIN_LEFT, FM_LABEL_T+4*RADIO_LABEL_H, MAINWIN_WIDTH, RADIO_LABEL_H,_(""));
	
	NeuxWidgetGetProp(nano.remaintime_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.remaintime_label, &wprop);
	
	NeuxLabelGetProp(nano.remaintime_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano.remaintime_label, &lprop);
	NeuxWidgetSetFont(nano.remaintime_label, sys_font_name, 16);
	NeuxWidgetShow(nano.remaintime_label, FALSE);

#else	
	Radio_Rec_CreateGc(pnano->formRecMain,pnano);
#if REC_USE_PREPARE
	pnano->remain_time = radio_rec_get_remaintime(data->rec_media);
	pnano->freespace = radio_rec_get_freespace(data->rec_media);
#else
	pnano->remain_time = radio_rec_get_fake_remaintime(data->rec_media);
#endif
	Radio_Rec_InitStringBuf(pnano,data->rec_filename);
	Radio_RecShow_Init(pnano,/*sys_font_size*/16);
#endif	

}

static void onRadioRecHotplug(WID__ wid, int device, int index, int action)
{
	if(nano.app_pause || nano.binfo) {
		DBGMSG("Hey!,I am in pause or info,so your key will be ingnore\n");
		return;
	}
		
	switch (action) {
	case MWHOTPLUG_ACTION_ADD:
		break;
	case MWHOTPLUG_ACTION_REMOVE:
		switch (device) {
		case MWHOTPLUG_DEV_MMCBLK:
			{
				DBGMSG("remove sdcard\n");
#if REC_USE_PREPARE
				hotplug_sd_remove_handler();
#else
				if(data.rec_stat == REC_RECORDING || data.rec_stat == REC_PAUSE) {
					hotplug_sd_remove_handler();
				}
#endif
			}
			break;
		default:
			return;
		}
		break;

	case MWHOTPLUG_ACTION_CHANGE:
		break;
	}
}


static void OnRadioRecDestroy(WID__ wid)
{
    DBGLOG("--------------OnRadioRecDestroy %d.\n",wid);

	NxSetWidgetFocus(nano.formMain);
		
	Radio_Rec_FreeStringBuf(&nano);
	if(data.ishotplugvoicedisable) {
		plextalk_set_hotplug_voice_disable(FALSE);
	}
	if(data.isSDhotplugvoicedisable)
		plextalk_set_SD_hotplug_voice_disable(FALSE);

//clean up
	radio_ui_freq_recover();	//recover radio ui(set text actually)

	led_ctrl(LED_GREEN, 0);
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RADIO);
	plextalk_schedule_unlock();
	plextalk_sleep_timer_unlock();
	plextalk_suspend_unlock();

	TimerEnable(nano.timer);
	/*recover when out of recorder*/
	NhelperTitleBarSetContent(_("Hearing Radio"));
	global_flags.recording = 0;
	nano.record_recover = 0;

	/* Tell desktop radio not in recording mode */
	NhelperRadioRecordingActive(0);
	NhelperRecordingActive(0);

	/* Resume titlebar ... */
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, 
		nano.bPause ? SBAR_CONDITION_ICON_PAUSE : SBAR_CONDITION_ICON_PLAY);	
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
//clean up end

		
	/*if(nano.formRecMain)
	{
		 DBGLOG("--------------OnRadioRecDestroy 1\n");
		TimerDelete(nano.rec_timer);
		 DBGLOG("--------------OnRadioRecDestroy 2\n",wid);
		NeuxFormDestroy(&nano.formRecMain);
		 DBGLOG("--------------OnRadioRecDestroy 3\n",wid);
		nano.formRecMain = NULL;
	}*/
}

#if 0
static void OnRecLongKeyTimer(WID__ wid)
{
	DBGMSG("OnRecLongKeyTimer!\n");
	switch(nano.rec_l_key.key)
	{
	case MWKEY_INFO:
		TimerDisable(nano.rec_l_key.lid);
		nano.rec_l_key.l_flag = 0;
		break;
	}
	
}
#endif


static void	radio_rec_monitor_switch()
{
	if (radio_is_muted()) {
		radio_set_mute(false, 0);
		radio_tts(RADIO_REC_MONITOR_SWITCH, 1);
	} else {
		radio_set_mute(true, 0);
		radio_tts(RADIO_REC_MONITOR_SWITCH, 0);
	}
}


static void OnRadioRecKeydown(WID__ wid, KEY__ key)
{
    DBGLOG("OnRadioRecKeydown, key.ch = %x\n", key.ch);

	if(nano.app_pause || nano.binfo) {
		DBGMSG("Hey!,I am in pause or info,so your key will be ingnore\n");
		return;
	}

	
	if (NeuxAppGetKeyLock(0)) {
		 if (!key.hotkey) {
		   voice_prompt_abort();
		   
		   voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		   voice_prompt_string_ex2(0, PLEXTALK_OUTPUT_SELECT_BYPASS, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		      tts_lang, tts_role, _("Keylock"), strlen(_("Keylock")));
		  }
		  return;
	 }

	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		DBGLOG("OnRadioRecKeydown help out\n");
		return;
	}
	switch (key.ch) {		
		case VKEY_RECORD:
			radio_rec_key_record_handler(&data);
			break;
		case MWKEY_ENTER:
			radio_rec_key_play_stop_handler(&data);
//			radio_rec_ungrab_key();
//			plextalk_schedule_unlock();
//			plextalk_sleep_timer_unlock();
//			radio_ui_freq_recover();			//in case for label exposure (manual will be shown)
			NeuxWidgetDestroy(&nano.formRecMain);
			radio_rec_exit();
			break;

#ifdef USE_LABEL_CONTROL			
		case VKEY_BOOK:
		case VKEY_MUSIC:
		case VKEY_RADIO:
		case MWKEY_DOWN:
		case MWKEY_UP:
			radio_tts(TTS_ERROR_BEEP, 0);
			break;
				
		case MWKEY_RIGHT:
		case MWKEY_LEFT:
			radio_rec_monitor_switch();
			break;
#else
		case VKEY_BOOK:
		case VKEY_MUSIC:
		case VKEY_RADIO:
			radio_tts(TTS_ERROR_BEEP, 0);
			break;
			
		case MWKEY_RIGHT:
		case MWKEY_LEFT:
			radio_rec_monitor_switch();
			break;
		case MWKEY_UP:
#if 0			
				if(Radio_RecShow_prevScreen(&nano)>=0) {
					Radio_Rec_showScreen();
					//NeuxFormShow(widget, TRUE);
					radio_tts(TTS_NORMAL_BEEP,0);
				}
				else
#endif					
					radio_tts(TTS_ERROR_BEEP,0);
				break;
		case MWKEY_DOWN:
#if 0
				if(Radio_RecShow_nextScreen(&nano)>=0) {
					Radio_Rec_showScreen();
					//NeuxFormShow(widget, TRUE);
					radio_tts(TTS_NORMAL_BEEP,0);
				}
				else
#endif
				radio_tts(TTS_ERROR_BEEP,0);
				break;

#endif
		case MWKEY_INFO:
			radio_rec_key_info_handler(&data);			
			break;
	}
}

static void OnRadioRecGetFocus(WID__ wid)
{
	DBGLOG("On Window Get Focus\n");
	if (data.rec_media)
	{
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
	}
	else{
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	}

	if(data.rec_stat == REC_RECORDING)
	{
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
	}
	else
	{
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
	}
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RADIO);
	
}

static void
OnRecTimer(WID__ wid)
{

#ifdef USE_LABEL_CONTROL
	if(nano.rec_first_logo)
	{
		//Show window
		NeuxWidgetShow(nano.filename_label, TRUE);
		NeuxWidgetShow(nano.rectime_label, TRUE);
		NeuxWidgetShow(nano.freshtime_label, TRUE);
		NeuxWidgetShow(nano.remain_label, TRUE);
		NeuxWidgetShow(nano.remaintime_label, TRUE);
		NeuxLabelSetText(nano.filename_label, data.rec_filename);
		NeuxLabelSetText(nano.remaintime_label, radio_rec_time_to_string(nano.remain_time));
		nano.rec_first_logo = 0;
		return;
	}
#endif

	if(nano.rec_err && radio_tts_get_stopped())
	{
		radio_rec_err_stop();
		nano.rec_err = 0;
		return;
	}

	if(REC_RECORDING == data.rec_stat && !nano.rec_err)
	{
		flag++;
		if(flag >= SEC_IN_MILLISECONDS/TIMER_PERIOD)
		{
			flag = 0;
			nano.cur_time++;
			nano.remain_time--;

			nano.freespace -= (128000/8);

			
			radio_rec_timer_handler(&data);

			if(nano.freespace <= (128000/8)){
				radio_tts(RADIO_REC_NOSPACE, 0);
				nano.rec_err = 1;
			}
			if(nano.remain_time <= 0 || radio_rec_file_uplimit(data.rec_filepath))
			{
				radio_tts(RADIO_REC_LIMIT, 0);
				nano.rec_err = 1;
			}
			
		}
	}
	else if(REC_PERPARE == data.rec_stat)
	{
		flag = 0;
		nano.cur_time = 0;
	}
	else if(REC_PAUSE== data.rec_stat)
	{
		flag = 0;
	}

	if ((road_flag == OUT_PUT_DAC) && (!global_flags.menu_flag) 
		&& (!NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP)) && (radio_tts_get_stopped())) {


		//去掉bypass切换,timer里面不再切换bypass
		//amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_BYPASS);
		road_flag = OUT_PUT_BYPASS;
//		printf("Radio_rec Set Bypass over\n");
	}

	//只有在录音过程中才刷新
	//radio_rec_timer_handler(&data);

}


static void OnRadioRecWindowRedraw(int clrBg)
{
	GR_WINDOW_ID wid;
	GR_GC_ID gc;
	
	wid = nano.wid;
	gc = nano.pix_gc;

	if(wid != GrGetFocus()){
		//DBGMSG("NOT FOCUS\n");
		//return;
	}
	
	if(clrBg){
		NxSetGCForeground(gc, theme_cache.background);
		GrFillRect(wid, gc, 0, 0, nano.textShow.w, nano.textShow.h);
		NxSetGCForeground(gc, theme_cache.foreground);
	}
	
	Radio_RecShow_showScreen(&nano);
}

static void OnRadioRecWindowExposure(WID__ wid)
{
	DBGLOG("------exposure-------\n");
	OnRadioRecWindowRedraw(1);
}


int Radio_Rec_showScreen(void)
{
	//DBGMSG("show\n");
	OnRadioRecWindowRedraw(1);
}




void radio_rec_createform()
{
	widget_prop_t wprop;
	
	nano.formRecMain = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	/* Create timer */
   	nano.rec_timer = NeuxTimerNew(nano.formRecMain, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano.rec_timer, OnRecTimer);
	TimerDisable(nano.rec_timer);

	NeuxFormSetCallback(nano.formRecMain, CB_KEY_DOWN, OnRadioRecKeydown);
	NeuxFormSetCallback(nano.formRecMain, CB_FOCUS_IN, OnRadioRecGetFocus);
	NeuxFormSetCallback(nano.formRecMain, CB_HOTPLUG,  onRadioRecHotplug);
	NeuxFormSetCallback(nano.formRecMain, CB_DESTROY,  OnRadioRecDestroy);
	NeuxFormSetCallback(nano.formRecMain, CB_EXPOSURE, OnRadioRecWindowExposure);

	NeuxWidgetGetProp(nano.formRecMain, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano.formRecMain, &wprop);
	NeuxWidgetSetFont(nano.formRecMain, sys_font_name, /*sys_font_size*/16);
	
}


static void Rec_show_logo()
{
	widget_prop_t wprop;
	label_prop_t lbprop;
	GR_GC_ID gc;
	int font_id;
	char *pstr = _("RECORDER");
	int w,h,b;
	int x,y;

	
	DBGMSG("rec_show_logo\n");

	gc=GrNewGC();

	font_id = GrCreateFont((GR_CHAR*)sys_font_name, 24, NULL);
	GrSetGCUseBackground(gc ,GR_FALSE);
	GrSetGCForeground(gc,theme_cache.foreground);
	GrSetGCBackground(gc,theme_cache.background);
	GrSetFontAttr(font_id , GR_TFKERNING, 0);
	GrSetGCFont(gc, font_id);

	NxGetGCTextSize(gc, pstr, -1, MWTF_UTF8, &w, &h, &b);
	x = (MAINWIN_WIDTH - w)/2;
	y = (MAINWIN_HEIGHT - 24)/2;
						
	GrText(NeuxWidgetWID(nano.formRecMain), gc, x,y,  pstr, strlen(pstr), MWTF_UTF8 | MWTF_TOP);

	GrFlush();

}

int radio_rec_dialog()
{
	flag = 0;
	//rec界面只关屏，不休眠
	plextalk_suspend_lock();
	plextalk_schedule_lock();
	plextalk_sleep_timer_lock();
	radio_rec_createform();
	NeuxFormShow(nano.formRecMain, TRUE);
	Rec_show_logo();
	NeuxWidgetFocus(nano.formRecMain);
//	radio_rec_grab_key();
	radio_rec_init();

	//创建四个LABEL用于显示字符串
	radio_rec_createlabels(&data,&nano);
	
    TimerEnable(nano.rec_timer);
	nano.rec_first_logo = 1;
}


