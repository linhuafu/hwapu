#ifndef __AUDIO_TYPE_H__
#define __AUDIO_TYPE_H__
#include <dirent.h>
#include <time.h>
#include <glib.h>
#include "file_cursor.h"
#include "xml-helper.h"


#define PLAY_TRACK			0x09				//选择单曲播放
#define PLAY_ALBUM			0x10				//选择文件夹进行播放

enum NAVI{
	NAVI_SEEK_BOOKMARK = 0,			//seek with bookmark
	NAVI_SEEK_SECONDS,				//seek with 30 s
	NAVI_SEEK_MINUTES,				//seek with 10mintues
	NAVI_SEEK_TRACK,				//seek with track
	NAVI_SEEK_ALBUM,				//seek with album
};



typedef enum  _PALY_STATE{

		PLAYING,
		PAUSED,
		STOPED,
		READY,
		PENDING,
		NO_START,
		OTHER,

}PLAY_STATE;

enum REPEAT_MODE{
	STANDARD_PLAY = 0 ,  
	TRACK_REPEAT,		 
	ALBUM_REPEAT,		
	ALL_ALBUM_REPEAT,	
	SHUFFLE_PLAY,
};


struct start{
	 char  path[512];				//全路径
	 char  dirpath[512];			//文件夹路径
	 char  backupfloder[512];		//back的文件夹
	unsigned long start_time;
	int   trackindex;
	float speed;
	double* bands;
};


struct _bmk_no_info {
	
	int current_bmk_no;
	int total_bmk_no;
};





typedef struct  _PLAY_INFO{
	
	PLAY_STATE   play_state;
	int   breakflg;
	int  resumeflg;
	void (*resumeback)(int );
	
}PLAY_INFO;

struct _dir_info {

	char dir_path[512];
	DIR* music_dir;
	struct dirent* music_dirent;
	char music_path[256];
	int cur_index;
	int max_index;

	GArray* offset_array;

};


struct audio_player{
	
	struct start start_place;		//start place for create pthread

	FileCursor*  filecur;			

	int play_mode;					//track mode or album mode
	int repeat_mode;				//repeat mode
	int effect_value;				//sound effect
	int navi_value;					//navigation value

	int manual_pause;				//indicate if user set paused
	int total_songs;					//表示当前play list的总的歌曲数	
	int cursong_index;				//当前play 的歌曲是第几首

	int out_put;
	
	int  speed;						//speed of music play
	int old_speed;		
	float pitch;						//pitch

	//I have set this to int
	int volume;						//volume
	int adjust;						//adjust volume 

	int  countforsound;					//表示在TTS发音多久后，自动恢复之前的状态
	

	struct _bmk_no_info bmk_no;		//player bookmark num info


	struct equalizer_10bands  equ;


	int help;
	int exp;						//这种情况下，播放的时候，不要播放tts歌名


	/*those for signal_handler in main loop*/
	
	int del_track;
	int del_album;
	int set_speed;
	int set_repeat;
	int set_bmk;
	int del_cur_bmk;
	int del_all_bmk;
	int set_equ;
	int set_guid_volume;
	int set_guid_speed;
	int set_font_size;
	int set_font_color;
	int set_language;
	int set_tts;

	int in_file_select;
	int deltitleok;
	
	/* Indicate tts is running */
	int is_running;
	int  check_flag;
	int track_num;
	int  patherror;
	int loadflag;			//表示是否加载完成标志，只有加载完成后，才能响应左右键进行切换上下首

	PLAY_INFO   play_info;
	

};


#endif
