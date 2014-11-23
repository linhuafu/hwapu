#ifndef __SYS_I18N_H__
#define __SYS_I18N_H__

#include <locale.h>
#include <libintl.h>
#include <ivTTS.h>

#define _(x) gettext(x)

#define PLEXTALK_LANG_MAX		32
#define PLEXTALK_LANG_NAME_MAX	128

struct plextalk_language_all {
	struct {
		char lang[PLEXTALK_LANG_MAX];
		char name[PLEXTALK_LANG_NAME_MAX];
	} *langs;
	int lang_nr;
};

int plextalk_get_all_lang(const char *filename, struct plextalk_language_all *langs);

int plextalk_find_lang(const struct plextalk_language_all *langs, const char *lang);

void plextalk_update_lang(const char *lang, const char *module);


extern char sys_font_name[];
extern int sys_font_size;

void plextalk_update_sys_font(const char *lang);


extern ivUInt32 tts_lang;
extern ivUInt32 tts_role;

void plextalk_update_tts(const char *lang);

ivUInt32 plextalk_get_tts_role(ivUInt32 tts_lang);

#endif
