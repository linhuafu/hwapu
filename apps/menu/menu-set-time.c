/*
 *	Copyright(C) 2006 Neuros Technology International LLC.
 *				 <www.neurostechnology.com>
 *
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, version 2 of the License.
 *
 *
 *	This program is distributed in the hope that, in addition to its
 *	original purpose to support Neuros hardware, it will be useful
 *	otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Filedialog gui routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-08-29 NW
 *
 */

#define OSD_DBG_MSG
#include "nc-err.h"

#include "widgets.h"
#include "desktop-ipc.h"

#include "plextalk-config.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-keys.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include <microwin/device.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <sys/types.h>
#include "rtc-helper.h"



#define _(S)			gettext(S)

#define FORMAT_12H 0
#define FORMAT_24H 1


#define YEAR_SET_UI		0
#define MONTH_SET_UI	1
#define DATE_SET_UI		2
#define HOUR_SET_UI		3
#define MINUTE_SET_UI	4
#define AMPM_SET_UI		5

#define SET_LONG_PRESS_TIME		300 //one increment per 300ms.)
#define SET_EXIT_TIME			500
#define SET_INPUT_TIME			(3*1000) // 2S


struct menu_set_time_date {
	TIMER* longPressedTimer;
	TIMER* ExitTimer;
	TIMER* InputTimer;
	LABEL* pTitle;
	LABEL* sValue;
	int timeformat;
	int max_hour;
	int max_ui;
	int exitflag;
	int cur_ui;
	int value;
	int isaddkey;
	int isInLongPressed;

	int year;
	int month;
	int day;
	int hour;
	int minute;
	int ampm;

};

extern int voice_prompt_end;
extern int menu_exit;

static int endOfModal = -1;
static int iMsgReturn = MSGBX_BTN_CANCEL;


static struct menu_set_time_date s_app;

static char str_buf[512];
// form handle globally available.
static FORM * form;
static int tts_exit_state;
static int menu_exit_state;
static int key_click_flag;

/* The path for each beep */
//static char bu_path[256];
//static char pu_path[256];
//static char cancel_path[256];


static char* set_title_text[AMPM_SET_UI+2];
static char* set_prompt_text[AMPM_SET_UI+2];

/*
= {
	_("Year"),
	_("Month"),
	_("Date"),
	_("Hour"),
	_("Minute"),
	_("AM/PM"),
	NULL
};
*/

//声明
static void
setime_ShowTheLabel(int cur_ui);
static int setime_IncrementOnUpKey(int needVoice);
static int setime_DecrementOnDownKey(int needVoice);
static void
setime_CreateExitTimer();
static void setime_TTS(const char* str,const char* lead);
static void setime_TTS_Beep(const char* lead,int abort);


static char* setime_getSringFormat(char *beforeformat,int cur_ui,char *afterformat)
{
	static char format_str[100];
	char buf[10];
	memset(format_str,0x00,100);
	memset(buf,0x00,10);

	switch(cur_ui) {
		case YEAR_SET_UI:
			strcpy(buf,"%04d");
			break;
		case MONTH_SET_UI:
			strcpy(buf,"%02d");
			break;
		case DATE_SET_UI:
			strcpy(buf,"%02d");
			break;
		case HOUR_SET_UI:
			strcpy(buf,"%02d");
			break;
		case MINUTE_SET_UI:
			strcpy(buf,"%02d");
			break;
		case AMPM_SET_UI:
			strcpy(buf,"%s");
			break;
		default:
			strcpy(buf,"%s");
			break;
	}

	if(beforeformat) {
		strcpy(format_str,beforeformat);
		strcat(format_str,buf);
	} else {
		strcpy(format_str,buf);
	}
	if(afterformat) {
		strcat(format_str,afterformat);
	}

	
	DBGMSG("setime_getSringFormat str:%s\n",format_str);

	return format_str;
}

int is_leap_year(int year)
{
	if(year < 0){
		return 0;
	}

	if ( ((year % 4 == 0) && !(year % 100 == 0))
					|| (year % 400 == 0) ){
		return 1;
	}

	return 0;
}

/*month: 0~11*/
int month_days(int year, int month)
{
	if(year < 0 || month <1 || month > 12){
		return 0;
	}

	switch(month){
	case 1:/*january*/
	case 3:/*March*/
	case 5:
	case 7:
	case 8:
	case 10:/*December*/
	case 12:
		return 31;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	case 2:
	{
		if(is_leap_year(year)){
			return 29;
		}
		else{
			return 28;
		}
	}
	default:
		return 0;
	}

	return 0;
}

//将24小时制转化为12小时制，并得到ampm
static int
setime_Transform24To12Time(int hour,int *ampm)
{
	*ampm = (hour>=12);
	if(hour == 0)
	{
		hour = 12;
	}
	else if(hour > 12)
	{
		hour -= 12;
	}
	else
	{
		//remain
	}
	return hour;
}
//将12小时制转化为24小时制
static int
setime_Transform12To24Time(int hour,int ampm)
{
	if(hour == 12)
	{
		if(ampm)
			return 12;
		else
			return 0;
	}

	if(ampm)
	{
		return hour+12;
	}
	return hour;
}




static void 
setime_setRealTime()
{
	sys_time_t stm;
	
	stm.sec = 0;
	stm.min = s_app.minute;
	stm.hour = s_app.hour;
	stm.day = s_app.day;
	stm.mon = s_app.month;
	stm.year = s_app.year;
	
	sys_set_time(&stm);
}

static void
setime_getTimeStr(char *buf,int size)
{
	sys_time_t stm;
	int hour_format,pattern;
	struct plex_info* info_id;
	char timebuf[100];
	int hour;
	const char* weekdayStr[] =
	{
		_("Sunday"),
		_("Monday"),
		_("Tuesday"),
		_("Wednesday"),
		_("Thursday"),
		_("Friday"),
		_("Saturday"),
	};

	if(!buf || (size < 20))
		return;

	CoolShmReadLock(g_config_lock);
	hour_format = g_config->hour_system;
	pattern = g_config->time_format;
	CoolShmReadUnlock(g_config_lock);

	memset(timebuf,0x00,100);

	sys_get_time(&stm);
	hour = stm.hour;//0-23

	if(hour_format == 0)//12h
	{
		int ampm = 0;
		hour = setime_Transform24To12Time(hour,&ampm);
		sprintf(timebuf,"%02d:%02d %s ", hour, stm.min, ampm?_("PM"):_("AM"));

	}
	else// 24h
	{
		sprintf(timebuf,"%02d:%02d ", hour, stm.min);
	}

	int weekday = stm.wday;

	if(weekday < 0 || weekday > 6)
	{
		DBGMSG("errror weekday\n");
		weekday = 0;
	}
	DBGMSG("weekday = %d\n", weekday);

	switch(pattern)
	{
	case 0: //0: hour, minute, year, month, day, weekday 
		snprintf(buf, size, "%s%04d %02d %02d %s",
			timebuf, stm.year, stm.mon, stm.day, weekdayStr[weekday]);
	break;
	case 1: // 1: hour, minute, weekday, month, day, year
		snprintf(buf, size, "%s%s %02d %02d %04d",
			timebuf, weekdayStr[weekday], stm.mon, stm.day, stm.year);
	break;
	//case 2:// 2: hour, minute, weekday, day, month, year
	default:
		snprintf(buf, size, "%s%s %02d %02d %04d",
			timebuf, weekdayStr[weekday], stm.day, stm.mon, stm.year);
	break;
	}
	DBGMSG("setime_getTimeStr buf = %s\n", buf);

}


static void 
set_time_func (int sec, int min, int hour, int mday, int mon, int year)
{
	struct rtc_time set_rtc_time;
	struct tm tm_time;
	time_t t_time;
	int fd;

	fd = open("/dev/rtc0", O_WRONLY);
	if (fd < 0) {
		perror("open rtc err!!");
		return;
	}
	tm_time.tm_sec = sec;
	tm_time.tm_min = min;
	tm_time.tm_hour = hour;
	tm_time.tm_mday = mday;
	tm_time.tm_mon = mon;
	tm_time.tm_year = year;

	ioctl(fd, RTC_SET_TIME, &tm_time);
	close(fd);

	t_time = mktime(&tm_time);
	stime(&t_time);
}



//exitflag 退出标志 : 是确认退出还是取消退出
//0 :确认退出 
//-1 :取消退出
static void
setime_setExit(int exitflag)
{
	int screensave;
	char datecmd[256];
	char prompt_str[256];
	
	DBGMSG("Get setime_setExit exitflag:%d\n",exitflag);

	TimerDisable(s_app.InputTimer);
	
	voice_prompt_abort();
	s_app.exitflag = exitflag;
	if(!exitflag)
	{
		if(FORMAT_12H == s_app.timeformat){
			//s_app.hour += s_app.ampm*12;
			s_app.hour = setime_Transform12To24Time(s_app.hour,s_app.ampm);
		}

#if 1		
		snprintf(datecmd, 256, "(date -s \'%d-%d-%d %d:%d\' ; hwclock -w)",
			s_app.year,s_app.month,s_app.day,s_app.hour,s_app.minute);

		//disable the screen off
		CoolShmReadLock(g_config_lock);
		screensave = g_config->setting.lcd.screensaver;
		CoolShmReadUnlock(g_config_lock);

		if(screensave != PLEXTALK_SCREENSAVER_ACTIVENOW
			&& screensave != PLEXTALK_SCREENSAVER_DEACTIVE) {
			printf("Pause screensaver firset!\n");
			NeuxSsaverPauseResume(1);
		}

		//use this instead
		printf("s_app.year = %d\n", s_app.year);
		//set_time_func(0, s_app.minute, s_app.hour, s_app.day, s_app.month, s_app.year - 1970);
		system(datecmd);
		//usleep(10000);				//等待0.01s，rtc设置完毕后，通知桌面读取最新时间并刷新
		usleep(300 * 1000);			//sleep 0.3s for rtc read time
		NhelperStatusBarSetIcon(SRQST_SET_TIME, 0);	
		
		//enable the screen again
		if(screensave != PLEXTALK_SCREENSAVER_ACTIVENOW
			&& screensave != PLEXTALK_SCREENSAVER_DEACTIVE) {
			printf("Resume screensaver last!\n");
			NeuxSsaverPauseResume(0);
		}
#else
		setime_setRealTime();
#endif
		memset(prompt_str,0x00,256);
		setime_getTimeStr(prompt_str,256);
		
		//enter 停顿一下，当前日期 Set ，停顿一下,close.....
		
#if 0	//close set the system 和cancel的提示音放到MenuExit里面去发
		snprintf(str_buf,256, "%s: %s%s: %s", 
			_("Enter"),
			prompt_str,
			_("Set"),
			_("Close set the system date and time menu"));
		setime_TTS(str_buf,VPROMPT_AUDIO_PU);
		setime_TTS_Beep(VPROMPT_AUDIO_CANCEL,0);

#else
		snprintf(str_buf,256, "%s: %s%s", 
			_("Enter"),
			prompt_str,
			_("Set"));

		voice_prompt_abort();
		voice_prompt_end = 1;
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		voice_prompt_string(1,  &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str_buf, strlen(str_buf)); 
		voice_prompt_end = 0;

#endif

	}
	else
	{
		//cancel (pause for a moment),close....
		snprintf(str_buf,256, "%s: %s", _("Cancel"),_("Close set the system date and time menu"));
		setime_TTS(str_buf,VPROMPT_AUDIO_PU);
		setime_TTS_Beep(VPROMPT_AUDIO_CANCEL,0);
	}

	tts_exit_state = 1;
	setime_CreateExitTimer();
	//NeuxClearModalActive();//需要发完后才能退出

	
}

static int
setime_getOneTimeByUI(int cur_ui)
{
	int ret;
	if(cur_ui < YEAR_SET_UI || cur_ui > s_app.max_ui)
	{
		return s_app.year;
	}
	switch(cur_ui)
	{
		case YEAR_SET_UI:
			ret = s_app.year;
			break;
		case MONTH_SET_UI:
			ret = s_app.month;
			break;
		case DATE_SET_UI:
			ret = s_app.day;
			break;
		case HOUR_SET_UI:
			ret = s_app.hour;
			break;
		case MINUTE_SET_UI:
			ret = s_app.minute;
			break;
		case AMPM_SET_UI:
			ret = s_app.ampm;
			break;	
		default:
			ret = s_app.year;
			break;
	}

	return ret;
	
}

static void
setime_OneTimeOnLongTimer(int cur_ui,int add)
{
	if(cur_ui < YEAR_SET_UI || cur_ui > s_app.max_ui)
	{
		return ;
	}
	s_app.isInLongPressed = 1;

	if(add)
		setime_IncrementOnUpKey(0);
	else
		setime_DecrementOnDownKey(0);

	
}


static void
InitSet(void)
{

	int timef;
	CoolShmReadLock(g_config_lock);
	timef = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);

	if(timef)
		s_app.timeformat = FORMAT_24H;
	else
		s_app.timeformat = FORMAT_12H;

	
	s_app.exitflag = -1;
	s_app.cur_ui = YEAR_SET_UI;
	s_app.isInLongPressed = 0;
	s_app.max_ui = (FORMAT_12H == s_app.timeformat)?(AMPM_SET_UI):(AMPM_SET_UI-1);

	DBGMSG("initset timef:%d,timeformat:%d,maxui:%d\n",timef,s_app.timeformat,s_app.max_ui);

	tts_exit_state = 0;
	key_click_flag = 0;
	//strcpy(bu_path,VPROMPT_AUDIO_BU);
	//strcpy(pu_path, VPROMPT_AUDIO_PU);
	//strcpy(cancel_path,  VPROMPT_AUDIO_CANCEL);

	set_title_text[YEAR_SET_UI] = 	_("Year");
	set_title_text[MONTH_SET_UI] = 	_("Month");
	set_title_text[DATE_SET_UI] = 	_("Date");
	set_title_text[HOUR_SET_UI] = 	_("Hour");
	set_title_text[MINUTE_SET_UI] = 	_("Minute");
	set_title_text[AMPM_SET_UI] = 	_(" ");
	set_title_text[AMPM_SET_UI+1] = 	NULL;

	set_prompt_text[YEAR_SET_UI] =	_("Enter the year");
	set_prompt_text[MONTH_SET_UI] =	_("Enter the month");
	set_prompt_text[DATE_SET_UI] =	_("Enter the date");
	set_prompt_text[HOUR_SET_UI] =	_("Enter the hour");
	set_prompt_text[MINUTE_SET_UI] = 	_("Enter the minute");
	set_prompt_text[AMPM_SET_UI] =	_("Enter the AM/PM");
	set_prompt_text[AMPM_SET_UI+1] = 	NULL;

	
	DBGMSG("InitSet success!\n");
}

static void
setime_OnWindowGetFocus(WID__ wid)
{
	DBGMSG("Get Focus\n");
	NhelperTitleBarSetContent(_("System date and time"));
}

static void
setime_OnWindowDestroy(WID__ wid)
{
	DBGMSG("---------------window destroy %d.\n",wid);
#if 1	
	if(s_app.InputTimer)
		TimerDisable(s_app.InputTimer);
	if(s_app.ExitTimer)
		TimerDisable(s_app.ExitTimer);
	plextalk_schedule_unlock();
	plextalk_sleep_timer_unlock();
#endif	
	
}

void setime_key_err_voice()
{
	voice_prompt_abort();
	voice_prompt_end = 1;
	voice_prompt_music(0,  &vprompt_cfg, VPROMPT_AUDIO_BU);
	voice_prompt_end = 0;
}
static void setime_TTS(const char* str,const char* lead)
{

	voice_prompt_abort();
	voice_prompt_end = 1;

	if (lead != NULL)
		voice_prompt_music(0, &vprompt_cfg, lead);
	
	if(str)
	{
		voice_prompt_string(0,  &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
					tts_role, str, strlen(str)); 
	}
	voice_prompt_end = 0;
}

static void setime_TTS_Beep(const char* lead,int abort)
{
	if(abort)
	{
		voice_prompt_abort();
		voice_prompt_end = 1;
	}
	if (lead != NULL)
	{
		voice_prompt_music(1, &vprompt_cfg, lead);
		voice_prompt_end = 0;
	}
}


static int setime_DecrementOnDownKey(int needVoice)
{
	switch(s_app.cur_ui)
	{
		case YEAR_SET_UI:
			if(s_app.year - 1 >= 2000){
				s_app.year--;
			}
			else{
			    s_app.year = 2037;
			}
			s_app.value = s_app.year;
			if(s_app.day > month_days(s_app.year, s_app.month)){
					s_app.day = month_days(s_app.year, s_app.month);
					s_app.value = s_app.day;
			}
			break;
		case MONTH_SET_UI:
			if(s_app.month -1 > 0){
				s_app.month--;
				}
				else
				{
					s_app.month = 12;
				}
				s_app.value = s_app.month;
				if(s_app.day > month_days(s_app.year, s_app.month)){
					s_app.day = month_days(s_app.year, s_app.month);
					s_app.value = s_app.day;
				}
			break;
		case DATE_SET_UI:
			if(s_app.day - 1 > 0){
				s_app.day--;
			}
			else{
				s_app.day = month_days(s_app.year, s_app.month);
			}
			s_app.value = s_app.day;
			break;
		case HOUR_SET_UI:
			if(FORMAT_12H == s_app.timeformat) {
				if(s_app.hour  > 1){
					s_app.hour--;
				}
				else{
					s_app.hour = 12;
				}
			} else {
				if(s_app.hour  > 0){
					s_app.hour--;
				}
				else{
					s_app.hour = s_app.max_hour -1;
				}
			}

			s_app.value = s_app.hour;
			break;
		case MINUTE_SET_UI:
			if(s_app.minute >  0){
				s_app.minute--;
			}
			else{
				s_app.minute = 59;
			}
			s_app.value = s_app.minute;
			break;
		case AMPM_SET_UI:
			s_app.ampm += 1;
			s_app.ampm %= 2;
			s_app.value = s_app.ampm;
			break;		
	}	


	if(s_app.cur_ui == AMPM_SET_UI) {
		if(s_app.value)
			snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("PM"));
		else
			snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("AM"));
	} else {
		snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), s_app.value);
	}
	

	NeuxLabelSetText(s_app.sValue, str_buf);
	NeuxWidgetShow(s_app.sValue, TRUE);

	if(needVoice)		
		setime_TTS(str_buf,VPROMPT_AUDIO_PU);
	else
		setime_TTS_Beep(VPROMPT_AUDIO_PU,1);

	return s_app.value;
}
	
static int setime_IncrementOnUpKey(int needVoice)
{
	switch(s_app.cur_ui)
	{
		case YEAR_SET_UI:
			if(s_app.year + 1 < 2038){
				s_app.year++;
			}
			else{
				s_app.year = 2000;
			}
			s_app.value = s_app.year;
			if(s_app.day > month_days(s_app.year, s_app.month)){
					s_app.day = month_days(s_app.year, s_app.month);
					s_app.value = s_app.day;
			}
			break;
		case MONTH_SET_UI:
			if(s_app.month + 1 < 13){
				s_app.month++;
				}
				else
				{
					s_app.month = 1;
				}
				s_app.value = s_app.month;
				if(s_app.day > month_days(s_app.year, s_app.month)){
					s_app.day = month_days(s_app.year, s_app.month);
					s_app.value = s_app.day;
				}
			break;
		case DATE_SET_UI:
			if(s_app.day + 1 <= month_days(s_app.year, s_app.month)){
				s_app.day++;
			}
			else{
				s_app.day = 1;
			}
			s_app.value = s_app.day;
			break;
		case HOUR_SET_UI:
			if(FORMAT_12H == s_app.timeformat) {
				if(s_app.hour +1 <= 12){
					s_app.hour++;
				} else{
					s_app.hour = 1;
				}
			} else {
				if(s_app.hour +1 < s_app.max_hour){
					s_app.hour++;
				}
				else{
					s_app.hour = 0;
				}
			}

			s_app.value = s_app.hour;
			break;
		case MINUTE_SET_UI:
			if(s_app.minute+1 <  60){
				s_app.minute++;
			}
			else{
				s_app.minute = 0;
			}
			s_app.value = s_app.minute;
			break;
		case AMPM_SET_UI:
			s_app.ampm += 1;
			s_app.ampm %= 2;
			s_app.value = s_app.ampm;
			break;		
	}

	if(s_app.cur_ui == AMPM_SET_UI) {
		if(s_app.value)
			snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("PM"));
		else
			snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("AM"));
	} else {
		snprintf(str_buf,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), s_app.value);
	}

	NeuxLabelSetText(s_app.sValue, str_buf);
	NeuxWidgetShow(s_app.sValue, TRUE);

	if(needVoice)		
		setime_TTS(str_buf,VPROMPT_AUDIO_PU);
	else
		setime_TTS_Beep(VPROMPT_AUDIO_PU,1);

	return s_app.value;

}

static void
setime_StartCount(int isaddkey)
{
	if(s_app.longPressedTimer)
	TimerEnable(s_app.longPressedTimer);
	s_app.isaddkey = isaddkey;
}
static void 
setime_PauseCount()
{
	if(s_app.longPressedTimer)
	{
		if(s_app.isInLongPressed)
		{
			s_app.isInLongPressed = 0;
			if(s_app.cur_ui == AMPM_SET_UI) {
				snprintf(str_buf,256, setime_getSringFormat("%s ",s_app.cur_ui,NULL), 
					set_title_text[s_app.cur_ui],
					(setime_getOneTimeByUI(s_app.cur_ui)?_("PM"):_("AM")));
			} else {
				snprintf(str_buf,256, setime_getSringFormat("%s ",s_app.cur_ui,NULL), set_title_text[s_app.cur_ui],setime_getOneTimeByUI(s_app.cur_ui));
			}
			
			setime_TTS(str_buf,NULL);

			//snprintf(str_buf,256, "%d", s_app.value);
			//setime_TTS(str_buf,VPROMPT_AUDIO_PU);
		}
		TimerDisable(s_app.longPressedTimer);
	}
	

}

static void
setime_OnWindowKeyup(WID__ wid, KEY__ key)	
{
	DBGMSG("setime_OnWindowKeyup \n");

	setime_PauseCount();	

}

static void
setime_OnWindowKeydown(WID__ wid, KEY__ key)
{	
	 //help在运行时，不响应任何按键(因为help按键grap key时用的NX_GRAB_HOTKEY)
	 if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	 {
	  	DBGLOG("help is running!!\n");
	  	return;
	 }

	 if(tts_exit_state || menu_exit) {
		NeuxTimerSetControl(s_app.InputTimer,SET_INPUT_TIME,0);
	} else {
		NeuxTimerSetControl(s_app.InputTimer,SET_INPUT_TIME,-1);
	}

	 if (menu_exit) {
	 	DBGMSG("I will close the menu,so this key will abort current voice and close menu immediately");
		voice_prompt_abort();
		NeuxAppStop();
		return;
	}

	 if(tts_exit_state)
	 {
	 	DBGMSG("I am in exit status,key down?OK,OK,I will close the timeset menu now\n");
		 if(s_app.exitflag) {//返回 到MENU
			 DBGMSG("exit to menu\n");
			 NeuxClearModalActive();
		 } else {
			 DBGMSG("close the menu\n");
			 MenuExit(0);
		 }

		return;
	 }

	 if(NeuxAppGetKeyLock(0))
	 {
	 	DBGLOG("the key is locked !!\n");
		snprintf(str_buf,256, "%s",_("Keylock"));
	 	setime_TTS(str_buf,VPROMPT_AUDIO_BU);
		return;
	 }
	 	
	voice_prompt_abort();
	DBGMSG("key down:%d\n",key.ch);
	switch(key.ch)
	{
		case VKEY_BOOK:
		case VKEY_MUSIC:
		case VKEY_RADIO:
		case VKEY_RECORD:
			setime_key_err_voice();
			break;
			
		case MWKEY_INFO:
			//snprintf(str_buf,256, "%s %s %d", 
			//		set_prompt_text[s_app.cur_ui],
			//		set_title_text[s_app.cur_ui],
			//		setime_getOneTimeByUI(s_app.cur_ui));
			if(s_app.cur_ui == AMPM_SET_UI) {
				snprintf(str_buf,256, setime_getSringFormat("%s ",s_app.cur_ui,NULL), 
					set_title_text[s_app.cur_ui],
					(setime_getOneTimeByUI(s_app.cur_ui)?_("PM"):_("AM")));
			} else {
				snprintf(str_buf,256, setime_getSringFormat("%s ",s_app.cur_ui,NULL), set_title_text[s_app.cur_ui],setime_getOneTimeByUI(s_app.cur_ui));
			}
			
			setime_TTS(str_buf,VPROMPT_AUDIO_PU);
			setime_TTS_Beep(VPROMPT_AUDIO_CANCEL,0);
			break;
#if 0			
		case MWKEY_MENU:
			//setime_setExit(1);
			//voice_prompt_abort();
			//voice_prompt_music(0, &vprompt_cfg, pu_path);
			//NeuxAppStop();
			//NeuxFormDestroy(&form);
			TimerDisable(s_app.InputTimer);
			//TimerDisable(s_app.ExitTimer);
			//plextalk_schedule_unlock();
			//plextalk_sleep_timer_unlock();
			tts_exit_state = 1;
			menu_exit_state = 1;
			DBGMSG("set time menu out\n");
			break;
#endif			
		case MWKEY_DOWN:

			setime_StartCount(0);
				
			setime_DecrementOnDownKey(1);
			
			break;

		case MWKEY_UP:

			setime_StartCount(1);
			
			setime_IncrementOnUpKey(1);
			
			break;	

			case MWKEY_RIGHT:
			case MWKEY_ENTER:
			{
				if(s_app.cur_ui >= s_app.max_ui)
				{
					setime_setExit(0);
					break;
				}
				s_app.cur_ui++;
				setime_ShowTheLabel(s_app.cur_ui);

				//enter 停顿一下，enter the year 停顿一下，year 2013
				if(s_app.cur_ui == AMPM_SET_UI) {
					snprintf(str_buf,256 ,setime_getSringFormat("%s: %s: %s: ",s_app.cur_ui,NULL),
						_("Enter"),
						set_prompt_text[s_app.cur_ui],
						set_title_text[s_app.cur_ui],
						(setime_getOneTimeByUI(s_app.cur_ui)?_("PM"):_("AM")));

				} else {
					snprintf(str_buf,256, setime_getSringFormat("%s: %s: %s ",s_app.cur_ui,NULL),
						_("Enter"),
						set_prompt_text[s_app.cur_ui],
						set_title_text[s_app.cur_ui],
						setime_getOneTimeByUI(s_app.cur_ui));
				}



				//snprintf(str_buf,256, "%s %d", set_title_text[s_app.cur_ui],setime_getOneTimeByUI(s_app.cur_ui));
				setime_TTS(str_buf,VPROMPT_AUDIO_PU);
			}
			break;
			case MWKEY_LEFT:
			{
				if(s_app.cur_ui <= YEAR_SET_UI)
				{
					setime_setExit(-1);
					break;
				}
				s_app.cur_ui--;
				setime_ShowTheLabel(s_app.cur_ui);

				//enter the year 停顿一下，year 2013
				if(s_app.cur_ui == AMPM_SET_UI) {
					snprintf(str_buf,256, setime_getSringFormat("%s: %s ",s_app.cur_ui,NULL),
						set_prompt_text[s_app.cur_ui],
						set_title_text[s_app.cur_ui],
						(setime_getOneTimeByUI(s_app.cur_ui)?_("PM"):_("AM")));
				} else {
					snprintf(str_buf,256, setime_getSringFormat("%s: %s ",s_app.cur_ui,NULL), 
						set_prompt_text[s_app.cur_ui],
						set_title_text[s_app.cur_ui],
						setime_getOneTimeByUI(s_app.cur_ui));
				}

				//snprintf(str_buf,256, "%s %d", set_title_text[s_app.cur_ui],setime_getOneTimeByUI(s_app.cur_ui));
				setime_TTS(str_buf,VPROMPT_AUDIO_PU);
			}
			break;			
	}
	
	
}

static void
setime_ShowTheLabel(int cur_ui)
{
	char str[256];
	int value = setime_getOneTimeByUI(cur_ui);
	DBGMSG("setime_ShowTheLabel cur_ui:%d,value:%d\n",cur_ui,value);

	memset(str,0x00,256);
	
	if(cur_ui == AMPM_SET_UI) {
		if(value)
			snprintf(str,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("PM"));
		else
			snprintf(str,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), _("AM"));
	} else {
		snprintf(str,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), value);
	}

	NeuxLabelSetText(s_app.pTitle, set_title_text[cur_ui]);	
	NeuxWidgetShow(s_app.pTitle, TRUE);

	NeuxLabelSetText(s_app.sValue, str);
	NeuxWidgetShow(s_app.sValue, TRUE);
	
}



static void
setime_ShowLabels(void)
{
	struct tm *tp;
	time_t resultp;
	char str[256];

	time(&resultp);	
	tp = localtime (&resultp);

	DBGMSG("setime_ShowLabels start!\n");
	
	s_app.year = tp->tm_year+1900;
	s_app.month = tp->tm_mon+1;
	s_app.day = tp->tm_mday;
	s_app.hour = tp->tm_hour;
	s_app.minute = tp->tm_min;
	s_app.ampm = s_app.hour/12;
	s_app.max_hour = 24;
	if(FORMAT_12H == s_app.timeformat){
		//s_app.hour = s_app.hour%12;	//12format
		s_app.hour = setime_Transform24To12Time(s_app.hour,&s_app.ampm);
		s_app.max_hour = 12;
	}

	DBGMSG("setime_ShowLabels start 2 !\n");

	snprintf(str,256, setime_getSringFormat(NULL,s_app.cur_ui,NULL), s_app.year);

	NeuxWidgetShow(s_app.pTitle, TRUE);

	DBGMSG("setime_ShowLabels start 3 !\n");

	NeuxLabelSetText(s_app.sValue, str);
	NeuxWidgetShow(s_app.sValue, TRUE);

	DBGMSG("setime_ShowLabels success!\n");
	
}

static void
setime_OnTimer()	
{	
	setime_OneTimeOnLongTimer(s_app.cur_ui,s_app.isaddkey);
}
static void
setime_OnExitTimer()	
{
	if(menu_exit)
	{
		DBGMSG("I will close the menu,so OnExitTimer callback will not work\n");
		return;
	}
	if(tts_exit_state && voice_prompt_end)
	{
		DBGLOG("setime_OnExitTimer tts_exit_state:%d",tts_exit_state);
		
		TimerDisable(s_app.ExitTimer);
		TimerDisable(s_app.InputTimer);
		//plextalk_schedule_unlock();
		//plextalk_sleep_timer_unlock();
		if(s_app.exitflag) {//返回 到MENU
			DBGMSG("exit to menu\n");
			NeuxClearModalActive();
		} else {
 			DBGMSG("close the menu\n");
			MenuExit(0);
		}
		
	}
}

static void
setime_OnInputTimer()	
{	
	DBGLOG("setime_OnInputTimer tts_exit_state:%d",tts_exit_state);
	if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
	{
		DBGMSG("Since help is running,Input Alarm will not work.But the timer is still running\n");
		return;
	}
	if(tts_exit_state)
	{
		TimerDisable(s_app.InputTimer);
	}
	else
	{
		setime_TTS_Beep(VPROMPT_AUDIO_INPUT,0);
	}
}



static void
setime_CreateTimer()
{
	s_app.longPressedTimer = NeuxTimerNew(form, SET_LONG_PRESS_TIME, -1);
	NeuxTimerSetCallback(s_app.longPressedTimer, setime_OnTimer);
	DBGLOG("setime_CreateTimer :%d",s_app.longPressedTimer);

	s_app.InputTimer= NeuxTimerNew(form, SET_INPUT_TIME, -1);
	NeuxTimerSetCallback(s_app.InputTimer, setime_OnInputTimer);
	DBGLOG("setime_CreateInputTimer :%d",s_app.InputTimer);
}

static void
setime_CreateExitTimer()
{
	s_app.ExitTimer = NeuxTimerNew(form, SET_EXIT_TIME, -1);
	NeuxTimerSetCallback(s_app.ExitTimer, setime_OnExitTimer);
	DBGLOG("setime_CreateExitTimer :%d",s_app.ExitTimer);
}


static void
setime_CreateLabels ()	
{
	widget_prop_t wprop;
	label_prop_t lprop;

	s_app.pTitle = NeuxLabelNew(form,
							0,5,MAINWIN_WIDTH,30,set_title_text[s_app.cur_ui]);
	NeuxWidgetSetFont(s_app.pTitle, sys_font_name, sys_font_size);
	
	NeuxWidgetGetProp(s_app.pTitle, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(s_app.pTitle, &wprop);
		
	NeuxLabelGetProp(s_app.pTitle, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(s_app.pTitle, &lprop);
	NeuxWidgetShow(s_app.pTitle, FALSE);
	
	
	s_app.sValue = NeuxLabelNew(form,
							0,5+30+2,MAINWIN_WIDTH,30,NULL);
	NeuxWidgetSetFont(s_app.sValue, sys_font_name, sys_font_size);
	
	NeuxWidgetGetProp(s_app.sValue, &wprop);
	wprop.fgcolor = theme_cache.background;
	wprop.bgcolor = theme_cache.selected;
	NeuxWidgetSetProp(s_app.sValue, &wprop);
		
	NeuxLabelGetProp(s_app.sValue, &lprop);
	lprop.align = LA_LEFT;
	lprop.autosize = GR_FALSE;
	NeuxLabelSetProp(s_app.sValue, &lprop);
	NeuxWidgetShow(s_app.sValue, FALSE);
	
}






static void
CreateDateSetDialog(void)
{
	listbox_prop_t lsprop;
	widget_prop_t wprop;

	form = NeuxFormNew( MAINWIN_LEFT,
						MAINWIN_TOP,
						MAINWIN_WIDTH,
						MAINWIN_HEIGHT);

	NeuxFormSetCallback(form, CB_KEY_DOWN, setime_OnWindowKeydown);
	NeuxFormSetCallback(form, CB_KEY_UP, setime_OnWindowKeyup);
	NeuxFormSetCallback(form, CB_FOCUS_IN, setime_OnWindowGetFocus);
	NeuxFormSetCallback(form, CB_DESTROY,  setime_OnWindowDestroy);
	//NeuxFormSetCallback(form, CB_HOTPLUG,  OnWindowHotplug);


	DBGMSG("CreateDateSetDialog success!\n");

	setime_CreateLabels();
	setime_CreateTimer();
	setime_PauseCount();
	DBGMSG("setime_CreateLabels success!\n");
}


/**
menu界面日期与时间的设置
*/
int sys_set_data_time(void)
{

	DBGMSG("sys_set_data_time!\n");
	InitSet();
	CreateDateSetDialog();

	NeuxWidgetShow(form, TRUE);
	NxSetWidgetFocus(form);
	setime_ShowLabels();
	DBGMSG(" set time and date UI init success!\n");

	//enter the year 停 一下，year 2013 
	snprintf(str_buf,256, setime_getSringFormat("%s:%s ",s_app.cur_ui,NULL), set_prompt_text[YEAR_SET_UI],set_title_text[YEAR_SET_UI],s_app.year);
	setime_TTS(str_buf,VPROMPT_AUDIO_PU);

	//时间设置界面禁止应用切换
	plextalk_schedule_lock();
	plextalk_sleep_timer_lock();
	

	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);

	NeuxFormDestroy(&form);

	return s_app.exitflag;
}


