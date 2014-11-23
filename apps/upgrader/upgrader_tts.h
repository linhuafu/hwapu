#ifndef   _upgrader_tts_
#define  _upgrader_tts_

enum {
	Guide_VerUp_UpdaterFound,
	Guide_VerUp_UpdaterFoundError,
	Guide_VerUp_StartUpdate,
	Guide_VerUp_USBPowerConnect,
	Guide_VerUp_Caution1,
	Guide_VerUp_Caution2,
	Guide_VerUp_Finish,
	Guide_VerUp_Restart,
	Guide_VerUp_CapaFullError,
	Guide_VerUp_WriteProtectError,
	Guide_VerUp_LockError,
	Guide_FileManage_MemoryReadError,
	Guide_FileManage_MemoryWriteError,
	Guide_VerUp_VerifyError,
	Guide_Common_BeepError,
	Max_num
};

char *tts_text[Max_num]={
		{"Available update was found"},
		{"-"},
		{"Starting update"},
		{"Connect AC adaptor for updating"},
		{"Please do not use or turn the player off until the player has restarted itself automatically"},
		{"Please do not use or turn the player off or remove SD card until the player has restarted itself automatically"},
		{"Update is completed successfully"},
		{"Restarting,Please wait"},
		{"Not enough space on the target media ,Processing is interrupted"},
		{"Read only media ,Processing is interrupted"},
		{"The media is locked, Processing is interrupted"},
		{"Read error.It may not access the media correctly"},
		{"Write error,It may not access the media correctly"},
		{"Failed to verify,Processing is interrupted"},
		{"AUDIO_BU.wav"}
};

void upgrader_init_tts_prop();
void upgrader_tts_set_stopped (void);
int upgrader_tts_get_stopped(void);
void upgrader_call(int  sig);
void upgrader_tts_init ();
void upgrader_tts_destroy (void);
void upgrader_play_tts (char *data);
void Wait_tts_play_over();
void upgrader_play_error_wav ();

#endif



