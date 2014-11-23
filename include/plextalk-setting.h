#ifndef __SYS_SETTING_H__
#define __SYS_SETTING_H__

#include "plextalk-i18n.h"

struct plextalk_book {
	int speed;
	int repeat;
	int audio_equalizer;
	int text_equalizer;
};

struct plextalk_music {
	int speed;
	int repeat;
	int equalizer;
};

struct plextalk_radio {
	int output;		// 0: speaker; 1: headphone
};

struct plextalk_record {
	int saveto;		// 0: internal memory; 1: SD card
};

struct plextalk_guide {
	int speed;
};

struct plextalk_timer {
	int enabled;
	int function;
#define PLEXTALK_TIMER_ALARM			0
#define PLEXTALK_TIMER_SLEEP			1
	int type;
#define PLEXTALK_TIMER_CLOCKTIME		0
#define PLEXTALK_TIMER_COUNTDOWN		1
	int elapse;
#define PLEXTALK_SLEEP_3MIN				0
#define PLEXTALK_SLEEP_5MIN				1
#define PLEXTALK_SLEEP_15MIN			2
#define PLEXTALK_SLEEP_30MIN			3
#define PLEXTALK_SLEEP_45MIN			4
#define PLEXTALK_SLEEP_60MIN			5
#define PLEXTALK_SLEEP_90MIN			6
#define PLEXTALK_SLEEP_120MIN			7
	int hour;
	int minute;
	/* alarm timer */
	int sound;
#define PLEXTALK_ALARM_SOUND_A			0
#define PLEXTALK_ALARM_SOUND_B			1
#define PLEXTALK_ALARM_SOUND_C			2
	int repeat;
#define PLEXTALK_ALARM_REPEAT_ONCE		0
#define PLEXTALK_ALARM_REPEAT_DAILY		1
#define PLEXTALK_ALARM_REPEAT_WEEKLY	2
	int weekday;
};

struct plextalk_lcd {
	int screensaver;
#define	PLEXTALK_SCREENSAVER_ACTIVENOW	0
#define	PLEXTALK_SCREENSAVER_5SEC		1
#define	PLEXTALK_SCREENSAVER_15SEC		2
#define	PLEXTALK_SCREENSAVER_30SEC		3
#define	PLEXTALK_SCREENSAVER_1MIN		4
#define	PLEXTALK_SCREENSAVER_5MIN		5
#define	PLEXTALK_SCREENSAVER_DEACTIVE	6
	int fontsize;		// 12, 16, or 24
	int theme;
	int backlight;
};

struct plextalk_language {
	char lang[PLEXTALK_LANG_MAX];
};

struct plextalk_tts {
	int voice_species;
#define PLEXTALK_TTS_MALE				0
#define PLEXTALK_TTS_FEMALE				1
	int chinese;
#define PLEXTALK_TTS_STANDARD_CHINESE	0
#define PLEXTALK_TTS_CONTONESE			1
};

struct plextalk_volume {
	int book_audio[2];
	int book_text[2];
	int music[2];
	int radio[2];
	int record;	// monitor's volume, only headphone
	int guide;	// indiscriminate between headphone and speaker
};

struct plextalk_setting {
	struct plextalk_book book;
	struct plextalk_music music;
	struct plextalk_radio radio;
	struct plextalk_record record;
	struct plextalk_guide guide;
	struct plextalk_timer timer[3];//appk 2->3
	struct plextalk_lcd lcd;
	struct plextalk_language lang;
	struct plextalk_tts tts;
};

int plextalk_setting_read_xml(const char *xml);
int plextalk_setting_write_xml(const char *xml);

int plextalk_volume_read_xml(const char *xml);
int plextalk_volume_write_xml(const char *xml);

#endif
