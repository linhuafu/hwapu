#include "plextalk-constant.h"

// -2, 8
static const double plextalk_sound_speed_table[] = {
//	0.5, 0.75, 1, 1.2, 1.4, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1
	0.5, 0.75, 1, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0
};

const double * const plextalk_sound_speed = &plextalk_sound_speed_table[2];

// 0, 25
static const plextalk_volume_t plextalk_sound_volume_table[] = {
	{0,  0 , 0 },
	{0,  5 , 5 },
	{0,  10, 10},
	{0,  15, 15},
	{0,  20, 20},
	{0,  28, 28},
	{0,  31, 31},
	{3,  31, 31},
	{5,  31, 31},
	{7,  31, 31},
	{9,  31, 31},
	{11, 31, 31},
	{12, 31, 31},
	{13, 31, 31},
	{14, 31, 31},
	{16, 31, 31},
	{17, 31, 31},
	{19, 31, 31},
	{20, 31, 31},
	{21, 31, 31},
	{23, 31, 31},
	{24, 31, 31},
	{25, 31, 31},
	{27, 31, 31},
	{29, 31, 31},
	{31, 31, 31},
/*
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 23, 25, 27, 29, 31
*/
};

const plextalk_volume_t * const plextalk_sound_volume = &plextalk_sound_volume_table[0];

// -6, 6
static const double plextalk_sound_equalizer_table[][3] = {
	{6, 0, 0},
	{5, 0, 0},
	{4, 0, 0},
	{3, 0, 0},
	{2, 0, 0},
	{1, 0, 0},
	{0, 0, 0},
	{0, 0, 1},
	{0, 0, 2},
	{0, 0, 3},
	{0, 0, 4},
	{0, 0, 5},
	{0, 0, 6},
};

const double (* const plextalk_sound_equalizer)[3] = &plextalk_sound_equalizer_table[6];

// -5, 5
static const int plextalk_lcd_brightless_table[] = {
	1, 3, 8, 15, 23, 30, 37, 44, 50, 75, 100
};

const int * const plextalk_lcd_brightless = &plextalk_lcd_brightless_table[5];

const int plextalk_screensaver_timeout_table[] = {
	1 /* shortest we can */, 5, 15, 30, 1*60, 5*60, 0
};

const int * const plextalk_screensaver_timeout = &plextalk_screensaver_timeout_table[0];
