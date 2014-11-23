/*
 *   ALSA command line mixer utility
 *   Copyright (c) 1999 by Jaroslav Kysela <perex@perex.cz>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __AMIXER_H
#define __AMIXER_H

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

#define PLEXTALK_PLAYBACK_VOL_MASTER		"Master"
#define PLEXTALK_PLAYBACK_VOL_DAC			"HiFi"
#define PLEXTALK_PLAYBACK_VOL_BYPASS		"Bypass"
#define PLEXTALK_CAPTURE_VOL				"Capture"

#define PLEXTALK_PLAYBACK_MUTE				"HP Mute"

#define PLEXTALK_OUTPUT_MUX					"Output Mux"
#define PLEXTALK_OUTPUT_BYPASS				"Bypass"
#define PLEXTALK_OUTPUT_DAC					"DAC"
#define PLEXTALK_OUTPUT_SIDETONE			"Sidetone1"

enum plextalk_output_select {
	PLEXTALK_OUTPUT_SELECT_SIDETONE = 0,
	PLEXTALK_OUTPUT_SELECT_BYPASS = 2,
	PLEXTALK_OUTPUT_SELECT_DAC = 3,
};

#define PLEXTALK_INPUT_MUX					"Input Mux"
#define PLEXTALK_INPUT_MIC					"Mic1"
#define PLEXTALK_INPUT_LINEIN				"Line In"

enum plextalk_input_select {
	PLEXTALK_INPUT_SELECT_MIC1 = 0,
	PLEXTALK_INPUT_SELECT_LINEIN = 2,
};

#define PLEXTALK_ADC_HIGH_PASS_FILTER		"ADC High Pass Filter"
#define PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND	"Analog Path Ignore Suspend"
#define PLEXTALK_SPEAKER					"Speaker"
#define PLEXTALK_HEADPHONE					"Headphone"
#define PLEXTALK_MIC_BOOST					"Mic1 Boost"
#define PLEXTALK_FM_RADIO					"FM Radio"	//ztz

#define PLEXTALK_AGC						"AGC"
#define PLEXTALK_AGC_ATTACK_TIME			"AGC Attack Time"
#define PLEXTALK_AGC_DECAY_TIME				"AGC Decay Time"
#define PLEXTALK_AGC_HOLD_TIME				"AGC Hold Time"
#define PLEXTALK_AGC_MAX					"AGC Max"
#define PLEXTALK_AGC_MIN					"AGC Min"
#define PLEXTALK_AGC_NG						"AGC NG"
#define PLEXTALK_AGC_NG_THRESHOLD			"AGC NG Threshold"
#define PLEXTALK_AGC_TARGET					"AGC Target"

#define PLEXTALK_CTRL_ON					"on"
#define PLEXTALK_CTRL_OFF					"off"

/*
 * if want to open deafult card, set @card as NULL
 */
snd_mixer_t *amixer_open(const char *card);
void amixer_close(snd_mixer_t *handle);

enum /* vol_type */ { VOL_RAW, VOL_DB, VOL_MAP };

int amixer_sset(snd_mixer_t *handle, int vol_type, unsigned int argc, char **argv);

int amixer_sset_str(snd_mixer_t *handle, int vol_type, char *id, char *val);
int amixer_sset_integer(snd_mixer_t *handle, int vol_type, char *id, int val);

int amixer_direct_sset_str(int vol_type, char *id, char *val);
int amixer_direct_sset_integer(int vol_type, char *id, int32_t val);

int amixer_get_playback_volume(snd_mixer_t *handle, const char *elem_name, int index,
						long *left, long *right);

int amixer_get_playback_switch(snd_mixer_t *handle, const char *name, int index);

int amixer_get_playback_enum(snd_mixer_t *handle, const char *name, int index);

#endif
