#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <libintl.h>

#include "nxutils.h"
#include "xml-helper.h"
#include "plextalk-config.h"
#include "neux/application.h"

int plextalk_get_all_lang(const char *filename, struct plextalk_language_all *langs)
{
	xmlDocPtr doc;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr nodes;
	int i;

	if (langs == NULL)
		return -1;

	/* Load XML document */
	doc = xmlParseFile(filename);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
		return -1;
	}

	xpathObj = get_xpath_object(doc, (xmlChar*) "/plextalk/languages/lang");
	if (xpathObj == NULL) {
		fprintf(stderr, "Error: unable to evaluate xpath expression \"%s\"\n", filename);
		xmlFreeDoc(doc);
		return -1;
	}

	nodes = xpathObj->nodesetval;

	langs->lang_nr = nodes->nodeNr;
	langs->langs = malloc(sizeof(*langs->langs) * nodes->nodeNr);

	for (i = 0; i < nodes->nodeNr; i++) {
		xmlChar* lang = xmlGetProp(nodes->nodeTab[i], BAD_CAST "locale");
		xmlChar* name = xmlNodeGetContent(nodes->nodeTab[i]);
		strlcpy(langs->langs[i].lang, lang, PLEXTALK_LANG_MAX);
		strlcpy(langs->langs[i].name, name, PLEXTALK_LANG_NAME_MAX);
		xmlFree(lang);
		xmlFree(name);
	}

	/* Cleanup of XPath data */
	xmlXPathFreeObject(xpathObj);

	/* free the document */
	xmlFreeDoc(doc);

	return 0;
}

int plextalk_find_lang(const struct plextalk_language_all *langs, const char *lang)
{
	int i;

	for (i = 0; i < langs->lang_nr; i++) {
		if (!strcmp(lang, langs->langs[i].lang))
			return i;
	}

	return -1;
}

void plextalk_update_lang(const char *lang, const char *module)
{
	setenv("LC_ALL", lang, 1);
	setenv("LC_COLLATE", lang, 1);
	setenv("LANG", lang, 1);

	setlocale(LC_ALL, lang);

	bindtextdomain(module, PLEXTALK_I18N_DIR);
	bind_textdomain_codeset(module, "UTF-8");
	textdomain(module);
}

char sys_font_name[32];
int sys_font_size;

void plextalk_update_sys_font(const char *lang)
{
	CoolShmReadLock(g_config_lock);
	sys_font_size = g_config->setting.lcd.fontsize;
	if (StringStartWith(lang, "zh_CN"))
		strlcpy(sys_font_name, "msyh.ttf", sizeof(sys_font_name));
	else if (StringStartWith(lang, "zh_TW"))
		strlcpy(sys_font_name, "mszh.ttf", sizeof(sys_font_name));
	else
		strlcpy(sys_font_name, "arialuni.ttf", sizeof(sys_font_name));
	CoolShmReadUnlock(g_config_lock);
}

ivUInt32 tts_lang;
ivUInt32 tts_role;

void plextalk_update_tts(const char *lang)
{
	if (StringStartWith(lang, "zh_CN") ||
		StringStartWith(lang, "zh_TW")) {
		tts_lang = ivTTS_LANGUAGE_CHINESE;
	} else if (StringStartWith(lang, "hi_IN")) {
		tts_lang = ivTTS_LANGUAGE_HINDI;
	} else {
		tts_lang = ivTTS_LANGUAGE_ENGLISH;
	}

	tts_role = plextalk_get_tts_role(tts_lang);
};

ivUInt32 plextalk_get_tts_role(ivUInt32 tts_lang)
{
	ivUInt32 role = ivTTS_ROLE_USER;

	CoolShmReadLock(g_config_lock);
	switch (tts_lang) {

	case ivTTS_LANGUAGE_ENGLISH:
		if (g_config->setting.tts.voice_species)
			role = ivTTS_ROLE_CATHERINE;	/* Catherine (female, US English) */
		else
			role = ivTTS_ROLE_JOHN;			/* John (male, US English) */
		break;

	case ivTTS_LANGUAGE_CHINESE:
		printf("plextalk_get_tts_role=%d\n",g_config->setting.tts.chinese);
		if (g_config->setting.tts.chinese) 
		{
			if (g_config->setting.tts.voice_species)
				role = ivTTS_ROLE_XIAOMEI;	/* Xiaomei (female, Cantonese) */
			else
				role = ivTTS_ROLE_DALONG;	/* Dalong (male, Cantonese) */
		}
		else 
		{
			if (g_config->setting.tts.voice_species)
				role = ivTTS_ROLE_XIAOYAN;	/* Xiaoyan (female, Chinese) */
			else
				role = ivTTS_ROLE_XIAOFENG;	/* Xiaofeng (male, Chinese) */
		}
		break;

	case ivTTS_LANGUAGE_HINDI:
//		if (g_config->setting.tts.voice_species)
			role = ivTTS_ROLE_ABHA;			/* Abha (female, India Hindi)*/
//		else
//			role = ivTTS_ROLE_JOHN;			/* John (male, US English) */
//		break;
	}
	CoolShmReadUnlock(g_config_lock);

	return role;
}
