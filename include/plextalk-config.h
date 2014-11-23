#ifndef __PLEXTALK_CONFIG_H__
#define __PLEXTALK_CONFIG_H__

#include <limits.h>
#include "plextalk-setting.h"
#include "shared-mem.h"

#define PLEXTALK_TOP_DIR			"/opt/plextalk/"
#define PLEXTALK_BINS_DIR			PLEXTALK_TOP_DIR "bin/"
#define PLEXTALK_DATA_DIR			PLEXTALK_TOP_DIR "data/"

#define PLEXTALK_CONFIG_DIR			PLEXTALK_DATA_DIR "config/"
#define PLEXTALK_EQU_PRESETS_DIR	PLEXTALK_DATA_DIR "equ-presets/"
#define PLEXTALK_IMAGES_DIR			PLEXTALK_DATA_DIR "images/"
#define PLEXTALK_VPROMPT_DIR		PLEXTALK_DATA_DIR "sounds/"
#define PLEXTALK_THEMES_DIR			PLEXTALK_DATA_DIR "themes/"
#define PLEXTALK_I18N_DIR			PLEXTALK_DATA_DIR "i18n/"

#define PLEXTALK_MOUNT_ROOT_STR		"/media"

/*
 * we will put setting.xml, volume.xml, channels.xml, app.history,
 * book.history, music.history and radio.history in this directory
 */
#define PLEXTALK_SETTING_DIR		"/opt/plextalk/usr_setting/"

#define PLEXTALK_GLOBAL_CONFIG_SHM_ID	"global-config"

#define PLEXTALK_MAX_DIR_DEPTH			8
#define PLEXTALK_MAX_ENTRIES_PER_DIR	4096

struct plextalk_global_config {
	int suspend_lock;
	int schedule_lock;
	int sleep_timer_lock;

	unsigned int timer_countdown[3];//appk 2->3

//wgp
	/* read only, don't modify */
	int  auto_poweroff_time;
	
	float   radio_hi;
	float   radio_low;
	int  langsetting;
	int  tts_setting;
	
//wgp
	
	int hour_system;	// 0: 12; 1: 24
	int time_format;	// 0: hour, minute, year, month, day, weekday
						// 1: hour, minute, weekday, month, day, year
						// 2: hour, minute, weekday, day, month, year

	int statusbar_category_icon;

	int alarm_disable;			// when record is actived, and source is MIC, and headphone is offline
	int menu_disable;			// forbid menu
	int help_disable;			// forbid help
	int volume_vprompt_disable;	// when book or music is playing, we don't prompt "volume ..."
	int hotplug_voice_disable;  //add by km 
	int SD_hotplug_voice_disable;//add bm appk
	int offtimer_disable;		//add by appk
	int auto_off_disable;

	int menu_is_running;		//added by ztz
	
	struct plextalk_setting setting;
	struct plextalk_volume volume;
};

extern struct plextalk_global_config* g_config;
extern shm_lock_t* g_config_lock;

/* desktop used only */
int plextalk_global_config_init(void);
/* others app used */
int plextalk_global_config_open(void);
void plextalk_global_config_close(void);

#define PLEXTALK_LOCK_FUNC_DEFINE(name) \
int plextalk_ ## name ## _lock(void); \
int plextalk_ ## name ## _unlock(void); \
int plextalk_ ## name ## _is_locked(void);

PLEXTALK_LOCK_FUNC_DEFINE(suspend)
PLEXTALK_LOCK_FUNC_DEFINE(schedule)
PLEXTALK_LOCK_FUNC_DEFINE(sleep_timer)

unsigned int plextalk_get_timer_countdown(int index);

#define PLEXTALK_ACCESS_FUNC_DEFINE(memb) \
int plextalk_get_ ## memb(void); \
void plextalk_set_ ## memb(int);

PLEXTALK_ACCESS_FUNC_DEFINE(statusbar_category_icon)
PLEXTALK_ACCESS_FUNC_DEFINE(alarm_disable)
PLEXTALK_ACCESS_FUNC_DEFINE(menu_disable)
PLEXTALK_ACCESS_FUNC_DEFINE(help_disable)
PLEXTALK_ACCESS_FUNC_DEFINE(hotplug_voice_disable)//add by km
PLEXTALK_ACCESS_FUNC_DEFINE(SD_hotplug_voice_disable)//add by km
PLEXTALK_ACCESS_FUNC_DEFINE(offtimer_disable)//add by km
PLEXTALK_ACCESS_FUNC_DEFINE(volume_vprompt_disable)
PLEXTALK_ACCESS_FUNC_DEFINE(auto_off_disable)


#endif
