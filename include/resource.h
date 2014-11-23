#ifndef __RESOURCE_H__
#define __RESOURCE_H__


/*Ïà¶ÔÂ·¾¶*/
#define RES_PATH   "res"
#define DATA_PATH "data"
#define BIN_PATH "bin"


#define THEME0_PATH "/theme_white_black"
#define THEME1_PATH "/theme_black_white"
#define THEME2_PATH "/theme_black_yellow"
#define THEME3_PATH "/theme_yellow_black"
#define THEME4_PATH "/theme_yellow_blue"
#define THEME5_PATH "/theme_blue_yellow"
#define THEME6_PATH "/theme_blue_white"
#define THEME7_PATH "/theme_white_blue"
#define THEME8_PATH "/theme_orange_black"
#define THEME9_PATH "/theme_black_orange"
#define THEME10_PATH "/theme_red_white"
#define THEME11_PATH "/theme_white_red"
#define THEME12_PATH "/theme_green_black"
#define THEME13_PATH "/theme_black_green"
#define THEME14_PATH "/theme_purple_white"
#define THEME15_PATH "/theme_white_purple"


#define ROOT_UDISK DATA_PATH"/mtd"
#define ROOT_SDCAR DATA_PATH"/mmc"
#define ROOT_STORAGE DATA_PATH"/storage"

#define UDISK_PATH_NAME "mtd"
#define SDCARD_PATH_NAME "mmc"
#define STORAGE_PATH_NAME "storage"

#define MACHINE_UDISK_PATH_NAME "mmcblk0p2"
#define MACHINE_SDCARD_PATH_NAME "mmcblk1"
#define MACHINE_STORAGE_PATH_NAME "sda"


#define BOOK_PATH DATA_PATH"/book"
#define MUSIC_PATH DATA_PATH"/music"

#define GLOBAL_CONFIG_FILE "config/global.xml"
#define BOOK_CONFIG_FILE "config/book.xml"
#define MUSIC_CONFIG_FILE "config/music.xml"
#define RADIO_CONFIG_FILE "config/radio.xml"


#define BOOK_PRESET_FLAT_FILE "preset/book_flat"
#define BOOK_PRESET_BASS_FILE "preset/book_bass"
#define BOOK_PRESET_MID_FILE "preset/book_mid"
#define BOOK_PRESET_TREBLE_FILE "preset/book_treble"


#define COMBO_LIST_BG   "/widget/combo_list_bg.bmp"
#define COMBO_LIST_SEL   "/widget/state0.bmp"
#define COMBO_LIST_UNSEL   "/widget/state1.bmp"

#define BAR_BG_PATH   "/bar/scroll_bar_bg_scale.png"
#define BAR_BLOCK_PATH "/bar/scroll_bar_block_scale.png"


#define VOL_PROGRESS_BG   "/widget/volume-bg.png"
#define VOL_PROGRESS_BLOCK   "/widget/block.png"

#define FILTER_FOLDER  "filter/folder.png"
#define FILTER_DAISY  "filter/daisy.png"
#define FILTER_DOCX  "filter/docx.png"
#define FILTER_DOC  "filter/doc.png"
#define FILTER_TEXT  "filter/text.png"
#define FILTER_MP3  "filter/music.png"
#define FILTER_WAV  "filter/music.png"




#define VOICE_NAME  "voice.mp3"
#define VOICE2_NAME  "voice2.mp3"
#define VOICE3_NAME  "voice3.wav"

//#define RECORD_NAME  RES_PATH"/record_test.wav"
#define RECORD_NAME  "/tmp/record_test.wav"

/*error*/
#define TIPS_AUDIO_BU  "audio_bu.wav"
/*normal*/
#define TIPS_AUDIO_PU  "audio_pu.wav"

#define TIPS_AUDIO_CANCEL  "audio_cancel.wav"
#define TIPS_AUDIO_INPUT  "audio_input.mp3"
#define TIPS_AUDIO_KON  "audio_kon.wav"
#define TIPS_AUDIO_POWER_OFF  "audio_power_off.wav"
#define TIPS_AUDIO_POWER_ON  "audio_power_on.wav"
#define TIPS_AUDIO_USB_OFF  "audio_usb_off.mp3"
#define TIPS_AUDIO_USB_ON  "audio_usb_on.mp3"
#define TIPS_AUDIO_WAIT_BEEP  "audio_wait_beep.mp3"
#define TIPS_AUDIO_YES  "audio_yes.wav"

#define ALARM_SOUND_A  "Sound_A.wav"
#define ALARM_SOUND_B  "Sound_B.wav"
#define ALARM_SOUND_C  "Sound_C.wav"


#define	REPEAT_NEVER  (1<< 8)
#define	REPEAT_EVERY_DAY (1<< 7)

#define	REPEAT_SUNDAY  (1<< 0)
#define	REPEAT_MONDAY  (1<< 1)
#define	REPEAT_TUESDAY  (1<< 2)
#define	REPEAT_WEDNESDAY  (1<< 3)
#define	REPEAT_THURSDAY  (1<< 4)
#define	REPEAT_FRIDAY  (1<< 5)
#define	REPEAT_SATURDAY  (1<< 6)

#define REPEAT_TYPE_NEVER 0
#define REPEAT_TYPE_EVERY_DAY 1
#define REPEAT_TYPE_WEEK  2


#define BACKUP_PATH   "/tmp/backup_path"


#endif
