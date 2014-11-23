#ifndef __FORMAT_TTS_H__
#define __FORMAT_TTS_H__

//#include "tipvoice.h"

enum tts_type_format {

	TTS_ENTER_FORMAT = 0,		//tts report enter the calculator
	TTS_REMOVE_ERROR,		//tts report the meida removed error
	TTS_ERROR,				//tts report error calculation 
	TTS_SUCCESS,
	TTS_READONLY,
	TTS_MEDIALOCKED,
	TTS_WRITEERROR,
	TTS_FORMAT_WAIT_BEEP,
	TTS_FORMAT_WAIT,
	TTS_FORMAT_MENU_WAIT,
	TTS_FORMAT_LOCK,
	TTS_FORMAT_WAIT_BEEP_NO_ABORT,
};
enum tts_end_state{

	TTS_END_STATE_NONE = 0,
	TTS_END_STATE_EXIT,
	TTS_END_STATE_SUCCESS,

};


void format_tts (enum tts_type_format type);

void format_tts_init (int* is_running);

void format_tts_destroy (void);

void format_tts_info (char* input_string);

enum tts_end_state format_get_tts_end_flag(void);
enum tts_end_state format_set_tts_end_flag(enum tts_end_state status);


#endif
