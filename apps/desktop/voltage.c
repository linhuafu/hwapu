/**
** 低电检测
** 基本流程:以一定间隔进行电量采样，保存到一个全局数组中
**                       读取时，从这个数组中取值，去掉最高最低，取其余
**                       值的平均值
**电量采样:当VBUS插入时，采样但不保存到数组中。
**                       干电池存在时，采样到的值保存干电池数组中
**                       锂电池存在时，采样到的值保存到锂电池数组中
**                       当VBUS拔出时，两个全局数组均要进行清除
**采样频率: 分两个级别电量比较高的时候采用较长的采样周期
**                        比较低的时候采用比较短的采样周期
**
**/


#include "voltage.h"

static int gBackVoltageArray[SAVE_VOLTAGE_NUM];
static int gMainVoltageArray[SAVE_VOLTAGE_NUM];
static int gBackVoltageIndex;
static int gMainVoltageIndex;
static int main_batt_voltage_min, main_batt_voltage_max;
static int backup_batt_voltage_min, backup_batt_voltage_max;

//初始化，清除两个全局数组
void PlextalkInitBattVol(void)
{
	int i;
	//battery saved value have to set invalid before get voltage
	for(i = 0; i<SAVE_VOLTAGE_NUM; i++)
	{
		gMainVoltageArray[i] = BATTERY_VALUE_INVALID;
		gBackVoltageArray[i] = BATTERY_VALUE_INVALID;
	}
	gBackVoltageIndex = 0;
	gMainVoltageIndex = 0;

	sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MIN_DESIGN, &main_batt_voltage_min);
	sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MAX_DESIGN, &main_batt_voltage_max);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MIN_DESIGN, &backup_batt_voltage_min);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MAX_DESIGN, &backup_batt_voltage_max);

	//if max = 3v min=2v the level = 2.5v
	//backup_vol_level = backup_batt_voltage_min+(backup_batt_voltage_max-backup_batt_voltage_min)/2;
	//if max = 4.15v min=3.2v then level = 3.5v 
	//main_vol_level = main_batt_voltage_min+(main_batt_voltage_max-main_batt_voltage_min)/3;
	
	PlextalkBatteryVoltagGet();
	
	DBGMSG("InitBattVol\n");
}

//main和backup均存在时，以backup为准
//VBUS存在时，不采
static power_type ReadBatteryVoltage(int *vol)
{
	int main_batt_present,backup_batt_present;
	power_type type = POWER_VBUS;

	if(!vol)
		return type;
	
	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	
	if(backup_batt_present) {
		sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_NOW, vol);
		type = POWER_BACKUP;
	} else {
		sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_NOW, vol);
		type = POWER_MAIN;
	}
	DBGMSG("ReadBatteryVoltage vol:%d,type:%d\n",*vol,type);

	return type;
}

//将采到的电量值保存到数组中
static void SaveBatteryVoltage(power_type type ,int voltage)
{

	DBGMSG("SaveBatteryVoltage vol:%d,type:%d\n",voltage,type);
	if(type == POWER_BACKUP) {
		
		if(voltage < backup_batt_voltage_min)
			voltage = backup_batt_voltage_min;
		if(voltage > backup_batt_voltage_max)
			voltage = backup_batt_voltage_max;
		gBackVoltageArray[gBackVoltageIndex] = voltage;
		gBackVoltageIndex= (gBackVoltageIndex + 1) % SAVE_VOLTAGE_NUM;
	} else if (type == POWER_MAIN) {
	
		if(voltage < main_batt_voltage_min)
			voltage = main_batt_voltage_min;
		if(voltage > main_batt_voltage_max)
			voltage = main_batt_voltage_max;
		gMainVoltageArray[gMainVoltageIndex] = voltage;
		gMainVoltageIndex= (gMainVoltageIndex + 1) % SAVE_VOLTAGE_NUM;
	}

}

//进行电量值采集,VBUS时不需要采集
void PlextalkBatteryVoltagGet()
{
	int voltage, value;
	power_type type;
	int vbus_present;

	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);
	if(vbus_present)
		return;
	
	type = ReadBatteryVoltage(&value);
	SaveBatteryVoltage(type,value);
	
}


static int PlextalkGetBatteryVoltage(void)
{
	int i;
	int sum;
	int sumcount;
	int voltage;
	int minVal = BATTERY_VALUE_INVALID;
	int maxVal = 0;

	int main_batt_present,backup_batt_present;
	
	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);

	sum = 0;
	sumcount = 0;

	if(backup_batt_present) {

		for (i=0; i<SAVE_VOLTAGE_NUM; i++)
		{
			if (gBackVoltageArray[i] != BATTERY_VALUE_INVALID)
			{
				minVal = gBackVoltageArray[i]<minVal ? gBackVoltageArray[i] : minVal;
				maxVal = gBackVoltageArray[i]>maxVal ? gBackVoltageArray[i] : maxVal;
				sum += gBackVoltageArray[i];
				sumcount++;
			}
		}
	} else {

		for (i=0; i<SAVE_VOLTAGE_NUM; i++)
		{
			if (gMainVoltageArray[i] != BATTERY_VALUE_INVALID)
			{
				minVal = gMainVoltageArray[i]<minVal ? gMainVoltageArray[i] : minVal;
				maxVal = gMainVoltageArray[i]>maxVal ? gMainVoltageArray[i] : maxVal;
				sum += gMainVoltageArray[i];
				sumcount++;
			}
		}
	}

	DBGMSG("PlextalkGetBatteryVoltage sumcount:%d\n",sumcount);
	DBGMSG("PlextalkGetBatteryVoltage minVal:%d,maxVal:%d,sum:%d\n",minVal,maxVal,sum);

	if (sumcount == SAVE_VOLTAGE_NUM)
	{
		voltage = (sum-minVal-maxVal) >> SAVE_VOLTAGE_SHIFT;		 //SAVE_VOLTAGE_NUM == 16, >>4 == /16
	}
	else if (sumcount != 0)
	{
		voltage = sum / sumcount;
	}
	else
	{
		ReadBatteryVoltage(&voltage);
	}

	DBGMSG("PlextalkGetBatteryVoltage voltage:%d\n",voltage);
	
	return voltage;
}


//获取电量采集的频率，单位为ms
//当前VBUS时，返回0
int PlextalkGetVolSampleInterval()
{
	int main_batt_present,backup_batt_present;
	int vbus_present;
	int vol;
	int ret;
	
	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

	if(vbus_present) {
		return 0;
	}
	vol = PlextalkGetBatteryVoltage();

	if(backup_batt_present) {
		if(vol <= BACK_VOL_LEVEL) {
			ret = BACK_BATTERY_SAMPLE_TIMER2;
		} else {
			ret = BACK_BATTERY_SAMPLE_TIMER1;
		}
	} else {
		if(vol <= MAIN_VOL_LEVEL) {
			ret = MAIN_BATTERY_SAMPLE_TIMER2;
		} else {
			ret = MAIN_BATTERY_SAMPLE_TIMER1;
		}
	}
	DBGMSG("PlextalkGetVolSampleInterval ret:%d\n",ret);
	return ret;
	
}

void PlextalkUpdateBatteryIcon(void)
{
	int main_batt_present = 0, backup_batt_present = 0;
	static int last_icon = SBAR_BATTERY_ICON_NO;
	int icon = SBAR_BATTERY_ICON_ACIN;

	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);

	drycell_supply = 0;

	if (main_batt_present | backup_batt_present) {
		int vbus_present = 0;

		sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

		if (vbus_present /*| ac_present*/) {
			if(backup_batt_present) {//bacup存在时以bacup为准，忽略掉main
				icon = SBAR_BATTERY_ICON_ACIN;//可充电干电池，硬件电路不支持
			} else {
#if 0//锂电池和vbus均存在时，不用判断状态，直接显示为充电				
				char status[32];
				sysfs_read(PLEXTALK_MAIN_BATT_STATUS, status, sizeof(status));
				if (!strcmp(status, "Charging"))
#endif					
				icon = SBAR_BATTERY_ICON_CHARGING;
			}
		} else {
			int batt_voltage_now;
			int batt_voltage_min, batt_voltage_max;

			if (!backup_batt_present) {
				//sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_NOW, &batt_voltage_now);
				batt_voltage_now = PlextalkGetBatteryVoltage();
				batt_voltage_min = MAIN_LOW_VOLTAGE_AUDIO;//main_batt_voltage_min;
				batt_voltage_max = main_batt_voltage_max;
			} else {
				//sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_NOW, &batt_voltage_now);
				batt_voltage_now = PlextalkGetBatteryVoltage();
				batt_voltage_min = BACK_LOW_VOLTAGE_AUDIO;//backup_batt_voltage_min;
				batt_voltage_max = backup_batt_voltage_max;
				drycell_supply = 1;
			}

			icon = (batt_voltage_now - batt_voltage_min)
					/ ((batt_voltage_max - batt_voltage_min) / 5);
			if (icon < 0)
				icon = 0;
			if (icon > 4)
				icon = 4;
			icon += 1;

#if 0
			if ((batt_voltage_now - batt_voltage_min) * 100 < (batt_voltage_max - batt_voltage_min) * 5) {
				if (active_app != M_DESKTOP) {
					NhelperSetAppState(FUNC_MAIN_STR[active_app], 1);
					printf("set %s state: pause\n", FUNC_MAIN_STR[active_app]);
					usleep(1000);
				}
				voice_prompt_abort();
				voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
						tts_lang, tts_role, _("Low battery. Power off in one minute."));
				if (!lowpower) {
					NeuxTimerSetEnabled(lowpowerTimer, GR_TRUE);
					NhelperLowPower();
					lowpower = 1;
				}
			}
#endif			
		}
	}

	char battery_staus[8];
	memset(battery_staus, 0, 8);
	sysfs_read(PLEXTALK_MAIN_BATT_STATUS, battery_staus, 8);

	if (!strncmp(battery_staus, "Full", 4)) {
		DBGMSG("Battery has been fully charged!\n");
		StatusBarSetIcon(STATUSBAR_BATTERY_INDEX, SBAR_BATTERY_ICON_LEVEL4);
	} else if (last_icon != icon) {
		StatusBarSetIcon(STATUSBAR_BATTERY_INDEX, icon);
		last_icon = icon;
	}
	DBGMSG("PlextalkUpdateBatteryIcon icon:%d",icon);
}





int IsLowVoltagePowerOff()
{
	int vbus_present;
	int backup_batt_present;
	int vol;
	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

	if(vbus_present)
		return 0;
	
	//sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	if(backup_batt_present) {
		vol = BACK_LOW_VOLTAGE_POWEROFF;
	} else {
		vol = MAIN_LOW_VOLTAGE_POWEROFF;
	}
	
	return (PlextalkGetBatteryVoltage() < vol);

}

int IsLowVoltageMusic()
{
	int vbus_present;
	int backup_batt_present;
	int vol;
	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

	if(vbus_present)
		return 0;
	
	//sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	if(backup_batt_present) {
		vol = BACK_LOW_VOLTAGE_AUDIO;
	} else {
		vol = MAIN_LOW_VOLTAGE_AUDIO;
	}
	
	return (PlextalkGetBatteryVoltage() < vol);

}




