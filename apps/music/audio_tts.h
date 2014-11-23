#ifndef __AUDIO_TTS_H__
#define __AUDIO_TTS_H__


enum tts_mode{
	TTS_LOGO = 0,					//tts report logoin music mode
	TTS_PREPARE,					//tts report the prepare progress
	TTS_WAIT,						// please wait
	TTS_NAVIGATION ,				//tts report navi value
	TTS_SEEK_BOOKMARK ,				//tts report jump bookmark
	TTS_SEEK_TRACK,					//tts report seek track
	TTS_SEEK_ALBUM,					//tts report jump album
	TTS_PAUSE_RECOVER,				//tts report pause or recover 
	TTS_DELETE_TRACK,				//tts report delete track
	TTS_DELETE_ALBUM,				//tts report result of delete album
	TTS_REPEAT_MODE,				//tts report repeat_mode
	TTS_DELETE_CUR_BMK,				//tts report del cur boomark
	TTS_DELETE_ALL_BMK,				//tts report del all bookmark
	TTS_SPEED_SET,					//tts report set speed
	TTS_SET_BMK,					//tts report set bookmark
	TTS_SET_EQU,					//tts report set equalizer
	TTS_END_MUSIC,					//it has been to the end of music queue
	TTS_VOLUME,					//进行音量的播报

	TTS_USBON_BEEP,
	TTTS_USBOFF_BEEP,


	TTS_ERROR_BEEP,
	TTS_NORMAL_BEEP,
	TTS_CANCEL_BEEP,
	TTS_READ_MEDIA_ERROR,      //读取mp3文件错误
	TTS_BEGIN_MUSIC,			
	TTS_KEYLOCK,
	TTS_PLEASE_WAIT,
	TTS_READ_MEDIA_ERROR1,      //读取mp3文件错误
	TTS_VOLUME_ERROR,
	
};


void audio_tts(enum tts_mode mode, int data);

void audio_tts_init (int* is_running);

void audio_tts_destroy (void);

void wait_tts (void);

#endif
