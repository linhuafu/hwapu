#ifndef __RADIO_TYPE_H__
#define __RADIO_TYPE_H__

#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include "widgets.h"

//lable hight
#define RADIO_LABEL_H          19
#define RADIO_LABEL_T          10
#define RADIO_LABEL_T1         24
/*label top */
#define FM_LABEL_T             0
#define FM_LABEL_H             35


//录音准备界面需要进行错误处理
#define REC_USE_PREPARE 0
//#define USE_LABEL_CONTROL 		1//使用控件显示方式

#define REC_MAX_LINES	50
#define MAX_SHOW_LINE	50
#define MAX_SCREEN_BYTE 1024
#define MAX_SCREEN_LINE 8


struct radio_rec_text{
	int x;
	int y;
	int w;
	int h;
	int font_size;
	int line_height;
	int screen_line;

	int 				total_line; 	//total line
	int 				start_line; 	//show start line
	int 				show_line;		//can show line

	int 				lineStart[REC_MAX_LINES];	//line offset to ptext
	int 				lineLen[REC_MAX_LINES]; 	//line len
	int 				bMoreText;

	int 				textSize;		//show line

	char				*pScrStart; 	//cur screen start
	char				*pScrEnd;		//cur screen end

	
	char *pText;
	int   totalItemSize;
	char *pFilename;
	int   FileNameLen;
	//char *pRecTimeLabel;
	char *pRecTimeValue;
	int   RecTimeValueLen;
	//char *pRemainTimeLabel;
	char *pRemainTimeValue;
	int   RemainTimeValueLen;
	
};




/* For Key long_press judgement */
struct long_press {

	TIMER *lid;
	int key;
	int l_flag;
	int pressed;
};


/* For Radio UI */
struct radio_nano{
#if 0
	GR_WINDOW_ID logo_wid;
	GR_GC_ID logo_gc;
	
	GR_WINDOW_ID root_wid;


	/*for radio error window*/
	GR_WINDOW_ID radio_error_wid;
	int sig_del_cur;
	int sig_del_all;
	int sig_swt_out;
	int sig_chg_mea;
	int sig_menu_exit;

	int set_guid_volume;             //system setting
	int set_guid_speed;
	int set_font_size;
	int set_font_color;
	int set_language;
	int set_tts;
	int set_backlight;
#endif


		
	FORM* formMain;
	FORM*  formRecMain;
	TIMER *rec_timer;
	TIMER* timer;

	LABEL* label1;
	LABEL* label2;
	LABEL* label_logo;

#ifdef 	USE_LABEL_CONTROL
	LABEL *filename_label;
	LABEL *rectime_label;
	LABEL *freshtime_label;
	LABEL *remain_label;
	LABEL *remaintime_label;
#endif

	struct long_press l_key;			//for left/right longpress 
//	struct long_press rec_l_key;			//for left/right longpress 

	int headphone;
	int key_lock;

	int h_volume;						//headphone volume
	int s_volume;						//Speaker	volume

	int first_logo;
	int rec_first_logo;
	int first_bypass;

	void* info_id;

	int auto_scaning;
	int need_scaning;

	double old_freq;		//记录auto_scan之前的freq
	int rec_err;
	int cur_time;
	int remain_time;
	int   record_recover;
	int fd;                 //TTS发音
	int bPause;
	int binfo;

	//int manu_pause;

	struct radio_rec_text textShow;
	int wid;
	int pix_wid;
	int pix_gc;
	int font_id;
	int fg_color;
	int bg_color;

	int app_pause;//app pause时置位，防止另一个应用还没起来，消息误发到了本应用

	unsigned long long freespace;
	
};


/* For Recorder state */
enum REC_STATE {

	REC_PERPARE = 0,
	REC_RECORDING,
	REC_PAUSE,
	
};


/* For Recorder date */
struct _rec_data{

	char* rec_filename ;
	char* rec_filepath;
	int   rec_stat;
	int   rec_media;// 1:internnal memory 0:sd card
	int ishotplugvoicedisable;
	int   isSDhotplugvoicedisable;

};


/* this for radio output */
enum {
	OUTPUT_SPEAKER = 0,
	OUTPUT_HEADPHONE, 
};


/* for radio global flags */
struct radio_flags {
	int radio_state;	// Radio current state
//	int out_put;		// Radio Output Mux
	int orient;			// Seek orient
	int recording;		// recording function or not, for sig_change_media handler
	int del_voc_runing; //删除发音是否在运行
	int menu_flag;		// if gmenu is running
};


#endif

