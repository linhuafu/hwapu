#ifndef __LIB_INFO_H__
#define __LIB_INFO_H__

#include "widgets.h"
#include "libvprompt.h"
#include "sysfs-helper.h"
#include "plextalk-i18n.h"
#include "vprompt-defines.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "mulitlist.h"
#include "plextalk-statusbar.h"
#include "plextalk-config.h"


#define INFO_ITEM_MAX	10
#define INFO_TIMER_LEN 500
#define INFO_SCR_LEN 1000

typedef enum{
	PLAY_AUDIO,
	PLAY_TTS,
}PLAY_TYPE;

struct vocinfo
{
	int cur_item;     //第几条信息
	int btext;        //是否需要播放文本
	char *vocfile;    //MP3语音文件
	int start;        //开始播放偏移
	int len;         //voc len
};

struct plex_info {

	FORM *info_wid;
	TIMER *timer;
	struct MultilineList *mlist;
	char tts_str[INFO_SCR_LEN];
	int *is_running;
	int  key_locked;
	int info_index;
	int item_count;

	PLAY_TYPE play_type;		
	int info_pause;			//info_pause
	int info_exit;			//info exit
	
	int is_mainmenu;
	int is_voc;              //have voc
	int mp3_item;
	int is_aftervoc;
	int voc_nums;
	int cur_item;
	int lines;
	int cur_lines;
	int next_lines;
	int screen_lines;
	char* tts_text[INFO_ITEM_MAX];
	struct vocinfo **voc;
};





/* Destroy Timer, Ungrab Key and  Out_Mux to DAC before in this module */

void* info_create (void);

void info_start_ex (void* info_id, char** info_text, int text_num, struct vocinfo** voc_info, int voc_num, int* is_running);

void info_start (void* info_id, char** info_text, int text_num, int* is_running);

/* Func: abort info TTS, for SIGNAL(Low_power, Alarm, ...) 
 * return:	1,abort TTS success, 0, There is no infomation TTS in progress
 */
int info_abort (void* info_id);

void info_destroy (void* info_id);

int info_pause();

int info_resume();

void info_setkeylock(int key_locked);

int info_set_menu();

#endif
