#include <stdio.h>
#include <signal.h>
#include <nano-X.h>

#define OSD_DBG_MSG
#include "nc-err.h"
#include "amixer.h"
#include "application.h"
#include "sysfs-helper.h"
#include "plextalk-keys.h"
#include "plextalk-constant.h"
#include "plextalk-config.h"

static int isOnlyDryCellPower(void)
{ 
	int backup_batt_present = 0;
	int vbus_present = 0;

	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

	if (backup_batt_present && !vbus_present)
		return 1;
	else
		return 0;
}


#define set_volume_member(mixer, dry_cell, curr_vol, channel, member) do { \
	unsigned int set_vol; \
	if (dry_cell) \
		set_vol = plextalk_sound_volume[curr_vol].member * 3 / 4; \
	else \
		set_vol = plextalk_sound_volume[curr_vol].member; \
	amixer_sset_integer(mixer, VOL_RAW, channel, set_vol); \
} while (0)


void app_volume_ctrl (const char* app)
{
	if (!app) 
		return;

	int app_vol;
	int dry_cell;
	int hp_on;
	int sw_hp = 1;
	unsigned int set_vol;
	snd_mixer_t *mixer;

	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_on);
	dry_cell = isOnlyDryCellPower();

	CoolShmReadLock(g_config_lock);
	if (!strcmp(app, "book_audio"))
		app_vol = g_config->volume.book_audio[hp_on];
 	else if (!strcmp(app, "book_text"))
		app_vol = g_config->volume.book_text[hp_on];
	else if (!strcmp(app, "music"))
		app_vol = g_config->volume.music[hp_on];
	else if (!strcmp(app, "radio")) {
		app_vol = g_config->volume.music[hp_on];
		sw_hp = 0;
	} else if (!strcmp(app, "record"))
		app_vol = g_config->volume.record;
	else if (!strcmp(app, "guide"))
		app_vol = g_config->volume.guide;
	else {
		printf("APP_VOLUME_CTRL ERROR!\n");
		return ;
	}
	CoolShmReadUnlock(g_config_lock);

	mixer = amixer_open(NULL);

	if (sw_hp) {
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, 
			hp_on ? PLEXTALK_CTRL_ON : PLEXTALK_CTRL_OFF);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, 
			hp_on ? PLEXTALK_CTRL_OFF : PLEXTALK_CTRL_ON);
	}

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);

	set_volume_member(mixer, dry_cell, app_vol, PLEXTALK_PLAYBACK_VOL_MASTER, master);
	set_volume_member(mixer, dry_cell, app_vol, PLEXTALK_PLAYBACK_VOL_DAC, DAC);
	set_volume_member(mixer, dry_cell, app_vol, PLEXTALK_PLAYBACK_VOL_BYPASS, bypass);

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);
	
	amixer_close(mixer);	
}
