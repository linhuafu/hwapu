/* fmlib.c - simple V4L2 compatible tuner for radio cards

   Copyright (C) 2009 Ben Pfaff <blp@cs.stanford.edu>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along with
   this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FMLIB_H
#define FMLIB_H 1

#include <linux/videodev2.h>
#include <stdint.h>
#include <stdbool.h>

struct tuner {
	int fd;
	int index;
	struct v4l2_queryctrl volume_ctrl;
	struct v4l2_tuner tuner;
};

/*
 * if device is NULL, use "/dev/radio0" default
 */
int tuner_open(struct tuner *, const char *device, int index);
void tuner_close(struct tuner *);

int tuner_is_muted(const struct tuner *, bool *mute);
int tuner_set_mute(const struct tuner *, bool mute);

bool tuner_has_volume_control(const struct tuner *);
int tuner_get_volume(const struct tuner *, double *volume);
int tuner_set_volume(const struct tuner *, double volume);

long long int tuner_get_min_freq(const struct tuner *);
long long int tuner_get_max_freq(const struct tuner *);

/*
 * frequency set the frequency in MHz * 16000
 */
int tuner_set_freq(const struct tuner *, long long int frequency,
                    bool override_range);
long long int tuner_get_freq(const struct tuner *);
int tuner_get_signal(const struct tuner *, int32_t *signal);

/*
 * perform a hardware frequency seek [VIDIOC_S_HW_FREQ_SEEK]
 *  @dir is 0 (seek downward) or 1 (seek upward)
 *  @wrap is 0 (do not wrap around) or 1 (wrap around)
 *  @spacing sets the seek resolution (use 0 for default)
 *  @low and high set the low and high seek frequency range in KHz * 16 (use 0 for default)
 */
int tuner_hw_freq_seek(const struct tuner *, int dir,
                    int wrap, int spacing,
                    long long int rangelow, long long int rangehigh);

void tuner_sleep(const struct tuner *, int secs);
void tuner_usleep(const struct tuner *, int usecs);

#endif /* fmlib.h */
