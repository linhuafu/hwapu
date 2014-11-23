#ifndef __BACKUP_TTS_H__
#define __BACKUP_TTS_H__

//#include "tipvoice.h"

enum tts_type {

	TTS_ENTER_BACKUP = 0,		//tts report enter the backup
	TTS_BG_MP3,            //bg music
	TTS_ELEMENT,			//tts report selectable element
	TTS_ERROR,				//tts report error calculation 
	TTS_ERRBEEP,			//tts report error beep
	TTS_NORBEEP,			//tts report a normal beep to inform a element has been inputed
	TTS_COMPELTE,
	TTS_CANCEL,
	TTS_FAILBACKUP,
	TTS_NOTMEM,
	TTS_REMOVE_MEDIA_READ,
	TTS_REMOVE_MEDIA_WRITE,
	TTS_MEDIA_LOCKED,
	TTS_MAX_FILELEVEL,
	TTS_MEN_KEY,
	TTS_CDDA_WRITE_ERROR,
};


void backup_tts (enum tts_type type, float data);

void backup_tts_init ();

void backup_tts_destroy (void);

void backup_tts_info (char* input_string);

void backup_tts_set_stopped (void);

int backup_tts_get_stopped(void);

int *backup_get_ttsstate(void);

#endif
