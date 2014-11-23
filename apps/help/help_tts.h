#ifndef __HELP_TTS_H__
#define __HELP_TTS_H__


enum {
	TTS_POWER = 0,
	TTS_BOOK, 
	TTS_MUSIC,
	TTS_RADIO,
	TTS_PLAY_STOP,
	TTS_UP,
	TTS_DOWN,
	TTS_LEFT,
	TTS_RIGHT,
	TTS_VOL_INC,
	TTS_VOL_DEC,
	TTS_RECORD,
	TTS_MENU,
	TTS_INFO,
	TTS_BEGIN,
	TTS_EXIT,
	TTS_ERROR,
	TTS_LOCK,
	TTS_MAX_NUM
};

enum tts_end_state{

	TTS_END_STATE_NONE = 0,
	TTS_END_STATE_EXIT,

};


void help_tts_init (void);

void help_tts (int mode);

void help_tts_end (void);

void wait_tts(void);

void help_tts_set_stop (void);

int help_tts_get_stop (void);

void help_on_tts_end_callback (void);
void rec_set_tts_end_state(enum tts_end_state state);
enum tts_end_state help_get_tts_state();
void help_set_tts_end_state(enum tts_end_state state);



#endif
