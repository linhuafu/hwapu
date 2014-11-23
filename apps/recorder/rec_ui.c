#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <microwin/device.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <locale.h>
#include <libintl.h>
#include <gst/gst.h>
#include <stdlib.h>
//#include "keydef.h"
//#include "menu_notify.h"
//#include "efont.h"
#define  DEBUG_PRINT 1
//#include "info_print.h"
#include "mic_rec.h"
#include "rec_draw.h"
#include "rec_tts.h"
#include "rec_led.h"
#include "rec_extern.h"
//#include "system.h"
//#include "sys_config.h"
//#include "arch_def.h"
//#include "p_mixer.h"
#include "libvprompt.h"
//#include "libinfo.h"
#include "Nx-widgets.h"
#include "neux.h"

#include "plextalk-config.h"
#include "Amixer.h"
#include "plextalk-statusbar.h"
#include "plextalk-keys.h"
#include "desktop-ipc.h"
#include "plextalk-theme.h"
#include "application-ipc.h"
#include "plextalk-ui.h"
#include "libinfo.h"

#define OSD_DBG_MSG
#include "nc-err.h"

#include "volume_ctrl.h"  

#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


#define TIMER_PERIOD		500
static FORM *  formMain;
#define widget formMain

#define FILE_NAME_LABEL_X	0
#define FILE_NAME_LABEL_Y	(0)
#define INPUT_SOURCE_LABEL_X	0
#define INPUT_SOURCE_LABEL_Y	(FILE_NAME_LABEL_Y+22)
#define REC_TIME_LABEL_X	0
#define REC_TIME_LABEL_Y	(INPUT_SOURCE_LABEL_Y+22)//64
#define FRESH_TIME_LABEL_X	0
#define FRESH_TIME_LABEL_Y	(REC_TIME_LABEL_Y+24)//88

#define REC_LABEL_HEIGHT	19


#define _(S)	gettext(S)

/* this define is for hotplug */

#define SEC_IN_MILLISECONDS		1000


struct app_flags {
	int menu;
	int help;
	int monitor_recover;
	int record_recover;
};


static struct app_flags app;


static struct _rec_data s_data;
static struct rec_nano s_nano;
static int rec_counter= 0;
struct plextalk_language_all all_langs;
static int key_locked;


#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))

static int radio_grab_table[] = { 

		VKEY_BOOK,
		VKEY_MUSIC,
		VKEY_RECORD,
		VKEY_RADIO,
		MWKEY_ENTER,
		MWKEY_RIGHT,
		MWKEY_LEFT,
		MWKEY_UP,
		MWKEY_DOWN,
};
static int Rec_showScreen(void);


static void
rec_grab_key(struct rec_nano* nano)
{
#if 0
	int i;

	for(i = 0; i < ARRAY_SIZE(radio_grab_table); i++)
		NeuxWidgetGrabKey(widget, radio_grab_table[i], NX_GRAB_HOTKEY_EXCLUSIVE);
#endif	
}


static void
rec_ungrab_key (struct rec_nano* nano)
{
#if 0
	int i;
	for(i = 0; i < ARRAY_SIZE(radio_grab_table); i++)
		NeuxWidgetUngrabKey(widget, radio_grab_table[i]);
#endif	
}

//是否需要响应闹铃
static int
rec_need_alarm(struct _rec_data* data)
{
	if (!mic_headphone_on() && data->rec_stat == REC_RECORDING) {
		data->isalarmdisable = 1;
		DBGMSG("disable alarm\n");
		return 0;
	}
	data->isalarmdisable = 0;
	DBGMSG("enable alarm\n");
	return 1;
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



static void
rec_read_keylock()
{
	char buf[16];
	int res;
	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0")){
	key_locked = 1;
	}else{
	key_locked = 0;
	}
	DBGMSG("keylock:%d\n",key_locked);
}



void p_mixer_set (int choice, int val)
{
	char* pra[2];
	char buf[32];
	int flag = 0;
	const char* info = NULL;

	int adjust_vol;

	int battery;
	
	switch (choice) {
		case ANALOG_PATH_IGNORE_SUSPEND:
			pra[0] = "\'Analog Path Ignore Suspend\'";
			if(!val){
				pra[1] = "off";
				info = "[pmixer]: Analog Path Ignore Suspend off!\n";
			}
			else{
				pra[1] = "on";
				info = "[pmixer]: Analog Path Ignore Suspend on!\n";
			}
			flag = 1;
			break;
		case OUTPUT_BYPASS:
			pra[0] = "\'Output Mux\'";
			pra[1] = "Bypass";
			info = "[pmixer]:output mux bypass!\n";
			flag = 1;
			break;

		case OUTPUT_DAC:
			pra[0] = "\'Output Mux\'";
			pra[1] = "DAC"; 
			info = "[pmixer]:output mux DAC!\n";
			flag = 1;
			break;

		case OUTPUT_SIDETONE:
			pra[0] = "\'Output Mux\'";
			pra[1] = "Sidetone1";
			info = "[pmixer]:output sidetone!\n";
			flag = 1;
			break;

		case INPUT_MIC1:
			pra[0] = "\'Input Mux\'";
			pra[1] = "Mic1";
			info = "[pimxer]:Input mux Mic1!\n";
			flag = 1;
			break;

		case INPUT_LINEIN:
			pra[0] = "\'Input Mux\'";
			pra[1] = "Line In";
			info = "[pmixer]:input linein!\n";
			flag = 1;
			break;

		case MIC1_BOOST:
			pra[0] = "\'Mic1 Boost\'";
			snprintf(buf, 32, "%d", val);
			pra[1] = buf;
			info = "[pmixer]:Mic1 boost set!\n";
			flag = 1;
			break;
			
		case CAPTURE:
			pra[0] = "\'Capture\'";
			snprintf(buf, 32, "%d", val);
			pra[1] = buf;
			info = "[pmixer]: set Capture!\n";
			flag = 1;
			break;

		case AGC_ON:
			pra[0] = "\'AGC\'";
			pra[1] = "on";
			info = "[pmixer]:AGC on!\n";
			flag = 1;
			break;

		//case SPEAKER_ON://NhelperRecordingActive has already implement this function
		//	pra[0] = "Speaker";
		//	pra[1] = "on";
		//	info = "[pmixer]: Speaker on!\n";
		//	break;

		//case SPEAKER_OFF://NhelperRecordingActive has already implement this function
		//	pra[0] = "Speaker";
		//	pra[1] = "off";
		//	info = "[pmixer]: Speaker off!\n";
		//	break;

		//case HEADPHONE_ON://NhelperRecordingActive has already implement this function
		//	pra[0] = "Headphone";
		//	pra[1] = "on";
		//	info = "[pmixer]: Headphone on!\n";
		//	break;

		//case HEADPHONE_OFF://NhelperRecordingActive has already implement this function
		//	pra[0] = "Headphone";
		//	pra[1] = "off";
		//	info = "[pmixer]: Headphone off!\n";
		//	break;

		case VOLUME_SET:
			break;

		default:
			info = "[pmixer]: No contorl!\n";
			break;
	}

	if(flag)
	amixer_direct_sset_str(VOL_RAW, pra[0], pra[1]);
	
	//printf("%s", info);
}

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


void 
rec_timer_handler (struct rec_nano* nano, struct _rec_data* data)
{
	//rec_draw_image(nano);
	//rec_draw_source(nano, data);
	//rec_draw_title(nano, data);
	//rec_draw_time(nano, data);
	//rec_draw_copy(nano);
#ifdef USE_LABEL_CONTROL
	NeuxLabelSetText(nano->rectime_value, time_to_string(nano->cur_time));
	NeuxWidgetShow(nano->rectime_value, TRUE);
	NeuxLabelSetText(nano->remaintime_value, time_to_string(nano->remain_time));
	NeuxWidgetShow(nano->remaintime_value, TRUE);
#else
	rec_replace_time(nano->textShow.pRecTimeValue,
		time_to_string(nano->cur_time),nano->textShow.RecTimeValueLen);

	if(nano->remain_time < 0) {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			time_to_string(0),nano->textShow.RemainTimeValueLen);
	} else {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			time_to_string(nano->remain_time),nano->textShow.RemainTimeValueLen);
	}

	
	Rec_showScreen();
	
#endif
	
}


void 
rec_key_record_handler(struct _rec_data* data)
{
	DBGLOG("key record handler!\n");
	int res;

	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");

	switch (data->rec_stat) {
		case REC_RECORDING:

			if(data->rec_media == 0 && !check_sd_exist()){
				rec_tts(REC_NOTARGET);
			}
			
			res = mic_rec_pause();
			if (res == -1) {
				rec_tts(CHANGE_STATE_ERROR);
				return;
			}
			data->rec_stat = REC_PAUSE;

			if(rec_need_alarm(&s_data)) {
				plextalk_set_alarm_disable(FALSE);
			} else {
				plextalk_set_alarm_disable(TRUE);
			}
			
			if(rec_need_hotplug_prompt(&s_data)) {
				plextalk_set_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_hotplug_voice_disable(TRUE);
			}
			
			if(rec_need_SD_hotplug_prompt(&s_data)) {
				plextalk_set_SD_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_SD_hotplug_voice_disable(TRUE);
			}

			//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_CONDITION_RECORD_BLINK);
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
 			//NhelperRecordingActive(0);
 			NhelperRecordingNow(0);
			led_ctrl(LED_ORANGE, 1);
			rec_tts(CHANGE_STATE_PAUSE);
			break;
			
		case REC_PERPARE:
		case REC_PAUSE:
#if 0			
			res = mic_rec_recording();
			if (res == -1) {
				return;
			}
			data->rec_stat = REC_RECORDING;
			rec_tts(NORMAL_BEEP);
			//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_CONDITION_RECORD);
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
			led_ctrl(LED_ORANGE, 0);
#else
			rec_tts(REC_STARTRECORD);

#endif			
			break;
	}
}

#define PLEXTALK_MMC_RO 		"/sys/class/block/mmcblk1/ro"

static int rec_isMediaReadOnly(const char *path)
{
	int ret = 0;

	if(!path)
		return 0;
	DBGMSG("format_isMediaReadOnly path:%s\n",path);

	if(strstr(path,"/mmc")) {
		sysfs_read_integer(PLEXTALK_MMC_RO, &ret);
	}
	DBGMSG("media is readonly:%d\n",ret);
	return ret;
}



int rec_process_before_recording(struct rec_nano* nano,struct _rec_data* data)
{
	// 1.先判断目标介质是否存在
	if ((!data->rec_media) && (!check_sd_exist())) {
		rec_tts(REC_NOTARGET);
		info_debug("No SD card\n");
		return -1;
	}
	// 2.再判断目标介质是否可写
	if((!data->rec_media) && rec_isMediaReadOnly("/mmcblk")) {
		rec_tts(REC_READONLY);
		info_debug("Read only.\n");
		return -1;
	}
	// 3.再往目标介质写文件夹
	data->rec_filepath = get_store_path(data->rec_filename, data->rec_media);
	if(!data->rec_filepath) {
		rec_tts(REC_WRITEERROR);
		DBGLOG("media write error!\n");
		return -1;
	}

#if 0//线程初始化可以放到准备界面去，因为可以在	mic_rec_recording里设置路径
	// 4.再初始化录音线程
	if (mic_rec_prepare(data->rec_filepath) == -1) {
		DBGLOG("mic_rec_prepare error!\n");
		rec_tts(REC_WRITEERROR);
		return -1;
	}
#endif	

	// 5.重新初始可录时间并刷新显示
	nano->remain_time = rec_get_remaintime(data->rec_media);
	nano->freespace = rec_get_freespace(data->rec_media);
	rec_replace_time(nano->textShow.pRecTimeValue,
		time_to_string(nano->cur_time),nano->textShow.RecTimeValueLen);
	if(nano->remain_time < 0) {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			time_to_string(0),nano->textShow.RemainTimeValueLen);
	} else {
		rec_replace_time(nano->textShow.pRemainTimeValue,
			time_to_string(nano->remain_time),nano->textShow.RemainTimeValueLen);
	}
	Rec_showScreen();	

	// 6.再判断是否够容量写
	if(nano->remain_time <= 0){
		rec_tts(REC_NOTENOUGH);
		DBGLOG("not enough!\n");
		return -1;
	}
	return 0;
	
}



void rec_start_record()
{
	int res;	
	static int firstin = 1;
	DBGMSG("stop all:%d\n",firstin);
	if(firstin) {
		NeuxStopAllOtherApp(TRUE);
		firstin = 0;
	}
	DBGMSG("stop all end \n");
#if REC_USE_PREPARE
#else
	if(s_data.rec_stat == REC_PERPARE) {
		res = rec_process_before_recording(&s_nano,&s_data);
		if(res == -1) {
			return;
		}
	}
#endif	
	res = mic_rec_recording(s_data.rec_filepath,(s_data.rec_stat == REC_PERPARE)?1:0);
	if (res == -1) {
		return;
	}
	s_data.rec_stat = REC_RECORDING;

	if(rec_need_alarm(&s_data)) {
		plextalk_set_alarm_disable(FALSE);
	} else {
		plextalk_set_alarm_disable(TRUE);
	}
	if(rec_need_hotplug_prompt(&s_data)) {
		plextalk_set_hotplug_voice_disable(FALSE);
	} else {
		plextalk_set_hotplug_voice_disable(TRUE);
	}
	if(rec_need_SD_hotplug_prompt(&s_data)) {
		plextalk_set_SD_hotplug_voice_disable(FALSE);
	} else {
		plextalk_set_SD_hotplug_voice_disable(TRUE);
	}

	
	//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_CONDITION_RECORD);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
	NhelperRecordingActive(1);
	NhelperRecordingNow(1);
	led_ctrl(LED_ORANGE, 0);
}

void 
rec_key_right_handler (void)
{	
	DBGLOG("rec_key_right_handler!\n");

	if (mic_headphone_on())
		mic_rec_switch_monitor();
	else
		rec_tts(ERROR_BEEP);

#if 0
	if (mic_headphone_on() && (s_data.rec_stat == REC_RECORDING))
		mic_rec_switch_monitor();
	else //if(s_data.rec_stat != REC_RECORDING)
		rec_tts(ERROR_BEEP);
#endif
}


void 
rec_key_left_handler (void)
{	
	if (mic_headphone_on())
		mic_rec_switch_monitor();
	else
		rec_tts(ERROR_BEEP);

#if 0	
	if (mic_headphone_on() && (s_data.rec_stat == REC_RECORDING))
		mic_rec_switch_monitor();
	else //if(s_data.rec_stat != REC_RECORDING)
		rec_tts(ERROR_BEEP);
#endif
}


static void
fill_rec_stat_info (char* buf, int size, struct _rec_data* data)
{
	char* info = NULL;

	switch ((data->rec_stat)) {
		case REC_PERPARE:
			info = _("Preparing the recording");
			break;

		case REC_PAUSE:
			info = _("pause recording");
			break;
			
		case REC_RECORDING:
			info = _("recording");
			break;

		default:
			DBGLOG("[Fatal ERROR]: not a correct state\n");
			info = _("Unkown");
			break;
	}
	snprintf(buf, size, "%s", info);

}


static void
fill_rec_elapse_info (char* buf, int size,int time)
{
	int elapse = time;//mic_rec_get_time();

	int hour, mins, sec;
	
	sec = elapse % 60;
	elapse /= 60;
	mins = elapse % 60;
	elapse /= 60;
	//hour = elapse % 24;
	hour = elapse % 100;

#if 0	
	snprintf(buf, size, "%s:\n%d %s %d %s %d %s", _("elapsed recording"),
		hour, _("hour"), mins, _("minutes"), sec, _("seconds"));
#else
	snprintf(buf, size, "%s:\n%d %s %d %s", _("Recording time past"),
		hour, _("hour"), mins, _("minutes"));

#endif
	DBGMSG("fill_rec_elapse_info 2 hour:%d,mins:%d,sec:%d\n",hour,mins,sec);
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
		_("Recording source: Microphone"));

}



#define MAX_INFO_ITEM	10

void 
rec_key_info_handler (struct rec_nano* nano, struct _rec_data* data)
{
#if 1
	int recover = 0;
	char rec_elapse_info[128];


	NhelperTitleBarSetContent(_("Information")); 

	if (get_mic_rec_monitor_state()) {
		mic_rec_monitor_off();
		recover = 1;
	}
	
	/* rec ungrab key */
	rec_ungrab_key(nano);

	int info_item_num = 0;

	/* Fill info item */
	char* mic_rec_info[MAX_INFO_ITEM];

	/* rec state info item */
	char rec_stat_info[128];
	fill_rec_stat_info(rec_stat_info, 128, data);
	mic_rec_info[info_item_num] = rec_stat_info;
	info_item_num ++;

	DBGMSG("rec_key_info_handler data->rec_stat:%d\n",data->rec_stat);

	if (data->rec_stat != REC_PERPARE) {
		/* elapse info item */
		fill_rec_elapse_info(rec_elapse_info, 128,nano->cur_time);
		mic_rec_info[info_item_num] = rec_elapse_info;
		info_item_num ++;
	}

	/* configure info item */
	char rec_config_info[128];
	fill_rec_config_info(rec_config_info, 128, data);
	mic_rec_info[info_item_num] = rec_config_info;
	info_item_num ++;

	nano->isIninfo = 1;
	/* Start info */
	nano->info_id = info_create();
	info_start (nano->info_id, mic_rec_info, info_item_num, &nano->is_running);
	info_destroy (nano->info_id);
	nano->isIninfo = 0;

	/* grab key again */
	rec_grab_key(nano);	

	/*recover the monitor state*/
	if (recover)
		mic_rec_monitor_on();

	NhelperTitleBarSetContent(_("Mic recording"));
#if 1
	if(data->rec_stat == REC_RECORDING){
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
		
	}else {
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
	}
#endif

#endif
}


void
rec_key_vol_inc_handler (void)
{
	DBGLOG("increase the volume!\n");
}


void 
rec_key_vol_dec_handler (void)
{
	DBGLOG("decrease the volume!\n");
	
}

void rec_tts_end(void)
{
	rec_set_tts_end_state(TTS_END_STATE_NONE);
	if(get_mic_rec_monitor_state())
		mic_rec_monitor_off();
	rec_tts_destroy ();

	NeuxAppStop();
	DBGLOG("Record App Stop!!\n");
}

void
rec_key_play_stop_handler(struct _rec_data* data)
{

	DBGLOG("recorder -> previous app!\n");
	TimerDisable(s_nano.rec_timer);

	/* stop the recording first */
	mic_rec_stop();

	rec_tts(REC_EXIT);

	//wait_tts();
	//rec_on_tts_end_callback(TTS_END_STATE_RUN1);
	
	
	//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_RECORD_EXIT);

#if 0	
	rec_tts_destroy ();

	NeuxAppStop();
	DBGLOG("Record App Stop!!\n");
#endif	

}


static void
event_headphone_hander(struct rec_nano* nano)
{
	if (mic_headphone_on()) {
		
		//system("amixer sset \'Speaker\' off");
		//system("amixer sset \'Headphone\' on");
		//p_mixer_set(SPEAKER_OFF, 0);
		//p_mixer_set(HEADPHONE_ON, 0);

	} else {
		if (get_mic_rec_monitor_state())
			mic_rec_monitor_off();
		
		//p_mixer_set(SPEAKER_ON, 0);
		//p_mixer_set(HEADPHONE_OFF, 0);
	}
	rec_tts_change_volume();

	if(rec_need_alarm(&s_data)) {
		plextalk_set_alarm_disable(FALSE);
	} else {
		plextalk_set_alarm_disable(TRUE);
	}


	
}



/* handler for sd card removed */
static void 
hotplug_sd_remove_handler (struct rec_nano* nano, struct _rec_data* data)
{
	/*这里可能需要语言提示*/
	if (!data->rec_media) {
		DBGLOG("SD_CARD removed, set to inner memory.\n");

		if(data->rec_stat == REC_RECORDING || data->rec_stat == REC_PAUSE) {
			DBGMSG("sd removed force exit!!\n");
			rec_tts_removed();
			return;
		}
		rec_tts(REC_REMOVED);

		/* set with inner memory */
		//sys_set_global_rec_media(1);
		CoolShmWriteLock(g_config_lock);
		g_config->setting.record.saveto = 1;
		CoolShmWriteUnlock(g_config_lock);
		
		data->rec_media = 1;

		/* clear up the record data */
		mic_rec_stop();
		check_record_file(data->rec_filepath);
	
		/* Reinit the record data */
		data->rec_filename = rec_get_filename();
		data->rec_media    = 1;
		data->rec_filepath = get_store_path(data->rec_filename, data->rec_media);
		data->rec_stat	  = REC_PERPARE;

		if(!data->rec_filepath) {
			rec_tts(REC_WRITEERROR);
			return;
		}

		/* Restart the recorder pipeline */
		mic_rec_prepare(data->rec_filepath);
		
		//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_MEDIA_INTERNAL);
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		
		led_ctrl(LED_ORANGE, 1);

		app.record_recover = 0;//回到准备状态，因此不能再恢复
		s_nano.remain_time = rec_get_remaintime(s_data.rec_media);
		s_nano.freespace = rec_get_freespace(s_data.rec_media);
		
		//标题要重新显示
#ifdef USE_LABEL_CONTROL		
		if(s_nano.filename_label && s_data.rec_filename)
			NeuxLabelSetText(s_nano.filename_label, s_data.rec_filename);

		NeuxLabelSetText(s_nano.remaintime_value, time_to_string(s_nano.remain_time));

#else
		rec_replace_time(s_nano.textShow.pRecTimeValue,
			time_to_string(s_nano.cur_time),s_nano.textShow.RecTimeValueLen);
		if(s_data.rec_filename)
			rec_replace_time(s_nano.textShow.pFilename,
				s_data.rec_filename,s_nano.textShow.FileNameLen);
		rec_replace_time(s_nano.textShow.pRemainTimeValue,
			time_to_string(s_nano.remain_time),s_nano.textShow.RemainTimeValueLen);

		Rec_showScreen();

#endif
		
		
	} else  {
		DBGLOG("Record media inner memory.\n");
		DBGLOG("No need to handler for sd_card removed\n");
	}
}

//信号处理


/* function for signal gmenu set recorder media*/
int 
mic_rec_change_media (void)
{
	int media;

	CoolShmReadLock(g_config_lock);
	media = g_config->setting.record.saveto;//sys_get_global_rec_media();
	CoolShmReadUnlock(g_config_lock);

	DBGLOG("[Debug]: media = %d\n", media);
	DBGLOG("[Debug]: rec_media = %d\n", s_data.rec_media);
	
#if REC_USE_PREPARE
	if ((!media) && (!check_sd_exist())) {
		//这里也应该给出语言提示吧
		//rec_tts(TTS_NO_SDCARD);
		rec_tts_no_target();
		info_debug("No SD card , set to inner memory.\n");
		/* set with inner memory */
		//sys_set_global_rec_media(1);
		CoolShmWriteLock(g_config_lock);
		g_config->setting.record.saveto = 1;
		CoolShmWriteUnlock(g_config_lock);
		
		s_data.rec_media = 1;

		return 0;
	}	
#endif	

	if (media != s_data.rec_media) {
		/*这里应当给出语言提示吧*/
		if (media)
			//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_MEDIA_INTERNAL);
			NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
			
		else
			//menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_MEDIA_SDCARD);
			NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);

#if REC_USE_PREPARE
		mic_rec_stop();
		check_record_file(s_data.rec_filepath);
	
		/* Reinit the record data */
		s_data.rec_filename = rec_get_filename();
		s_data.rec_media    = media;
		s_data.rec_filepath = get_store_path(s_data.rec_filename, s_data.rec_media);
		s_data.rec_stat	 = REC_PERPARE;

		if(!s_data.rec_filepath) {
			rec_tts(REC_WRITEERROR);
			return -1;
		}

		s_nano.remain_time = rec_get_remaintime(s_data.rec_media);
		s_nano.freespace = rec_get_freespace(s_data.rec_media)
		mic_rec_prepare(s_data.rec_filepath);
		led_ctrl(LED_ORANGE, 1);
#endif
		s_data.rec_media   = media;
		app.record_recover = 0;//回到准备状态，因此不能再恢复

#ifdef USE_LABEL_CONTROL
		//标题要重新显示
		if(s_nano.filename_label && s_data.rec_filename)
			NeuxLabelSetText(s_nano.filename_label, s_data.rec_filename);
		NeuxLabelSetText(s_nano.remaintime_value, time_to_string(s_nano.remain_time));
#else

#if REC_USE_PREPARE
#else
		s_nano.remain_time = rec_get_fake_remaintime(s_data.rec_media);
#endif
		rec_replace_time(s_nano.textShow.pRecTimeValue,
			time_to_string(s_nano.cur_time),s_nano.textShow.RecTimeValueLen);
		if(s_data.rec_filename)
			rec_replace_time(s_nano.textShow.pFilename,
				s_data.rec_filename,s_nano.textShow.FileNameLen);
		rec_replace_time(s_nano.textShow.pRemainTimeValue,
			time_to_string(s_nano.remain_time),s_nano.textShow.RemainTimeValueLen);
	
		Rec_showScreen();
#endif		
		
	}

	return 0;
}



static void
sig_menu_exit_handler (struct rec_nano* nano)
{
	/* Regrab the key when menu exit */
	
	//rec_grab_key(nano);

	app.menu = 0;
	
	if (app.monitor_recover) {
		mic_rec_monitor_on();
		app.monitor_recover = 0;
	}
	
	/* recover the recording if needed */
	if (app.record_recover) {
		app.record_recover = 0;
		s_data.rec_stat = REC_RECORDING;
		mic_rec_recording(s_data.rec_filepath,0);
	}

	if(s_data.rec_stat == REC_RECORDING){
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
		
	}else {
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
	}
	nano->sig_menu_exit = 0;
}



int  
rec_data_init (struct _rec_data* data)
{
	/* media :1, means inner memory, 0, means sd card */
	CoolShmReadLock(g_config_lock);
	data->rec_media = g_config->setting.record.saveto;//sys_get_global_rec_media();
	CoolShmReadUnlock(g_config_lock);
	DBGLOG("[Debug]: init: media = %d\n", data->rec_media);

#if REC_USE_PREPARE//准备界面 不更改其他录音介质，等到开始录音时再提示后退出
	if ((!data->rec_media) && (!check_sd_exist())) {
		info_debug("No SD card , set to inner memory.\n");
		/* set with inner memory */
		//sys_set_global_rec_media(1);
		CoolShmWriteLock(g_config_lock);
		g_config->setting.record.saveto = 1;		
		CoolShmWriteUnlock(g_config_lock);
		
		data->rec_media = 1;
	}
#endif	
	
	if (data->rec_media)
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		
	else
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);

	data->rec_filename = rec_get_filename();
	data->rec_stat = REC_PERPARE;

	return 0;
}


#if REC_USE_PREPARE
static int rec_perpare_recording(struct _rec_data* data)
{
	//data->rec_filename = rec_get_filename();
	data->rec_filepath = get_store_path(data->rec_filename, data->rec_media);
	if(!data->rec_filepath) {
		rec_tts(REC_WRITEERROR);
		return -1;
	}
	if (mic_rec_prepare(data->rec_filepath) == -1) {
		DBGLOG("rec_data_init error!\n");
		return -1;
	}
	data->rec_stat = REC_PERPARE;
	
	//rec_tts(TTS_USAGE);
	rec_tts(REC_AGC);
	return 0;
}
#else
static int rec_perpare_recording(struct _rec_data* data)
{
	data->rec_filepath = get_store_fake_path(data->rec_filename, data->rec_media);

	if (mic_rec_prepare(data->rec_filepath) == -1) {
		DBGLOG("rec_data_init error!\n");
		return -1;
	}

	data->rec_stat = REC_PERPARE;
	
	rec_tts(REC_AGC);
	return 0;
}
#endif


/**
MIC1_BOOST:GIM[2:0]
CAPTURE:GID[4:0]

**/
#if 0//最大音量版本
static void
record_road_set (void)
{
	p_mixer_set(INPUT_MIC1, 0);
	p_mixer_set(MIC1_BOOST, 7);
	p_mixer_set(CAPTURE, 31);
	//p_mixer_set(AGC_ON, 0); 
	amixer_direct_sset_str(VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);
	//system("amixer sset 'AGC NG' on");
	//system("amixer sset 'AGC NG Threshold' 1");
	system("amixer sset 'ADC High Pass Filter' on");

}
#endif

#if 0//最大音量的封装版本+ 11.26客户需求参数版本
static void
record_road_set (void)
{
	snd_mixer_t *mixer;

	mixer = amixer_open(NULL);
	
	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_MIC);//p_mixer_set(INPUT_MIC1, 0);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_MIC_BOOST, 7);//p_mixer_set(MIC1_BOOST, 7);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_CAPTURE_VOL, 31);//p_mixer_set(CAPTURE, 31);
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);//AGC ON 
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_ADC_HIGH_PASS_FILTER,PLEXTALK_CTRL_ON);

	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_TARGET, 15);//AGC TARGET -6dB

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_AGC_NG, PLEXTALK_CTRL_ON);//AGC NG on
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_NG_THRESHOLD, 13);//AGC NG Threshold -42dB
	
	amixer_close(mixer);

}
#endif

#if 0//最大音量的封装版本+ 12.2客户需求参数版本
static void
record_road_set (void)
{
	snd_mixer_t *mixer;

	mixer = amixer_open(NULL);
	
	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_MIC);//p_mixer_set(INPUT_MIC1, 0);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_MIC_BOOST, 7);//p_mixer_set(MIC1_BOOST, 7);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_CAPTURE_VOL, 31);//p_mixer_set(CAPTURE, 31);
	
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_ADC_HIGH_PASS_FILTER,PLEXTALK_CTRL_ON);

	//[AGC1] = 0X80
	//PLEXTALK_AGC_TARGET 	的值与寄存器的值是反向的
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);//AGC ON 
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_TARGET, 15);//AGC TARGET -6dB

	//[AGC2] = 0xD7
	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_AGC_NG, PLEXTALK_CTRL_ON);//AGC NG on
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_NG_THRESHOLD, 5);//AGC NG Threshold -42dB
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_HOLD_TIME, 7);//AGC NG Hold time 2^7ms
	
	amixer_close(mixer);

}
#endif

//AGC开启，会导致声音前4S比较小，后面变大

#if 0//最大音量的封装版本
static void
record_road_set (void)
{
	snd_mixer_t *mixer;

	mixer = amixer_open(NULL);
	
	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_MIC);//p_mixer_set(INPUT_MIC1, 0);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_MIC_BOOST, 7);//p_mixer_set(MIC1_BOOST, 7);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_CAPTURE_VOL, 31);//p_mixer_set(CAPTURE, 31);
	//amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);//AGC ON 
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_ADC_HIGH_PASS_FILTER,PLEXTALK_CTRL_ON);

	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_TARGET, 15);//AGC TARGET -6dB

	//amixer_sset_str(mixer, VOL_RAW, PLEXTALK_AGC_NG, PLEXTALK_CTRL_ON);//AGC NG on
	//amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_NG_THRESHOLD, 5);//AGC NG Threshold -42dB
	
	amixer_close(mixer);

}
#endif



#if 1//参考T7设置值的版本，也即我们目前自己设置的最好版本
static void
record_road_set (void)
{
	snd_mixer_t *mixer;

	mixer = amixer_open(NULL);

	//amixer sset 'Master' 18 && amixer sset 'Bypass' 0 
	//&& amixer sset 'Mic1 Boost' 0 && amixer sset 'Mic2 Boost' 0 
	//&& amixer sset 'Capture' 31 && amixer sset 'AGC' off  
	//&& amixer sset 'AGC Target' 2 && amixer sset 'Headphone' off 
	//&& amixer sset 'Speaker' off && amixer sset 'FM Radio' off
	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_MIC);//p_mixer_set(INPUT_MIC1, 0);

//	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, 31);//amixer sset 'Master' 18
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_PLAYBACK_VOL_BYPASS, 0);//amixer sset 'Bypass' 0 
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_MIC_BOOST, 7);//amixer sset 'Mic1 Boost' 0
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_CAPTURE_VOL, 31);//p_mixer_set(CAPTURE, 31);
//	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_OFF);// amixer sset 'AGC' off 
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_TARGET, 2);//amixer sset 'AGC Target' 2 -25.5db
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_ADC_HIGH_PASS_FILTER,PLEXTALK_CTRL_ON);
	
	
	amixer_close(mixer);

	system("amixer sset 'Mic2 Boost' 7");	//amixer sset 'Mic2 Boost' 0 
	system("amixer sset 'FM Radio' off");	//amixer sset 'FM Radio' off

//del	---
//	system("amixer sset 'Mic Stereo Input' on");//amixer sset 'Mic Stereo Input' on

	system("amixer sset 'Mic Bias' on");//Mic Bias on

}
#endif

#if 0//客户要求 的V09的版本
static void
record_road_set (void)
{
	snd_mixer_t *mixer;

	mixer = amixer_open(NULL);

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_MIC);//p_mixer_set(INPUT_MIC1, 0);

//	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, 31);//amixer sset 'Master' 18
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_PLAYBACK_VOL_BYPASS, 0);//amixer sset 'Bypass' 0 
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_MIC_BOOST, 7);//amixer sset 'Mic1 Boost' 0
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_CAPTURE_VOL, 31);//p_mixer_set(CAPTURE, 31);
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);// AGC ON
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_TARGET, 15);//TARGET -6dB
	amixer_sset_str(mixer, VOL_RAW,PLEXTALK_ADC_HIGH_PASS_FILTER,PLEXTALK_CTRL_ON);

	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_NG, PLEXTALK_CTRL_ON);//AGC NG on
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_ATTACK_TIME, 0);//Attack time 32ms
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_HOLD_TIME, 0);//Hold time 0
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_DECAY_TIME, 10);//Decay Time 352ms
	amixer_sset_integer(mixer, VOL_RAW, PLEXTALK_AGC_NG_THRESHOLD, 0);//AGC NG Threshold -72dB
	
	amixer_close(mixer);

	system("amixer sset 'Mic2 Boost' 7");	//amixer sset 'Mic2 Boost' 0 
	system("amixer sset 'FM Radio' off");	//amixer sset 'FM Radio' off
	system("amixer sset 'Mic Stereo Input' on");//amixer sset 'Mic Stereo Input' on

	system("amixer sset 'Mic Bias' on");//Mic Bias on

}
#endif


#if 0//一般版本
static void
record_road_set (void)
{	
	p_mixer_set(INPUT_MIC1, 0);
	p_mixer_set(MIC1_BOOST, 3);
	p_mixer_set(CAPTURE, 20);
	p_mixer_set(AGC_ON, 0); 
	system("amixer sset 'AGC NG' on");
	system("amixer sset 'AGC NG Threshold' 1");
	system("amixer sset 'ADC High Pass Filter' on");	
}

#endif
#if 0//最大音量版本
static void
record_road_set (void)
{
	p_mixer_set(INPUT_MIC1, 0);
	p_mixer_set(MIC1_BOOST, 7);
	p_mixer_set(CAPTURE, 31);
	//p_mixer_set(AGC_ON, 0); 
	amixer_direct_sset_str(VOL_RAW,PLEXTALK_AGC,PLEXTALK_CTRL_ON);
	//system("amixer sset 'AGC NG' on");
	//system("amixer sset 'AGC NG Threshold' 1");
	system("amixer sset 'ADC High Pass Filter' on");	
}
#endif

static void Rec_onSWOn(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 1;
		break;
	case SW_HP_INSERT:
		DBGLOG("------- Rec_onSWOn	--------\n");
		event_headphone_hander(&s_nano);
		break;
	}
}

static void Rec_onSWOff(int sw)
{
	switch (sw) {
	case SW_KBD_LOCK:
		key_locked = 0;
		break;
	case SW_HP_INSERT:
		DBGLOG("------- Rec_onSWOff	--------\n");
		event_headphone_hander(&s_nano);
		break;
	}
}

static void 
Rec_OnKeyTtimer()
{
	if(rec_is_in_tts_exit_state())
	{
		return;
	}
	if(REC_RECORDING == s_data.rec_stat)
	{
		rec_counter++;
		if(rec_counter >= SEC_IN_MILLISECONDS/TIMER_PERIOD)
		{
			rec_counter = 0;
			s_nano.cur_time++;
			s_nano.remain_time--;

			s_nano.freespace -= (32000/8);

			rec_timer_handler(&s_nano, &s_data);

			if(s_nano.freespace <= (32000/8)){
				rec_tts(REC_NOTENOUGH);
				return;
			}
			if(s_nano.remain_time <= 0 || rec_file_uplimit(s_data.rec_filepath))
			{
				rec_tts(REC_BIGMAX);
				return;
			}
		}
	}
	else if(REC_PERPARE == s_data.rec_stat)
	{
		rec_counter = 0;
		s_nano.cur_time = 0;
	}
	else if(REC_PAUSE== s_data.rec_stat)
	{
		rec_counter = 0;
	}
#if 0
	if(s_nano.first_logo)
	{
		s_nano.first_logo = 0;
		NeuxWidgetShow(s_nano.inputsource_label, TRUE);
		NeuxWidgetShow(s_nano.rectime_label, TRUE);

		NeuxLabelSetText(s_nano.filename_label, s_data.rec_filename);
		NeuxWidgetShow(s_nano.filename_label, TRUE);
	}
#endif	
	//优化显示，因为准备状态和暂停状态是不需要更新时间的
	//rec_timer_handler(&s_nano, &s_data);

}

static void
Rec_InitForm()
{
	//NeuxWidgetGrabKey(widget, MWKEY_POWER,		 NX_GRAB_HOTKEY);
#if 0
	NeuxWidgetGrabKey(widget, VKEY_BOOK,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, VKEY_MUSIC,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, VKEY_RADIO,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, VKEY_RECORD,		 NX_GRAB_HOTKEY_EXCLUSIVE);

	NeuxWidgetGrabKey(widget, MWKEY_VOLUME_UP,	 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_VOLUME_DOWN, NX_GRAB_HOTKEY_EXCLUSIVE);

	//NeuxWidgetGrabKey(widget, MWKEY_MENU,		 NX_GRAB_HOTKEY);

	NeuxWidgetGrabKey(widget, MWKEY_INFO,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_ENTER,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_RIGHT,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_LEFT,		 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_UP,		 	 NX_GRAB_HOTKEY_EXCLUSIVE);
	NeuxWidgetGrabKey(widget, MWKEY_DOWN,		 NX_GRAB_HOTKEY_EXCLUSIVE);
#else
	rec_grab_key(&s_nano);
#endif

	
}


static char fre_min[8];
static char fre_max[8];

static void
initApp(struct rec_nano* nano)
{
	int res;

	/* global config */
	//plextalk_global_config_init();
	plextalk_global_config_open();

	/* theme */
	//CoolShmWriteLock(g_config_lock);
	NeuxThemeInit(g_config->setting.lcd.theme);
	//CoolShmWriteUnlock(g_config_lock);

	app_volume_ctrl(VOL_CTRL_STR_RECORD);

	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");
	
#if 0
	/* language */
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
	plextalk_update_lang(g_config->setting.lang.lang, "recorder");
	//CoolShmReadUnlock(g_config_lock);

	plextalk_update_sys_font(getenv("LANG"));

	app.menu = 0;
	app.help = 0;
	app.monitor_recover = 0;
	app.record_recover = 0;
	rec_counter = 0;
	nano->freespace = 0;
	nano->app_pause = 0;
	nano->isIninfo = 0;
	s_data.ishotplugvoicedisable = 0;
	s_data.isSDhotplugvoicedisable = 0;

	init_monitor_state();

	nano->is_running = 0;
	rec_tts_init(&(nano->is_running));
	rec_tts(TTS_USAGE);

	record_road_set();
	nano->headphone = mic_headphone_on();
	if (nano->headphone) {
		p_mixer_set(SPEAKER_OFF, 0);
		p_mixer_set(HEADPHONE_ON, 0);
	}
	
	//rec_read_keylock();

}



static void
signal_handler(int signum)
{
	DBGLOG("------- signal caught:[%d] --------\n", signum);
	if ((signum == SIGINT) || (signum == SIGTERM) || (signum == SIGQUIT))
	{
		CoolShmPut(PLEXTALK_GLOBAL_CONFIG_SHM_ID);
		DBGLOG("------- global config destroyed --------\n");
		NeuxAppStop();
		DBGLOG("------- NEUX stopped			--------\n");
		exit(0);
	}
}

static
void signalInit(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

	if (sigaction(SIGINT, &sa, NULL)){}
	if (sigaction(SIGTERM, &sa, NULL)){}
	if (sigaction(SIGQUIT, &sa, NULL)){}
}




#define CPU_GOVERNOR_PATH	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPU_FREQ_MAX_PATH	"/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
#define CPU_FREQ_SET_PATH	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"

static int cpu_max_freq;

void cpu_freq_set_user (void)
{
//	system("echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
}


void cpu_freq_set_min (void)
{
//	system("echo 270000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
}


void cpu_freq_set_max(void)
{
//	system("echo 540000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
}





//new tts add function!
void rec_tip_voice_sig(int sig)
{
	DBGMSG("recieve signal of tts stopped!\n");
	s_nano.is_running = 0;
}


static void
OnWindowKeydown(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeydown %d.\n",key.ch);

	//help在运行时，不响应任何按键(因为help按键grap key时用的NX_GRAB_HOTKEY)
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP) || s_nano.isIninfo || s_nano.app_pause)
	{
		DBGLOG("help is running or is in info or is pause !!\n");
		return;
	}

	if(NeuxAppGetKeyLock(0))
	{
		DBGMSG("the key is locked!!!\n");
		rec_tts(REC_KEYLOCK);
		return;
	}

	if(rec_is_in_tts_exit_state())
	{
		DBGMSG("I am in exit state,OK,if you are in a hurry,I will close recorder\n");
		rec_tts_end();
		return;
	}

	switch (key.ch) {
	
		case VKEY_RECORD:
			rec_key_record_handler(&s_data);			//do only when the info tts is not played
			break;
			
		case MWKEY_RIGHT:
			rec_key_right_handler();				//do only when the info tts is not played
			break;
								
		case MWKEY_LEFT: 	
			rec_key_left_handler();
			break;
															
		case MWKEY_INFO: 								//this may need a judge
			rec_key_info_handler(&s_nano, &s_data);
			break;
#if 0			
		case MWKEY_VOLUME_UP:
			rec_key_vol_inc_handler();					//no handler
			break;
			
		case MWKEY_VOLUME_DOWN:
			rec_key_vol_dec_handler();					//no handler
			break;
#endif	
		case MWKEY_ENTER:
			rec_key_play_stop_handler(&s_data);
			return;

#ifdef USE_LABEL_CONTROL										
		case MWKEY_UP:
		case MWKEY_DOWN:
		case VKEY_BOOK: 
		case VKEY_MUSIC:
		case VKEY_RADIO:
			//if(s_data.rec_stat != REC_RECORDING)
				rec_tts(ERROR_BEEP);
			break;
#else
		case MWKEY_UP:
#if 0			
			if(RecShow_prevScreen(&s_nano)>=0) {
				Rec_showScreen();
				NeuxFormShow(widget, TRUE);
				rec_tts(NORMAL_BEEP);
			}
			else
#endif				
				rec_tts(ERROR_BEEP);
			break;
		case MWKEY_DOWN:
#if 0			
			if(RecShow_nextScreen(&s_nano)>=0) {
				Rec_showScreen();
				NeuxFormShow(widget, TRUE);
				rec_tts(NORMAL_BEEP);
			}
			else
#endif				
				rec_tts(ERROR_BEEP);
			break;
		case VKEY_BOOK: 
		case VKEY_MUSIC:
		case VKEY_RADIO:
			//if(s_data.rec_stat != REC_RECORDING)
				rec_tts(ERROR_BEEP);
			break;
#endif
			
	}

}

static void
OnWindowKeyup(WID__ wid, KEY__ key)
{
	DBGLOG("---------------OnWindowKeyup %d.\n",key.ch);
}

static void
OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	Rec_FreeStringBuf(&s_nano);

	plextalk_schedule_unlock();
	plextalk_suspend_unlock();
	plextalk_sleep_timer_unlock();

	// set cpu back
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "ondemand");	


	//let led green on
	led_ctrl(LED_GREEN, 0);

	/* stop the recording first */
	mic_rec_stop();
	system("amixer sset 'Mic Bias' off");//Mic Bias off

	check_record_file(s_data.rec_filepath);

	NhelperRecordingActive(0);
	NhelperRecordingNow(0);
	
	if(get_mic_rec_monitor_state())
		mic_rec_monitor_off();

	if(s_data.isalarmdisable) {
		plextalk_set_alarm_disable(FALSE);
	}
	if(s_data.ishotplugvoicedisable)
		plextalk_set_hotplug_voice_disable(FALSE);
	if(s_data.isSDhotplugvoicedisable)
		plextalk_set_SD_hotplug_voice_disable(FALSE);
	
	plextalk_global_config_close();
#if 0	
	if (get_mic_rec_monitor_state()) {
		mic_rec_monitor_off();
	}
	mic_rec_stop();
	led_ctrl(LED_GREEN, 0);
	//TimerDisable(s_nano.rec_timer);
	
	TimerDelete(s_nano.rec_timer);
	NeuxFormDestroy(widget);	
	widget = NULL;
#endif	
}

void
rec_RefreshFormMain(void)
{
	widget_prop_t wprop;
	struct rec_nano* nano;
	nano = &s_nano;
	
	NeuxThemeInit(g_config->setting.lcd.theme);
	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(widget, &wprop);
	
#ifdef USE_LABEL_CONTROL	
	NeuxWidgetGetProp(nano->filename_label, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->filename_label, &wprop);
	
	NeuxWidgetGetProp(nano->rectime_label, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->rectime_label, &wprop);
	
	NeuxWidgetGetProp(nano->rectime_value, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->rectime_value, &wprop);
	
	NeuxWidgetGetProp(nano->remaintime_label, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->remaintime_label, &wprop);

	NeuxWidgetGetProp(nano->remaintime_value, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->remaintime_value, &wprop);
	
	NeuxWidgetShow(s_nano.filename_label, TRUE);
	NeuxWidgetShow(s_nano.rectime_label, TRUE);
	NeuxWidgetShow(s_nano.rectime_value, TRUE);
	NeuxWidgetShow(s_nano.remaintime_label, TRUE);
	NeuxWidgetShow(s_nano.remaintime_value, TRUE);
#else
	GrSetGCForeground(s_nano.pix_gc, theme_cache.foreground);
	GrSetGCBackground(s_nano.pix_gc, theme_cache.background);
	Rec_showScreen();
#endif
	
	NhelperTitleBarSetContent(_("Mic recording"));
	NeuxFormShow(widget, TRUE);

}

static void OnWindowRedraw(int clrBg)
{
	GR_WINDOW_ID wid;
	GR_GC_ID gc;
	
	wid = s_nano.wid;
	gc = s_nano.pix_gc;

	if(wid != GrGetFocus()){
		//DBGMSG("NOT FOCUS\n");
		//return;
	}
	
	if(clrBg){
		NxSetGCForeground(gc, theme_cache.background);
		GrFillRect(wid, gc, 0, 0, s_nano.textShow.w, s_nano.textShow.h);
		NxSetGCForeground(gc, theme_cache.foreground);
	}
	
	RecShow_showScreen(&s_nano);
}

static void OnWindowExposure(WID__ wid)
{
	DBGLOG("------exposure-------\n");
	OnWindowRedraw(1);
}


static int Rec_showScreen(void)
{
	//DBGMSG("show\n");
	OnWindowRedraw(1);
}


static void onAppMsg(const char * src, neux_msg_t * msg)
{
	DBGLOG("app msg %d .\n", msg->msg.msg.msgId);

	switch (msg->msg.msg.msgId)
	{
	case PLEXTALK_TIMER_MSG_ID:
		break;		
	case PLEXTALK_APP_MSG_ID:
		{
			
			//app_setting_rqst_t * req = (app_setting_rqst_t*)msg->msg.msg.msgTxt;
			app_rqst_t *rqst = msg->msg.msg.msgTxt;
			DBGLOG("onAppMsg PLEXTALK_APP_MSG_ID : %d\n",rqst->type);
			switch (rqst->type)
			{
			case APPRQST_SET_STATE:
				{
					app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
					if(rqst->pause){

						if(s_nano.isIninfo)
						{
							info_pause();
						}
						else
						{
							//暂停标志位，防止两次暂停
							if(s_nano.app_pause){
								DBGMSG("Hey! Rec already paused\n");
								return;
							}
							s_nano.app_pause = 1;
						
							rec_ungrab_key(&s_nano);
							plextalk_suspend_unlock();
					
							app.menu = 1;
					
							if (get_mic_rec_monitor_state()) {
								mic_rec_monitor_off();
								app.monitor_recover = 1;
							}

#if 0//正在recording 的时候，不能暂停,要与不能响应menu配合
							/* pause current recording */
							if (s_data.rec_stat == REC_RECORDING) {
								mic_rec_pause();
								app.record_recover = 1;
								s_data.rec_stat = REC_PAUSE;
							}
#endif							
						}
						
					}else{
						//通知恢复

						if(s_nano.isIninfo)
						{
							info_resume();
						}
						else
						{
							if(!s_nano.app_pause){
								DBGMSG("Hey! No need Resume\n");
								return;
							}
							s_nano.app_pause = 0;
						
							sig_menu_exit_handler(&s_nano);
							s_nano.first_logo = 1;
							NeuxWidgetFocus(widget);
#ifdef USE_LABEL_CONTROL							
							NeuxWidgetShow(s_nano.filename_label, TRUE);
#else
							s_nano.pix_gc = NeuxWidgetGC(widget);
							Rec_showScreen();
#endif
							NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RECODER);
							plextalk_suspend_lock();
						}

					}	
				}
				break;

			case APPRQST_SET_RECORD_TARGET:
#if REC_USE_PREPARE
				mic_rec_change_media();
#else
				if(s_data.rec_stat == REC_PERPARE){
					mic_rec_change_media();
				}

#endif
				break;

			case APPRQST_GUIDE_VOL_CHANGE:
			case APPRQST_GUIDE_SPEED_CHANGE:
			case APPRQST_GUIDE_EQU_CHANGE:
				rec_init_tts_prop();
				break;	
			case APPRQST_SET_TTS_VOICE_SPECIES:
			case APPRQST_SET_TTS_CHINESE_STANDARD:
				plextalk_update_tts(getenv("LANG"));
				break;	
			
			case APPRQST_SET_THEME:
				NeuxThemeInit(rqst->theme.index);
				rec_RefreshFormMain();
				break;
			case APPRQST_SET_LANGUAGE:
			case APPRQST_SET_FONTSIZE:	
				CoolShmReadLock(g_config_lock);
				plextalk_update_lang(g_config->setting.lang.lang, "recorder");
				CoolShmReadUnlock(g_config_lock);
				
				plextalk_update_tts(getenv("LANG"));
				plextalk_update_sys_font(getenv("LANG"));
				Rec_FreeStringBuf(&s_nano);
				Rec_InitStringBuf(&s_nano,s_data.rec_filename);	

#ifdef USE_LABEL_CONTROL
				NeuxWidgetSetFont(s_nano.filename_label, sys_font_name, 16);
				NeuxWidgetShow(s_nano.filename_label, TRUE);

				NeuxWidgetSetFont(s_nano.rectime_label, sys_font_name, 16);
				NeuxLabelSetText(s_nano.rectime_label, _("Recording Time:"));
				NeuxWidgetShow(s_nano.rectime_label, TRUE);

				NeuxWidgetSetFont(s_nano.remaintime_label, sys_font_name, 16);
				NeuxLabelSetText(s_nano.remaintime_label, _("Remaining Time:"));
				NeuxWidgetShow(s_nano.remaintime_label, TRUE);

				NeuxWidgetSetFont(s_nano.rectime_value, sys_font_name, 16);
				NeuxWidgetShow(s_nano.rectime_value, TRUE);
				
				NeuxWidgetSetFont(s_nano.remaintime_value, sys_font_name, 16);
				NeuxWidgetShow(s_nano.remaintime_value, TRUE);
#else
				NeuxWidgetSetFont(widget, sys_font_name, /*sys_font_size*/16);
				s_nano.pix_gc = NeuxWidgetGC(widget);
				RecShow_Init(&s_nano, 16/*sys_font_size*/);
				Rec_showScreen();
#endif
				
//				NhelperTitleBarSetContent(_("Mic recording"));

				break;	

			case APPRQST_RESYNC_SETTINGS:
				{
					CoolShmReadLock(g_config_lock);
					
					NeuxThemeInit(g_config->setting.lcd.theme);
					plextalk_update_lang(g_config->setting.lang.lang, "recorder");
					plextalk_update_sys_font(g_config->setting.lang.lang);
					plextalk_update_tts(g_config->setting.lang.lang);
					CoolShmReadUnlock(g_config_lock);

					Rec_FreeStringBuf(&s_nano);
					Rec_InitStringBuf(&s_nano,s_data.rec_filename);	

					NeuxWidgetSetFont(widget, sys_font_name, /*sys_font_size*/16);
					s_nano.pix_gc = NeuxWidgetGC(widget);
					RecShow_Init(&s_nano, /*sys_font_size*/16);
					Rec_showScreen();
				}
				break;
			case APPRQST_LOW_POWER://低电要关机了
				DBGMSG("Yes,I have received the lowpower message\n");
				rec_tts_end();
				break;
			}
		}
		break;
		
	}
}


static void
onHotplug(WID__ wid, int device, int index, int action)
{
	DBGLOG("onHotplug device: %d ,action=%d\n", device,action);

	if((device == MWHOTPLUG_DEV_UDC)
		&& (action == MWHOTPLUG_ACTION_ONLINE)) {

		DBGMSG("PC connector\n");
		if(s_data.rec_stat == REC_RECORDING){
			
			mic_rec_pause();
			s_data.rec_stat = REC_PAUSE;

			if(get_mic_rec_monitor_state())
				mic_rec_monitor_off();
			
			if(rec_need_alarm(&s_data)) {
				plextalk_set_alarm_disable(FALSE);
			} else {
				plextalk_set_alarm_disable(TRUE);
			}
			
			if(rec_need_hotplug_prompt(&s_data)) {
				plextalk_set_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_hotplug_voice_disable(TRUE);
			}
			
			if(rec_need_SD_hotplug_prompt(&s_data)) {
				plextalk_set_SD_hotplug_voice_disable(FALSE);
			} else {
				plextalk_set_SD_hotplug_voice_disable(TRUE);
			}
			NhelperRecordingNow(0);
			
		}		
	}
	

	if(s_nano.isIninfo|| s_nano.app_pause) {

		DBGMSG("in info or pause\n");
		return;
	}


	if ((device == MWHOTPLUG_DEV_MMCBLK) 
	&& (action == MWHOTPLUG_ACTION_REMOVE)) {

#if REC_USE_PREPARE
	hotplug_sd_remove_handler(&s_nano,&s_data);
#else
	if(s_data.rec_stat == REC_RECORDING || s_data.rec_stat == REC_PAUSE) {
		hotplug_sd_remove_handler(&s_nano,&s_data);
	}
#endif
		
	}
	
}

static void rec_OnGetFocus(WID__ wid)
{
	DBGLOG("On Window Get Focus\n");
	NhelperTitleBarSetContent(_("Mic recording"));
#if 1
	if(!wid) {
		if(s_data.rec_stat == REC_RECORDING){
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
			
		}else {
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
		}
	}
#endif	
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RECODER);

	if (s_data.rec_media)
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
		
	else
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	
}

#ifdef USE_LABEL_CONTROL
static void
Rec_CreateLabels (FORM *widget,struct rec_nano* nano, struct _rec_data* data)	
{
	widget_prop_t wprop;
	label_prop_t lprop;

	nano->filename_label= NeuxLabelNew(widget,
						REC_TIME_LABEL_X,FILE_NAME_LABEL_Y,NeuxScreenWidth(),REC_LABEL_HEIGHT,data->rec_filename);
	
	NeuxWidgetGetProp(nano->filename_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->filename_label, &wprop);
	
	NeuxLabelGetProp(nano->filename_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->filename_label, &lprop);
	//NeuxLabelSetText(nano->filename_label, _("RECORDER"));
	NeuxWidgetSetFont(nano->filename_label, sys_font_name, 16);
	NeuxWidgetShow(nano->filename_label, FALSE);

//	nano->first_logo = 1;
	

	//显示recording time:
	nano->rectime_label= NeuxLabelNew(widget,
						REC_TIME_LABEL_X,FILE_NAME_LABEL_Y+REC_LABEL_HEIGHT,NeuxScreenWidth(),REC_LABEL_HEIGHT,_("Recording Time:"));
	
	NeuxWidgetGetProp(nano->rectime_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->rectime_label, &wprop);
	
	NeuxLabelGetProp(nano->rectime_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->rectime_label, &lprop);
	//NeuxLabelSetText(nano->rectime_label, _("Recording Time:"));
	NeuxWidgetSetFont(nano->rectime_label, sys_font_name, 16);
	NeuxWidgetShow(nano->rectime_label, FALSE);


#if 1//修改为显示已录音的时间
	nano->rectime_value= NeuxLabelNew(widget,
					  REC_TIME_LABEL_X,FILE_NAME_LABEL_Y+2*REC_LABEL_HEIGHT,NeuxScreenWidth(),REC_LABEL_HEIGHT,"00:00:00");
	
	
	NeuxWidgetGetProp(nano->rectime_value, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->rectime_value, &wprop);
	
	NeuxLabelGetProp(nano->rectime_value, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->rectime_value, &lprop);
	NeuxWidgetSetFont(nano->rectime_value, sys_font_name, 16);
	NeuxWidgetShow(nano->rectime_value, FALSE);
#endif

	//显示Remaining time:
	nano->remaintime_label= NeuxLabelNew(widget,
						REC_TIME_LABEL_X,FILE_NAME_LABEL_Y+3*REC_LABEL_HEIGHT,NeuxScreenWidth(),REC_LABEL_HEIGHT,_("Remaining Time:"));
	
	NeuxWidgetGetProp(nano->remaintime_label, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->remaintime_label, &wprop);
	
	NeuxLabelGetProp(nano->remaintime_label, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->remaintime_label, &lprop);
	//NeuxLabelSetText(nano->freshtime_label, "00:00:00");
	NeuxWidgetSetFont(nano->remaintime_label, sys_font_name, 16);
	NeuxWidgetShow(nano->remaintime_label, FALSE);


	//显示具体的剩余时间
	nano->remain_time = rec_get_remaintime(data->rec_media);
	nano->freespace = rec_get_freespace(data->rec_media)
	nano->remaintime_value= NeuxLabelNew(widget,
						REC_TIME_LABEL_X,FILE_NAME_LABEL_Y+4*REC_LABEL_HEIGHT,NeuxScreenWidth(),REC_LABEL_HEIGHT,time_to_string(nano->remain_time));
	
	NeuxWidgetGetProp(nano->remaintime_value, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->remaintime_value, &wprop);
	
	NeuxLabelGetProp(nano->remaintime_value, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(nano->remaintime_value, &lprop);
	NeuxWidgetSetFont(nano->remaintime_value, sys_font_name, 16);
	NeuxWidgetShow(nano->remaintime_value, FALSE);

}
#endif

static void
Rec_CreateTimer(FORM *widget,struct rec_nano* nano)
{
	nano->rec_timer = NeuxTimerNew(widget, TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano->rec_timer, Rec_OnKeyTtimer);
	DBGLOG("Rec_CreateTimer nano->rec_timer:%d",nano->rec_timer);
	//TimerDisable(nano->rec_timer);
}

void Rec_CreateForm()
{
	widget_prop_t wprop;

	widget = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, OnWindowKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP,	 OnWindowKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  OnWindowDestroy);
	NeuxFormSetCallback(widget, CB_HOTPLUG, onHotplug);	
	NeuxFormSetCallback(widget, CB_FOCUS_IN, rec_OnGetFocus);
	NeuxFormSetCallback(widget, CB_EXPOSURE, OnWindowExposure);
	
	NeuxWidgetGetProp(widget, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(widget, &wprop);
	NeuxWidgetSetFont(widget, sys_font_name, /*sys_font_size*/16);

	Rec_InitForm();
	NeuxFormShow(widget, TRUE);
	//NeuxWidgetFocus(widget);
}


// start of public APIs.

/* Function creates the main form of the application. */
void
Rec_CreateFormControl (struct rec_nano* nano,struct _rec_data* data)
{
#ifdef USE_LABEL_CONTROL
	//创建四个LABEL用于显示字符串
	Rec_CreateLabels(widget,nano,data);
#else
	Rec_CreateGc(widget,nano);
#if REC_USE_PREPARE
	nano->remain_time = rec_get_remaintime(data->rec_media);
	nano->freespace = rec_get_freespace(data->rec_media)
#else
	nano->remain_time = rec_get_fake_remaintime(data->rec_media);
#endif
	Rec_InitStringBuf(nano,data->rec_filename);
	RecShow_Init(nano,/*sys_font_size*/16);
#endif
	//创建定时器用于计时和显示
	Rec_CreateTimer(widget,nano);
	


	//Rec_InitForm();
	//NhelperTitleBarSetContent(_("Mic recording"));

	NeuxFormShow(widget, TRUE);
	NeuxWidgetFocus(widget);

#ifdef USE_LABEL_CONTROL	
	NeuxWidgetShow(nano->filename_label, TRUE);
	NeuxWidgetShow(nano->rectime_label, TRUE);
	NeuxWidgetShow(nano->rectime_value, TRUE);
	NeuxWidgetShow(nano->remaintime_label, TRUE);
	NeuxWidgetShow(nano->remaintime_value, TRUE);
#endif	

}


void DesktopScreenSaver(GR_BOOL activate)
{
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

	font_id = GrCreateFont((GR_CHAR*)sys_font_name, /*sys_font_size*/24, NULL);
	GrSetGCUseBackground(gc ,GR_FALSE);
	GrSetGCForeground(gc,theme_cache.foreground);
	GrSetGCBackground(gc,theme_cache.background);
	GrSetFontAttr(font_id , GR_TFKERNING, 0);
	GrSetGCFont(gc, font_id);

	NxGetGCTextSize(gc, pstr, -1, MWTF_UTF8, &w, &h, &b);
	x = (MAINWIN_WIDTH - w)/2;
	y = (MAINWIN_HEIGHT - 24)/2;
						
	GrText(NeuxWidgetWID(widget), gc, x,y,  pstr, strlen(pstr), MWTF_UTF8 | MWTF_TOP);

	GrFlush();

	s_nano.first_logo = 1;
}


int
main(int argc,char **argv)

{
	int ret;
	struct rec_nano* nano;
	struct _rec_data* data;
	nano = &s_nano;
	data = &s_data;

	//NeuxAppCreate必须在所有初始化最前，尤其是TTS
	// create desktop as the WM/desktop
	if (NeuxAppCreate(argv[0]))
	{
		FATAL("unable to create application.!\n");
	}

	initApp(nano);
	DBGMSG("application initialized\n");

	Rec_CreateForm();
	Rec_show_logo();
	NeuxWidgetFocus(widget);

	rec_data_init(data);
	rec_OnGetFocus(0);//手动调用一次

	
	//rec界面只关屏，不休眠
	plextalk_suspend_lock();
	//界面禁止应用切换
	plextalk_schedule_lock();
	//禁止sleep
	plextalk_sleep_timer_lock();

	cpu_freq_set_user();
	cpu_freq_set_max();
	
	NhelperRecordingActive(0);
	NhelperRecordingNow(0);
	
	ret = rec_perpare_recording(data);
	
	Rec_CreateFormControl(nano,data);

	NeuxAppSetCallback(CB_MSG,	 onAppMsg);
	NeuxAppSetCallback(CB_SW_ON,	Rec_onSWOn);
	NeuxAppSetCallback(CB_SW_OFF,  	Rec_onSWOff);
	DBGMSG("mainform shown\n");

	/*Actually, this preparation is dummy*/
	led_ctrl(LED_ORANGE, 1);

	// start application, events starts to feed in.
	NeuxAppStart();

	return 0;

	
}

