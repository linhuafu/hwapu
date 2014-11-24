/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Main App routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
#include <signal.h>

#include <nano-X.h>

#define OSD_DBG_MSG
#include "nc-err.h"

#include "amixer.h"
#include "application.h"
#include "neux.h"
#include "widgets.h"
#include "desktop-ipc.h"
#include "application-ipc.h"
#include "plextalk-i18n.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "sysfs-helper.h"
#include "nw-statusbar.h"
#include "nw-titlebar.h"
#include "nw-main.h"
#include "plextalk-keys.h"
#include "nxutils.h"
#include "plextalk-constant.h"
#include "volume.h"

static int hp_online;
static int book_is_audio;
static int file_list_mode;
int info_running;
int recording_active;
int recording_now;//appk
int drycell_supply;//appk
extern int vprompt_change_state;	// ztz
extern int specical_for_info;		// ztz

struct plextalk_radio radio_cache;
struct plextalk_volume volume_cache;

static int curr_volume_offset;

#define CURR_VOLUME (*(int*)((void*)(&volume_cache) + curr_volume_offset))

static inline int OutputIsHeadphone(int app)
{
	switch (app) {
	case M_RADIO:
		return hp_online && radio_cache.output;
	case M_ALARM:
		if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_RADIO))
			return hp_online && radio_cache.output;
		if (!NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_RECORD))
			return hp_online;
		/* fall through */
	case M_RECORD:
		return hp_online || /*recording_active*/recording_now;//appk
	default:
		return hp_online;
	}
}

static void
UpdateVolumeIcon(void)
{
	static int last_icon = SBAR_VOLUME_ICON_NO;
	int icon = (CURR_VOLUME +3) / 4 + 1;//modify lhf

	if (last_icon != icon) {
		StatusBarSetIcon(STATUSBAR_VOLUME_INDEX, icon);
		last_icon = icon;
	}
}

static int
UpdateVolumeOffset(int app)
{
	int volume_offset = curr_volume_offset;

	if (!file_list_mode && !info_running) {
		switch (app) {
		case M_BOOK:
			if (book_is_audio)
				curr_volume_offset = offsetof(struct plextalk_volume, book_audio);
			else
				curr_volume_offset = offsetof(struct plextalk_volume, book_text);
			if (hp_online)
				curr_volume_offset += sizeof(int);
			break;
		case M_MUSIC:
			curr_volume_offset = offsetof(struct plextalk_volume, music);
			if (hp_online)
				curr_volume_offset += sizeof(int);
			break;
		case M_RADIO:
			curr_volume_offset = offsetof(struct plextalk_volume, radio);
			if (hp_online && radio_cache.output)
				curr_volume_offset += sizeof(int);
			break;
		case M_RECORD:
			curr_volume_offset = offsetof(struct plextalk_volume, record);
			break;
		default:
			curr_volume_offset = offsetof(struct plextalk_volume, guide);
			break;
		}
	} else
		curr_volume_offset = offsetof(struct plextalk_volume, guide);

	return (volume_offset != curr_volume_offset);
}

#if 0
#define UpdateHWVolume(last_vol, curr_vol, channel, member) \
	if (/*plextalk_sound_volume[last_vol].member != \
		plextalk_sound_volume[curr_vol].member*/1) { \
		amixer_sset_integer(mixer, VOL_RAW, channel, \
				plextalk_sound_volume[curr_vol].member>>(drycell_supply?1:0)); \
		}//appk
#endif

/* Adjust volume from 1/2 --> 3/4 */

#define UpdateHWVolume(last_vol, curr_vol, channel, member) do { \
	unsigned int set_vol; \
	if (drycell_supply) \
		set_vol = plextalk_sound_volume[curr_vol].member * 3 / 4; \
	else \
		set_vol = plextalk_sound_volume[curr_vol].member; \
	amixer_sset_integer(mixer, VOL_RAW, channel, set_vol); \
} while (0)


static void
SetVolume(int app, int set_always)
{
	snd_mixer_t *mixer;
	int last_vol, curr_vol;

	last_vol = CURR_VOLUME;
	if (!UpdateVolumeOffset(app) && !set_always)
		/*return*/;
	curr_vol = CURR_VOLUME;

	mixer = amixer_open(NULL);

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);
	if (OutputIsHeadphone(app)) {
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_ON);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_OFF);
	} else {
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
	}

	if (curr_vol > 0) {
		UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_MASTER, master);
		if (app == M_RADIO) {
			UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_BYPASS, bypass);
		} else {
			UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_DAC, DAC);
		}
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);
	}

	amixer_close(mixer);

	UpdateVolumeIcon();
}

void
AppChangeVolumeHook(int app, int is_new)
{
	if (is_new) {
		if (app == M_BOOK || app == M_MUSIC || app == M_RADIO || app == M_RECORD) {
			file_list_mode = 0;
			book_is_audio = 0;
			recording_active = 0;
			recording_now = 0;//appk
		}
		info_running = 0;
	}

	SetVolume(app, 0);
}

void
InitVolume(void)
{
	CoolShmReadLock(g_config_lock);
	radio_cache = g_config->setting.radio;
	volume_cache = g_config->volume;
	CoolShmReadUnlock(g_config_lock);

	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);

	SetVolume(M_DESKTOP, 1);

	amixer_direct_sset_integer(VOL_RAW, PLEXTALK_AGC_TARGET, 15);//AGC TARGET -6dB
}

int
HandleVolumeKey(int up)
{
	snd_mixer_t *mixer;
	int last_vol, curr_vol;
	int change;

	last_vol = CURR_VOLUME;
	if (up) {
		change = (last_vol < PLEXTALK_SOUND_VOLUME_MAX);
		if (change)
			curr_vol = last_vol + 1;
	} else {
		//change = (last_vol > (curr_volume_offset == offsetof(struct plextalk_volume, guide) ? 1 : 0));
		change = last_vol > 1;
		if (change)
			curr_vol = last_vol - 1;
	}

	if (change) {
		mixer = amixer_open(NULL);

		if (curr_vol > 0) {
			UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_MASTER, master);
			if (active_app == M_RADIO) {
				UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_BYPASS, bypass);
			} else {
				UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_DAC, DAC);
			}
		} else
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);

		if (last_vol == 0)
			amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);

		amixer_close(mixer);

		CURR_VOLUME = curr_vol;
		UpdateVolumeIcon();

		CoolShmWriteLock(g_config_lock);
		*(int*)((void*)(&g_config->volume) + curr_volume_offset) = curr_vol;
		CoolShmWriteUnlock(g_config_lock);

//		if (curr_volume_offset == offsetof(struct plextalk_volume, guide)) {
//			NhelperGuideVolumeChange(curr_vol);
//			vprompt_cfg.volume = curr_vol;
//		}

		plextalk_volume_write_xml(PLEXTALK_SETTING_DIR "volume.xml");
	} else
		curr_vol = last_vol;

	if (curr_vol > 0 && active_app != M_HELP) {
		if (active_app != M_DESKTOP && active_app != M_BOOK && active_app != M_MUSIC)
			NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
		else {
			if (info_running) {
				specical_for_info = 1;
				NhelperSetAppStateTwo(FUNC_MAIN_STR[active_app], 1, 1);
			}
		}
		voice_prompt_abort();
		if (active_app == M_BOOK || active_app == M_MUSIC) {
			int do_vp;

			CoolShmReadLock(g_config_lock);
			do_vp = !g_config->volume_vprompt_disable;
			CoolShmReadUnlock(g_config_lock);

//Ztz -start
#if 0
			if (do_vp || info_running || file_list_mode) {
				if (change)
					voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
				else
					voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
				last_msgid = voice_prompt_printf(1, &vprompt_cfg,
								ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
								_("Volume %d"), curr_vol);
#endif
			if (do_vp || info_running || file_list_mode) {

					if (change)
						voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
					else
						voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
					last_msgid = voice_prompt_printf(1, &vprompt_cfg,
									ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
									_("Volume %d"), curr_vol);
					if (info_running)
						vprompt_change_state = 1;
//ztz	-end
			} else {
				if (change)
					last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_PU);
				else
					last_msgid = voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_KON);
			}
		} else {
			if (change)
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			else
				voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_KON);
			last_msgid = voice_prompt_printf(1, &vprompt_cfg,
									ivTTS_CODEPAGE_UTF8, tts_lang, tts_role,
									_("Volume %d"), curr_vol);
			vprompt_change_state = 1;		//ztz
		}
	}

	return curr_vol > 0;
}

void
HpSwitch(int online)
{
	snd_mixer_t *mixer;
	int last_vol, curr_vol;

//	if (hp_online == online)
//		return;

	hp_online = online;
	last_vol = CURR_VOLUME;
	if (!UpdateVolumeOffset(active_app))
		/*return*/;
	curr_vol = CURR_VOLUME;

	mixer = amixer_open(NULL);

	amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);

	if (OutputIsHeadphone(active_app)) {
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_ON);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_OFF);
	} else {
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_SPEAKER, PLEXTALK_CTRL_ON);
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_HEADPHONE, PLEXTALK_CTRL_OFF);
	}

	if (curr_vol > 0) {
		UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_MASTER, master);
		if (active_app == M_RADIO) {
			UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_BYPASS, bypass);
		} else {
			UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_DAC, DAC);
		}
		amixer_sset_str(mixer, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);
	}

	amixer_close(mixer);

	UpdateVolumeIcon();
}

void
HandleFilelistModeMsg(int active)
{
	file_list_mode = active;

//	if (file_list_mode)
//		SetVolume(M_DESKTOP, 0);
//	else
		SetVolume(active_app, 0);
}

void
HandleBookContentMsg(int is_audio)
{
	book_is_audio = is_audio;

//	if (active_app == M_BOOK)
		SetVolume(M_BOOK, 0);
}

void
HandleRecordingMsg(int active)
{
	recording_active = active;

	if (active_app == M_RECORD)
		SetVolume(M_RECORD, 0);
}

//appk start
void
HandleRecordingNowMsg(int active)
{
	recording_now = active;

	if (active_app == M_RECORD)
		SetVolume(M_RECORD, 0);
}
//appk end


void
HandleGuideVolChangeMsg(int curr_vol)
{
	volume_cache.guide = curr_vol;
	vprompt_cfg.volume = curr_vol;

	if (curr_volume_offset == offsetof(struct plextalk_volume, guide)) {
		snd_mixer_t *mixer;
		int last_vol;

		last_vol = CURR_VOLUME;
		CURR_VOLUME = curr_vol;

		mixer = amixer_open(NULL);
		UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_MASTER, master);
		UpdateHWVolume(last_vol, curr_vol, PLEXTALK_PLAYBACK_VOL_DAC, DAC);
		amixer_close(mixer);

		UpdateVolumeIcon();
	}

	plextalk_volume_write_xml(PLEXTALK_SETTING_DIR "volume.xml");
}

void
HandleRadioOutputMsg(int hp)
{
	radio_cache.output = hp;

	if (active_app == M_RADIO)
		SetVolume(M_RADIO, 0);
}

void
HandleuInfoMsg(int active)
{
	info_running = active;

	SetVolume(active_app, 0);
}
int 
GetInfoStat(void)
{
	return !!info_running;
}

void
SetInfoStat(int state) 
{
	info_running = state;
}
