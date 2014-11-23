#ifndef __VOLUME_CTRL_H__
#define __VOLUME_CTRL_H__


#define VOL_CTRL_STR_BOOK_AUDIO		"book_audio"
#define VOL_CTRL_STR_BOOK_TEXT		"book_text"
#define VOL_CTRL_STR_MUSIC			"music"
#define VOL_CTRL_STR_RADIO			"radio"
#define VOL_CTRL_STR_RECORD			"record"
#define VOL_CTRL_STR_GUIDE			"guide"


//must called after g_config_open()
void app_volume_ctrl (const char* app);


#endif	//__VOLUME_CTRL_H__
