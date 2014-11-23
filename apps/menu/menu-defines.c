#include "menu-defines.h"
#include "application-ipc.h"
#include "plextalk-setting.h"
#include "nxutils.h"

/* define _ here only to make xgettext happy */
#undef _
#define _(x) x

static const char* sound_speed_menu_items[] = {
//	_("8"),
//	_("7"),
//	_("6"),
//	_("5"),
	_("4"),
	_("3"),
	_("2"),
	_("1"),
	_("0"),
	_("-1"),
	_("-2"),
};

static const char* sound_equalizer_menu_items[] = {
	_("6"),
	_("5"),
	_("4"),
	_("3"),
	_("2"),
	_("1"),
	_("0"),
	_("-1"),
	_("-2"),
	_("-3"),
	_("-4"),
	_("-5"),
	_("-6"),
};

/* main menu */
static const char* main_menu_items[] = {
	_("Book"),
	_("Music"),
	_("Bookmark"),
	_("Radio"),
	_("Recording"),
	_("Backup"),
	_("Timer"),
	_("General"),
};

static const struct menu_action main_menu_actions[] = {
	{ .submenu = &book_menu,      },
	{ .submenu = &music_menu,     },
	{ .submenu = &bookmark_menu, .callback = bookmark_validate, },
	{ .submenu = &radio_menu,     },
	{ .submenu = &recording_menu, },
	{ .submenu = &backup_menu, .callback = backup_media_menu_init, },
	{ .submenu = &timer_menu,     },
	{ .submenu = &general_menu,   },
};

const struct menu main_menu = {
	.caption		= _("Main Menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(main_menu_items),
	.items			= main_menu_items,
	.actions		= main_menu_actions,
};

/* book menu */
static const char* book_menu_items[] = {
	_("Jump by page"),
	_("Playback speed"),
	_("Repeat the current title"),
	_("Delete the current title"),
};

static const struct menu_action book_menu_actions[] = 
{
	{ .callback = book_jump_page,.data = (void*)0, },
	{ .submenu  = &book_speed_menu,  },
	{ .submenu  = &book_repeat_menu, },
	{ .callback = book_delete_operation, .data = (void*)APP_DELETE_TITLE, },
};

const struct menu book_menu = {
	.caption		= _("Book menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(book_menu_items),
	.items			= book_menu_items,
	.actions		= book_menu_actions,
};

static const struct menu_action book_speed_menu_actions[] = {
//	{ .callback = book_set_speed, .data = (void*)8 },
//	{ .callback = book_set_speed, .data = (void*)7 },
//	{ .callback = book_set_speed, .data = (void*)6 },
//	{ .callback = book_set_speed, .data = (void*)5 },
	{ .callback = book_set_speed, .data = (void*)4 },
	{ .callback = book_set_speed, .data = (void*)3 },
	{ .callback = book_set_speed, .data = (void*)2 },
	{ .callback = book_set_speed, .data = (void*)1 },
	{ .callback = book_set_speed, .data = (void*)0 },
	{ .callback = book_set_speed, .data = (void*)(-1) },
	{ .callback = book_set_speed, .data = (void*)(-2) },
};

const struct menu book_speed_menu = {
	.caption		= _("Book menu"),
	.items_nr		= ARRAY_SIZE(sound_speed_menu_items),
	.items			= sound_speed_menu_items,
	.actions		= book_speed_menu_actions,
	.has_default	= 1,
	.index_offset	= -4,
	.value_offset	= offsetof(struct plextalk_setting, book.speed),
};

static const char* book_repeat_menu_items[] = {
	_("Standard play"),
	_("Title repeat"),
};

static const struct menu_action book_repeat_menu_actions[] = {
	{ .callback = book_set_repeat, .data = (void*)0 },
	{ .callback = book_set_repeat, .data = (void*)1 },
};

const struct menu book_repeat_menu = {
	.caption		= _("Book menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(book_repeat_menu_items),
	.items			= book_repeat_menu_items,
	.actions		= book_repeat_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, book.repeat),
};

/* music menu */
static const char* music_menu_items[] = {
	_("Playback speed"),
	_("Repeat"),
	_("Delete the current track"),
	_("Delete the current album"),
};

static const struct menu_action music_menu_actions[] = {
	{ .submenu  = &music_speed_menu,  },
	{ .submenu  = &music_repeat_menu, },
	{ .callback = music_delete_operation, .data = (void*)APP_DELETE_TRACK, },
	{ .callback = music_delete_operation, .data = (void*)APP_DELETE_ALBUM, },
};

const struct menu music_menu = {
	.caption		= _("Music menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(music_menu_items),
	.items			= music_menu_items,
	.actions		= music_menu_actions,
};

static const struct menu_action music_speed_menu_actions[] = {
//	{ .callback = music_set_speed, .data = (void*)8 },
//	{ .callback = music_set_speed, .data = (void*)7 },
//	{ .callback = music_set_speed, .data = (void*)6 },
//	{ .callback = music_set_speed, .data = (void*)5 },
	{ .callback = music_set_speed, .data = (void*)4 },
	{ .callback = music_set_speed, .data = (void*)3 },
	{ .callback = music_set_speed, .data = (void*)2 },
	{ .callback = music_set_speed, .data = (void*)1 },
	{ .callback = music_set_speed, .data = (void*)0 },
	{ .callback = music_set_speed, .data = (void*)(-1) },
	{ .callback = music_set_speed, .data = (void*)(-2) },
};

const struct menu music_speed_menu = {
	.caption		= _("Music menu"),
	.items_nr		= ARRAY_SIZE(sound_speed_menu_items),
	.items			= sound_speed_menu_items,
	.actions		= music_speed_menu_actions,
	.has_default	= 1,
	.index_offset	= -4,
	.value_offset	= offsetof(struct plextalk_setting, music.speed),
};

static const char* music_repeat_menu_items[] = {
	_("Off"),
	_("Track repeat"),
	_("Album repeat"),
	_("All album repeat"),
	_("Shuffle repeat"),
};

static const struct menu_action music_repeat_menu_actions[] = {
	{ .callback = music_set_repeat, .data = (void*)0 },
	{ .callback = music_set_repeat, .data = (void*)1 },
	{ .callback = music_set_repeat, .data = (void*)2 },
	{ .callback = music_set_repeat, .data = (void*)3 },
	{ .callback = music_set_repeat, .data = (void*)4 },
};

const struct menu music_repeat_menu = {
	.caption		= _("Music menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(music_repeat_menu_items),
	.items			= music_repeat_menu_items,
	.actions		= music_repeat_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, music.repeat),
};

/* bookmark menu */
static const char* bookmark_menu_items[] = {
	_("Set bookmark"),
	_("Delete bookmark"),
};

static const struct menu_action bookmark_menu_actions[] = {
	{ .callback = bookmark_operation, .data = (void*) APP_BOOKMARK_SET, },
	{ .submenu  = &bookmark_delete_menu, },
};

const struct menu bookmark_menu = {
	.caption		= _("Bookmark menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(bookmark_menu_items),
	.items			= bookmark_menu_items,
	.actions		= bookmark_menu_actions,
};

static const char* bookmark_delete_menu_items[] = {
	_("Delete the current bookmark"),
	_("Delete all bookmarks of the current title"),
};

static const struct menu_action bookmark_delete_menu_actions[] = {
	{ .callback = bookmark_operation, .data = (void*) APP_BOOKMARK_DELETE,     },
	{ .callback = bookmark_operation, .data = (void*) APP_BOOKMARK_DELETE_ALL, },
};

const struct menu bookmark_delete_menu = {
	.caption		= _("Bookmark menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(bookmark_delete_menu_items),
	.items			= bookmark_delete_menu_items,
	.actions		= bookmark_delete_menu_actions,
};

/* radio menu */
static const char* radio_menu_items[] = {
	_("Audio output"),
	_("Delete preset channel"),
};

static const struct menu_action radio_menu_actions[] = {
	{ .submenu = &radio_output_menu, },
	{ .submenu = &radio_delete_menu, .callback = radio_delete_menu_validate, },
};

const struct menu radio_menu = {
	.caption		= _("Radio menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(radio_menu_items),
	.items			= radio_menu_items,
	.actions		= radio_menu_actions,
};

static const char* radio_delete_menu_items[] = {
	_("Delete the current channel"),
	_("Delete all preset channels"),
};

static const struct menu_action radio_delete_menu_actions[] = {
	{ .callback = radio_delete_operation, .data = (void*)APP_DELETE_CURRENT_CHANNEL, },
	{ .callback = radio_delete_operation, .data = (void*)APP_DELETE_ALL_CHANNELS,    },
};

const struct menu radio_delete_menu = {
	.caption		= _("Radio menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(radio_delete_menu_items),
	.items			= radio_delete_menu_items,
	.actions		= radio_delete_menu_actions,
};

static const char* radio_output_menu_items[] = {
	_("Speaker"),
	_("Headphone"),
};

static const struct menu_action radio_output_menu_actions[] = {
	{ .callback = radio_set_output, .data = (void*)0, },
	{ .callback = radio_set_output, .data = (void*)1, },
};

const struct menu radio_output_menu = {
	.caption		= _("Radio menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(radio_output_menu_items),
	.items			= radio_output_menu_items,
	.actions		= radio_output_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, radio.output),
};

/* record menu */
static const char* recording_menu_items[] = {
	_("Recording medium"),
};

static const struct menu_action recording_menu_actions[] = {
	{ .submenu = &recording_target_menu, },
};

const struct menu recording_menu = {
	.caption		= _("Recording menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(recording_menu_items),
	.items			= recording_menu_items,
	.actions		= recording_menu_actions,
};

static const char* recording_target_menu_items[] = {
	_("SD card"),
	_("Internal memory"),
};

static const struct menu_action recording_target_menu_actions[] = {
	{ .callback = recording_set_target, .data = (void*)0, },
	{ .callback = recording_set_target, .data = (void*)1, },
};

const struct menu recording_target_menu = {
	.caption		= _("Recording menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(recording_target_menu_items),
	.items			= recording_target_menu_items,
	.actions		= recording_target_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, record.saveto),
};

/* backup menu */
static const char* backup_menu_items[] = {
	_("Target medium"),
};

static const struct menu_action backup_menu_actions[] = {
	{ .submenu = &backup_media_menu, },
};

// tgh
char* backup_media_menu_items[2];

static const struct menu_action backup_media_menu_actions[2] = {
	{ .callback = do_backup, .data = (void*)0, },
	{ .callback = do_backup, .data = (void*)1, },
};

const struct menu backup_menu = {
	.caption		= _("Backup menu"),
	.voice_prompt	= _("Backup mode. Select backup target media."),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(backup_media_menu_items),
	.items			= backup_media_menu_items,
	.actions		= backup_media_menu_actions,
};

//tgh

struct menu backup_media_menu = {
	.caption		= _("Backup menu"),
	.wrap_around	= 1,
	.items_nr		= 2,
	.items			= backup_media_menu_items,
	.actions		= backup_media_menu_actions,
};

/* timer menu */
static const char* timer_menu_items[] = {
	_("Timer 1"),//modified by lfh
	_("Timer 2"),//lhf
};

static const struct menu_action timer_menu_actions[] = {
	{ .submenu = &timer_switch_menu, .callback = timer_set_active, .data = (void*)0, },
	{ .submenu = &timer_switch_menu, .callback = timer_set_active, .data = (void*)1, },
};

const struct menu timer_menu = {
	.caption		= _("Timer settings"),
	.voice_prompt	= _("Select the timer"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_menu_items),
	.items			= timer_menu_items,
	.actions		= timer_menu_actions,
};

static const char* timer_switch_menu_items[] = {
	_("Enable"),
	_("Disable"),
};

static const struct menu_action timer_switch_menu_actions[] = {
	{ .submenu  = &timer_func_menu, },
	{ .callback = timer_disable,    },
};

const struct menu timer_switch_menu = {
	.caption		= _("Timer menu"),
	.voice_prompt	= _("Select the timer ON or OFF"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_switch_menu_items),
	.items			= timer_switch_menu_items,
	.actions		= timer_switch_menu_actions,
	.has_default	= 1,
	.index_offset	= -1,
	.value_offset	= offsetof(struct plextalk_timer, enabled),
};

static const char* timer_func_menu_items[] = {
	_("Alarm"),
	_("Sleep"),
};

static const struct menu_action timer_func_menu_actions[] = {
	{ .submenu = &timer_sound_menu, .callback = timer_set_func, .data = (void*)0, },
	{ .submenu = &timer_type_menu,  .callback = timer_set_func, .data = (void*)1, },
};

const struct menu timer_func_menu = {
	.caption		= _("Timer menu"),
	.voice_prompt	= _("Select the timer function"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_func_menu_items),
	.items			= timer_func_menu_items,
	.actions		= timer_func_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_timer, function),
};

static const char* timer_sound_menu_items[] = {
	_("Sound A"),
	_("Sound B"),
	_("Sound C"),
};

static const struct menu_action timer_sound_menu_actions[] = {
	{ .submenu = &timer_type_menu, .callback = timer_set_sound, .data = (void*)0, },
	{ .submenu = &timer_type_menu, .callback = timer_set_sound, .data = (void*)1, },
	{ .submenu = &timer_type_menu, .callback = timer_set_sound, .data = (void*)2, },
};

const struct menu timer_sound_menu = {
	.caption		= _("Timer menu"),
	.voice_prompt	= _("Select the alarm sound"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_sound_menu_items),
	.items			= timer_sound_menu_items,
	.actions		= timer_sound_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_timer, sound),
};

static const char* timer_type_menu_items[] = {
	_("By clock timer"),
	_("By count down timer"),
};

static const struct menu_action timer_type_menu_actions[] = {
	{ .submenu = &timer_repeat_menu,    .callback = timer_set_clocktime,.data = (void*)0,},
	{ .submenu = &timer_countdown_menu, .callback = timer_set_type, .data = (void*)1, },
};

const struct menu timer_type_menu = {
	.caption		= _("Timer menu"),
	.voice_prompt	= _("Select the timer setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_type_menu_items),
	.items			= timer_type_menu_items,
	.actions		= timer_type_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_timer, type),
};

/*
 * it selects the repeat operation when
 * the alarm timer operates with the clock time
 */
static const char* timer_repeat_menu_items[] = {
	_("One time"),
	_("Everyday"),
	_("Day of the week"),
};

static const struct menu_action timer_repeat_menu_actions[] = {
	{ .callback = timer_set_repeat, .data = (void*)0, },
	{ .callback = timer_set_repeat, .data = (void*)1, },
	{ .callback = timer_set_repeat, .data = (void*)2, },
};

const struct menu timer_repeat_menu = {
	.caption		= _("Timer menu"),
	.voice_prompt	= _("Select the repeat setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(timer_repeat_menu_items),
	.items			= timer_repeat_menu_items,
	.actions		= timer_repeat_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_timer, repeat),
};

static const char* timer_countdown_menu_items[] = {
	_("3 minutes"),
	_("5 minutes"),
	_("15 minutes"),
	_("30 minutes"),
	_("45 minutes"),
	_("60 minutes"),
	_("90 minutes"),
	_("120 minutes"),
};

static const struct menu_action timer_countdown_menu_actions[] = {
	{ .callback = timer_set_countdown, .data = (void*)0, },
	{ .callback = timer_set_countdown, .data = (void*)1, },
	{ .callback = timer_set_countdown, .data = (void*)2, },
	{ .callback = timer_set_countdown, .data = (void*)3, },
	{ .callback = timer_set_countdown, .data = (void*)4, },
	{ .callback = timer_set_countdown, .data = (void*)5, },
	{ .callback = timer_set_countdown, .data = (void*)6, },
	{ .callback = timer_set_countdown, .data = (void*)7, },
};

const struct menu timer_countdown_menu = {
	.caption		= _("Timer menu"),
	.items_nr		= ARRAY_SIZE(timer_countdown_menu_items),
	.items			= timer_countdown_menu_items,
	.actions		= timer_countdown_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_timer, elapse),
};

/* general menu */
static const char* general_menu_items[] = {
	_("System properties"),
	_("Guide volume"),
	_("Guide speed"),
	_("Equalizer"),
	_("LCD"),
	_("Language"),
	_("Text to speech"),
	_("System date and time"),
	_("Calculator"),
	_("File management"),
	_("Format"),
	_("Initialize all settings to default"),
};

static const struct menu_action general_menu_actions[] = {
	{ .callback = do_system_properties, },
	{ .submenu  = &guide_volume_menu,   },
	{ .submenu  = &guide_speed_menu,    },
	{ .submenu  = &equalizer_menu,      },
	{ .submenu  = &lcd_menu,            },
	{ .submenu  = &language_menu,
	  .callback = language_menu_init,   },
	{ .submenu  = &tts_menu,        .callback=tts_menu_init   ,},//app
	{ .callback = do_set_time,          },
	{ .callback = do_calculator,        },
	{ .callback = do_file_management,   },
	{ .submenu  = &format_menu,
	  .callback = format_menu_init,     },
	{ .callback = do_restore_settings,  },
};

const struct menu general_menu = {
	.caption		= _("General menu"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(general_menu_items),
	.items			= general_menu_items,
	.actions		= general_menu_actions,
};

static const char* guide_volume_menu_items[] = {
	_("25"),
	_("24"),
	_("23"),
	_("22"),
	_("21"),
	_("20"),
	_("19"),
	_("18"),
	_("17"),
	_("16"),
	_("15"),
	_("14"),
	_("13"),
	_("12"),
	_("11"),
	_("10"),
	_("9"),
	_("8"),
	_("7"),
	_("6"),
	_("5"),
	_("4"),
	_("3"),
	_("2"),
	_("1"),
//	_("0"),
};

static const struct menu_action guide_volume_menu_actions[] = {
	{ .callback = guide_set_volume, .data = (void*)25, },
	{ .callback = guide_set_volume, .data = (void*)24, },
	{ .callback = guide_set_volume, .data = (void*)23, },
	{ .callback = guide_set_volume, .data = (void*)22, },
	{ .callback = guide_set_volume, .data = (void*)21, },
	{ .callback = guide_set_volume, .data = (void*)20, },
	{ .callback = guide_set_volume, .data = (void*)19, },
	{ .callback = guide_set_volume, .data = (void*)18, },
	{ .callback = guide_set_volume, .data = (void*)17, },
	{ .callback = guide_set_volume, .data = (void*)16, },
	{ .callback = guide_set_volume, .data = (void*)15, },
	{ .callback = guide_set_volume, .data = (void*)14, },
	{ .callback = guide_set_volume, .data = (void*)13, },
	{ .callback = guide_set_volume, .data = (void*)12, },
	{ .callback = guide_set_volume, .data = (void*)11, },
	{ .callback = guide_set_volume, .data = (void*)10, },
	{ .callback = guide_set_volume, .data = (void*)9, },
	{ .callback = guide_set_volume, .data = (void*)8, },
	{ .callback = guide_set_volume, .data = (void*)7, },
	{ .callback = guide_set_volume, .data = (void*)6, },
	{ .callback = guide_set_volume, .data = (void*)5, },
	{ .callback = guide_set_volume, .data = (void*)4, },
	{ .callback = guide_set_volume, .data = (void*)3, },
	{ .callback = guide_set_volume, .data = (void*)2, },
	{ .callback = guide_set_volume, .data = (void*)1, },
//	{ .callback = guide_set_volume, .data = (void*)0, },
};

const struct menu guide_volume_menu = {
	.caption		= _("Guide volume setting"),
	.items_nr		= ARRAY_SIZE(guide_volume_menu_items),
	.items			= guide_volume_menu_items,
	.actions		= guide_volume_menu_actions,
	.has_default	= 1,
	.index_offset	= -25,
};

static const struct menu_action guide_speed_menu_actions[] = {
//	{ .callback = guide_set_speed, .data = (void*)8 },
//	{ .callback = guide_set_speed, .data = (void*)7 },
//	{ .callback = guide_set_speed, .data = (void*)6 },
//	{ .callback = guide_set_speed, .data = (void*)5 },
	{ .callback = guide_set_speed, .data = (void*)4 },
	{ .callback = guide_set_speed, .data = (void*)3 },
	{ .callback = guide_set_speed, .data = (void*)2 },
	{ .callback = guide_set_speed, .data = (void*)1 },
	{ .callback = guide_set_speed, .data = (void*)0 },
	{ .callback = guide_set_speed, .data = (void*)(-1) },
	{ .callback = guide_set_speed, .data = (void*)(-2) },
};

const struct menu guide_speed_menu = {
	.caption		= _("Guide speed setting"),
	.items_nr		= ARRAY_SIZE(sound_speed_menu_items),
	.items			= sound_speed_menu_items,
	.actions		= guide_speed_menu_actions,
	.has_default	= 1,
	.index_offset	= -4,
	.value_offset	= offsetof(struct plextalk_setting, guide.speed),
};

static const char* equalizer_menu_items[] = {
	_("Book"),
	_("Music"),
};

static const struct menu_action equalizer_menu_actions[] = {
	{ .submenu = &book_equalizer_menu,  },
	{ .submenu = &music_equalizer_menu, },
};

const struct menu equalizer_menu = {
	.caption		= _("Equalizer setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(equalizer_menu_items),
	.items			= equalizer_menu_items,
	.actions		= equalizer_menu_actions,
};

static const char* book_equalizer_menu_items[] = {
	_("Audio"),
	_("Text"),
};

static const struct menu_action book_equalizer_menu_actions[] = {
	{ .submenu = &book_audio_equalizer_menu, },
	{ .submenu = &book_text_equalizer_menu,  },
};

const struct menu book_equalizer_menu = {
	.caption		= _("Equalizer setting"),
	.items_nr		= ARRAY_SIZE(book_equalizer_menu_items),
	.items			= book_equalizer_menu_items,
	.actions		= book_equalizer_menu_actions,
};

static const struct menu_action book_audio_equalizer_menu_actions[] = {
	{ .callback = book_set_audio_equalizer, .data = (void*)6 },
	{ .callback = book_set_audio_equalizer, .data = (void*)5 },
	{ .callback = book_set_audio_equalizer, .data = (void*)4 },
	{ .callback = book_set_audio_equalizer, .data = (void*)3 },
	{ .callback = book_set_audio_equalizer, .data = (void*)2 },
	{ .callback = book_set_audio_equalizer, .data = (void*)1 },
	{ .callback = book_set_audio_equalizer, .data = (void*)0 },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-1) },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-2) },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-3) },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-4) },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-5) },
	{ .callback = book_set_audio_equalizer, .data = (void*)(-6) },
};

const struct menu book_audio_equalizer_menu = {
	.caption		= _("Equalizer setting"),
	.items_nr		= ARRAY_SIZE(sound_equalizer_menu_items),
	.items			= sound_equalizer_menu_items,
	.actions		= book_audio_equalizer_menu_actions,
	.has_default	= 1,
	.index_offset	= -6,
	.value_offset	= offsetof(struct plextalk_setting, book.audio_equalizer),
};

static const struct menu_action book_text_equalizer_menu_actions[] = {
	{ .callback = book_set_text_equalizer, .data = (void*)6 },
	{ .callback = book_set_text_equalizer, .data = (void*)5 },
	{ .callback = book_set_text_equalizer, .data = (void*)4 },
	{ .callback = book_set_text_equalizer, .data = (void*)3 },
	{ .callback = book_set_text_equalizer, .data = (void*)2 },
	{ .callback = book_set_text_equalizer, .data = (void*)1 },
	{ .callback = book_set_text_equalizer, .data = (void*)0 },
	{ .callback = book_set_text_equalizer, .data = (void*)(-1) },
	{ .callback = book_set_text_equalizer, .data = (void*)(-2) },
	{ .callback = book_set_text_equalizer, .data = (void*)(-3) },
	{ .callback = book_set_text_equalizer, .data = (void*)(-4) },
	{ .callback = book_set_text_equalizer, .data = (void*)(-5) },
	{ .callback = book_set_text_equalizer, .data = (void*)(-6) },
};

const struct menu book_text_equalizer_menu = {
	.caption		= _("Equalizer setting"),
	.items_nr		= ARRAY_SIZE(sound_equalizer_menu_items),
	.items			= sound_equalizer_menu_items,
	.actions		= book_text_equalizer_menu_actions,
	.has_default	= 1,
	.index_offset	= -6,
	.value_offset	= offsetof(struct plextalk_setting, book.text_equalizer),
};

static const char* music_equalizer_menu_items[] = {
	_("Normal"),
	_("Bass"),
	_("Classical"),
	_("Club"),
	_("Dance"),
	_("Flat"),
	_("Hall"),
	_("Headphones"),
	_("Live"),
	_("Party"),
	_("Pop"),
	_("Reggae"),
	_("Rock"),
	_("Ska"),
	_("Soft"),
	_("Techno"),
	_("Treble"),
};

static const struct menu_action music_equalizer_menu_actions[] = {
	{ .callback = music_set_equalizer, .data = (void*)0 },
	{ .callback = music_set_equalizer, .data = (void*)1 },
	{ .callback = music_set_equalizer, .data = (void*)2 },
	{ .callback = music_set_equalizer, .data = (void*)3 },
	{ .callback = music_set_equalizer, .data = (void*)4 },
	{ .callback = music_set_equalizer, .data = (void*)5 },
	{ .callback = music_set_equalizer, .data = (void*)6 },
	{ .callback = music_set_equalizer, .data = (void*)7 },
	{ .callback = music_set_equalizer, .data = (void*)8 },
	{ .callback = music_set_equalizer, .data = (void*)9 },
	{ .callback = music_set_equalizer, .data = (void*)10 },
	{ .callback = music_set_equalizer, .data = (void*)11 },
	{ .callback = music_set_equalizer, .data = (void*)12 },
	{ .callback = music_set_equalizer, .data = (void*)13 },
	{ .callback = music_set_equalizer, .data = (void*)14 },
	{ .callback = music_set_equalizer, .data = (void*)15 },
	{ .callback = music_set_equalizer, .data = (void*)16 },
};

const struct menu music_equalizer_menu = {
	.caption		= _("Equalizer setting"),
	.items_nr		= ARRAY_SIZE(music_equalizer_menu_items),
	.items			= music_equalizer_menu_items,
	.actions		= music_equalizer_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, music.equalizer),
};

static const char* lcd_menu_items[] = {
	_("Turn off LCD backlight"),
	_("Font size"),
	_("Font color / Backlight color"),//app
	_("Brightness"),
};

static const struct menu_action lcd_menu_actions[] = {
	{ .submenu = &screensaver_menu, },
	{ .submenu = &font_size_menu,   },
	{ .submenu = &theme_menu,       },
	{ .submenu = &brightness_menu,  },
};

const struct menu lcd_menu = {
	.caption		= _("LCD setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(lcd_menu_items),
	.items			= lcd_menu_items,
	.actions		= lcd_menu_actions,
};

static const char* screensaver_menu_items[] = {
	_("Always off"),
	_("5 seconds"),
	_("15 seconds"),
	_("30 seconds"),
	_("1 minute"),
	_("5 minutes"),
	_("Always on"),
};

static const struct menu_action screensaver_menu_actions[] = {
	{ .callback = set_screensaver_timeout, .data = (void*)0, },
	{ .callback = set_screensaver_timeout, .data = (void*)1, },
	{ .callback = set_screensaver_timeout, .data = (void*)2, },
	{ .callback = set_screensaver_timeout, .data = (void*)3, },
	{ .callback = set_screensaver_timeout, .data = (void*)4, },
	{ .callback = set_screensaver_timeout, .data = (void*)5, },
	{ .callback = set_screensaver_timeout, .data = (void*)6, },
};

const struct menu screensaver_menu = {
	.caption		= _("LCD setting"),
	.items_nr		= ARRAY_SIZE(screensaver_menu_items),
	.items			= screensaver_menu_items,
	.actions		= screensaver_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, lcd.screensaver),
};

static const char* font_size_menu_items[] = {
	_("24 points"),
	_("16 points"),
	_("12 points"),
};

static const struct menu_action font_size_menu_actions[] = {
	{ .callback = set_font_size, .data = (void*)24, },
	{ .callback = set_font_size, .data = (void*)16, },
	{ .callback = set_font_size, .data = (void*)12, },
};

const struct menu font_size_menu = {
	.caption		= _("LCD setting"),
	.items_nr		= ARRAY_SIZE(font_size_menu_items),
	.items			= font_size_menu_items,
	.actions		= font_size_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, lcd.fontsize),
	.find_index		= find_fontsize_index,
};

static const char* theme_menu_items[] = {
	_("White / Black"),
	_("Black / White"),
	_("Black / Yellow"),
	_("Yellow / Black"),
	_("Yellow / Blue"),
	_("Blue / Yellow"),
	_("Blue / White"),
	_("White / Blue"),
	_("Orange / Black"),
	_("Black / Orange"),
	_("Red / White"),
	_("White / Red"),
	_("Green / Black"),
	_("Black / Green"),
	_("Purple / White"),
	_("White / Purple"),
};

static const struct menu_action theme_menu_actions[] = {
	{ .callback = set_theme, .data = (void*)0, },
	{ .callback = set_theme, .data = (void*)1, },
	{ .callback = set_theme, .data = (void*)2, },
	{ .callback = set_theme, .data = (void*)3, },
	{ .callback = set_theme, .data = (void*)4, },
	{ .callback = set_theme, .data = (void*)5, },
	{ .callback = set_theme, .data = (void*)6, },
	{ .callback = set_theme, .data = (void*)7, },
	{ .callback = set_theme, .data = (void*)8, },
	{ .callback = set_theme, .data = (void*)9, },
	{ .callback = set_theme, .data = (void*)10, },
	{ .callback = set_theme, .data = (void*)11, },
	{ .callback = set_theme, .data = (void*)12, },
	{ .callback = set_theme, .data = (void*)13, },
	{ .callback = set_theme, .data = (void*)14, },
	{ .callback = set_theme, .data = (void*)15, },
};

const struct menu theme_menu = {
	.caption		= _("LCD setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(theme_menu_items),
	.items			= theme_menu_items,
	.actions		= theme_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, lcd.theme),
};

static const char* brightness_menu_items[] = {
	_("5"),
	_("4"),
	_("3"),
	_("2"),
	_("1"),
	_("0"),
	_("-1"),
	_("-2"),
	_("-3"),
	_("-4"),
	_("-5"),
};

static const struct menu_action brightness_menu_actions[] = {
	{ .callback = set_brightness, .data = (void*)5 },
	{ .callback = set_brightness, .data = (void*)4 },
	{ .callback = set_brightness, .data = (void*)3 },
	{ .callback = set_brightness, .data = (void*)2 },
	{ .callback = set_brightness, .data = (void*)1 },
	{ .callback = set_brightness, .data = (void*)0 },
	{ .callback = set_brightness, .data = (void*)(-1) },
	{ .callback = set_brightness, .data = (void*)(-2) },
	{ .callback = set_brightness, .data = (void*)(-3) },
	{ .callback = set_brightness, .data = (void*)(-4) },
	{ .callback = set_brightness, .data = (void*)(-5) },
};

const struct menu brightness_menu = {
	.caption		= _("LCD setting"),
	.items_nr		= ARRAY_SIZE(brightness_menu_items),
	.items			= brightness_menu_items,
	.actions		= brightness_menu_actions,
	.has_default	= 1,
	.index_offset	= -5,
	.value_offset	= offsetof(struct plextalk_setting, lcd.backlight),
};

struct menu language_menu = {
	.caption		= _("Language setting"),
	.wrap_around	= 1,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, lang.lang),
	.find_index		= find_language_index,
};

//app start
 const char * tts_menu_items[] = {
	_("Male / Female"),
	_("Standard Chinese / Cantonese"),
};

 const  char* tts_menu_items_1[] = {
	_("Male / Female"),

};


const struct  menu_action tts_menu_actions[] = 
{
	{ .submenu = &tts_voice_species_menu, },
	{ .submenu = &tts_chinese_menu,       },
};


 const struct  menu_action tts_menu_actions_1[] = 
{
	{ .submenu = &tts_voice_species_menu, },

};


 struct menu tts_menu = {
	.caption		= _("Text to speech setting"),
	.wrap_around	= 1,
//	.items_nr		= ARRAY_SIZE(tts_menu_items),
//	.items			= tts_menu_items,
//	.actions		= tts_menu_actions,
};
//app end


static const char* tts_voice_species_menu_items[] = {
	_("Male"),
	_("Female"),
};

static  const struct menu_action tts_voice_species_menu_actions[] = {
	{ .callback = tts_set_voice_species, .data = (void*)0 },
	{ .callback = tts_set_voice_species, .data = (void*)1 },
};

const  struct menu tts_voice_species_menu = {
	.caption		= _("Text to speech setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(tts_voice_species_menu_items),
	.items			= tts_voice_species_menu_items,
	.actions		= tts_voice_species_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, tts.voice_species),
};

static  const char* tts_chinese_menu_items[] = {
	_("Standard Chinese"),
	_("Cantonese"),
};

static const struct menu_action tts_chinese_menu_actions[] = {
	{ .callback = tts_set_chinese_standard, .data = (void*)0, },
	{ .callback = tts_set_chinese_standard, .data = (void*)1, },
};

 const struct   menu tts_chinese_menu = {
	.caption		= _("Text to speech setting"),
	.wrap_around	= 1,
	.items_nr		= ARRAY_SIZE(tts_chinese_menu_items),
	.items			= tts_chinese_menu_items,
	.actions		= tts_chinese_menu_actions,
	.has_default	= 1,
	.value_offset	= offsetof(struct plextalk_setting, tts.chinese),
};

char* format_menu_items[FORMAT_MENU_ITEM_NUM] = {//appk
	_("Internal Memory"),
//	_("SD Card"),
//	_("USB Memory"),
};

struct menu_action format_menu_actions[FORMAT_MENU_ITEM_NUM] = {//appk
	{ .callback = do_format, .data = "/dev/mmcblk0p2", },
	{ .callback = do_format, /*.data = "/dev/mmcblk1p1",*/ },
	{ .callback = do_format, /*.data = "/dev/sda",*/ },
	{ .callback = do_format, /*.data = "/dev/sda1",*/ },
	{ .callback = do_format, /*.data = "/dev/sda2",*/ },
	{ .callback = do_format, /*.data = "/dev/sda3",*/ },
	{ .callback = do_format, /*.data = "/dev/sda4",*/ },
	{ .callback = do_format, /*.data = "/dev/sda5",*/ },
	{ .callback = do_format, /*.data = "/dev/sda6",*/ },
	{ .callback = do_format, /*.data = "/dev/sda7",*/ },
	{ .callback = do_format, /*.data = "/dev/sda8",*/ },
	{ .callback = do_format, /*.data = "/dev/sda9",*/ },
};

struct menu format_menu = {
	.caption		= _("Format"),
	.wrap_around	= 1,
	.items_nr		= 1,
	.items			= format_menu_items,
	.actions		= format_menu_actions,
};
