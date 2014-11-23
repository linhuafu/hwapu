#ifndef _VOLUME_H_
#define _VOLUME_H_

#include "nw-main.h"

//extern int file_list_mode;
extern int info_running;
extern int recording_active;
extern int recording_now;//appk
extern int drycell_supply;//appk
extern struct plextalk_radio radio_cache;
extern struct plextalk_volume volume_cache;

void
InitVolume(void);
//void
//SetVolume(int app);
void
AppChangeVolumeHook(int app, int is_new);
int
HandleVolumeKey(int up);
void
HpSwitch(int online);

void
HandleFilelistModeMsg(int active);
void
HandleBookContentMsg(int is_audio);
void
HandleGuideVolMenuMsg(int active);
void
HandleRecordingMsg(int active);
void
HandleRecordingNowMsg(int active);//appk
void
HandleGuideVolChangeMsg(int vol);
void
HandleRadioOutputMsg(int hp);
void
HandleuInfoMsg(int active);

#endif // _VOLUME_H_
