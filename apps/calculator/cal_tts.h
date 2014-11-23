#ifndef __CAL_TTS_H__
#define __CAL_TTS_H__

//#include "tipvoice.h"

enum tts_type {

	TTS_ENTER_CAL = 0,		//tts report enter the calculator
	TTS_ELEMENT,			//tts report selectable element
	TTS_ERROR,				//tts report error calculation 
	TTS_NUMBER,				//tts report operandr
	TTS_ERRBEEP,			//tts report error beep
	TTS_NORBEEP,			//tts report a normal beep to inform a element has been inputed
	TTS_RESULT,				//tts report the calculation result
	TTS_CANCEL,
	TTS_PRENUM,				//tts report the previous numbers
	TTS_CLEARALL,			//tts report clear all the inputs
	TTS_NOINPUT,			//tts report there is no input num
	TTS_KEYLOCK,
	TTS_BACKCANCEL,
	TTS_TOPFIGURE,
	TTS_MAXINPUTVALUE,
	TTS_RESULTERROR,
	TTS_EXIT_CAL,
	TTS_NEWCALC,
	TTS_INPUTSET,
	TTS_INPUTSET_POINT,
};


void cal_tts (enum tts_type type, char* data);

void cal_tts_init (int* is_running);

void cal_tts_destroy (void);

void cal_tts_info (char* input_string);

int cal_get_exit_flag();


#endif
