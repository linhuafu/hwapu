/*
 *   ALSA command line mixer utility
 *   Copyright (c) 1999-2000 by Jaroslav Kysela <perex@perex.cz>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <sys/poll.h>
#include <stdint.h>
#include "amixer.h"
#include "volume_mapping.h"

static void error(const char *fmt,...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "amixer: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

#define check_range(val, min, max) \
	((val < min) ? (min) : (val > max) ? (max) : (val))

/* Function to convert from percentage to volume. val = percentage */

#define convert_prange1(val, min, max) \
	ceil((val) * ((max) - (min)) * 0.01 + (min))

struct volume_ops {
	int (*get_range)(snd_mixer_elem_t *elem, long *min, long *max);
	int (*get)(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t c,
		   long *value);
	int (*set)(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t c,
		   long value, int dir);
};

struct volume_ops_set {
	int (*has_volume)(snd_mixer_elem_t *elem);
	struct volume_ops v[3];
};

static int set_playback_dB(snd_mixer_elem_t *elem,
			   snd_mixer_selem_channel_id_t c, long value, int dir)
{
	return snd_mixer_selem_set_playback_dB(elem, c, value, dir);
}

static int set_capture_dB(snd_mixer_elem_t *elem,
			  snd_mixer_selem_channel_id_t c, long value, int dir)
{
	return snd_mixer_selem_set_capture_dB(elem, c, value, dir);
}

static int set_playback_raw_volume(snd_mixer_elem_t *elem,
				   snd_mixer_selem_channel_id_t c,
				   long value, int dir)
{
	return snd_mixer_selem_set_playback_volume(elem, c, value);
}

static int set_capture_raw_volume(snd_mixer_elem_t *elem,
				  snd_mixer_selem_channel_id_t c,
				  long value, int dir)
{
	return snd_mixer_selem_set_capture_volume(elem, c, value);
}

/* FIXME: normalize to int32 space to be compatible with other types */
#define MAP_VOL_RES	(INT32_MAX / 100)

static int get_mapped_volume_range(snd_mixer_elem_t *elem,
				   long *pmin, long *pmax)
{
	*pmin = 0;
	*pmax = MAP_VOL_RES;
	return 0;
}

static int get_playback_mapped_volume(snd_mixer_elem_t *elem,
				      snd_mixer_selem_channel_id_t c,
				      long *value)
{
	*value = (rint)(get_normalized_playback_volume(elem, c) * MAP_VOL_RES);
	return 0;
}

static int set_playback_mapped_volume(snd_mixer_elem_t *elem,
				      snd_mixer_selem_channel_id_t c,
				      long value, int dir)
{
	return set_normalized_playback_volume(elem, c,
					      (double)value / MAP_VOL_RES, dir);
}

static int get_capture_mapped_volume(snd_mixer_elem_t *elem,
				     snd_mixer_selem_channel_id_t c,
				     long *value)
{
	*value = (rint)(get_normalized_capture_volume(elem, c) * MAP_VOL_RES);
	return 0;
}

static int set_capture_mapped_volume(snd_mixer_elem_t *elem,
				     snd_mixer_selem_channel_id_t c,
				     long value, int dir)
{
	return set_normalized_capture_volume(elem, c,
					     (double)value / MAP_VOL_RES, dir);
}

static const struct volume_ops_set vol_ops[2] = {
	{
		.has_volume = snd_mixer_selem_has_playback_volume,
		.v = {{ snd_mixer_selem_get_playback_volume_range,
			snd_mixer_selem_get_playback_volume,
			set_playback_raw_volume },
		      { snd_mixer_selem_get_playback_dB_range,
			snd_mixer_selem_get_playback_dB,
			set_playback_dB },
		      { get_mapped_volume_range,
			get_playback_mapped_volume,
			set_playback_mapped_volume },
		},
	},
	{
		.has_volume = snd_mixer_selem_has_capture_volume,
		.v = {{ snd_mixer_selem_get_capture_volume_range,
			snd_mixer_selem_get_capture_volume,
			set_capture_raw_volume },
		      { snd_mixer_selem_get_capture_dB_range,
			snd_mixer_selem_get_capture_dB,
			set_capture_dB },
		      { get_mapped_volume_range,
			get_capture_mapped_volume,
			set_capture_mapped_volume },
		},
	},
};

static int set_volume_simple(snd_mixer_elem_t *elem,
			     snd_mixer_selem_channel_id_t chn,
			     int vol_type, char **ptr, int dir)
{
	long val, orig, pmin, pmax;
	char *p = *ptr, *s;
	int invalid = 0, percent = 0, err = 0;
	double scale = 1.0;
	int correct = 0;

	if (! vol_ops[dir].has_volume(elem))
		invalid = 1;

	if (*p == ':')
		p++;
	if (*p == '\0' || (!isdigit(*p) && *p != '-'))
		goto skip;

	s = p;
	val = strtol(s, &p, 10);
	if (*p == '.') {
		p++;
		strtol(p, &p, 10);
	}
	if (*p == '%') {
		percent = 1;
		p++;
	} else if (p[0] == 'd' && p[1] == 'B') {
		vol_type = VOL_DB;
		p += 2;
		scale = 100;
	} else
		vol_type = VOL_RAW;

	val = (long)(strtod(s, NULL) * scale);
	if (vol_ops[dir].v[vol_type].get_range(elem, &pmin, &pmax) < 0)
		invalid = 1;
	if (percent)
		val = (long)convert_prange1(val, pmin, pmax);
	if (*p == '+' || *p == '-') {
		if (! invalid) {
			if (vol_ops[dir].v[vol_type].get(elem, chn, &orig) < 0)
				invalid = 1;
			if (*p == '+') {
				val = orig + val;
				correct = 1;
			} else {
				val = orig - val;
				correct = -1;
			}
		}
		p++;
	}
	if (! invalid) {
		val = check_range(val, pmin, pmax);
		err = vol_ops[dir].v[vol_type].set(elem, chn, val, correct);
	}
 skip:
	if (*p == ',')
		p++;
	*ptr = p;
	return err ? err : (invalid ? -ENOENT : 0);
}

static int get_bool_simple(char **ptr, char *str, int invert, int orig)
{
	if (**ptr == ':')
		(*ptr)++;
	if (!strncasecmp(*ptr, str, strlen(str))) {
		orig = 1 ^ (invert ? 1 : 0);
		while (**ptr != '\0' && **ptr != ',' && **ptr != ':')
			(*ptr)++;
	}
	if (**ptr == ',' || **ptr == ':')
		(*ptr)++;
	return orig;
}

static int simple_skip_word(char **ptr, char *str)
{
	char *xptr = *ptr;
	if (*xptr == ':')
		xptr++;
	if (!strncasecmp(xptr, str, strlen(str))) {
		while (*xptr != '\0' && *xptr != ',' && *xptr != ':')
			xptr++;
		if (*xptr == ',' || *xptr == ':')
			xptr++;
		*ptr = xptr;
		return 1;
	}
	return 0;
}

static int parse_simple_id(const char *str, snd_mixer_selem_id_t *sid)
{
	int c, size;
	char buf[128];
	char *ptr = buf;

	while (*str == ' ' || *str == '\t')
		str++;
	if (!(*str))
		return -EINVAL;
	size = 1;	/* for '\0' */
	if (*str != '"' && *str != '\'') {
		while (*str && *str != ',') {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
	} else {
		c = *str++;
		while (*str && *str != c) {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
		if (*str == c)
			str++;
	}
	if (*str == '\0') {
		snd_mixer_selem_id_set_index(sid, 0);
		*ptr = 0;
		goto _set;
	}
	if (*str != ',')
		return -EINVAL;
	*ptr = 0;	/* terminate the string */
	str++;
	if (!isdigit(*str))
		return -EINVAL;
	snd_mixer_selem_id_set_index(sid, atoi(str));
       _set:
	snd_mixer_selem_id_set_name(sid, buf);
	return 0;
}


typedef struct channel_mask {
	char *name;
	unsigned int mask;
} channel_mask_t;
static const channel_mask_t chanmask[] = {
	{"frontleft", 1 << SND_MIXER_SCHN_FRONT_LEFT},
	{"frontright", 1 << SND_MIXER_SCHN_FRONT_RIGHT},
	{"frontcenter", 1 << SND_MIXER_SCHN_FRONT_CENTER},
	{"front", ((1 << SND_MIXER_SCHN_FRONT_LEFT) |
		   (1 << SND_MIXER_SCHN_FRONT_RIGHT))},
	{"center", 1 << SND_MIXER_SCHN_FRONT_CENTER},
	{"rearleft", 1 << SND_MIXER_SCHN_REAR_LEFT},
	{"rearright", 1 << SND_MIXER_SCHN_REAR_RIGHT},
	{"rear", ((1 << SND_MIXER_SCHN_REAR_LEFT) |
		  (1 << SND_MIXER_SCHN_REAR_RIGHT))},
	{"woofer", 1 << SND_MIXER_SCHN_WOOFER},
	{NULL, 0}
};

static unsigned int channels_mask(char **arg, unsigned int def)
{
	const channel_mask_t *c;

	for (c = chanmask; c->name; c++) {
		if (strncasecmp(*arg, c->name, strlen(c->name)) == 0) {
			while (**arg != '\0' && **arg != ',' && **arg != ' ' && **arg != '\t')
				(*arg)++;
			if (**arg == ',' || **arg == ' ' || **arg == '\t')
				(*arg)++;
			return c->mask;
		}
	}
	return def;
}

static unsigned int dir_mask(char **arg, unsigned int def)
{
	int findend = 0;

	if (strncasecmp(*arg, "playback", 8) == 0)
		def = findend = 1;
	else if (strncasecmp(*arg, "capture", 8) == 0)
		def = findend = 2;
	if (findend) {
		while (**arg != '\0' && **arg != ',' && **arg != ' ' && **arg != '\t')
			(*arg)++;
		if (**arg == ',' || **arg == ' ' || **arg == '\t')
			(*arg)++;
	}
	return def;
}

static int get_enum_item_index(snd_mixer_elem_t *elem, char **ptrp)
{
	char *ptr = *ptrp;
	int items, i, len;
	char name[40];

	items = snd_mixer_selem_get_enum_items(elem);
	if (items <= 0)
		return -1;

	for (i = 0; i < items; i++) {
		if (snd_mixer_selem_get_enum_item_name(elem, i, sizeof(name)-1, name) < 0)
			continue;
		len = strlen(name);
		if (! strncmp(name, ptr, len)) {
			if (! ptr[len] || ptr[len] == ',' || ptr[len] == '\n') {
				ptr += len;
				*ptrp = ptr;
				return i;
			}
		}
	}
	return -1;
}

static int sset_enum(snd_mixer_elem_t *elem, unsigned int argc, char **argv)
{
	unsigned int idx, chn = 0;
	int check_flag = 0;

	for (idx = 1; idx < argc; idx++) {
		char *ptr = argv[idx];
		while (*ptr) {
			int ival = get_enum_item_index(elem, &ptr);
			if (ival < 0)
				return check_flag;
			if (snd_mixer_selem_set_enum_item(elem, chn, ival) >= 0)
				check_flag = 1;
			/* skip separators */
			while (*ptr == ',' || isspace(*ptr))
				ptr++;
		}
	}
	return check_flag;
}

static int sset_channels(snd_mixer_elem_t *elem, int vol_type, unsigned int argc, char **argv)
{
	unsigned int channels = ~0U;
	unsigned int dir = 3, okflag = 3;
	unsigned int idx;
	snd_mixer_selem_channel_id_t chn;
	int check_flag = 0;

	for (idx = 1; idx < argc; idx++) {
		char *ptr = argv[idx], *optr;
		int multi, firstchn = 1;
		channels = channels_mask(&ptr, channels);
		if (*ptr == '\0')
			continue;
		dir = dir_mask(&ptr, dir);
		if (*ptr == '\0')
			continue;
		multi = (strchr(ptr, ',') != NULL);
		optr = ptr;
		for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
			char *sptr = NULL;
			int ival;

			if (!(channels & (1 << chn)))
				continue;

			if ((dir & 1) && snd_mixer_selem_has_playback_channel(elem, chn)) {
				sptr = ptr;
				if (!strncmp(ptr, "mute", 4) && snd_mixer_selem_has_playback_switch(elem)) {
					snd_mixer_selem_get_playback_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_playback_switch(elem, chn, get_bool_simple(&ptr, "mute", 1, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "off", 3) && snd_mixer_selem_has_playback_switch(elem)) {
					snd_mixer_selem_get_playback_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_playback_switch(elem, chn, get_bool_simple(&ptr, "off", 1, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "unmute", 6) && snd_mixer_selem_has_playback_switch(elem)) {
					snd_mixer_selem_get_playback_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_playback_switch(elem, chn, get_bool_simple(&ptr, "unmute", 0, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "on", 2) && snd_mixer_selem_has_playback_switch(elem)) {
					snd_mixer_selem_get_playback_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_playback_switch(elem, chn, get_bool_simple(&ptr, "on", 0, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "toggle", 6) && snd_mixer_selem_has_playback_switch(elem)) {
					if (firstchn || !snd_mixer_selem_has_playback_switch_joined(elem)) {
						snd_mixer_selem_get_playback_switch(elem, chn, &ival);
						if (snd_mixer_selem_set_playback_switch(elem, chn, (ival ? 1 : 0) ^ 1) >= 0)
							check_flag = 1;
					}
					simple_skip_word(&ptr, "toggle");
				} else if (isdigit(*ptr) || *ptr == '-' || *ptr == '+') {
					if (set_volume_simple(elem, chn, vol_type, &ptr, 0) >= 0)
						check_flag = 1;
				} else if (simple_skip_word(&ptr, "cap") || simple_skip_word(&ptr, "rec") ||
					   simple_skip_word(&ptr, "nocap") || simple_skip_word(&ptr, "norec")) {
					/* nothing */
				} else {
					okflag &= ~1;
				}
			}
			if ((dir & 2) && snd_mixer_selem_has_capture_channel(elem, chn)) {
				if (sptr != NULL)
					ptr = sptr;
				sptr = ptr;
				if (!strncmp(ptr, "cap", 3) && snd_mixer_selem_has_capture_switch(elem)) {
					snd_mixer_selem_get_capture_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_capture_switch(elem, chn, get_bool_simple(&ptr, "cap", 0, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "rec", 3) && snd_mixer_selem_has_capture_switch(elem)) {
					snd_mixer_selem_get_capture_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_capture_switch(elem, chn, get_bool_simple(&ptr, "rec", 0, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "nocap", 5) && snd_mixer_selem_has_capture_switch(elem)) {
					snd_mixer_selem_get_capture_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_capture_switch(elem, chn, get_bool_simple(&ptr, "nocap", 1, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "norec", 5) && snd_mixer_selem_has_capture_switch(elem)) {
					snd_mixer_selem_get_capture_switch(elem, chn, &ival);
					if (snd_mixer_selem_set_capture_switch(elem, chn, get_bool_simple(&ptr, "norec", 1, ival)) >= 0)
						check_flag = 1;
				} else if (!strncmp(ptr, "toggle", 6) && snd_mixer_selem_has_capture_switch(elem)) {
					if (firstchn || !snd_mixer_selem_has_capture_switch_joined(elem)) {
						snd_mixer_selem_get_capture_switch(elem, chn, &ival);
						if (snd_mixer_selem_set_capture_switch(elem, chn, (ival ? 1 : 0) ^ 1) >= 0)
							check_flag = 1;
					}
					simple_skip_word(&ptr, "toggle");
				} else if (isdigit(*ptr) || *ptr == '-' || *ptr == '+') {
					if (set_volume_simple(elem, chn, vol_type, &ptr, 1) >= 0)
						check_flag = 1;
				} else if (simple_skip_word(&ptr, "mute") || simple_skip_word(&ptr, "off") ||
					   simple_skip_word(&ptr, "unmute") || simple_skip_word(&ptr, "on")) {
					/* nothing */
				} else {
					okflag &= ~2;
				}
			}
			if (okflag == 0) {
//				if (dir & 1)
//					error("Unknown playback setup '%s'..", ptr);
//				if (dir & 2)
//					error("Unknown capture setup '%s'..", ptr);
				return 0; /* just skip it */
			}
			if (!multi)
				ptr = optr;
			firstchn = 0;
		}
	}
	return check_flag;
}

snd_mixer_t *amixer_open(const char *card)
{
	snd_mixer_t *handle = NULL;
	int err = 0;

	err = snd_mixer_open(&handle, 0);
	if (err < 0) {
		error("Mixer %s open error: %s\n", card, snd_strerror(err));
		return NULL;
	}

	if (card == NULL)
		card = "default";

	err = snd_mixer_attach(handle, card);
	if (err < 0) {
		error("Mixer attach %s error: %s", card, snd_strerror(err));
		snd_mixer_close(handle);
		return NULL;
	}

	err = snd_mixer_selem_register(handle, NULL, NULL);
	if (err < 0) {
		error("Mixer register error: %s", snd_strerror(err));
		snd_mixer_close(handle);
		return NULL;
	}

	err = snd_mixer_load(handle);
	if (err < 0) {
		error("Mixer %s load error: %s", card, snd_strerror(err));
		snd_mixer_close(handle);
		return NULL;
	}

	return handle;
}

void amixer_close(snd_mixer_t *handle)
{
	if (handle != NULL)
		snd_mixer_close(handle);
}

int amixer_sset(snd_mixer_t *handle, int vol_type, unsigned int argc, char **argv)
{
	int err = 0;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);

	if (handle == NULL)
		return -1;

	if (parse_simple_id(argv[0], sid)) {
		fprintf(stderr, "Wrong scontrol identifier: %s\n", argv[0]);
		return -1;
	}

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		error("Unable to find simple control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		return -1;
	}

	if (snd_mixer_selem_is_enumerated(elem))
		err = sset_enum(elem, argc, argv);
	else
		err = sset_channels(elem, vol_type, argc, argv);
	if (err < 0) {
		error("Invalid command!");
		return -1;
	}

	return 0;
}

int amixer_sset_str(snd_mixer_t *handle, int vol_type, char *id, char *val)
{
	char *argv[2] = { id, val };
	return amixer_sset(handle, vol_type, 2, argv);
}

int amixer_sset_integer(snd_mixer_t *handle, int vol_type, char *id, int val)
{
	/* range of int32_t is from -2147483648 to 2147483647 */
	char val_buf[12];
	char *argv[2] = { id, val_buf };

	snprintf(val_buf, sizeof(val_buf), "%d", val);
	return amixer_sset(handle, vol_type, 2, argv);
}

int amixer_direct_sset_str(int vol_type, char *id, char *val)
{
	snd_mixer_t *handle;
	char *argv[2] = { id, val };
	int err;

	handle = amixer_open(NULL);
	if (handle == NULL)
		return -1;

	err = amixer_sset(handle, vol_type, 2, argv);

	amixer_close(handle);

	return err;
}

int amixer_direct_sset_integer(int vol_type, char *id, int32_t val)
{
	snd_mixer_t *handle;
	/* range of int32_t is from -2147483648 to 2147483647 */
	char val_buf[12];
	char *argv[2] = { id, val_buf };
	int err;

	handle = amixer_open(NULL);
	if (handle == NULL)
		return -1;

	snprintf(val_buf, sizeof(val_buf), "%d", val);
	err = amixer_sset(handle, vol_type, 2, argv);

	amixer_close(handle);

	return err;
}

int amixer_get_playback_volume(snd_mixer_t *handle, const char *name, int index,
						long *left, long *right)
{
	int err;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	if (handle == NULL)
		return -1;

	//allocate simple id
	snd_mixer_selem_id_alloca(&sid);

	//sets simple-mixer index and name
	snd_mixer_selem_id_set_index(sid, index);
	snd_mixer_selem_id_set_name(sid, name);

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		error("Unable to fine simple control %s:%d",
		       snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		return -1;
	}

	if (left)
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, left);
	if (right)
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, right);

	return 0;
}

int amixer_get_playback_switch(snd_mixer_t *handle, const char *name, int index)
{
	int err;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	int psw;

	if (handle == NULL)
		return -1;

	//allocate simple id
	snd_mixer_selem_id_alloca(&sid);

	//sets simple-mixer index and name
	snd_mixer_selem_id_set_index(sid, index);
	snd_mixer_selem_id_set_name(sid, name);

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		error("Unable to fine simple control %s:%d",
		       snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		return -1;
	}

	if (!snd_mixer_selem_has_common_switch(elem) &&
		!snd_mixer_selem_has_playback_switch(elem))
		return -1;

	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &psw);

	return psw;
}

int amixer_get_playback_enum(snd_mixer_t *handle, const char *name, int index)
{
	int err;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	int i, idx;

	if (handle == NULL)
		return -1;

	//allocate simple id
	snd_mixer_selem_id_alloca(&sid);

	//sets simple-mixer index and name
	snd_mixer_selem_id_set_index(sid, index);
	snd_mixer_selem_id_set_name(sid, name);

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		error("Unable to fine simple control %s:%d",
		       snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		return -1;
	}

	if (!snd_mixer_selem_is_enumerated(elem))
		return -1;

	for (i = 0; !snd_mixer_selem_get_enum_item(elem, i, &idx); i++);

	return idx;
}
