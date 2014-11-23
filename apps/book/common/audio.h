#ifndef __AUDIO_H__
#define __AUDIO_H__



struct BookAudio;

#define INVALID_TIME 0xffffffff


struct BookAudio *book_audio_create(void);
void book_audio_destroy(struct BookAudio *thiz);
int book_audio_seek (struct BookAudio *thiz, 
	unsigned long beg_time, unsigned long end_time);
void book_audio_play(struct BookAudio *thiz, 
	const char *fname, unsigned long beg_time,
	unsigned long end_time);
int book_audio_pause(struct BookAudio *thiz);
int book_get_pause(struct BookAudio *thiz);
unsigned long book_audio_get_time(struct BookAudio *thiz);
void book_audio_stop(struct BookAudio *thiz);
int book_audio_isruning(struct BookAudio *thiz);
void book_audio_set_band(struct BookAudio *thiz, int band);
void book_audio_set_speed(struct BookAudio *thiz, int speed);
#endif