#ifndef _TIMER_H_
#define _TIMER_H_

#include "nw-main.h"

void
InitTimer(WIDGET * owner);
void
TimerSettingChange(int index);
int 
isOffTimerSettingEnable(void);
void
OffTimerSettingChange(int enable,int elapse,int schdule);
void
InitOffTimer(void);	
void
HandleSetTimeMsg(void);
void
TimerSuspendEnterHook(void);
void
TimerSuspendExitHook(void);
int
HasAlarmTimerPending(void);
void
TimerPoweroffHook(void);
void
disableAllTimer(void);
#endif // _TIMER_H_
