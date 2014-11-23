#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "nxutils.h"
#include "plextalk-config.h"
#include "plextalk-setting.h"

typedef struct {
	const char *key;
	int offset;
} setting_key_val;


static void parseGeneric(xmlDocPtr doc, xmlNodePtr cur, void *setting,
		const setting_key_val *pair, int count)
{
	xmlChar *content;
	int val, i;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		content = xmlNodeGetContent(cur);
		val = atoi(content);
		xmlFree(content);

		for (i = 0; i < count; i++) {
			if (xmlStrEqual(cur->name, BAD_CAST pair[i].key)) {
				*((int *)(setting + pair[i].offset)) = val;
				break;
			}
		}

		cur = cur->next;
	}
}

static const setting_key_val book_parser[] = {
	{ "speed",           offsetof(struct plextalk_book, speed), },
	{ "repeat",          offsetof(struct plextalk_book, repeat), },
	{ "audio_equalizer", offsetof(struct plextalk_book, audio_equalizer), },
	{ "text_equalizer",  offsetof(struct plextalk_book, text_equalizer), },
};

static const setting_key_val music_parser[] = {
	{ "speed",           offsetof(struct plextalk_music, speed), },
	{ "repeat",          offsetof(struct plextalk_music, repeat), },
	{ "equalizer",       offsetof(struct plextalk_music, equalizer), },
};

static const setting_key_val radio_parser[] = {
	{ "output",           offsetof(struct plextalk_radio, output), },
};

static const setting_key_val record_parser[] = {
	{ "saveto",           offsetof(struct plextalk_record, saveto), },
};

static const setting_key_val guide_parser[] = {
	{ "speed",           offsetof(struct plextalk_guide, speed), },
};

static const setting_key_val timer_parser[] = {
	{ "enabled",         offsetof(struct plextalk_timer, enabled), },
	{ "function",        offsetof(struct plextalk_timer, function), },
	{ "type",            offsetof(struct plextalk_timer, type), },
	{ "elapse",          offsetof(struct plextalk_timer, elapse), },
	{ "hour",            offsetof(struct plextalk_timer, hour), },
	{ "minute",          offsetof(struct plextalk_timer, minute), },
	{ "sound",           offsetof(struct plextalk_timer, sound), },
	{ "repeat",          offsetof(struct plextalk_timer, repeat), },
	{ "weekday",         offsetof(struct plextalk_timer, weekday), },
};

static const setting_key_val lcd_parser[] = {
	{ "screensaver",     offsetof(struct plextalk_lcd, screensaver), },
	{ "fontsize",        offsetof(struct plextalk_lcd, fontsize), },
	{ "theme",           offsetof(struct plextalk_lcd, theme), },
	{ "backlight",       offsetof(struct plextalk_lcd, backlight), },
};

static const setting_key_val tts_parser[] = {
	{ "voice_species",   offsetof(struct plextalk_tts, voice_species), },
	{ "chinese",         offsetof(struct plextalk_tts, chinese), },
};

#define KEY_VAL_PAIR_PARSR(name, var, parser) \
static void parse ## name(xmlDocPtr doc, xmlNodePtr cur) \
{ \
	parseGeneric(doc, cur, &g_config->setting.var, parser, ARRAY_SIZE(parser)); \
}

KEY_VAL_PAIR_PARSR(Book, book, book_parser)
KEY_VAL_PAIR_PARSR(Music, music, music_parser)
KEY_VAL_PAIR_PARSR(Radio, radio, radio_parser)
KEY_VAL_PAIR_PARSR(Record, record, record_parser)
KEY_VAL_PAIR_PARSR(Guide, guide, guide_parser)
KEY_VAL_PAIR_PARSR(LCD, lcd, lcd_parser)
KEY_VAL_PAIR_PARSR(TTS, tts, tts_parser)

static void parseTimer(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar *prop;
	int index;

	prop = xmlGetProp(cur, BAD_CAST "num");
	index = atoi(prop);
	xmlFree(prop);

	parseGeneric(doc, cur, &g_config->setting.timer[index - 1], timer_parser, ARRAY_SIZE(timer_parser));
}

static void parseLanguage(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar *content;

	content = xmlNodeGetContent(cur);
	strlcpy(g_config->setting.lang.lang, content, sizeof(g_config->setting.lang.lang));
	xmlFree(content);
}

int plextalk_setting_read_xml(const char *xml)
{
	static const struct {
		const char *key;
		void (*parse)(xmlDocPtr doc, xmlNodePtr cur);
	} parser[] = {
		{ "book",     parseBook },
		{ "music",    parseMusic },
		{ "radio",    parseRadio },
		{ "record",   parseRecord },
		{ "guide",    parseGuide },
		{ "timer",    parseTimer },
		{ "lcd",      parseLCD },
		{ "language", parseLanguage },
		{ "tts",      parseTTS },
	};

	xmlDocPtr doc;
	xmlNodePtr cur;

	xmlKeepBlanksDefault(0);

	/* Load XML document */
	doc = xmlParseFile(xml);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse \"%s\"\n", xml);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Error: empty document \"%s\"\n", xml);
		xmlFreeDoc(doc);
		return -1;
    }

	CoolShmWriteLock(g_config_lock);

//not do this here!
//	memset(g_config, 0, sizeof(struct plextalk_global_config)); 

    cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		int i;

		for (i = 0; i < ARRAY_SIZE(parser); i++) {
			if (xmlStrEqual(cur->name, BAD_CAST parser[i].key)) {
				parser[i].parse(doc, cur);
				break;
			}
		}

		cur = cur->next;
	}

	CoolShmWriteUnlock(g_config_lock);

	xmlFreeDoc(doc);
	return 0;
}


static int writeGeneric(xmlDocPtr doc, xmlNodePtr root, const char *name,
		void *setting, const setting_key_val *pair, int count)
{
	char buf[12];
	int i;
	xmlNodePtr cur, child;

	cur = xmlNewNode(NULL, BAD_CAST name);
	if (cur == NULL)
		return -1;

	xmlAddChild(root, cur);

	for (i = 0; i < count; i++) {
		child = xmlNewNode(NULL, BAD_CAST pair[i].key);
		if (child == NULL)
			return -1;
		snprintf(buf, sizeof(buf), "%d", *((int *)(setting + pair[i].offset)));
		xmlNodeSetContent(child, BAD_CAST buf);
		xmlAddChild(cur, child);
	}

	return 0;
}

static int writeTimer(xmlDocPtr doc, xmlNodePtr root, int index)
{
	char buf[12];
	int i;
	xmlNodePtr cur, child;
	void *setting;

	cur = xmlNewNode(NULL, BAD_CAST "timer");
	if (cur == NULL)
		return -1;
	snprintf(buf, sizeof(buf), "%d", index + 1);
	xmlSetProp(cur, BAD_CAST "num", BAD_CAST buf);

	xmlAddChild(root, cur);

	setting = &g_config->setting.timer[index];

	for (i = 0; i < ARRAY_SIZE(timer_parser); i++) {
		child = xmlNewNode(NULL, BAD_CAST timer_parser[i].key);
		if (child == NULL)
			return -1;
		snprintf(buf, sizeof(buf), "%d", *((int *)(setting + timer_parser[i].offset)));
		xmlNodeSetContent(child, BAD_CAST buf);
		xmlAddChild(cur, child);
	}

	return 0;
}

static int writeLanguage(xmlDocPtr doc, xmlNodePtr root)
{
	xmlNodePtr cur;

	cur = xmlNewNode(NULL, BAD_CAST "language");
	if (cur == NULL)
		return -1;
	xmlNodeSetContent(cur, BAD_CAST g_config->setting.lang.lang);
	xmlAddChild(root, cur);

	return 0;
}

int plextalk_setting_write_xml(const char *xml)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	int ret = -1;

	xmlKeepBlanksDefault(0);

	doc = xmlNewDoc(BAD_CAST "1.0");
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to new xml document\n");
		return -1;
	}

	root = xmlNewNode(NULL, BAD_CAST "settings");
    if (root == NULL) {
        fprintf(stderr, "Error: unable to new root node\n");
		goto err;
    }

	xmlDocSetRootElement(doc, root);

	CoolShmReadLock(g_config_lock);
	ret = writeGeneric(doc, root, "book", &g_config->setting.book, book_parser, ARRAY_SIZE(book_parser));
	if (!ret)
		ret = writeGeneric(doc, root, "music", &g_config->setting.music, music_parser, ARRAY_SIZE(music_parser));
	if (!ret)
		ret = writeGeneric(doc, root, "radio", &g_config->setting.radio, radio_parser, ARRAY_SIZE(radio_parser));
	if (!ret)
		ret = writeGeneric(doc, root, "record", &g_config->setting.record, record_parser, ARRAY_SIZE(record_parser));
	if (!ret)
		ret = writeGeneric(doc, root, "guide", &g_config->setting.guide, guide_parser, ARRAY_SIZE(guide_parser));
	if (!ret)
		ret = writeTimer(doc, root, 0);
	if (!ret)
		ret = writeTimer(doc, root, 1);
	if (!ret)
		ret = writeGeneric(doc, root, "lcd", &g_config->setting.lcd, lcd_parser, ARRAY_SIZE(lcd_parser));
	if (!ret)
		ret = writeLanguage(doc, root);
	if (!ret)
		ret = writeGeneric(doc, root, "tts", &g_config->setting.tts, tts_parser, ARRAY_SIZE(tts_parser));
	CoolShmReadUnlock(g_config_lock);

	if (ret == -1)
		goto err;

	ret = xmlSaveFormatFileEnc(xml, doc, "utf-8", 1);

err:
	xmlFreeDoc(doc);
	return ret;
}


static const setting_key_val volume_parser[] = {
	{ "book_audio",     offsetof(struct plextalk_volume, book_audio), },
	{ "book_text",      offsetof(struct plextalk_volume, book_text), },
	{ "music",          offsetof(struct plextalk_volume, music), },
	{ "radio",          offsetof(struct plextalk_volume, radio), },
	{ "record",         offsetof(struct plextalk_volume, record), },
	{ "guide",          offsetof(struct plextalk_volume, guide), },
};

int plextalk_volume_read_xml(const char *xml)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	xmlKeepBlanksDefault(0);

	/* Load XML document */
	doc = xmlParseFile(xml);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", xml);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Error: empty document \"%s\"\n", xml);
		xmlFreeDoc(doc);
		return -1;
    }

	CoolShmWriteLock(g_config_lock);

    cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		xmlChar *prop;
		xmlChar *content;
		int val, i, index = 0;

		prop = xmlGetProp(cur, BAD_CAST "output");
		if (prop != NULL) {
			if (xmlStrEqual(prop, "headphone"))
				index = 1;
			xmlFree(prop);
		}

		content = xmlNodeGetContent(cur);
		val = atoi(content);
		xmlFree(content);

		for (i = 0; i < ARRAY_SIZE(volume_parser); i++) {
			if (xmlStrEqual(cur->name, BAD_CAST volume_parser[i].key)) {
				((int *)((void*)(&g_config->volume) + volume_parser[i].offset))[index] = val;
				break;
			}
		}

		cur = cur->next;
	}

	CoolShmWriteUnlock(g_config_lock);

	xmlFreeDoc(doc);
	return 0;
}

int plextalk_volume_write_xml(const char *xml)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	int ret = 0;
	int i;

	xmlKeepBlanksDefault(0);

	doc = xmlNewDoc(BAD_CAST "1.0");
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to new xml document\n");
		return -1;
	}

	root = xmlNewNode(NULL, BAD_CAST "volume");
    if (root == NULL) {
        fprintf(stderr, "Error: unable to new root node\n");
		goto err;
    }

	xmlDocSetRootElement(doc, root);

	CoolShmReadLock(g_config_lock);
	for (i = 0; i < ARRAY_SIZE(volume_parser); i++) {
		xmlNodePtr cur;
		char buf[12];

		if (i < 4) {
			cur = xmlNewNode(NULL, BAD_CAST volume_parser[i].key);
			if (cur == NULL) {
				ret = -1;
				break;
			}
			xmlSetProp(cur, BAD_CAST "output", BAD_CAST "speaker");
			snprintf(buf, sizeof(buf), "%d", ((int *)((void*)(&g_config->volume) + volume_parser[i].offset))[0]);
			xmlNodeSetContent(cur, BAD_CAST buf);
			xmlAddChild(root, cur);

			cur = xmlNewNode(NULL, BAD_CAST volume_parser[i].key);
			if (cur == NULL) {
				ret = -1;
				break;
			}
			xmlSetProp(cur, BAD_CAST "output", BAD_CAST "headphone");
			snprintf(buf, sizeof(buf), "%d", ((int *)((void*)(&g_config->volume) + volume_parser[i].offset))[1]);
			xmlNodeSetContent(cur, BAD_CAST buf);
			xmlAddChild(root, cur);
		} else {
			cur = xmlNewNode(NULL, BAD_CAST volume_parser[i].key);
			if (cur == NULL) {
				ret = -1;
				break;
			}
			snprintf(buf, sizeof(buf), "%d", ((int *)((void*)(&g_config->volume) + volume_parser[i].offset))[0]);
			xmlNodeSetContent(cur, BAD_CAST buf);
			xmlAddChild(root, cur);
		}
	}
	CoolShmReadUnlock(g_config_lock);

	if (ret == -1)
		goto err;

	ret = xmlSaveFormatFileEnc(xml, doc, "utf-8", 1);

err:
	xmlFreeDoc(doc);
	return ret;
}
