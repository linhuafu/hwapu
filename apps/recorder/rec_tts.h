#ifndef __REC_TTS_H__#define __REC_TTS_H__//#include "tipvoice.h"#include "libvprompt.h"enum REC_STATE{	REC_PERPARE = 0,	REC_RECORDING,	REC_PAUSE,	};
enum tts_type{

	CHANGE_STATE_PREPARE = 0,
	CHANGE_STATE_ERROR ,
	CHANGE_STATE_RECORDING ,
	CHANGE_STATE_PAUSE,	TTS_SWITCH_MONITOR_ON,	TTS_SWITCH_MONITOR_OFF,	TTS_NO_SDCARD,	ERROR_BEEP,	NORMAL_BEEP,	CANCEL_BEEP,	TTS_USAGE,	REC_EXIT,	REC_KEYLOCK,
	REC_AGC,	REC_NOTARGET,	REC_REMOVED,	REC_NOTENOUGH,	REC_WRITEERROR,	REC_BIGMAX,	REC_STARTRECORD,	REC_READONLY,};enum tts_end_state{
	TTS_END_STATE_NONE = 0,	TTS_END_STATE_EXIT,	TTS_END_STATE_MONITOR_ON,
	TTS_END_STATE_START_RECORD,	TTS_END_STATE_RUN4,
};
void rec_tts(enum tts_type type);void rec_tts_init (int* is_running);void rec_tts_destroy (void);void wait_tts (void);void rec_on_tts_end_callback (void);voidrec_init_tts_prop();void rec_set_tts_end_state(enum tts_end_state state);void rec_tts_change_volume(void);int rec_tts_get_stop (void);extern struct voice_prompt_cfg vprompt_cfg;int rec_is_in_tts_exit_state();void rec_tts_no_target();void rec_tts_removed();void DoVpromptUsbSound (int on);#endif