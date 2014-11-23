#ifndef __PLEXTALK_CONSTANT_H__
#define __PLEXTALK_CONSTANT_H__

#define PLEXTALK_SOUND_SPEED_MIN -2
#define PLEXTALK_SOUND_SPEED_MAX 8
extern const double * const plextalk_sound_speed;

typedef struct {
	unsigned int master;
	unsigned int DAC;
	unsigned int bypass;
} plextalk_volume_t;

#define PLEXTALK_SOUND_VOLUME_MIN 0
#define PLEXTALK_SOUND_VOLUME_MAX 25
extern const plextalk_volume_t * const plextalk_sound_volume;

#define PLEXTALK_SOUND_EQUALIZER_MIN -6
#define PLEXTALK_SOUND_EQUALIZER_MAX 6
extern const double (* const plextalk_sound_equalizer)[3];

#define PLEXTALK_LCD_BRIGHTLESS_MIN -5
#define PLEXTALK_LCD_BRIGHTLESS_MAX 5
extern const int * const plextalk_lcd_brightless;

extern const int * const plextalk_screensaver_timeout;

#endif
