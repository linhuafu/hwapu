#ifndef __AUDIO_TTS_H__
#define __AUDIO_TTS_H__

#include "radio_type.h"

enum tts_type{

	RADIO_MODE_START = 0,

	RADIO_CUR_FREQ,

	RADIO_SET_STATION_SUCCESS ,
	RADIO_SET_STATION_ALREADY,
	RADIO_SET_STATION_MAX_NUMS,

	RADIO_CHANNEL_NOTFOUND,
	RADIO_CHANNEL_FOUND,

	RADIO_DEL_STATION_SUCCESS,
	RADIO_DEL_STATION_FAILURE,
	RADIO_DEL_ALL_STATION,
	
	AUTO_SCAN_ING,
	AUTO_SCAN_BEGIN,

	RECORD_USAGE,
	RECORD_EXIT,
	RECORD_EXIT_ERROR,
	CHANGE_STATE_ERROR,
	CHANGE_STATE_RECORDING,
	CHANGE_STATE_PAUSE,
	CHANGE_STATE_PREPARE,
	RADIO_REC_MONITOR_SWITCH,

	TTS_ERROR_BEEP,
	TTS_NORMAL_BEEP,
	TTS_CANCEL_BEEP,
	
	RADIO_TTS_NO_SDCARD,
	RADIO_SET_MUTE,
	RADIO_AUTO_FAIL,
	RADIO_REC_SD_REMOVE,
	RADIO_REC_SD_READONLY,
	RADIO_REC_NOSPACE,
	RADIO_REC_WRITE_ERR,
	RADIO_REC_LIMIT, 
	
};


#define OUT_PUT_BYPASS	0x01
#define OUT_PUT_DAC		0x02

/* road_flag indicate the current Output Mux */
extern int road_flag;

extern struct voice_prompt_cfg vprompt_cfg ;

void radio_tts_init (void);

void radio_tts_destroy (void);

void radio_tts_abort(void);

void radio_tts(enum tts_type type, int data);

void wait_tts (void);					//wait for tts. maybe not needed

int* radio_tts_isrunning (void);

void radio_tts_set_stopped (void);

int radio_tts_get_stopped (void);

void radio_tts_change_volume(void);

void radio_reset_tts_cfg(void);

#endif
