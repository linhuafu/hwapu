#include <stdio.h>
#include "clang.h"

const char* get_text_lang (int encoding_hint, const char* buffer, int buffer_length)
{
	const char* lang_str;
    enum Language lang = CldDetectLanguage(CHINESE, encoding_hint ,buffer, buffer_length);
	switch (lang) {
		case ENGLISH:
			lang_str = "en_US";
			break;

		case CHINESE:
			lang_str = "zh_CN";
			break;
	
		case HINDI:
			lang_str = "hi_IN";
			break;

		case CHINESE_T:
			lang_str = "zh_TW";
			break;

		default:
			lang_str = "en_US";
			break;
	}

	return lang_str;
}

