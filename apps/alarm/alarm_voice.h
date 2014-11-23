#ifndef __VOICE_H__
#define __VOICE_H__


struct AudioVoice;

typedef void (*AudioVoiceEndFunc)(void* ctx, void* data);

struct AudioVoice *audio_voice_create(void);

int audio_voice_play(struct AudioVoice *thiz, char *name);


int audio_voice_play_end(struct AudioVoice *thiz, 
	char *name, AudioVoiceEndFunc callback, void*ctx);

int audio_voice_isruning(struct AudioVoice *thiz);
void audio_voice_stop(struct AudioVoice *thiz);

/*
void audio_voice_set_speed(struct AudioVoice *thiz, float speed);
void audio_voice_set_band(struct AudioVoice *thiz, double *bands);
*/

void audio_voice_destroy(struct AudioVoice *thiz);

struct SysVoice;

struct SysVoice *sys_voice_create(void);

void sys_voice_start(struct SysVoice *thiz, char *fname);
void sys_voice_stop(struct SysVoice *thiz);
void sys_voice_destroy(struct SysVoice *thiz);

#endif
