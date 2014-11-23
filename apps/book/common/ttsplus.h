#ifndef __TTS_PLAY_H__
#define __TTS_PLAY_H__

#include "ivTTS.h"

struct TtsObj;

struct TtsObj *tts_obj_create(void);

void tts_obj_start(struct TtsObj *thiz, char *buff, 
		ivSize size, ivSize codepage);

int tts_obj_isruning(struct TtsObj *thiz);

void tts_obj_stop(struct TtsObj *thiz);
void tts_obj_pause (struct TtsObj* thiz);
void tts_obj_recover (struct TtsObj* thiz);

void tts_obj_set_speed(struct TtsObj *thiz, int speed);
void tts_obj_set_band(struct TtsObj *thiz, int band);

void tts_obj_destroy(struct TtsObj *thiz);

#endif	/* __VOICE_PROMPT_INTERNAL_H__ */
