#ifndef __JUMP_PAGE_TTS_H__
#define __JUMP_PAGE_TTS_H__

//#include "tipvoice.h"

enum tts_type {

	TTS_ENTER_JUMP_PAGE = 0,		//tts report enter the calculator
	TTS_ELEMENT,			//tts report selectable element
	TTS_ERROR,				//tts report error calculation 
	TTS_NUMBER,				//tts report operandr
	TTS_ERRBEEP,			//tts report error beep
	TTS_NORBEEP,			//tts report a normal beep to inform a element has been inputed
	TTS_CANCEL,
	TTS_CLEARALL,			//tts report clear all the inputs
	TTS_NOINPUT,			//tts report there is no input num
	TTS_KEYLOCK,
	TTS_BACKCANCEL,
	TTS_TOPFIGURE,
	TTS_MAXINPUTVALUE,
	TTS_EXIT_JUMP_PAGE,
	TTS_INPUTSET,
	TTS_FAILED_MOVE_PAGE,
	TTS_THERE_NO_PAGE,
	TTS_PLEASE_WAIT,
};


void page_tts (enum tts_type type, char* data);

void page_tts_init (int* is_running);

void page_tts_destroy (void);

void page_tts_info (char* input_string);

int page_get_exit_flag();


#endif
