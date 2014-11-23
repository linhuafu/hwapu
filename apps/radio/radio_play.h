#ifndef __RADIO_PLAY_H__
#define __RADIO_PLAY_H__
#include <stdbool.h>
#include "radio_type.h"

struct seek_freq{
	int orient;
	void(*callback) (void);			//useless
};

int radio_start (void);
int radio_set_freq (double freq);
double radio_get_freq (void);

int radio_is_muted (void);
int radio_set_mute (bool mute, int immediately);

int radio_manual_scan (int orient);
int radio_get_autoscan_state(void);
int radio_is_autoscan (void);

void radio_set_region();
double radio_get_fmMax();
double radio_get_fmMin();


enum {SCAN_FORWARD = 0, SCAN_BACKWARD,};

void radio_seek (void *arg);

#endif
