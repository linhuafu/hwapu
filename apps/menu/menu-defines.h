#ifndef MENU_DEFINES_H
#define MENU_DEFINES_H

#define MENU_MAX_DEPTH 8

struct menu_action {
	struct menu *submenu;
	int (*callback)(void *);
	void *data;
};

struct menu {
	const char *caption;
	const char *voice_prompt;
	int wrap_around;
	unsigned int items_nr;
	const char **items;
	struct menu_action *actions;
	int has_default;
	int index_offset;
	int value_offset;
	int (*find_index)(void*);
};

#define MENU_GENERAL_SYSTEM_DATETIME_ID 7

#define MENU_CALLBACK_OK		0
#define MENU_CALLBACK_ERR		-1
#define MENU_CALLBACK_ERR_EXIT	-2

int find_fontsize_index(void*);
int find_language_index(void*);

extern const struct menu main_menu;

extern const struct menu book_menu;
extern const struct menu book_speed_menu;
extern const struct menu book_repeat_menu;

extern const struct menu music_menu;
extern const struct menu music_speed_menu;
extern const struct menu music_repeat_menu;

extern const struct menu bookmark_menu;
extern const struct menu bookmark_delete_menu;

extern const struct menu radio_menu;
extern const struct menu radio_delete_menu;
extern const struct menu radio_output_menu;

extern const struct menu recording_menu;
extern const struct menu recording_target_menu;

extern const struct menu backup_menu;
extern struct menu backup_media_menu;	/* dynamic menu */

extern const struct menu timer_menu;
extern const struct menu timer_switch_menu;
extern const struct menu timer_func_menu;
extern const struct menu timer_sound_menu;
extern const struct menu timer_type_menu;
extern const struct menu timer_countdown_menu;
extern const struct menu timer_repeat_menu;

extern const struct menu general_menu;

extern const struct menu guide_volume_menu;
extern const struct menu guide_speed_menu;

extern const struct menu equalizer_menu;
extern const struct menu book_equalizer_menu;
extern const struct menu book_audio_equalizer_menu;
extern const struct menu book_text_equalizer_menu;
extern const struct menu music_equalizer_menu;
extern const struct menu guide_equalizer_menu;

extern const struct menu lcd_menu;
extern const struct menu screensaver_menu;
extern const struct menu font_size_menu;
extern const struct menu theme_menu;
extern const struct menu brightness_menu;

extern struct menu language_menu;	/* dynamic menu */

extern struct menu tts_menu;
extern const struct menu tts_voice_species_menu;
extern const struct menu tts_chinese_menu;

extern struct menu format_menu;		/* dynamic menu */

extern char* backup_media_menu_items[2];

#define FORMAT_MENU_ITEM_NUM 12//appk
extern char* format_menu_items[FORMAT_MENU_ITEM_NUM];
extern struct menu_action format_menu_actions[FORMAT_MENU_ITEM_NUM];

int book_jump_page(void *arg);
int book_delete_operation(void*);
int book_set_speed(void*);
int book_set_repeat(void*);

int music_delete_operation(void*);
int music_set_speed(void*);
int music_set_repeat(void*);

int bookmark_validate(void*);
int bookmark_operation(void*);

int radio_delete_menu_validate(void*);
int radio_delete_operation(void*);
int radio_set_output(void*);

int recording_set_target(void*);

int backup_media_menu_init(void*);
int do_backup(void*);

int timer_set_active(void*);
int timer_disable(void*);
int timer_set_func(void*);
int timer_set_sound(void*);
int timer_set_type(void*);
int timer_set_clocktime(void*);
int timer_set_repeat(void*);
int timer_set_countdown(void*);

int do_system_properties(void*);
int language_menu_init(void*);
int  tts_menu_init(void * );//app
int do_set_time(void*);
int do_calculator(void*);
int do_file_management(void*);
int format_menu_init(void*);
int do_restore_settings(void*);

int guide_set_volume(void*);
int guide_set_speed(void*);

int book_set_audio_equalizer(void*);
int book_set_text_equalizer(void*);
int music_set_equalizer(void*);
int guide_set_equalizer(void*);

int set_screensaver_timeout(void*);
int set_font_size(void*);
int set_theme(void*);
int set_brightness(void*);

int set_language(void*);

int tts_set_voice_species(void*);
int tts_set_chinese_standard(void*);

int do_format(void*);

#endif
