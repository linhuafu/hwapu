#ifndef __AUDIO_H__
#define __AUDIO_H__
#include "audio_type.h"
#include <time.h>

#define __USE_AUDIO_PLAY__
#ifdef __USE_AUDIO_PLAY__


struct _audio_stat{

	int pthread_flag;	
	time_t total_play_time;
	time_t current_play_time;
	int pause_flag;
	int stop_flag;
	int error_flag; 
	int trackflag;
	int  suspendflag;		//1    表示可以进入suspend ,0 退出suspend
};


//enum {POS_BEGIN = 0, POS_CUR, POS_END};

extern struct _audio_stat audio_stat;

int audio_start(struct start* start_place);

void audio_pause (int tts, int set_cpu,int filelist );

void audio_recover(int tts);

/*it will return -1 if seek failure*/
int audio_seek(int time);

int audio_stop(void);

int audio_tempo_set(float tempo);

int audio_pitch_set(float pitch);

int audio_speed_set (float old_speed, float new_speed);

int audio_volume_set(float volume);

/*for audio to set equalizer*/
int audio_equalizer_set(double* band);
#endif

#endif
