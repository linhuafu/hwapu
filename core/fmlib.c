/* fmlib.c - simple V4L2 compatible tuner for radio cards

   Copyright (C) 2009, 2012 Ben Pfaff <blp@cs.stanford.edu>

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

#include "fmlib.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int query_control(const struct tuner *, uint32_t id, struct v4l2_queryctrl *);
static int get_control(const struct tuner *, uint32_t id, int32_t *value);
static int set_control(const struct tuner *, uint32_t id, int32_t value);
static int query_tuner(const struct tuner *, struct v4l2_tuner *);

int
tuner_open(struct tuner *tuner, const char *device, int index)
{
	int err;

	memset(tuner, 0, sizeof *tuner);

	if (!device)
		device = "/dev/radio0";

	tuner->fd = open(device, O_RDONLY);
	if (tuner->fd < 0) {
		perror("open");
		return -1;
	}

	tuner->index = index;

	err = query_control(tuner, V4L2_CID_AUDIO_VOLUME, &tuner->volume_ctrl);
	if (err < 0)
		goto fail;

	err = query_tuner(tuner, &tuner->tuner);
	if (err < 0)
		goto fail;

	return 0;

fail:
	close(tuner->fd);
	memset(tuner, 0, sizeof *tuner);
	return err;
}

int tuner_get_freq (struct tuner* tuner)
{

	struct v4l2_frequency vf;

	vf.tuner = tuner->index;
	vf.type = tuner->tuner.type;
	
	ioctl(tuner->fd, VIDIOC_G_FREQUENCY, &vf);

	return vf.frequency;
}
void tuner_close(struct tuner *tuner)
{
	if (tuner->fd > 0)
		close(tuner->fd);
	memset(tuner, 0, sizeof *tuner);
}

int
tuner_is_muted(const struct tuner *tuner, bool *mute)
{
	int32_t imute;

	if (get_control(tuner, V4L2_CID_AUDIO_MUTE, &imute) < 0)
		return -1;

	*mute = imute ? true : false;
	return 0;
}

int
tuner_set_mute(const struct tuner *tuner, bool mute)
{
	return set_control(tuner, V4L2_CID_AUDIO_MUTE, mute);
}

bool
tuner_has_volume_control(const struct tuner *tuner)
{
	const struct v4l2_queryctrl *vqc = &tuner->volume_ctrl;
	return vqc->maximum > vqc->minimum;
}

int
tuner_get_volume(const struct tuner *tuner, double *volume)
{
	const struct v4l2_queryctrl *vqc = &tuner->volume_ctrl;
	int32_t ivolume;

	if (!tuner_has_volume_control(tuner)) {
		*volume = 100.0;
		return 0;
	}

	if (get_control(tuner, V4L2_CID_AUDIO_VOLUME, &ivolume) < 0)
		return -1;

	*volume = (100.0 * (ivolume - vqc->minimum) / (vqc->maximum - vqc->minimum));
	return 0;
}

int
tuner_set_volume(const struct tuner *tuner, double volume)
{
	struct v4l2_queryctrl *vqc = &tuner->volume_ctrl;

	if (!tuner_has_volume_control(tuner))
		return 0;

	return set_control(tuner, V4L2_CID_AUDIO_VOLUME,
                       (volume / 100.0 * (vqc->maximum - vqc->minimum) + vqc->minimum));
}

long long int
tuner_get_min_freq(const struct tuner *tuner)
{
	long long int rangelow = tuner->tuner.rangelow;
	if (!(tuner->tuner.capability & V4L2_TUNER_CAP_LOW))
		rangelow *= 1000;
	return rangelow;
}

long long int
tuner_get_max_freq(const struct tuner *tuner)
{
	long long int rangehigh = tuner->tuner.rangehigh;
	if (!(tuner->tuner.capability & V4L2_TUNER_CAP_LOW))
		rangehigh *= 1000;
	return rangehigh;
}

int
tuner_set_freq(const struct tuner *tuner, long long int freq,
               bool override_range)
{
	long long int adj_freq;
	struct v4l2_frequency vf;

	adj_freq = freq;
	if (!(tuner->tuner.capability & V4L2_TUNER_CAP_LOW))
		adj_freq = (adj_freq + 500) / 1000;

	if ((adj_freq < tuner->tuner.rangelow
             || adj_freq > tuner->tuner.rangehigh)
            && !override_range) {
		fprintf(stderr, "Frequency %.1f MHz out of range (%.1f - %.1f MHz)",
                freq / 16000.0,
                tuner_get_min_freq(tuner) / 16000.0,
                tuner_get_max_freq(tuner) / 16000.0);
        return -1;
	}

	memset(&vf, 0, sizeof vf);
	vf.tuner = tuner->index;
	vf.type = tuner->tuner.type;
	vf.frequency = adj_freq;

	if (ioctl(tuner->fd, VIDIOC_S_FREQUENCY, &vf) == -1) {
		perror("VIDIOC_S_FREQUENCY");
        return -1;
    }

	return 0;
}

int
tuner_hw_freq_seek(const struct tuner *tuner,
                   int dir, int wrap, int spacing,
                   long long int rangelow, long long int rangehigh)
{
	struct v4l2_hw_freq_seek hwseek;

	memset(&hwseek, 0, sizeof hwseek);
	hwseek.tuner = tuner->index;
	hwseek.type = tuner->tuner.type;
	hwseek.seek_upward = !!dir;
	hwseek.wrap_around = !!wrap;
    hwseek.spacing = spacing;
    if (!(tuner->tuner.capability & V4L2_TUNER_CAP_LOW)) {
    	hwseek.rangelow = rangelow / 1000;
    	hwseek.rangehigh = rangehigh / 1000;
    } else {
		hwseek.rangelow = rangelow;
    	hwseek.rangehigh = rangehigh;
	}

	if (ioctl(tuner->fd, VIDIOC_S_HW_FREQ_SEEK, &hwseek) == -1) {
		perror("VIDIOC_S_HW_FREQ_SEEK");
        return -1;
    }

	return 0;
}

int tuner_get_signal(const struct tuner *tuner, int32_t *signal)
{
	struct v4l2_tuner vt;

	if (query_tuner(tuner, &vt) < 0)
		return -1;

	*signal = vt.signal;
    return 0;
}

void tuner_usleep(const struct tuner *tuner, int usecs)
{
	usleep(usecs);
}

void tuner_sleep(const struct tuner *tuner, int secs)
{
	sleep(secs);
}

/* Queries 'tuner' for a property with the given 'id' and fills in 'qc' with
 * the result.  If 'tuner' doesn't have such a property, clears 'qc' to
 * all-zeros. */
static int query_control(const struct tuner *tuner, uint32_t id,
                         struct v4l2_queryctrl *qc)
{
	memset(qc, 0, sizeof *qc);
	qc->id = id;

	if (ioctl(tuner->fd, VIDIOC_QUERYCTRL, qc) != -1) {
		/* Success. */
	} else if (errno == ENOTTY || errno == EINVAL) {
		/* This tuner doesn't support either query operation
                 * or the specific control ('id') respectively */
		memset(qc, 0, sizeof *qc);
	} else {
		perror("VIDIOC_QUERYCTRL");
		return -1;
	}

	return 0;
}

static int get_control(const struct tuner *tuner, uint32_t id, int32_t *value)
{
	struct v4l2_control control;

	memset(&control, 0, sizeof control);
	control.id = id;

	if (ioctl(tuner->fd, VIDIOC_G_CTRL, &control) == -1) {
		perror("VIDIOC_G_CTRL");
		return -1;
	}

	*value = control.value;
	return 0;
}

static int set_control(const struct tuner *tuner, uint32_t id, int32_t value)
{
	struct v4l2_control control;

	memset(&control, 0, sizeof control);
	control.id = id;
	control.value = value;

	if (ioctl(tuner->fd, VIDIOC_S_CTRL, &control) == -1) {
		perror("VIDIOC_S_CTRL");
		return -1;
	}

	return 0;
}

static int query_tuner(const struct tuner *tuner, struct v4l2_tuner *vt)
{
	memset(vt, 0, sizeof *vt);
	vt->index = tuner->index;

	if (ioctl(tuner->fd, VIDIOC_G_TUNER, vt) == -1) {
		perror("VIDIOC_G_TUNER");
		return -1;
	}

	return 0;
}
