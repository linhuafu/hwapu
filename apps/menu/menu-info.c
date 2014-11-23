#include "nw-menu.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "nxutils.h"
#include "key-value-pair.h"
#include "mulitlist.h"
#include "xml-helper.h"
#include "file-helper.h"
#include "device.h"
#include "rtc-helper.h"
#include "menu-backup.h"
#define OSD_DBG_MSG
#include "nc-err.h" 
//#include "libinfo.h"

//extern int voice_prompt_end;
//extern int info_tts_running;
extern struct voice_prompt_cfg vprompt_cfg;
//将24小时制转化为12小时制，并得到ampm
static int
menu_Transform24To12Time(int hour,int *ampm)
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

void
do_Infomation(const char* caption,const char* items)
{
	sys_time_t stm;
	int hour_format,pattern;
//	struct plex_info* info_id;
	char info[256];
	char timebuf[100];
	char allbuf[256];
	int hour;
	int size = 256;
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

	if(caption == NULL || items == NULL)
		return;

	CoolShmReadLock(g_config_lock);
	hour_format = g_config->hour_system;
	pattern = g_config->time_format;
	CoolShmReadUnlock(g_config_lock);

	memset(info,0x00,256);
	memset(timebuf,0x00,100);
	memset(allbuf,0x00,256);

	sys_get_time(&stm);
	hour = stm.hour;

	if(hour_format == 0)
	{
		int ampm = 0;
		hour = menu_Transform24To12Time(hour,&ampm);
		sprintf(timebuf,"%02d:%02d %s ", hour, stm.min, ampm?_("PM"):_("AM"));

	}
	else
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
		snprintf(allbuf, size, "%s %s%04d %02d %02d %s", _("Current date and time:"),
			timebuf, stm.year, stm.mon, stm.day, weekdayStr[weekday]);
	break;
	case 1: // 1: hour, minute, weekday, month, day, year
		snprintf(allbuf, size, "%s %s%s %02d %02d %04d", _("Current date and time:"),
			timebuf, weekdayStr[weekday], stm.mon, stm.day, stm.year);
	break;
	//case 2:// 2: hour, minute, weekday, day, month, year
	default:
		snprintf(allbuf, size, "%s %s%s %02d %02d %04d", _("Current date and time:"),
			timebuf, weekdayStr[weekday], stm.day, stm.mon, stm.year);
	break;
	}
	//DBGMSG("allbuf = %s\n", allbuf);

	voice_prompt_abort();
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
	voice_prompt_string(0,  &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
				tts_role, allbuf, strlen(allbuf)); 


	snprintf(info, 256, "%s: %s: %s", _("Current selected menu name"),
		caption,items);

	voice_prompt_string(0,  &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
				tts_role, info, strlen(info)); 
	voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
	
	//DBGMSG("do_information:%s\n",info);

	

	//file_info[info_item_num] = info;
	//info_item_num ++;

	//info_tts_running = !voice_prompt_end;
	/* Start info */
	//info_id = info_create();
	//info_id->is_mainmenu = 1;
	//info_start (info_id, file_info, info_item_num, &info_tts_running);
	//info_destroy (info_id);

}


