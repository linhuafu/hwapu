#ifndef _VOLTAGE_H_
#define _VOLTAGE_H_


#include <signal.h>

#include <nano-X.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nw-statusbar.h"
#include "sysfs-helper.h"
#include "volume.h"

typedef enum _POWER_TYPE {

	POWER_VBUS = 0,
	POWER_BACKUP,//干电池
	POWER_MAIN,//锂电池
}power_type;

#define SAVE_VOLTAGE_SHIFT          1
#define SAVE_VOLTAGE_NUM            4   // 1 << SAVE_VOLTAGE_SHIFT

#define BATTERY_VALUE_INVALID       0x7FFFFFFF

#define HALF_VOLUME_WHEN_BACKUP		1

//将电压电量值写死
#define MAIN_VOL_LEVEL					(3500000)
#define BACK_VOL_LEVEL					(2500000)

//关机电压
#define BACK_LOW_VOLTAGE_POWEROFF		(2280000)
//mp3播放电压
#if HALF_VOLUME_WHEN_BACKUP
#define BACK_LOW_VOLTAGE_AUDIO			(2300000)//(2650000)//(2400000)	因为干电池升压IC没有稳压功能，被瞬时拉低到掉电			 
#else
#define BACK_LOW_VOLTAGE_AUDIO			(2650000)//(2400000) 因为干电池升压IC没有稳压功能，被瞬时拉低到掉电			 
#endif
//锂电池关机电压
#define MAIN_LOW_VOLTAGE_POWEROFF		(3250000)
//锂电池mp3播放电压
#define MAIN_LOW_VOLTAGE_AUDIO			(3410000)

#define MAIN_BATTERY_SAMPLE_TIMER1		(60*1000)// 1min
#define MAIN_BATTERY_SAMPLE_TIMER2		(10*1000)// 10s

#define BACK_BATTERY_SAMPLE_TIMER1		(60*1000)// 1min
#define BACK_BATTERY_SAMPLE_TIMER2		(6*1000)// 6s


void PlextalkInitBattVol(void);
void PlextalkBatteryVoltagGet();
int PlextalkGetVolSampleInterval();
void PlextalkUpdateBatteryIcon(void);
int IsLowVoltagePowerOff();
int IsLowVoltageMusic();

#endif //_VOLTAGE_H_
