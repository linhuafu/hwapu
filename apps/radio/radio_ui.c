#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <libintl.h>
#include <locale.h>
#include <gst/gst.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h> 
#include <fcntl.h>

#include "radio_rec_ui.h"
#include "radio_rec_extern.h"
#include "radio_preset.h"
#include "radio_type.h"
#include "radio_play.h"
#include "radio_tts.h"
#include "radio_tts.h"
#include "fmlib.h"

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
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "plextalk-keys.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "plextalk-config.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "key-value-pair.h"
#include "libinfo.h"
#include "libvprompt.h"
#include "radio_rec_newui.h"

/* For Auto Scan */
int radio_auto_scan_mode_active;
int radio_auto_scan_end_flag;
int radio_auto_scan_end_set_unmute;
int radio_auto_scan_first_seek;

pthread_t radio_pid;

/* this for fm ui flush */
#define FM_TIMER_PERIOD		500

/* long key timer */
#define FM_TIMER_LKEY  		1000

/* this for radio state */
enum {
	RADIO_STATE_MANU_SEARCH = 0,
	RADIO_STATE_AUTO_SEARCH,
	RADIO_STATE_PRESET,
};

/* global flags for radio */
struct radio_flags global_flags;
struct radio_nano nano;
extern struct _rec_data data;
struct seek_freq autoseek;

/* for radio power dissipation */
static double backup_freq ;

/* For radio if in mute state */
int radio_mute_state;

/* Radio Kill autoseek thread */
static void 
radio_auto_seek_kill (void)
{
	DBGMSG("Radio Auto Seek Kill called!\n");
	
	if (radio_pid != -1) {

		DBGMSG("Radio Auto Seek pthread_kill!!!\n");
		pthread_kill(radio_pid, SIGUSR1);
		pthread_join(radio_pid, NULL);
		DBGMSG("Radio Auto Seek kill end!\n");

		radio_pid = -1;
	}
}


/* Radio Update StautsBar .. */
static void 
radio_auto_stop_update (void)
{
	global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
	nano.bPause = 0;
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);
	NhelperTitleBarSetContent(_("Hearing radio"));
	NeuxLabelSetText(nano.label1, _("Manual"));	
}


static void 
radio_auto_stop(void)
{
	radio_auto_seek_kill();
	radio_auto_stop_update();

	nano.auto_scaning = 0;
}


static void 
radio_set_playstatus (void)
{
	global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
	NhelperTitleBarSetContent(_("Hearing Radio"));
}

#if 0
static void
check_auto_scan_stopped (double freq)
{
	if (nano.auto_scaning) {
		float new_freq = freq;
		float old_freq = nano.old_freq;
		double ret = (new_freq - old_freq);

		if ((ret < 0.01) && (ret > -0.01)) {
			DBGMSG("Auto scan stopped!\n");
			global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
			nano.bPause = 0;
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);
			NhelperTitleBarSetContent(_("Hearing radio"));
			radio_tts(RADIO_CUR_FREQ, (int)(10 * freq));
			radio_ui_freq(radio_get_freq());
			nano.auto_scaning = 0;
			
		} else
			nano.old_freq = new_freq;
	}
}
#endif


/* Added this function to avoid widonw blink when radio_ui_freq called */
static void
gr_direct_set_text (WIDGET* widget, const char* text)
{
	GR_GC_ID gc;
	GR_WINDOW_ID wid;
	int w, h, pos_x;
	int retwidth, retheight, retbase;

	/* Get gc and window_id */
	wid = NeuxWidgetWID(widget);
	gc = NeuxWidgetGC(widget);
	NeuxWidgetGetSize(widget, &w, &h);

	/* Fill with background */
	GrSetGCForeground(gc, theme_cache.background);
	GrSetGCBackground(gc, theme_cache.background);	
	GrFillRect(wid, gc, 0, 0, w, h);

	/* Cal positon (x, y = 0) */
	GrGetGCTextSize(gc, text, strlen(text), MWTF_UTF8, &retwidth, &retheight, &retbase);
	pos_x = (MAINWIN_WIDTH  - retwidth) / 2;

	/* Recover gc and GrText */
	GrSetGCForeground(gc, theme_cache.foreground);
	GrText(wid, gc, pos_x, 0, text, strlen(text),  MWTF_UTF8 | GR_TFTOP);
}


/* Set manual, Auto, (x)CH */
void 
radio_ui_freq (double freq)
{
	char buf[64];
	int nleft;
//	DBGMSG("Radio UI freq called!\n");
	if (nano.first_logo) {
		DBGMSG("TIPS: nano.first_logo , No UI Freq!\n");
		return ;
	}
	
	switch (global_flags.radio_state) {
	case RADIO_STATE_AUTO_SEARCH:
		//NeuxLabelSetText(nano.label1, _("Auto"));
		gr_direct_set_text(nano.label1, _("Auto"));
		break;
	case RADIO_STATE_MANU_SEARCH:
		//NeuxLabelSetText(nano.label1, _("Manual"));
		gr_direct_set_text(nano.label1, _("Manual"));
		break;
	case RADIO_STATE_PRESET:
		snprintf(buf, 8, "%dCH", get_cur_station_no());
		//NeuxLabelSetText(nano.label1, buf);
		gr_direct_set_text(nano.label1, buf);
		break;
	}
	
	//NeuxLabelSetText(nano.label2, radio_get_freq_str(freq));
	gr_direct_set_text(nano.label2, radio_get_freq_str(freq));
}

/* handler for sig menu exit handler */
static void
sig_menu_exit_handler (void)
{			
	DBGMSG("radio menu exit handler!\n");

	if (global_flags.menu_flag) {
		if (global_flags.recording) {
			if (nano.record_recover) {
				radio_recorder_recording(data.rec_filepath,0);
				nano.record_recover = 0;
				data.rec_stat = REC_RECORDING;
			}
			if(data.rec_stat == REC_RECORDING){
				NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD);
				
			}else {
				NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_RECORD_BLINK);
			}
	
			NhelperTitleBarSetContent(_("Radio Recording"));		
			plextalk_suspend_lock();
		} else {
			radio_ui_freq(radio_get_freq());
		//	global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
			NhelperTitleBarSetContent(_("Hearing Radio"));
		}
		
		if(road_flag == OUT_PUT_DAC && !global_flags.del_voc_runing)
			road_flag =  OUT_PUT_BYPASS;
		
		global_flags.menu_flag = 0;
	}
}


/* handler for sig menu exit handler */
static void
sig_menu_popup_handler (void)
{
	DBGMSG("radio sig menu popup handler!\n");
	
	global_flags.menu_flag = 1;

	if(road_flag == OUT_PUT_BYPASS) 
		road_flag =  OUT_PUT_DAC;

	if (global_flags.recording) {
		plextalk_suspend_unlock();
		if (data.rec_stat == REC_RECORDING) {
			DBGMSG("global_flags.recording = 1, but state is recording can not set radio recorder pause!\n");
#if 0	//录音过程中不能能暂停		
			radio_recorder_pause();
			nano.record_recover = 1;
			data.rec_stat = REC_PAUSE;
#endif			
		}
	}
}


/* handler for del current preset channels */
static void
sig_del_cur_preset_handler (void)
{
	DBGMSG("sig del current preset handler!\n");
	int ret ;

	if (!global_flags.recording) {
		ret = del_cur_preset_station();

		if (ret == -1)
			NhelperDeleteResult(APP_DELETE_ERR); 
		else {
			global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
			NhelperTitleBarSetContent(_("Hearing Radio"));
			radio_ui_freq(radio_get_freq());
			NhelperDeleteResult(APP_DELETE_OK);
		}
		
		global_flags.del_voc_runing = 1;
	} else
		NhelperDeleteResult(APP_DELETE_ERR);

	sig_menu_exit_handler();
}


/* handler for del all preset channels */
static void
sig_del_all_preset_handler (void)
{
	DBGMSG("sig del all preset handler!\n");
	
	if (!global_flags.recording) {
		del_all_preset_station();
		global_flags.radio_state = RADIO_STATE_MANU_SEARCH;

		NhelperTitleBarSetContent(_("Hearing Radio"));
		radio_ui_freq(radio_get_freq());	
		DBGMSG("sig_del_all_preset_handler\n");

		NhelperDeleteResult(APP_DELETE_OK);
		global_flags.del_voc_runing = 1;
	} else
		NhelperDeleteResult(APP_DELETE_ERR);
}


/* handler for change meida (!this function is spciall for recording) */
static void
sig_change_media_handler (void)
{			
	DBGMSG("Radio recorder change media!\n");
	/* only do handler in recording function */
	if (global_flags.recording)
		radio_rec_change_media();
}


/*stop mute state*/
static void 
radio_stop_mutestate (void)
{
	radio_set_mute(false, 0);		//hopeless...
}


/* Radio key info (start vprompt information) */
static void 
radio_key_info_handler (void)
{
	DBGMSG("Radio key information handler!\n");
	
	nano.binfo = 1;
	NhelperTitleBarSetContent(_("Information"));
	TimerDisable(nano.timer);
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);

	/* Create Info id */
	nano.info_id = info_create();
	int* is_running = radio_tts_isrunning();

	/* Fill info item */
	char* radio_info[1];

	char channel_info[128];
	if (global_flags.radio_state == RADIO_STATE_PRESET)
		snprintf(channel_info, 128, "%s%d, %s%g%s", _("current channel:") 
		, get_cur_station_no(), _("current frequency:"), radio_get_freq(), _("mega hertz"));		
 	else
		snprintf(channel_info, 128, "%s%g%s", _("current frequency:"), radio_get_freq(), _("mega hertz"));	

	radio_info[0] = channel_info;
	info_start (nano.info_id, radio_info, 1, is_running);
	info_destroy (nano.info_id);

	/* Recover radio state */
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_BYPASS);
	DBGMSG("DEBUG: Set OutPut Bypass!!!\n");
		
	road_flag = OUT_PUT_BYPASS;
	TimerEnable(nano.timer);
	NhelperTitleBarSetContent(_("Hearing Radio"));
	nano.binfo = 0;
}


/*function handler for key_left or key_right
 *orient : 1, means manual scan right
 *orient : 0, means manual scan left
 */
static void 
radio_key_left_right_handler (int orient)
{
	DBGMSG("radio_key_left_right_handler\n");
	radio_manual_scan(orient ? SCAN_FORWARD : SCAN_BACKWARD);

	/*set radio state to manu search */
	global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
	if (radio_is_muted()) {
		DBGMSG("radio_key_left_right_handler  11\n");
		nano.bPause = 0;
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);
	}
	radio_stop_mutestate();
	double freq = radio_get_freq();
	DBGMSG("radio get freq , freq = %f\n", freq);
	radio_ui_freq(freq);
	radio_tts(RADIO_CUR_FREQ, (int)(10 * freq));

	CoolDeleteSectionKey("/tmp/.radio", "chan", "index");
	CoolDeleteSectionKey("/tmp/.radio", "chan", "freq");
}


/* function handler for key_up or key_down
 * orient : 1 means get next station
 * orient : 0 means get prev station
 */
static void 
radio_key_up_down_handler (int orient)
{
	DBGMSG("radio key up-down handler!\n");
	
	double new_freq = get_preset_station(orient ? NEXT_STATION : PRE_STATION);

	if (new_freq == 0) {
		/*means there is no frequency preseted*/
		radio_tts(RADIO_CHANNEL_NOTFOUND, 0);
		return;
	} else {
		/*set radio to the state of preset*/
		global_flags.radio_state = RADIO_STATE_PRESET;

		if ((new_freq < radio_get_fmMin()) || (new_freq > radio_get_fmMax())) {
			DBGMSG("WARNING: get preset station err!!!\n");
			DBGMSG("Use fm Min instead and delete all preset station!\n");
			
			radio_stop_mutestate();
			radio_tts(TTS_ERROR_BEEP, 0);
			del_all_preset_station();
			new_freq = radio_get_fmMin();
			global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
			radio_set_freq(new_freq);
			radio_ui_freq(new_freq);
			return ;
		}
		
		radio_set_freq(new_freq);	

		if (radio_is_muted()) {
			DBGMSG("Radio key up-down in mute mode!\n");
			nano.bPause = 0;
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);
		}
		
		radio_stop_mutestate();
		radio_tts(RADIO_CHANNEL_FOUND, get_cur_station_no());
	}

	char index[10];
	memset(index, 0x00, 10);
	sprintf(index, "%d", get_cur_station_no());
	CoolSetCharValue("/tmp/.radio", "chan", "index", index);
	CoolSetCharValue("/tmp/.radio", "chan", "freq", radio_get_freq_str(radio_get_freq()));
	
	radio_ui_freq(radio_get_freq());
}


/* Handler for short pressed of key_play/stop */
static void 
radio_key_play_stop_handler (void)
{
	DBGMSG("Radio key play-stop handler!\n");

	/* !No need to set radio state to manu search */
	//global_flags.radio_state = RADIO_STATE_MANU_SEARCH;

	if (radio_is_muted()) {
		radio_set_mute(false, 0);
		radio_tts(TTS_NORMAL_BEEP, 0);
		nano.bPause = 0;

		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);

		DBGMSG("Set radio unmute success!\n");
	} else {
		radio_set_mute(true, 0);
		radio_tts(RADIO_SET_MUTE, 0);
		nano.bPause = 1;

		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PAUSE);

		DBGMSG("Set raido mute success!\n");
	}
}


/* Handler for long pressed of key_left/key_right */
static void
radio_autoscan_time_out (int orient)
{
	DBGMSG("radio autoscan time out!\n");
	int ret;

	TimerDisable(nano.l_key.lid);
	nano.l_key.l_flag = 0;

	/*set radio state to auto scan*/
	global_flags.radio_state = RADIO_STATE_AUTO_SEARCH;

	NhelperTitleBarSetContent(_("Radio search"));
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON,  SBAR_CONDITION_ICON_SEARCH);
	NeuxLabelSetText(nano.label1, _("Auto"));

	
	/* Set Radio mute when begin radio seek */
	radio_set_mute(true, 0);
	radio_tts(AUTO_SCAN_BEGIN, 0);

	CoolDeleteSectionKey("/tmp/.radio", "chan", "index");
	CoolDeleteSectionKey("/tmp/.radio", "chan", "freq");
	
	/* Start Auto Seek NOW! */
	DBGMSG("Start Radio auto seek now!\n");
	
	plextalk_set_menu_disable(1);
	nano.old_freq = radio_get_freq();
	autoseek.orient = orient;
	ret = pthread_create(&radio_pid, NULL, radio_seek, (void*)&autoseek);
	if (ret < 0) {
		DBGMSG("WARNING: radio_seek start pthread failure!\n");
		return;
	}	
	nano.auto_scaning = 1;		//auto seek in progress flag
}


/* Handler for long pressed of play/stop */
static void
radio_play_stop_time_out (void)
{	
	DBGMSG("radio play-stop time_out!\n");
	
	TimerDisable(nano.l_key.lid);
	nano.l_key.l_flag = 0;
	
	switch (set_preset_station(radio_get_freq())) {
		case PRESET_SUCCESS:
			{
				radio_tts(RADIO_SET_STATION_SUCCESS, get_cur_station_no());

				/*radio set to preset state*/
				global_flags.radio_state = RADIO_STATE_PRESET;
				{
					char index[10];
					memset(index, 0x00, 10);
					sprintf(index, "%d", get_cur_station_no());
					CoolSetCharValue("/tmp/.radio", "chan", "index", index);
					CoolSetCharValue("/tmp/.radio", "chan", "freq", radio_get_freq_str(radio_get_freq()));
				}
				radio_ui_freq(radio_get_freq());
			}
			break;

		case PRESET_ALREADY:
			radio_tts(RADIO_SET_STATION_ALREADY, 0);
			break;

		case PRESET_MAX_NUMS:
			radio_tts(RADIO_SET_STATION_MAX_NUMS, 0);
			break;
	}
}

#if 0
static void
check_start_auto_scan (void)
{
	if (nano.need_scaning && radio_tts_get_stopped()) {
		DBGMSG("nano.need_scaning && tts_stopped, start auto scan!\n");

		int ret;

		/* Disable menu when in radio auto mode! */
		plextalk_set_menu_disable(1);

		nano.old_freq = radio_get_freq();
		nano.auto_scaning = 1;
		nano.need_scaning = 0;
		autoseek.orient = global_flags.orient;
		ret = pthread_create(&radio_pid, NULL, radio_seek, (void*)&autoseek);
		if (ret < 0) {
			DBGMSG("radio_seek start pthread failure!\n");
			return;
		}
//		radio_seek(global_flags.orient);
	}
}
#endif

static void
radio_set_hp (void)
{
	snd_mixer_t *mixer;
	int setting_hp ;	//0: speaker, 1: headphone
	int hp_on;

	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_on);
	mixer = amixer_open(NULL);
	
	CoolShmReadLock(g_config_lock);
	setting_hp = g_config->setting.radio.output;
	CoolShmReadUnlock(g_config_lock);

	if (!setting_hp) {		//speaker
		DBGMSG("Radio set speaker on!\n");
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
	} else {		//headphone
		if (!hp_on) {
			DBGMSG("Radio set speaker on ,because headphone ont on!\n");
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
		} else {
			DBGMSG("Radio set headphone!\n");
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_ON);
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_OFF);
		}
	}
}


/* Radio set tts_flag */
static void 
radio_tip_voice_sig(int sig)
{
	DBGMSG("radio_tip_voice_sig.\n");
	radio_tts_set_stopped();
}


static void 
radio_fd_read(void *pdata)
{
	DBGMSG("radio fd read!\n");
	
	struct signalfd_siginfo fdsi;

   	ssize_t ret = read(nano.fd, &fdsi, sizeof(fdsi));

   	if (ret != sizeof(fdsi))
		DBGMSG("Radio read fd err!!\n");
   
	radio_tts_set_stopped();
}


static void 
radio_reg_ttsSingal(void)
{
	char* pdata;   //private data

	nano.fd = voice_prompt_handle_fd();
	NeuxAppFDSourceRegister(nano.fd, pdata, radio_fd_read, NULL);
	NeuxAppFDSourceActivate(nano.fd, 1, 0);
}


static int 
radio_init (struct radio_nano* nano)
{
	DBGMSG("radio init!\n");
	
	plextalk_global_config_open();

	DBGMSG("Radio set menu disable before radio ready!\n");
	plextalk_set_menu_disable(1);

	DBGMSG("Radio get suspend lock value!!\n");

	// debug infomation about suspend of radio
	CoolShmReadLock(g_config_lock);
	DBGMSG("When in radio, g_config->suspend_lock = %d!!!\n", g_config->suspend_lock);
	CoolShmReadUnlock(g_config_lock);
	
	radio_auto_scan_end_flag = 0;
	radio_auto_scan_mode_active = 0;
	radio_auto_scan_end_set_unmute = 1;
	radio_pid = -1;
	radio_mute_state = 0;
	radio_auto_scan_first_seek = 1;

	memset(nano, 0x00, sizeof(struct radio_nano));

	radio_set_region();

	DIR *dir = opendir(PLEXTALK_SETTING_DIR);
	if (dir == NULL) {
		mkdir(PLEXTALK_SETTING_DIR, O_CREAT);
		DBGMSG("create PLEXTALK_SETTING_DIR\n");
	} else {
		closedir(dir);
		DBGMSG("PLEXTALK_SETTING_DIR exit\n");
	}

	radio_reg_ttsSingal();
	
	/* Init language */
	CoolShmReadLock(g_config_lock);
	plextalk_update_sys_font(g_config->setting.lang.lang);
	NeuxThemeInit(g_config->setting.lcd.theme);
	plextalk_update_lang(g_config->setting.lang.lang, "radio");
	plextalk_update_tts(getenv("LANG"));
	CoolShmReadUnlock(g_config_lock);

	road_flag = OUT_PUT_DAC;
	
	/* reocrding function not in progress */
	global_flags.recording = 0;
	global_flags.menu_flag = 0;

	nano->first_logo = 1;

	CreateRadioFormMain();
	/* radio tts "Radio Mode" */
	radio_tts_init();
	radio_set_hp();		//in case for radio guid voice output wrong!!!
	radio_set_mute(true, 0);			//set mute when in "Radio Mode"
	radio_tts(RADIO_MODE_START, 0);
	nano->first_logo = 1;

	/* radio preset station init */
	if (preset_init() == -1) {
		DBGMSG("Radio Preset init failure!\n");
		recreate_station_file();			//no expect!!!!
	}
	
	/* open fm tuner */
	if (radio_start() == -1) {
		DBGMSG("Open radio error!\n");
		return -1;
	}

	/* get history status */
	int ret;
	int status = 0;
	ret = CoolGetIntValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "status", &status);
	if (ret != COOL_INI_OK) {
		status = 0;
		CoolSetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "status", "0");
	}

	/* recover radio state */
	global_flags.radio_state = status;
	if ((global_flags.radio_state > RADIO_STATE_PRESET) 
		|| (global_flags.radio_state < RADIO_STATE_MANU_SEARCH))
	{
		global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
	}
	
	NhelperTitleBarSetContent(_("Hearing Radio"));
	
	/* if in autoscan mode , set to manu search mode */
	if (global_flags.radio_state == RADIO_STATE_AUTO_SEARCH) {
		global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
		CoolSetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "status", "0");
	}

	/* recover last frequence */
	double freq, fmMin;
	char buf[10];
	fmMin = radio_get_fmMin();
	memset(buf, 0x00, 10);
	ret = CoolGetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "freq", buf, 10);
	if (ret != COOL_INI_OK) {
		freq = fmMin;
		CoolSetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "freq", radio_get_freq_str(freq));
	} else {
		double fmMax;

		freq = atof(buf);
		DBGMSG("Get History freq = %f\n", freq);
		
		fmMax = radio_get_fmMax();
		if ((freq <= fmMin) || (freq >= fmMax))
			freq = fmMin;
	}
	radio_set_freq(freq);

	if (global_flags.radio_state == RADIO_STATE_PRESET)
	{
		char index[10];
		memset(index, 0x00, 10);
		sprintf(index, "%d", get_cur_station_no());
		CoolSetCharValue("/tmp/.radio", "chan", "index", index);
		CoolSetCharValue("/tmp/.radio", "chan", "freq", radio_get_freq_str(freq));
	}

//	nano->first_logo = 1;
	nano->key_lock = check_keylocedstatus();
	
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_ON);
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_INPUT_MUX, PLEXTALK_INPUT_LINEIN);

	return 0;
}


/* handle signal */
static void 
signal_handler(int signum)
{
		DBGMSG("signal USR1 recived!\n");
}

static void
signalInit (void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	if (sigaction(SIGUSR1, &sa, NULL)){}
}

static void RefreshRadioMainForm (void)
{
	widget_prop_t wprop;
	
	if (global_flags.recording)
	{
		NeuxWidgetGetProp(nano.formRecMain, &wprop);
		wprop.bgcolor = theme_cache.background;
		wprop.fgcolor = theme_cache.foreground;
		NeuxWidgetSetProp(nano.formRecMain, &wprop);
		
#ifdef USE_LABEL_CONTROL	
		NeuxWidgetGetProp(nano.filename_label, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano.filename_label, &wprop);

		NeuxWidgetGetProp(nano.rectime_label, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano.rectime_label, &wprop);

		NeuxWidgetGetProp(nano.freshtime_label, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano.freshtime_label, &wprop);
		
		NeuxWidgetGetProp(nano.remain_label, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano.remain_label, &wprop);

		NeuxWidgetGetProp(nano.remaintime_label, &wprop);
		wprop.fgcolor = theme_cache.foreground;
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano.remaintime_label, &wprop);
#else
		GrSetGCForeground(nano.pix_gc, theme_cache.foreground);
		GrSetGCBackground(nano.pix_gc, theme_cache.background);
		Radio_Rec_showScreen();

#endif		
	} 
	
	NeuxWidgetGetProp(nano.label1, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.label1, &wprop);

	NeuxWidgetGetProp(nano.label2, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.label2, &wprop);

	NeuxWidgetGetProp(nano.formMain, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.formMain, &wprop);
}


/* App Message form system */
static void 
onRadioAppMsg(const char * src, neux_msg_t * msg)
{
	DBGMSG("app msg %d .\n", msg->msg.msg.msgId);

	if (nano.first_logo) {
		DBGMSG("First logo, returned!\n");
		return 0;
	}
	
	switch (msg->msg.msg.msgId)
	{
	case PLEXTALK_APP_MSG_ID:
		{
			app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;

			switch (app->type)
			{
			case APPRQST_SET_STATE:
			{		
				DBGMSG("AppMsg set state!\n");
				
				app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;
				if (rqst->pause)
				{
					DBGMSG("App set pause!\n");
					
					if (nano.binfo)
					{
						DBGMSG("nano.binfo, info pause!\n");
						info_pause();
					}
					else
					{
						int bChanle;

						//暂停标志位，防止两次暂停
						if(nano.app_pause){
							DBGMSG("Hey! Radio already paused\n");
							return;
						}
						nano.app_pause = 1;
						
						bChanle = (global_flags.radio_state == RADIO_STATE_PRESET)? 1 : 0;
						DBGMSG("sig menu popup handler before!\n");
						sig_menu_popup_handler();
						amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_OFF);
						DBGMSG("sig menu popup handler after!\n");
					}
				}
				else
				{
					DBGMSG("App set resume!\n");
					if (nano.binfo)
					{
						DBGMSG("nano.binfo, info resume!\n");
						info_resume();
					}
					else
					{
						DBGMSG("sig menu exit before!\n");

						if(!nano.app_pause){
							DBGMSG("Hey! No need Resume\n");
							
						} else {		//在radio的录音模式下会出现的一个问题
							nano.app_pause = 0;
							
							sig_menu_exit_handler();
						}
						
						/* set to bypass */
						if (!radio_mute_state)
							amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_BYPASS);

						amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_ON);

						DBGMSG("DEBUG: Set OutPut Bypass!!!\n");
						DBGMSG("sig menu exit after!\n");
					}
				}				
			}
			break;
			
			case APPRQST_DELETE_CONTENT:
				{
					app_delete_rqst_t* del = (app_delete_rqst_t*)msg->msg.msg.msgTxt;
					switch (del->op)
					{
					case APP_DELETE_CURRENT_CHANNEL:
						DBGMSG("AppMSG   APP_DELETE_CURRENT_CHANNEL !\n");
						sig_del_cur_preset_handler();
						break;
					case APP_DELETE_ALL_CHANNELS:
						DBGMSG("AppMSG   APP_DELETE_ALL_CHANNELS !\n");
						sig_del_all_preset_handler();
						break;
					}

					/* This should be delete only sig_del_cur(all)_preset_handler return OK */
					CoolDeleteSectionKey("/tmp/.radio", "chan", "index");
					CoolDeleteSectionKey("/tmp/.radio", "chan", "freq");
				}
				break;
				
			case APPRQST_SET_RECORD_TARGET:
				if (global_flags.recording)
				{
					DBGMSG("AppMsg change media\n");
#if REC_USE_PREPARE
					radio_rec_change_media();
#else
					if(data.rec_stat == REC_PERPARE) {
						radio_rec_change_media();
					}
#endif
				}
				break;

			case APPRQST_GUIDE_VOL_CHANGE:
			case APPRQST_GUIDE_SPEED_CHANGE:
			case APPRQST_GUIDE_EQU_CHANGE:
				radio_tts_set_promptcfg();
				break;

			case APPRQST_SET_THEME:
				{
					DBGMSG("AppMsg set theme!\n");
					if (theme_index !=app->theme.index) 
					{
						theme_index=app->theme.index;
						NeuxThemeInit(theme_index);
						RefreshRadioMainForm();
					}
				}
				break;
				
			case APPRQST_SET_FONTSIZE:	
				DBGMSG("App set fontsize!\n");
				
				plextalk_update_sys_font(getenv("LANG"));
				if(global_flags.recording)
				{
#ifdef USE_LABEL_CONTROL
					NeuxWidgetShow(nano.filename_label, TRUE);
				    NeuxLabelSetText(nano.rectime_label, _("Recording Time:"));
				    NeuxWidgetShow(nano.rectime_label, TRUE);					    
				    NeuxWidgetShow(nano.freshtime_label, TRUE);
				    NeuxLabelSetText(nano.remain_label, _("Remaining Time:"));
				    NeuxWidgetShow(nano.remain_label, TRUE);					    
				    NeuxWidgetShow(nano.remaintime_label, TRUE);
#else
									
					Radio_Rec_FreeStringBuf(&nano);
					Radio_Rec_InitStringBuf(&nano,data.rec_filename);	
			
					NeuxWidgetSetFont(nano.formRecMain, sys_font_name, /*sys_font_size*/16);
					nano.pix_gc = NeuxWidgetGC(nano.formRecMain);
					Radio_RecShow_Init(&nano, /*sys_font_size*/16);
					Radio_Rec_showScreen();
#endif
						
				    NhelperTitleBarSetContent(_("Recording Radio"));
				}
				break;

			case APPRQST_SET_LANGUAGE:
				{
					DBGMSG("App set language!\n");
					
					CoolShmReadLock(g_config_lock);
					plextalk_update_lang(g_config->setting.lang.lang, "radio");
					CoolShmReadUnlock(g_config_lock);

					plextalk_update_tts(getenv("LANG"));
					plextalk_update_sys_font(getenv("LANG"));


					if (global_flags.recording)
					{
						DBGMSG("Set language , in recording!\n");
#ifdef USE_LABEL_CONTROL
						NeuxWidgetShow(nano.filename_label, TRUE);
					    NeuxLabelSetText(nano.rectime_label, _("Recording Time:"));
					    NeuxWidgetShow(nano.rectime_label, TRUE);					    
					    NeuxWidgetShow(nano.freshtime_label, TRUE);
					    NeuxLabelSetText(nano.remain_label, _("Remaining Time:"));
					    NeuxWidgetShow(nano.remain_label, TRUE);					    
					    NeuxWidgetShow(nano.remaintime_label, TRUE);
#else
									
						Radio_Rec_FreeStringBuf(&nano);
						Radio_Rec_InitStringBuf(&nano,data.rec_filename);	
				
						NeuxWidgetSetFont(nano.formRecMain, sys_font_name, /*sys_font_size*/16);
						nano.pix_gc = NeuxWidgetGC(nano.formRecMain);
						Radio_RecShow_Init(&nano, /*sys_font_size*/16);
						Radio_Rec_showScreen();
#endif
						
				//	    NhelperTitleBarSetContent(_("Recording Radio"));
					} else {
						DBGMSG("Set language , in radio!\n");

						char buf[64];
						
						NeuxWidgetShow(nano.label1, TRUE);
						switch (global_flags.radio_state) {
							case RADIO_STATE_AUTO_SEARCH:
								NeuxLabelSetText(nano.label1, _("Auto"));
								break;
							case RADIO_STATE_MANU_SEARCH:
								NeuxLabelSetText(nano.label1, _("Manual"));
								break;
							case RADIO_STATE_PRESET:
								snprintf(buf, 8, "%dCH", get_cur_station_no());
								NeuxLabelSetText(nano.label1, buf);
								break;
						}
						
						NeuxWidgetShow(nano.label2, TRUE);
					//	NhelperTitleBarSetContent(_("Hearing Radio"));
					// 	when set language, do not set title bar!
					}
				}
				break;
				
			case APPRQST_SET_TTS_VOICE_SPECIES:
			case APPRQST_SET_TTS_CHINESE_STANDARD:
				{	//tts role and lang
					DBGMSG("AppMsg set tts!\n");
					
					plextalk_update_tts(getenv("LANG"));
				}
				break;
				
			case APPRQST_RESYNC_SETTINGS:
				{
					DBGMSG("AppMsg resync!\n");
					
					CoolShmReadLock(g_config_lock);
					
					NeuxThemeInit(g_config->setting.lcd.theme);
					plextalk_update_lang(g_config->setting.lang.lang, "radio");
					plextalk_update_sys_font(g_config->setting.lang.lang);
					plextalk_update_tts(g_config->setting.lang.lang);

					radio_reset_tts_cfg ();
					
					CoolShmReadUnlock(g_config_lock);

					if (global_flags.recording)
					{
						Radio_Rec_FreeStringBuf(&nano);
						Radio_Rec_InitStringBuf(&nano,data.rec_filename);	
				
						NeuxWidgetSetFont(nano.formRecMain, sys_font_name, /*sys_font_size*/16);
						nano.pix_gc = NeuxWidgetGC(nano.formRecMain);
						Radio_RecShow_Init(&nano, /*sys_font_size*/16);
						Radio_Rec_showScreen();
					}

					RefreshRadioMainForm();
				}
				break;
				
			case APPRQST_LOW_POWER:
				DBGMSG("AppMsg low power!!!\n");
				NeuxAppStop();
				break;	

			default:
				DBGMSG("Other App Msg.\n");
				break;
			}
		}
		break;
	}
}

#define FIRST_LOGO_COUNT_OUT 10
static int first_logo_count;

/* Radio Timer 0.5s */
static void
OnRadioTimer(WID__ wid)
{
	if (nano.first_logo)
	{	
		first_logo_count ++;
		
		DBGMSG("first_logo_count = %d\n", first_logo_count);
	
		if (radio_tts_get_stopped() || (first_logo_count > FIRST_LOGO_COUNT_OUT)) {
			NeuxWidgetDestroy(&nano.label_logo);
			NeuxWidgetShow(nano.label1, TRUE);
			NeuxWidgetShow(nano.label2, TRUE);
			
			DBGMSG("first_logo && radio_tts_get_stopped, set first_logo 0!!\n");
			nano.first_logo = 0;			//must before radio_ui_freq()
			radio_ui_freq(radio_get_freq());	

			DBGMSG("first_logo && radio_tts_get_stopped, set menu disable to 0!!\n");
			plextalk_set_menu_disable(0);
			
			nano.first_bypass = 1;
			radio_set_mute(false, 0);
			radio_tts(RADIO_CUR_FREQ, (int)(10 * radio_get_freq()));
			NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, 
				radio_is_muted() ? SBAR_CONDITION_ICON_PAUSE : SBAR_CONDITION_ICON_PLAY);
		}

		DBGMSG("First logo 1, return directly!\n");
		return;
	}

	radio_ui_freq(radio_get_freq());		//update freq... every timer comes
	
	/* Do when radio auto scan end! */
	if (radio_auto_scan_end_flag) {
		DBGMSG("UI timer detect radio auto scan end!!!\n");

		/* Enable menu when in auto mode ended */
		plextalk_set_menu_disable(0);
		
		float new_freq;				
		double ret;
		radio_pid = -1;

		global_flags.radio_state = RADIO_STATE_MANU_SEARCH;
		nano.bPause = 0;
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_PLAY);
		NhelperTitleBarSetContent(_("Hearing radio"));

		new_freq = radio_get_freq();
		ret = (new_freq - nano.old_freq);

		/* set unmute when radio seek end! */
		radio_set_mute(false, 0);
		
		if ((ret < 0.01) && (ret > -0.01))
			radio_tts(RADIO_AUTO_FAIL, 0);
		else
			radio_tts(RADIO_CUR_FREQ, (int)(10 * new_freq));
		radio_ui_freq(radio_get_freq());

		nano.auto_scaning = 0;
		nano.old_freq = new_freq;
		radio_auto_scan_end_flag = 0;
//		radio_auto_scan_end_set_unmute = 1;
	}


	/* Check if need to do auto scan */
	//check_start_auto_scan();

	/* Is these needed ? */
	if ((road_flag == OUT_PUT_DAC) && (!global_flags.menu_flag) 
			&& (!NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP)) && (radio_tts_get_stopped())) {
		road_flag = OUT_PUT_BYPASS;
		if(global_flags.del_voc_runing)
			global_flags.del_voc_runing = 0;
	}

}


/* Long key pressed */
static void 
OnLongKeyTimer(WID__ wid)
{
	DBGMSG("Radio OnLongKeyTimer!\n");

	switch (nano.l_key.key) {
	case MWKEY_RIGHT:
		radio_autoscan_time_out(0);
		break;
	case MWKEY_LEFT:
		radio_autoscan_time_out(1);
		break;
	case MWKEY_ENTER:
		radio_play_stop_time_out();
		break;
	}
}

/* For radio_rec widget destroy , label text(old)will be showed */	//unexpectly...
void radio_ui_freq_recover (void)
{
	char buf[8];
	double freq = radio_get_freq();
		
	switch (global_flags.radio_state) {
	case RADIO_STATE_AUTO_SEARCH:
		NeuxLabelSetText(nano.label1, _("Auto"));
		break;

	case RADIO_STATE_MANU_SEARCH:
		NeuxLabelSetText(nano.label1, _("Manual"));
		break;
		
	case RADIO_STATE_PRESET:
		snprintf(buf, 8, "%dCH", get_cur_station_no());
		NeuxLabelSetText(nano.label1, buf);
		break;
	}

	NeuxLabelSetText(nano.label2, radio_get_freq_str(freq));
}


void radio_rec_exit()
{
	DBGMSG("radio_rec_exit\n");	
}


/* Radio key down handler */
static void
OnRadioKeydown(WID__ wid, KEY__ key)
{
	DBGMSG("OnRadioKeydown key.ch = %d\n", key.ch);

	if (nano.first_logo) {
		DBGMSG("nano.first_logo is 1, return!\n");
		return ;
	}

	if (nano.binfo || nano.app_pause) {
		DBGMSG("key down , nano.binfo || app_pause return.\n");
		DBGMSG("nano.binfo = %d\n", nano.binfo);
		DBGMSG("nano.app_pause = %d\n", nano.app_pause);
		return;
	}

	if (NeuxAppGetKeyLock(0)) {
		 if (!key.hotkey) {
		   voice_prompt_abort();
		   voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		   voice_prompt_string_ex2(0, PLEXTALK_OUTPUT_SELECT_BYPASS, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			    tts_lang, tts_role, _("Keylock"), strlen(_("Keylock")) );
		  }
		  return;
	 }
	
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		DBGMSG("key down , help running, return.\n");
		return;
	}

	switch (key.ch)
    {
    case MWKEY_UP:
		if(nano.auto_scaning) {
			DBGMSG("nano.auto_scaning, return.\n");
			
			radio_tts(TTS_ERROR_BEEP, 0);
			return;
		}
		radio_key_up_down_handler(0);
		break;		

	case MWKEY_DOWN:
		if(nano.auto_scaning) {
			DBGMSG("nano.auto_scaning, return.\n");

			radio_tts(TTS_ERROR_BEEP, 0);
			return;
		}
		radio_key_up_down_handler(1);
		break;		
		
	case MWKEY_RIGHT:
	case MWKEY_LEFT:
	case MWKEY_ENTER:
		{
			if (nano.auto_scaning) {
				DBGMSG("nano.auto_scaning, stop auto scaning!\n");
			//	radio_tts(TTS_ERROR_BEEP, 0);
			//	return;
				radio_auto_stop();
			//
			}
			
			DBGMSG("Enable timer, juduge long key!\n");
			TimerEnable(nano.l_key.lid);
			nano.l_key.l_flag = 1;
			nano.l_key.key = key.ch;
			switch (key.ch)
			{
			case MWKEY_RIGHT:
				global_flags.orient = SCAN_FORWARD;
				break;
			case MWKEY_LEFT:
				global_flags.orient = SCAN_BACKWARD;
				break;
			}
		}
		break;
		
	case MWKEY_MENU:
			if(nano.auto_scaning) {
				DBGMSG("Mwkey Menu : nano.auto_scaning, return.\n");
				//radio_auto_stop();
				radio_tts(TTS_ERROR_BEEP, 0);
				return;
			}
			break;
			
	case VKEY_RECORD:
		{
			if(nano.auto_scaning)
			{	
				DBGMSG("Mwkey record : nano.auto_scaning, return.\n");
				//radio_auto_stop();
				radio_tts(TTS_ERROR_BEEP, 0);
				return;
			}
			
			global_flags.recording = 1;
			TimerDisable(nano.timer);
			
			/* Tell desktop in readio recording mode */
			NhelperRadioRecordingActive(1);
			nano.record_recover = 0;
			radio_rec_dialog();	
			
			//NeuxWidgetFocus(nano.formMain);	这里不能设置，radio_rec_dialog并不是一个do_modal
		}		
		break;
		
	case MWKEY_INFO:
		if(nano.auto_scaning)
		{
			DBGMSG(" nano.auto_scaning, return.\n");
			radio_tts(TTS_ERROR_BEEP, 0);
			return;
		}
		radio_key_info_handler();
		break;
		
	case VKEY_RADIO:
		radio_tts(TTS_ERROR_BEEP, 0);
		break;
	}
}


/* up key handler in radio mode */
static void
OnRadioKeyup(WID__ wid, KEY__ key)
{
	DBGMSG("OnRadioKeyUp key.ch = %d\n", key.ch);
	if(nano.binfo || nano.app_pause) {
		DBGMSG("nano.binfo ,app_pause, return.\n");
		return;
	}
	
	/* Judge Longkey in key up */
    switch (key.ch)
    {
	case MWKEY_RIGHT:
		{
			DBGMSG("MWKEY_RIGHT nano.l_key.l_flag = %d\n", nano.l_key.l_flag);
			if (nano.l_key.l_flag) {						
				TimerDisable(nano.l_key.lid);
				nano.l_key.l_flag = 0;
				radio_key_left_right_handler(1);
			}			
		}
		break;
		
	case MWKEY_LEFT:
		{
			if (nano.l_key.l_flag) {
				TimerDisable(nano.l_key.lid);
				nano.l_key.l_flag = 0;
				radio_key_left_right_handler(0);
			}
		}
    	break;
		
	case MWKEY_ENTER:
		{
			if (nano.l_key.l_flag) {
				DBGMSG("Handler for short press!\n");
				TimerDisable(nano.l_key.lid);
				nano.l_key.l_flag = 0;
				radio_key_play_stop_handler();
			}
		}
		break;
	}
}


static void
OnRadioDestroy(WID__ wid)
{
    DBGMSG("OnRadioDestroy destroy!\n");

	radio_tts_destroy();

	char buf[10];
	memset(buf, 0x00, 10);
	sprintf(buf, "%d", global_flags.radio_state);
	CoolSetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "status", buf);
	CoolSetCharValue(PLEXTALK_SETTING_DIR"radio.ini", "history", "freq", radio_get_freq_str(radio_get_freq()));
	CoolDeleteSectionKey("/tmp/.radio", "chan", "index");
	CoolDeleteSectionKey("/tmp/.radio", "chan", "freq");

	radio_auto_seek_kill();
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_OFF);

	radio_uninit();

	/* Sure menu enabled after level radio mode! */
	plextalk_set_menu_disable(0);

	/* Set Bypass to DAC */
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);

	Radio_Rec_FreeStringBuf(&nano);

	close(nano.fd);	
	plextalk_global_config_close();
}


static void OnRadioAppDestroy()
{
	DBGMSG("OnRadioAppDestroy destroy!\n");
}


/* Need to set icon when get focus */
static void OnRadioGetFocus(WID__ wid)
{
	DBGMSG("On Window Get Focus\n");

	/* for power dissipation!~ */
	//radio_start();
	//radio_set_freq(backup_freq);
	//radio_ui_freq(radio_get_freq());

	radio_ui_freq(radio_get_freq());
	NhelperTitleBarSetContent(_("Hearing Radio"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_RADIO);	
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);

	if (nano.first_logo != 1)
		NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, 
			radio_is_muted() ? SBAR_CONDITION_ICON_PAUSE : SBAR_CONDITION_ICON_PLAY);
}

static void OnRadioLoseFocus (WID__ wid)
{
	DBGMSG("Radio Window lose focus!\n");

	/* for power dissipation!~ */
	//backup_freq = radio_get_freq();
	//radio_uninit();
}

static void radio_show_logo (int wid)
{
	/* Show logo */
	GR_GC_ID gc;
	int font_id;
	char *pstr = _("RADIO");
	int w,h,b;
	int x,y;
	widget_prop_t wprop;
	label_prop_t lbprop;

	
	gc = GrNewGC();
	font_id = GrCreateFont((GR_CHAR*)sys_font_name, 24, NULL);
	GrSetGCUseBackground(gc ,GR_FALSE);
	GrSetGCForeground(gc,theme_cache.foreground);
	GrSetGCBackground(gc,theme_cache.background);
	GrSetFontAttr(font_id , GR_TFKERNING, 0);
	GrSetGCFont(gc, font_id);

	GrGetGCTextSize(gc, pstr, -1, MWTF_UTF8, &w, &h, &b);
	y = (MAINWIN_HEIGHT - h)/2;
	x = (MAINWIN_WIDTH - w)/2;
	
//	GrText(wid, gc, x, y, pstr, strlen(pstr), MWTF_UTF8 | MWTF_TOP);
//	GrFlush();


	nano.label_logo = NeuxLabelNew(nano.formMain, MAINWIN_LEFT, y, MAINWIN_WIDTH, FM_LABEL_H, _("RADIO"));
	NeuxWidgetGetProp(nano.label_logo, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.label_logo, &wprop);

	NeuxLabelGetProp(nano.label_logo, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_CENTER;
	lbprop.transparent = FALSE;//TRUE;
	NeuxLabelSetProp(nano.label_logo, &lbprop);
	NeuxWidgetSetFont(nano.label_logo, sys_font_name, 24);

	NeuxLabelSetText(nano.formMain, pstr);
	NeuxWidgetShow(nano.label_logo, true);
}



void
CreateRadioFormMain (void)
{
    widget_prop_t wprop;
	label_prop_t lbprop;

	/* Create a New window */
	nano.formMain = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	/* Create timer */
   	nano.timer = NeuxTimerNew(nano.formMain, FM_TIMER_PERIOD, -1);
	NeuxTimerSetCallback(nano.timer, OnRadioTimer);
	TimerDisable(nano.timer);
	
	nano.l_key.lid = NeuxTimerNew(nano.formMain, FM_TIMER_LKEY, -1);
	NeuxTimerSetCallback(nano.l_key.lid, OnLongKeyTimer);
	TimerDisable(nano.l_key.lid);
	
	/* Set callback for window events */	
	
	NeuxFormSetCallback(nano.formMain, CB_KEY_DOWN,  OnRadioKeydown);
	NeuxFormSetCallback(nano.formMain, CB_KEY_UP,    OnRadioKeyup);
	NeuxFormSetCallback(nano.formMain, CB_DESTROY,   OnRadioDestroy);
	NeuxFormSetCallback(nano.formMain, CB_FOCUS_IN,  OnRadioGetFocus);
	NeuxFormSetCallback(nano.formMain, CB_FOCUS_OUT, OnRadioLoseFocus);

	/* Set proproty(color)*/
	NeuxWidgetGetProp(nano.formMain, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano.formMain, &wprop);

	/* Show window */
    NeuxFormShow(nano.formMain, TRUE);

	/* label1 */
	nano.label1 = NeuxLabelNew(nano.formMain, MAINWIN_LEFT, RADIO_LABEL_T, MAINWIN_WIDTH, FM_LABEL_H, _("RADIO"));
	NeuxWidgetGetProp(nano.label1, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.label1, &wprop);

	NeuxLabelGetProp(nano.label1, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_CENTER;
	lbprop.transparent = FALSE;//TRUE;
	NeuxLabelSetProp(nano.label1, &lbprop);

	NeuxWidgetSetFont(nano.label1, sys_font_name, 24);
	//NeuxLabelSetScroll(label1, TITLEBAR_SCROLL_TIME);
	NeuxLabelSetText(nano.label1, _(" "));		// with null
	NeuxWidgetShow(nano.label1, FALSE);

	/* label2 */
	nano.label2 = NeuxLabelNew(nano.formMain, MAINWIN_LEFT, RADIO_LABEL_T+FM_LABEL_H, MAINWIN_WIDTH, FM_LABEL_H, _(""));

	NeuxWidgetGetProp(nano.label2, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano.label2, &wprop);

	NeuxLabelGetProp(nano.label2, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_CENTER;
	lbprop.transparent = FALSE;//TRUE;
	NeuxLabelSetProp(nano.label2, &lbprop);
	NeuxWidgetSetFont(nano.label2, sys_font_name, 24);	
	NeuxLabelSetText(nano.label2, _(" "));	//with null
	
	NeuxWidgetShow(nano.label2, FALSE);
	
	NeuxWidgetFocus(nano.formMain);

	/* Radio show logo */
	radio_show_logo(NeuxWidgetWID(nano.formMain));
}


void DesktopScreenSaver(GR_BOOL activate)
{

}


int main (void)
{
	if (NeuxAppCreate("radio"))
		FATAL("unable to create application.!\n");

	DBGMSG("signal init!\n");
	
	signalInit();

	if (radio_init(&nano) == -1) {
		DBGMSG("FATAL: radio init failure!\n");
		return 0;
	}

	/* Set callback for app events */		
	NeuxAppSetCallback(CB_MSG,     onRadioAppMsg);
	NeuxAppSetCallback(CB_DESTROY, OnRadioAppDestroy);

	/* avoid for set bypass than  expected */
	TimerEnable(nano.timer);
	
	/* Main loop */
	NeuxAppStart();

	return 0;
}
