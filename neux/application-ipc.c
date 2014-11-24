//#define OSD_DBG_MSG
#include "nc-err.h"
#include "neux.h"
#include "desktop-ipc.h"
#include "application-ipc.h"


static inline void
NhelperAppMsgSimple(const char *app, int msgID, int val)
{
	neux_msg_t msg;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(int);
	msg.msg.msg.msgId = msgID;
	*((int*)msg.msg.msg.msgTxt) = val;

	NeuxMsgSend(app, &msg);
}

void NhelperReschduleTimer(int timer_nr)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_TIMER_MSG_ID, timer_nr);
}

void NhelperReschduleOffTimer(int enable)//appk
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_OFFTIMER_MSG_ID, enable);
}


void NhelperFilelistMode(int active)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_FILELIST_MSG_ID, active);
}

void NhelperBookContent(int is_audio)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_BOOK_CONTENT_MSG_ID, is_audio);
}

void NhelperMenuGuideVol(int active)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_MENU_GUIDE_VOL_MSG_ID, active);
}

void NhelperRecordingActive(int active)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_RECORD_ACTIVE_MSG_ID, active);
}

void NhelperRecordingNow(int active)//appk
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_RECORD_PAUSE_MSG_ID, active);
}


void NhelperRadioRecordingActive(int active)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_RADIO_RECORD_ACTIVE_MSG_ID, active);
}

void NhelperInfoActive(int active)
{
	NhelperAppMsgSimple(PLEXTALK_IPC_NAME_DESKTOP, PLEXTALK_INFO_MSG_ID, active);
}


void NhelperSetAppStateTwo(const char *app, int pause, int spec_info)
{
	neux_msg_t msg;
	app_state_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_state_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_STATE;
	rqst->pause = pause;
	rqst->spec_info = spec_info;

	NeuxMsgSend(app, &msg);
}

void NhelperTerminateInfo(const char *app)
{
	neux_msg_t msg;
	app_terminate_info_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_terminate_info_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_TERMINATE_INFO;

	NeuxMsgSend(app, &msg);
}

void NhelperBookmarkOp(const char *app, APP_BOOKMARK_OPERATION op)
{
	neux_msg_t msg;
	app_bookmark_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_bookmark_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_BOOKMARK;
	rqst->op = op;

	NeuxMsgSend(app, &msg);
}

void NhelperDeleteOp(const char *app, APP_DELETE_OPERATION op)
{
	neux_msg_t msg;
	app_delete_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_delete_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_DELETE_CONTENT;
	rqst->op = op;

	NeuxMsgSend(app, &msg);
}

/* broadcast all apps guide volume has changed */
void NhelperGuideVolumeChange(int vol)
{
	neux_msg_t msg;
	app_volume_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_volume_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_GUIDE_VOL_CHANGE;
	rqst->volume = vol;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperGuideSpeedChange(int speed)
{
	neux_msg_t msg;
	app_speed_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_speed_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_GUIDE_SPEED_CHANGE;
	rqst->speed = speed;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperGuideEquChange(int equ)
{
	neux_msg_t msg;
	app_equ_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_equ_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_GUIDE_EQU_CHANGE;
	rqst->equ = equ;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperSetSpeed(const char *app, int speed)
{
	neux_msg_t msg;
	app_speed_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_speed_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_SPEED;
	rqst->speed = speed;

	NeuxMsgSend(app, &msg);
}

void NhelperSetRepeat(const char *app, int repeat)
{
	neux_msg_t msg;
	app_repeat_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_repeat_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_REPEAT;
	rqst->repeat = repeat;

	NeuxMsgSend(app, &msg);
}

void NhelperSetEqu(const char *app, int equ, int is_audio)
{
	neux_msg_t msg;
	app_equ_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_equ_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_EQU;
	rqst->equ = equ;
	rqst->is_audio = is_audio;

	NeuxMsgSend(app, &msg);
}

void NhelperSetRadioOutput(const char *app, int output)
{
	neux_msg_t msg;
	app_output_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_output_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_RADIO_OUTPUT;
	rqst->output = output;

	NeuxMsgSend(app, &msg);
}

void NhelperSetRecordTarget(const char *app, int saveto)
{
	neux_msg_t msg;
	app_target_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_target_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_RECORD_TARGET;
	rqst->saveto = saveto;

	NeuxMsgSend(app, &msg);
}

void NhelperSetFontsize(int fontsize)
{
	neux_msg_t msg;
	app_fontsize_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_fontsize_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_FONTSIZE;
	rqst->fontsize = fontsize;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperSetTheme(const char *app, int index, int confirm)	//modify by ztz
{
	neux_msg_t msg;
	app_theme_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_theme_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_THEME;
	rqst->index = index;
	rqst->confirm = confirm;				//added by ztz
		
	NeuxMsgSend(app, &msg);
}

void NhelperBroadcastSetTheme(const char *exclude, int index)
{
	neux_msg_t msg;
	app_theme_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_theme_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_THEME;
	rqst->index = index;
	rqst->confirm = 1;		//added by ztz : set all theme in confirm mode

	NeuxMsgBroadcast(exclude, &msg);
}

void NhelperSetLanguage(const char *lang)
{
	neux_msg_t msg;
	app_language_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_LANGUAGE;
	strlcpy(rqst->lang, lang, PLEXTALK_LANG_MAX);
	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(rqst->type) + strlen(rqst->lang) + 1;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperSetTTSVoiceSpecies(int female)
{
	neux_msg_t msg;
	app_voice_species_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_voice_species_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_TTS_VOICE_SPECIES;
	rqst->female = female;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperSetTTSChinese(int contonese)
{
	neux_msg_t msg;
	app_chinese_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_chinese_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_SET_TTS_CHINESE_STANDARD;
	rqst->contonese = contonese;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperRsyncSettings(void)
{
	neux_msg_t msg;
	app_resync_settings_rqst_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_resync_settings_rqst_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_RESYNC_SETTINGS;

	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperLowPower(void)
{
	neux_msg_t msg;
	app_low_power_t *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_low_power_t);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_LOW_POWER;

	NeuxMsgBroadcast(NULL, &msg);
}


//lhf 
void NhelperBookMarkResult(APP_BOOKMARK_RESULT result, int addtion)
{
	neux_msg_t msg;
	app_bookmark_result *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_bookmark_result);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_BOOKMARK_RESULT;
	rqst->result = result;
	rqst->addtion = addtion;
	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperDeleteResult(APP_DELETE_RESULT result)
{
	neux_msg_t msg;
	app_delete_result *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_delete_result);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_DELETE_RESULT;
	rqst->result = result;
	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperJumpPage(unsigned long ulpage)
{
	neux_msg_t msg;
	app_jump_page *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_jump_page);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_JUMP_PAGE;
	rqst->page = ulpage;
	NeuxMsgBroadcast(NULL, &msg);
}

void NhelperJumpPageResult(APP_JUMP_RESULT result)
{
	neux_msg_t msg;
	app_jump_page_result *rqst = msg.msg.msg.msgTxt;

	msg.bytes = sizeof(msg.msg.msg.msgId) + sizeof(app_jump_page_result);
	msg.msg.msg.msgId = PLEXTALK_APP_MSG_ID;
	rqst->type = APPRQST_JUMP_PAGE_RESULT;
	rqst->result = result;
	NeuxMsgBroadcast(NULL, &msg);
}
//lhf
