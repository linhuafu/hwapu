#ifndef _IPC_H_
#define _IPC_H_

#include "plextalk-i18n.h"

typedef enum {
	APPRQST_SET_STATE,
	APPRQST_TERMINATE_INFO,
	APPRQST_BOOKMARK,
	APPRQST_DELETE_CONTENT,
	APPRQST_GUIDE_VOL_CHANGE,
	APPRQST_GUIDE_SPEED_CHANGE,
	APPRQST_GUIDE_EQU_CHANGE,
	APPRQST_SET_SPEED,
	APPRQST_SET_REPEAT,
	APPRQST_SET_EQU,
	APPRQST_SET_RADIO_OUTPUT,
	APPRQST_SET_RECORD_TARGET,
	APPRQST_SET_FONTSIZE,
	APPRQST_SET_THEME,
	APPRQST_SET_LANGUAGE,
	APPRQST_SET_TTS_VOICE_SPECIES,
	APPRQST_SET_TTS_CHINESE_STANDARD,
	APPRQST_RESYNC_SETTINGS,
	APPRQST_LOW_POWER,
	APPRQST_BOOKMARK_RESULT,		//lhf
	APPRQST_DELETE_RESULT,	//lhf
	APPRQST_JUMP_PAGE,	//lhf
	APPRQST_JUMP_PAGE_RESULT,	//lhf
} APP_REQUEST;

typedef struct
{
	APP_REQUEST	type;
	int pause;
	int spec_info;		//bad!!!: for info pause in every app
} app_state_rqst_t;

typedef struct
{
	APP_REQUEST	type;
} app_terminate_info_rqst_t;

typedef enum {
	APP_BOOKMARK_SET,
	APP_BOOKMARK_DELETE,
	APP_BOOKMARK_DELETE_ALL,
} APP_BOOKMARK_OPERATION;

typedef struct
{
	APP_REQUEST	type;
	APP_BOOKMARK_OPERATION op;
} app_bookmark_rqst_t;

typedef enum {
	APP_DELETE_TRACK,
	APP_DELETE_ALBUM,

	APP_DELETE_TITLE           = APP_DELETE_TRACK,

	APP_DELETE_CURRENT_CHANNEL = APP_DELETE_TRACK,
	APP_DELETE_ALL_CHANNELS    = APP_DELETE_ALBUM,

} APP_DELETE_OPERATION;

typedef struct
{
	APP_REQUEST	type;
	APP_DELETE_OPERATION op;
} app_delete_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int volume;
} app_volume_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int speed;
} app_speed_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int repeat;
} app_repeat_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int equ;
	int is_audio;	/* only meaningful for BOOK */
} app_equ_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int output;
} app_output_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int saveto;
} app_target_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int fontsize;
} app_fontsize_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int index;
	int confirm;		//added by ztz
} app_theme_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	char lang[PLEXTALK_LANG_NAME_MAX];
} app_language_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int female;
} app_voice_species_rqst_t;

typedef struct
{
	APP_REQUEST	type;
	int contonese;
} app_chinese_rqst_t;

typedef struct
{
	APP_REQUEST	type;
} app_resync_settings_rqst_t;

typedef struct
{
	APP_REQUEST	type;
} app_low_power_t;


//lfh start
/*bookmark result*/
typedef enum {
	/*set bookmark result*/
	APP_BOOKMARK_SET_OK,
	APP_BOOKMARK_EXIST_ERR,
	APP_BOOKMARK_UPPER_LIMIT_ERR,
	APP_BOOKMARK_SET_ERR,
	
	/*del bookmark result*/
	APP_BOOKMARK_DEL_OK,
	APP_BOOKMARK_NOT_EXIST_ERR,
	APP_BOOKMARK_DEL_ERR,

	APP_BOOKMARK_READ_ONLY_ERR,
	APP_BOOKMARK_MEDIA_LOCKED_ERR,
} APP_BOOKMARK_RESULT;

typedef struct
{
	APP_REQUEST	type;
	APP_BOOKMARK_RESULT result;
	int addtion;
} app_bookmark_result;


/*delete result*/
typedef enum {
	/*delete result*/
	APP_DELETE_OK,
	APP_DELETE_ERR,
	APP_DELETE_READ_ONLY_ERR,
	APP_DELETE_MEDIA_LOCKED_ERR,
} APP_DELETE_RESULT;

typedef struct
{
	APP_REQUEST	type;
	APP_DELETE_RESULT result;
} app_delete_result;

typedef struct
{
	APP_REQUEST	type;
	unsigned long page;
} app_jump_page;

/*jump result*/
typedef enum {
	/*jump result*/
	APP_JUMP_OK = 0,
	APP_JUMP_ERR = -1,
	APP_JUMP_NOT_EXIST_PAGE = -2,
} APP_JUMP_RESULT;

typedef struct
{
	APP_REQUEST	type;
	APP_JUMP_RESULT result;
} app_jump_page_result;

//lhf end
typedef union
{
	APP_REQUEST	type;
	app_state_rqst_t state;
	app_terminate_info_rqst_t terminate_info;
	app_bookmark_rqst_t bookmark;
	app_delete_rqst_t delete; 
	app_volume_rqst_t guide_vol;
	app_speed_rqst_t guide_speed;
	app_equ_rqst_t guide_equ;
	app_speed_rqst_t speed;
	app_repeat_rqst_t repeat;
	app_equ_rqst_t equ;
	app_output_rqst_t radio_output;
	app_target_rqst_t record_target;
	app_fontsize_rqst_t fontsize;
	app_theme_rqst_t theme;
	app_language_rqst_t language;
	app_voice_species_rqst_t tts_voice_species;
	app_chinese_rqst_t tts_chinese;
	app_resync_settings_rqst_t resync_settings;
	app_low_power_t low_power;
	app_bookmark_result bookmark_result; 	//lhf
	app_delete_result delete_result; 	//lhf
	app_jump_page jump_page;	//lhf
	app_jump_page_result jump_page_result; //lhf
} app_rqst_t;


void NhelperReschduleTimer(int timer_nr);

void NhelperReschduleOffTimer(int enable);//appk

void NhelperFilelistMode(int active);

void NhelperBookContent(int is_audio);

void NhelperMenuGuideVol(int active);

void NhelperRecordingActive(int active);

void NhelperRecordingNow(int active);//appk

void NhelperRadioRecordingActive(int active);

void NhelperInfoActive(int active);


//void NhelperSetAppState(const char *app, int pause);

#define NhelperSetAppState(app, pause) do { \
	NhelperSetAppStateTwo(app, pause, 0); \
} while (0)
void NhelperSetAppStateTwo(const char *app, int pause, int spec_info);

void NhelperTerminateInfo(const char *app);

void NhelperBookmarkOp(const char *app, APP_BOOKMARK_OPERATION op);

void NhelperDeleteOp(const char *app, APP_DELETE_OPERATION op);

/* broadcast all apps guide volume has changed */
void NhelperGuideVolumeChange(int volume);

void NhelperGuideSpeedChange(int speed);

void NhelperGuideEquChange(int equ);

void NhelperSetSpeed(const char *app, int speed);

void NhelperSetRepeat(const char *app, int repeat);

void NhelperSetEqu(const char *app, int equ, int is_audio);

void NhelperSetRadioOutput(const char *app, int output);

void NhelperSetRecordTarget(const char *app, int target);

void NhelperSetFontsize(int fontsize);

void NhelperSetTheme(const char *app, int index, int confirm);	//ztz

void NhelperBroadcastSetTheme(const char *exclude, int index);

void NhelperSetLanguage(const char *lang);

void NhelperSetTTSVoiceSpecies(int female);

void NhelperSetTTSChinese(int contonese);

void NhelperRsyncSettings(void);

void NhelperLowPower(void);

void NhelperBookMarkResult(APP_BOOKMARK_RESULT result, int addtion); 	//lhf

void NhelperDeleteResult(APP_DELETE_RESULT result);	//lhf

void NhelperJumpPage(unsigned long ulpage); 	//lhf

void NhelperJumpPageResult(APP_JUMP_RESULT result);	//lhf
#endif //_STATUSBAR_H_
