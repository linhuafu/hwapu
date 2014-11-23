#ifndef __REC_DRAW_H__
#define __REC_DRAW_H__
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include "Widgets.h"


#define __(S)				gettext(S)

#define REC_USE_PREPARE				0//录音准备界面需要进行错误处理
//#define USE_LABEL_CONTROL  		1//使用控件显示方式
#define REC_MAX_LINES	50
#define MAX_SHOW_LINE 	50
#define MAX_SCREEN_BYTE 1024
#define MAX_SCREEN_LINE 8


struct rec_text{
	int x;
	int y;
	int w;
	int h;
	int font_size;
	int line_height;
	int screen_line;

	int 				total_line;		//total line
	int 				start_line;		//show start line
	int 				show_line;		//can show line

	int 				lineStart[REC_MAX_LINES];	//line offset to ptext
	int 				lineLen[REC_MAX_LINES]; 	//line len
	int 				bMoreText;

	int 				textSize;		//show line

	char				*pScrStart;		//cur screen start
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


struct rec_nano{

#ifdef USE_LABEL_CONTROL
	LABEL *filename_label;
	LABEL *rectime_label;
	LABEL *rectime_value;
	LABEL *remaintime_label;
	LABEL *remaintime_value;
#endif	
	TIMER *rec_timer;

	struct rec_text textShow;

	int wid;
	int pix_wid;
	int pix_gc;
	int font_id;
	int fg_color;
	int bg_color;
	
	int    remain_time;
	int    isIninfo;

	int first_logo;
	int headphone;
	int keylocked;

	/* This for signal handler in timeout */
	int sig_chg_mea;
	int sig_menu_exit;

	int set_guid_volume;
	int set_guid_speed;
	int set_font_size;
	int set_font_color;
	int set_language;
	int set_tts;

	/* ID for mic recording infomation */
	void* info_id;


	/* If tts is running */
	int is_running;

	/*current recording time seconds*/
	int cur_time;

	int app_pause;

	unsigned long long freespace;
};


struct _rec_data{

	char* rec_filename;
	char* rec_filepath;
	int   rec_stat;
	int   rec_media; // 1:internnal memory 0:sd card
	int   isalarmdisable;
	int   ishotplugvoicedisable;
	int   isSDhotplugvoicedisable;

};


#endif
