#include <stdio.h> 
#define MWINCLUDECOLORS
#include <microwin/nano-X.h>
#include <libintl.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "libinfo.h"
#include "xml-helper.h"
#include "rtc-helper.h"
#include "plextalk-config.h"
#include "plextalk-titlebar.h"
#include "Application-ipc.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))
#endif

#undef _
//#define _(x) x

//ztz
#define _(S)	dgettext("libinfo", (S))


enum {
	SUPPLY_BATTERY = 0,
	SUPPLY_ACIN,
	SUPPLY_USB,
};


static int info_progress = 0;
static int endOfModal = -1;
static struct plex_info* id = NULL;

//这里放置一个全局的prompt，这个要放到后面去
extern struct voice_prompt_cfg vprompt_cfg ;


//ztz	--start--

static char* weekdayStr[7];

static void 
WeekDayStrInit(void)
{
	weekdayStr[0] = _("Sunday");
	weekdayStr[1] = _("Monday");
	weekdayStr[2] = _("Tuesday");
	weekdayStr[3] = _("Wednesday");
	weekdayStr[4] = _("Thursday");
	weekdayStr[5] = _("Friday");
	weekdayStr[6] = _("Saturday");
}

//ztz ---end--

//appk add start

static char* battery_tts_str[5];

static void 
BatteryTTSStrInit(void)
{
	battery_tts_str[4] = _("five fifths");
	battery_tts_str[3] = _("four fifths");
	battery_tts_str[2] = _("three fifths");
	battery_tts_str[1] = _("two fifths");
	battery_tts_str[0] = _("one fifths");	
}

//appk add end


#if 0
static int grab_table[] = {
		VKEY_BOOK,
		VKEY_MUSIC,
		VKEY_RADIO,
		VKEY_RECORD,
		MWKEY_MENU,
		MWKEY_INFO,
		MWKEY_ENTER,
		MWKEY_UP,
		MWKEY_DOWN,
		MWKEY_LEFT,
		MWKEY_RIGHT,
};

static void info_grab_key ()
{
	int i;
	
	for(i = 0; i < ARRAY_SIZE(grab_table); i++)
		NeuxWidgetGrabKey(id->info_wid, grab_table[i], GR_GRAB_HOTKEY_EXCLUSIVE);
}


static void info_ungrab_key ()
{
	int i;

	for(i = 0; i < ARRAY_SIZE(grab_table); i++)
		NeuxWidgetUngrabKey(id->info_wid, grab_table[i]);
}
#endif

static int hour24To12(int hour,int *ampm)
{
	 *ampm = (hour>=12);
	 if (hour == 0){
	 	hour = 12;
	 } else if(hour > 12) {
		 hour -= 12;
	 }
	 return hour;
}
#if 0
static int hour12To24(int hour,int ampm)
{
	if(hour == 12){
		if(ampm)
			return 12;
		else
			return 0;
	}

	if(ampm){
		hour += 12;
	}
	return hour;
}
#endif
static int get_power_supply (void)
{

//	char val[32];
	int supply = SUPPLY_USB;

//	int acin_on = 0;
//	int usb_on = 0;
//	int battery_on = 0;
	int main_batt_present = 0, backup_batt_present = 0;
	int vbus_present = 0;


	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);
	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);

	if(vbus_present)
	{
		supply = SUPPLY_USB;
	}
	else{
		if(main_batt_present || backup_batt_present)
			supply = SUPPLY_BATTERY;
	}
	
#if 0
	/*
	//Check AC_IN is on or not
	ret = sysfs_read(PLEXTALK_AC_PRESENT, val, 32);
	if (ret < 1) {
		DBGMSG("error when read battery online!\n");
		ret = -1;
	}

	DBGMSG("ACIN_ON:val = %s\n", val);
	if (!strncmp(val, "1", 32)) {
		ret = 0;
		acin_on = 1;
	}
*/
	/* Check USB is on or not */
	ret = sysfs_read(PLEXTALK_VBUS_PRESENT, val, 32);
	if (ret < 0) {
		DBGMSG("error when read usb online!\n");
		ret = -1;
	}
	else
	{
		DBGMSG("USB_ON:ret = %d\n", ret);
		val[ret] = 0;
		usb_on = atoi(val);
		DBGMSG("USB_ON:usb_on = %d\n", usb_on);
	}

	if(usb_on == 0){
		ret = sysfs_read(PLEXTALK_BATT_PRESENT, val, 32);
		if (ret < 0) {
			DBGMSG("error when read battery online!\n ");
			ret = -1;
		}

		DBGMSG("BATTERY_ON:val = %s\n", val);

		if (!strncmp(val, "1", 32)) {
			ret = 0;
			battery_on = 1;
		}
	}

	if (ret == -1) {
			DBGMSG("[fatal]: get wrong power supply source!\n");
	}
	else {
		if (usb_on) 
			supply = SUPPLY_USB;
		else if (battery_on) 
			supply = SUPPLY_BATTERY;
		else {
			supply = SUPPLY_ACIN;
		}
	}
#endif
	DBGMSG("supply = %d\n", supply);
	return supply;
}

static int get_battery_module(void)
{
	char buf[256];
//	int ret = 0;
	int type = -1;
	int main_batt_present, backup_batt_present;

	/*Discharging,	Charging, Not charging*/	
	memset(buf,0, 256);

	sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
	sysfs_read_integer(PLEXTALK_BACKUP_BATT_PRESENT, &backup_batt_present);

	if(backup_batt_present)
	{
		type = 1;
	}
	else if(main_batt_present)
	{
		type = 0;
	}

#if 0
	ret = sysfs_read(PLEXTALK_BATT_MODEL_NAME, buf, 256);
	if(ret >= 0)
	{		
		if (strncmp(buf, "main-battery", 12) == 0)
		{
			type = 0;
		}
		else if (strncmp(buf, "backup-battery", strlen("backup-battery"))== 0)
		{
			type = 1;
		}
		else
		{
			type = -1;
		}
	}
#endif

	return type;

}

#if 0
static int get_battery_capacity(void)
{
	char buf[256];
	int val = 0, ret;

	memset(buf,0, 256);
	ret = sysfs_read(PLEXTALK_BATT_CAPACITY, buf, 256);
	if(ret >= 0)
	{
		val = atoi(buf);
	}
	else{
		DBGMSG("get invalid\n");
		val = 99;
	}
	
	return val;

}
#endif
#define HALF_VOLUME_WHEN_BACKUP 	1
//mp3播放电压
#if HALF_VOLUME_WHEN_BACKUP
#define BACK_LOW_VOLTAGE_AUDIO			(2300000)//(2650000)//(2400000) 因为干电池升压IC没有稳压功能，被瞬时拉低到掉电			 
#else
#define BACK_LOW_VOLTAGE_AUDIO			(2650000)//(2400000) 因为干电池升压IC没有稳压功能，被瞬时拉低到掉电 		 
#endif

//关机电压
#define BACK_LOW_VOLTAGE_POWEROFF		(2280000)
//锂电池关机电压
#define MAIN_LOW_VOLTAGE_POWEROFF		(3250000)
//锂电池mp3播放电压
#define MAIN_LOW_VOLTAGE_AUDIO			(3410000)

static int get_battery_level(int type)
{
	int level = 0;
	int main_batt_voltage_min, main_batt_voltage_max;
	int backup_batt_voltage_min, backup_batt_voltage_max;
	int batt_voltage_now;

	sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_NOW, &batt_voltage_now);

	if(0 == type)
	{
		sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MIN_DESIGN, &main_batt_voltage_min);
		sysfs_read_integer(PLEXTALK_MAIN_BATT_VOLTAGE_MAX_DESIGN, &main_batt_voltage_max);
		level = (batt_voltage_now - MAIN_LOW_VOLTAGE_AUDIO)/ ((main_batt_voltage_max - MAIN_LOW_VOLTAGE_AUDIO) / 5);
	}
	else if(1 == type)
	{
		sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MIN_DESIGN, &backup_batt_voltage_min);
		sysfs_read_integer(PLEXTALK_BACKUP_BATT_VOLTAGE_MAX_DESIGN, &backup_batt_voltage_max);
		level = (batt_voltage_now - BACK_LOW_VOLTAGE_AUDIO)/ ((backup_batt_voltage_max - BACK_LOW_VOLTAGE_AUDIO) / 5);
	}
	//与voltage.c同步
	if (level < 0)
		level = 0;
	if (level > 4)
		level = 4;
	level += 1;
	
	return level;
}

int get_battery_status_charge(void)
{
	int charge = 0;
	int vbus_present = 0;

	char health[32];
	sysfs_read(PLEXTALK_MAIN_BATT_HEALTH, health, sizeof(health));
	printf("health:%s\n", health);
	if(!strcmp(health, "Overheat") ||
		!strcmp(health, "Cold")||
		!strcmp(health, "Unspecified failure")){
		//charge = 0;
		return charge;
	}

	sysfs_read_integer(PLEXTALK_VBUS_PRESENT, &vbus_present);
	if(!vbus_present){
		//charge = 0;
		return charge;
	}

	char status[32];
	sysfs_read(PLEXTALK_MAIN_BATT_STATUS, status, sizeof(status));
	if (!strcmp(status, "Charging")){
		charge = 1; //charging
	}else{
		int main_batt_present = 0;
		sysfs_read_integer(PLEXTALK_MAIN_BATT_PRESENT, &main_batt_present);
		if(main_batt_present){
			charge = 2;	//full charge
		}else{
			charge = 0;
		}
	}
	
#if 0
	/*Discharging,  Charging, Not charging*/
	memset(buf,0, 256);
	ret = sysfs_read(PLEXTALK_BATT_STATUS, buf, 256);
	if(ret >= 0)
	{
		if (strncmp(buf, "Discharging", 11) == 0)
		{
			DBGMSG("Discharging !\n");
			charge = 0;
		}
		else if (strncmp(buf, "Charging", 8) == 0)
		{
			DBGMSG("Charging !\n");
			charge = 1;
		}
		else if (strncmp(buf, "Not charging", 8) == 0)
		{
			DBGMSG("Not charging !\n");
			charge = 2;
		}
		else
		{
			DBGMSG("invalid state!\n");
			charge = 0;
		}

	}
#endif	
	return charge;

}

static void add_datetime_info(struct plex_info* id, char* datetime_info, int size)
{
	int hour_format; // 0: 12; 1: 24
	int pattern; //0: hour, minute, year, month, day, weekday 1: hour, minute, weekday, month, day, year 2: hour, minute, weekday, day, month, year
	sys_time_t date_time;
	char strhour[64];
	char *ampm_info[2];
	
	ampm_info[0] = _("AM");
	ampm_info[1] = _("PM");

	CoolShmReadLock(g_config_lock);
	pattern = g_config->time_format;            
	hour_format = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);

	memset(strhour, 0x00, sizeof(strhour));
	
	sys_get_time(&date_time);
	int hour = date_time.hour;

	if(hour_format == 0)
	{//12hour
		int ampm;
		hour = hour24To12(hour,&ampm);
		snprintf(strhour,sizeof(strhour),"%02d:%02d %s ", hour, date_time.min, ampm_info[ampm]);
	}
	else
	{
		snprintf(strhour,sizeof(strhour),"%02d:%02d ", hour, date_time.min);
	}

	int weekday = date_time.wday;

	if(weekday < 0 || weekday > 6)
	{
		DBGMSG("errror weekday\n");
		weekday = 0;
	}
	DBGMSG("weekday = %d\n", weekday);

	switch(pattern)
	{
	case 0: //0: hour, minute, year, month, day, weekday 
		snprintf(datetime_info, size, "%s %s%04d %02d %02d %s", _("Current date and time:"),
			strhour, date_time.year, date_time.mon, date_time.day, weekdayStr[weekday]);
	break;
	case 1: // 1: hour, minute, weekday, month, day, year
		snprintf(datetime_info, size, "%s %s%s %02d %02d %04d", _("Current date and time:"),
			strhour, weekdayStr[weekday], date_time.mon, date_time.day, date_time.year);
	break;
	case 2:// 2: hour, minute, weekday, day, month, year
		snprintf(datetime_info, size, "%s %s%s %02d %02d %04d", _("Current date and time:"),
			strhour, weekdayStr[weekday], date_time.day, date_time.mon, date_time.year);
	break;
	}
	DBGMSG("datetime_info = %s\n", datetime_info);
	
	multiline_list_add_item(id->mlist, datetime_info);

	id->tts_text[id->item_count] = datetime_info;

	id->item_count ++;	
}

static void 
add_battery_info (struct plex_info* id, char* battery_info, char* battery_info_tts,int size)
{
	int battery_lev;
	int battery_chg;
	int battery_mod;

	char* power_source;
	char* charge_state;

	int battery_levels = 5;
	int index;
	
	int supply = get_power_supply();

	DBGMSG("\ninfo supply = %d\n", supply);

	switch (supply) {

		case SUPPLY_BATTERY: {

			BatteryTTSStrInit();
			
			battery_mod = get_battery_module();
			battery_lev = get_battery_level(battery_mod);
			
			if (battery_mod == 0)
				power_source = _("power source: battery.");
			else if (battery_mod == 1)
				power_source = _("power source: dry cell.");
			else
				power_source = _("power source: error.");

			snprintf(battery_info, size, "%s\n%s%d/%d", power_source, 
				_("battery level:"), battery_lev,battery_levels);

			index = (battery_lev - 1)%battery_levels;
			snprintf(battery_info_tts, size, "%s:%s %s", power_source, 
				_("battery level:"), battery_tts_str[index]);

			multiline_list_add_item(id->mlist, battery_info);
			id->tts_text[id->item_count] = battery_info_tts;
			id->item_count ++;

		}
		break;

		case SUPPLY_USB: {

			power_source = _("power source: AC.");

			battery_chg = get_battery_status_charge();

			if (battery_chg == 1)
				charge_state = _("Battery charging.");
			else if(battery_chg == 2)
				charge_state = _("Battery fully charged.");
			else 
				charge_state = _("Battery error.");

			snprintf(battery_info, size, "%s\n%s", power_source, charge_state);

			multiline_list_add_item(id->mlist, battery_info);
			id->tts_text[id->item_count] = battery_info;
			id->item_count ++;
		}
		break;
		default:
			return;
	}
	
}

static void catTimerInfo(struct plextalk_timer *timer, int index, int hour_system,char* str, int size)
{
	int elapsed[] = {3, 5, 15, 30, 45, 60, 90, 120};
	int elapsedtime=0, remaintime=0;
	char tmstr[128];
	char *ampm_info[2];
	ampm_info[0] = _("AM");
	ampm_info[1] = _("PM");

	if(PLEXTALK_TIMER_ALARM == timer->function){
		strlcat(str, _("Alarm Timer"), size);
	}else{
		strlcat(str, _("Sleep Timer"), size);
	}

	memset(tmstr, 0x00, sizeof(tmstr));
	if(timer->type == PLEXTALK_TIMER_COUNTDOWN)
	{
		remaintime = plextalk_get_timer_countdown(index);
		elapsedtime = elapsed[timer->elapse] - remaintime;
		snprintf(tmstr,sizeof(tmstr), " %d %s %d %s", elapsedtime,
			_("minutes elapsed"), remaintime, _("minutes remaining.")); 			
	}
	else
	{
		if(hour_system){//24
			snprintf(tmstr, sizeof(tmstr)," %02d:%02d",  timer->hour, timer->minute); 
		}else{//12
			int ampm;
			int hour = hour24To12(timer->hour, &ampm);
			snprintf(tmstr, sizeof(tmstr)," %02d:%02d %s", hour, timer->minute, ampm_info[ampm]); 
		}
	}

	strlcat(str, tmstr, size);
	strlcat(str, "\n", size);
}

static void
add_timer_info (struct plex_info* id, char* timer_info, int size)
{
	struct plextalk_timer timer[2];
	int hour_system;

	CoolShmReadLock(g_config_lock);
	timer[0] = g_config->setting.timer[0];
	timer[1] = g_config->setting.timer[1];
	hour_system = g_config->hour_system;
	CoolShmReadUnlock(g_config_lock);
	
	if(0 == timer[0].enabled && 0 == timer[1].enabled)
	{
		DBGMSG("no add_timer_info d\n");
		return; 
	}
	
	strlcpy(timer_info, _("Timer infomation: "),size);
	strlcat(timer_info, "\n", size);
	if(timer[0].enabled)
	{
		strlcat(timer_info, _("Timer 1"), size);
		strlcat(timer_info, _(" "), size);
		catTimerInfo(&timer[0], 0, hour_system, timer_info, size);
	}
	
	if(timer[1].enabled)
	{
		strlcat(timer_info, _("Timer 2"), size);
		strlcat(timer_info, _(" "), size);
		catTimerInfo(&timer[1], 1, hour_system, timer_info, size);
	}
	DBGMSG("add_timer_info timer_info = %s\n", timer_info);

	multiline_list_add_item(id->mlist, timer_info);

	id->tts_text[id->item_count] = timer_info;

	id->item_count ++;
}

static int GetVerInfo(char *str, int len)
{
	int ret = 0;
	xmlDocPtr doc;
	xmlChar *buf = NULL;
		
	doc = xml_open((const xmlChar*)PLEXTALK_CONFIG_DIR "version.xml");
	if(doc){
		buf = xml_get(doc, (const xmlChar*)"/version/software");
		if(buf){
			ret = 1;
			strlcpy(str, (const char*)buf,len);
			xmlFree(buf);
		}
		xml_close(doc);
	}
	else{
		DBGMSG("read version.xml fail\n");	
	}
	return ret;
}


static void
add_version_info (struct plex_info* id, char* version_info, int size)
{
	char verStr[20];
	
	if (!GetVerInfo(verStr, sizeof(verStr))) 
		snprintf(version_info, size, "%s", _("Version: Unknown."));
	else
		snprintf(version_info, size, "%s%s", _("Version: "), verStr);

	DBGMSG("add_version_info version_no = %s\n", version_info);
	
	multiline_list_add_item(id->mlist, version_info);

	id->tts_text[id->item_count] = version_info;

	id->item_count ++;
}


void* info_create (void)
{	
	struct plex_info* new_info = malloc(sizeof(struct plex_info));
	if (!new_info) {
		printf("ERROR: malloc plex_info_create failure!\n");
		return NULL;
	}
	memset(new_info, 0, sizeof(struct plex_info));

	//ztz	--start--
	bindtextdomain("libinfo", PLEXTALK_I18N_DIR);
	bind_textdomain_codeset("libinfo", "UTF-8");
	WeekDayStrInit();
	//ztz	--end--
	
	info_progress = 1;

	return new_info;
}

static char *getContent(void)
{
	char *tts_str;
	//char *pstr = NULL;
	if(id->lines > id->cur_lines)
	{
		id->screen_lines = mulitline_list_get_lineScreen(id->mlist);
		multiline_list_keydown_nextScreen(id->mlist, id->next_lines);
		tts_str = multiline_list_get_current_NextScreen_text(id->mlist, id->cur_lines);
		if(tts_str){
			strlcpy(id->tts_str, tts_str, sizeof(id->tts_str));
		}
		if(id->cur_lines + id->screen_lines > id->lines)
		{
			id->next_lines = id->lines - id->cur_lines;
			id->cur_lines = id->lines;
		}
		else
		{
			id->cur_lines += id->screen_lines;
			id->next_lines = id->screen_lines; 
		}
	}
	else
	{
		tts_str = multiline_list_get_current_text(id->mlist);
		strlcpy(id->tts_str, tts_str, sizeof(id->tts_str));
	}
	DBGMSG("id->tts_str = %s\n", id->tts_str);
	char *p = id->tts_str;
	/* 过滤<  > 替换为空格，不发音*/
	while (*p) {
		if ((*p) == '<' || (*p) == '>') {
			*p = ' ';
		}
		p = g_utf8_next_char(p);
	}
	return id->tts_str;
}


char* get_special_voice(const char* express)
{
	char *pBuf;
	char *pStart;
	char *pEnd;
	int cnt = 0;
	int index = -1;
	if (express==NULL || !(*express))
		return NULL;
	DBGMSG("get_special_voice :%s\n",express);

	pBuf = (char*)malloc(INFO_SCR_LEN);
	memset(pBuf, 0, INFO_SCR_LEN);
	pStart = express;
	pEnd = express + strlen(express) - 1;


	while(cnt < INFO_SCR_LEN && pStart<=pEnd)
	{

		if(*pStart == 0x2F && *(pStart+1) == 0x35 && (pStart > express) && *(pStart - 1)) {// "/5"
			switch(*(pStart-1)){
				case '1':
					index = 0;
					break;
				case '2':
					index = 1;
					break;
				case '3':
					index = 2;
					break;
				case '4':
					index = 3;
					break;
				case '5':
					index = 4;
					break;		
			}
			if(index != -1){
				*(pBuf + cnt - 1) = 0x20;
				strcpy(pBuf + cnt, battery_tts_str[index]);
				cnt += strlen(battery_tts_str[index]);
				pStart++;
			}
			
		}else {
			*(pBuf + cnt++) = *pStart;
		}
		pStart++;
		
	}
	DBGMSG("get_special_voice :%s,cnt=%d\n",pBuf,cnt);
	return pBuf;
}

void free_special_voice(char* pBuf)
{
	if(pBuf)
		free(pBuf);
}


int multiNextItem(int iNextItem)
{
	int ret = 0;
	
	if(iNextItem){
		multiline_list_key_down(id->mlist);
	}
	if(multiline_list_get_end(id->mlist)){
		ret = -1; //end
		goto lexit;
	}
	id->mp3_item = -1;
	id->is_aftervoc = 0;
	id->cur_lines = 0;
	id->next_lines = 0;
	id->lines = multiline_list_isNextScreen(id->mlist); 
	if(id->is_voc)
	{
		int i;
		id->cur_item = multiline_list_get_curItem(id->mlist);
		id->mp3_item = -1;
		for(i = 0; i < id->voc_nums; i++)
		{
			if(id->cur_item == id->voc[i]->cur_item)
			{
				id->mp3_item = i;
				break;
			}
		}
		
		if(id->mp3_item >= 0)
		{
			if(!id->voc[id->mp3_item]->btext)
			{
				ret = 1; //end
				goto lexit;
			}
			else
			{
				id->is_aftervoc = 1;
			}
		}
	}
lexit:
	*(id->is_running) = 1;
	if(-1 == ret){
		DBGMSG("last item\n");
		id->info_exit = 1;
		voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
	}else if (1 == ret){
		DBGMSG("play audio\n");
		voice_prompt_music_ex(1, PLEXTALK_OUTPUT_SELECT_DAC,  &vprompt_cfg, id->voc[id->mp3_item]->vocfile, 
			id->voc[id->mp3_item]->start, id->voc[id->mp3_item]->len);
		id->play_type = PLAY_AUDIO;
	}else{
		char *tts_str = getContent();

#if 1
		tts_str = get_special_voice(tts_str);

#endif
		DBGMSG("play str\n");
		voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			tts_role, tts_str, strlen(tts_str));
		id->play_type = PLAY_TTS;

#if 1
		free_special_voice(tts_str);
	
#endif

		
	}
	return 0;	//play tts
}

static void OnInfoKeydown(WID__ wid, KEY__ key)
{
	printf("OnInfoKeydown key.ch = %d\n", key.ch);
	if(id->info_pause){
		DBGMSG("pause now\n");
		return;
	}

	if(id->info_exit) {
		voice_prompt_abort();
		*(id->is_running) = 0;
		endOfModal = 1;
		DBGMSG("endOfModal = 1\n");
		return;
	}
	
	if (NeuxAppGetKeyLock(0)) {
		if (!key.hotkey) {
			voice_prompt_abort();
			info_pause();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
			voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
				tts_lang, tts_role, _("Keylock"));
			info_resume();
		}
		return;
	}
	 
	switch (key.ch)
    {
		case MWKEY_INFO:	//next item
		{
			DBGMSG("Key info!\n");
			voice_prompt_abort();
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			*(id->is_running) = 1;
			
			multiNextItem(1);
		}
		break;	
 		case MWKEY_UP:
		case MWKEY_DOWN:
		case MWKEY_ENTER:
		case MWKEY_RIGHT:
		case MWKEY_LEFT:
		case VKEY_BOOK:
		case VKEY_MUSIC:
		case VKEY_RECORD:
		case VKEY_RADIO:
		case MWKEY_MENU:
		{
			voice_prompt_abort();
			*(id->is_running) = 1;
			voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
			voice_prompt_string(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
							tts_role, _("cancel"), strlen(_("cancel")));
			voice_prompt_music(1, &vprompt_cfg, VPROMPT_AUDIO_CANCEL);
			id->info_exit = 1;
			DBGMSG("cancle sound\n");
		}
		break;
	}
}

static void OnInfoTimer(WID__ wid)
{
	if (id->info_pause){
		return;
	}
	if ((*(id->is_running))) {
		return;
	}
	
	if(id->info_exit){
		endOfModal = 1;
		DBGMSG("endOfModal = 1\n");
		return;
	}

	*(id->is_running) = 1;
	if(id->is_aftervoc)
	{
		DBGMSG("voice_prompt_music_ex id->voc[id->mp3_item]->vocfile = %s,id->voc[id->mp3_item]->start = %d id->voc[id->mp3_item]->len=%d\n", 
			id->voc[id->mp3_item]->vocfile, id->voc[id->mp3_item]->start,
			id->voc[id->mp3_item]->len);
		voice_prompt_music_ex(1, PLEXTALK_OUTPUT_SELECT_DAC,  &vprompt_cfg, id->voc[id->mp3_item]->vocfile, 
			id->voc[id->mp3_item]->start, id->voc[id->mp3_item]->len);
		id->is_aftervoc = 0;
		id->play_type = PLAY_AUDIO;
		return;
	}

	if(id->lines > id->cur_lines)
	{	//more content
		char *tts_str = getContent();
#if 1
		tts_str = get_special_voice(tts_str);

#endif
		voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			tts_role, tts_str, strlen(tts_str)); 
		id->play_type = PLAY_TTS;
#if 1
		free_special_voice(tts_str);
			
#endif

		
		return;
	}
	multiNextItem(1);
}

static void OnInfoRedraw(WID__ wid)
{
	if(id->mlist)
		multiline_list_show(id->mlist);
}

static void OnInfoDestory(WID__ wid)
{
	
}

static void OnInfoGetFocus(WID__ wid)
{
	NhelperTitleBarSetContent(_("Information"));
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_INFO);
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_NO);
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, SBAR_CONDITION_ICON_NO);
}

static void CreateInfoFormMain ()
{
    widget_prop_t wprop;

	/* Create a New window */
	id->info_wid = NeuxFormNew(MAINWIN_LEFT, MAINWIN_TOP, MAINWIN_WIDTH, MAINWIN_HEIGHT);

	/* Grab Key */ 
//	info_grab_key();
	
	id->mlist = multiline_list_create(id->info_wid, NeuxWidgetWID(id->info_wid), MAINWIN_LEFT, 0, MAINWIN_WIDTH, MAINWIN_HEIGHT, 
		theme_cache.foreground, theme_cache.background);
	
	/* Create timer */
   	id->timer = NeuxTimerNew(id->info_wid, INFO_TIMER_LEN, -1);
	NeuxTimerSetCallback(id->timer, OnInfoTimer);
	TimerEnable(id->timer);
	
	/* Set callback for window events */	
	NeuxFormSetCallback(id->info_wid, CB_KEY_DOWN, OnInfoKeydown);
	NeuxFormSetCallback(id->info_wid, CB_EXPOSURE, OnInfoRedraw);
	NeuxFormSetCallback(id->info_wid, CB_FOCUS_IN, OnInfoGetFocus);
	NeuxFormSetCallback(id->info_wid, CB_DESTROY, OnInfoDestory);
	
	/* Set proproty(color)*/	
	NeuxWidgetGetProp(id->info_wid, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(id->info_wid, &wprop);


}

#if 0
static void info_onSWOn(int sw)
{
	if(sw == SW_KBD_LOCK)
	{
		id->key_locked = 1;
	}
}

static void info_onSWOff(int sw)
{
	if(sw == SW_KBD_LOCK)
	{
		id->key_locked = 0;
	}
}
#endif

void info_start (void* info_id, char** info_text, int text_num, int* is_running)
{
	info_start_ex(info_id, info_text, text_num, NULL, 0, is_running);
}

void info_start_ex (void* info_id, char** info_text, int text_num, struct vocinfo** voc_info, int voc_num, int* is_running)
{
	int i;
	char battery_info[128];
	char battery_info_tts[128];
	char timer_info[256];
	char version_info[128];
	char datetime_info[256];
	int timer_locked = 0, bchange = 0;

	if(voc_num >= INFO_ITEM_MAX || text_num >= INFO_ITEM_MAX)
	{
		DBGMSG("voc_num is error\n");
		return;
	}
	
	id = info_id;
	if(id == NULL)
	{
		DBGMSG("info_id is null\n");
		return;
	}

	plextalk_schedule_lock();
//	plextalk_set_menu_disable(1);
	NhelperInfoActive(1);

	if(voc_num > 0 && voc_info)
	{
		id->is_voc = 1;
	}

	id->is_running = is_running;
	
	/* Abort TTS first */
	voice_prompt_abort();
	*(id->is_running) = 0;

	/* Create multiline */
	id->info_index = 0;
	id->item_count = 0;
	timer_locked = plextalk_sleep_timer_is_locked();
	if(timer_locked)
	{
		bchange = 1;
		plextalk_sleep_timer_unlock();
	}

	CreateInfoFormMain();

	add_datetime_info(id, datetime_info, 256);

	/* Battery and Timer infomation (This are common) */
	if(!id->is_mainmenu)
	{
		add_battery_info (id, battery_info, battery_info_tts,128);
		add_timer_info (id, timer_info, 256);
	}

	/* Add infomation */
	if(id->is_voc)
	{
		int i;
		id->voc = voc_info;
		id->voc_nums = voc_num;
		for(i=0; i<voc_num; i++){
			voc_info[i]->cur_item += id->item_count;
		}
	}
	
	for (i = 0; i < text_num; i++) {
		multiline_list_add_item(id->mlist, info_text[i]);
		id->tts_text[id->item_count] = info_text[i];
		id->item_count ++;
	}

	DBGMSG("id->item_count = %d\n", id->item_count);
	/* Version infomation */
	if(!id->is_mainmenu)
	{
		add_version_info (id, version_info, 128);
	}

	multiNextItem(0);
	id->info_pause = 0;
	
	/* Show window */
    NeuxWidgetShow(id->info_wid, TRUE);
	NeuxWidgetFocus(id->info_wid);

//	NeuxWMAppSetCallback(CB_SW_ON,   info_onSWOn);
//	NeuxWMAppSetCallback(CB_SW_OFF,  info_onSWOff);

	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	
	multiline_list_destroy(id->mlist);
//	info_ungrab_key();
	NeuxFormDestroy(&id->info_wid);
	plextalk_schedule_unlock();
//	plextalk_set_menu_disable(0);

	NhelperInfoActive(0);
	if(bchange)
	{
		plextalk_sleep_timer_lock();
	}

}

int info_pause()
{
	DBGMSG("puase\n");
	if(id && !id->info_pause)
	{
		TimerDisable(id->timer);
		*(id->is_running) = 0;
		id->info_pause = 1;
	}
	return 0;
}

int info_resume()
{
	DBGMSG("resume\n");
	if(id && id->info_pause)
	{
		id->info_pause = 0;
		if(id->info_exit){
			endOfModal = 1;
			return 1;
		}
		
		TimerEnable(id->timer);
		*(id->is_running) = 1;
		if (PLAY_AUDIO == id->play_type){
			DBGMSG("play audio\n");
			voice_prompt_music_ex(1, PLEXTALK_OUTPUT_SELECT_DAC,  &vprompt_cfg, id->voc[id->mp3_item]->vocfile, 
				id->voc[id->mp3_item]->start, id->voc[id->mp3_item]->len);
		}else if(PLAY_TTS == id->play_type){
			char *tts_str = id->tts_str;
			DBGMSG("play str\n");
			voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
				tts_role, tts_str, strlen(tts_str));
			id->play_type = PLAY_TTS;
		}
	}
	return 1;
}

int info_set_menu()
{
	if(id)
	{
		id->is_mainmenu = 1;
	}
	return 1;
}

/* func:Abort infomation tts
 * it will do nothing when there is no info progress
 */
int info_abort (void* info_id)
{
	struct plex_info* id = info_id;

	if (!info_progress)
		return 0;
	voice_prompt_abort();
	*(id->is_running) = 0;

	endOfModal = 1;

	return 1;
}

void info_setkeylock(int key_locked)
{
	if(id)
	{
		id->key_locked = key_locked;
	}
}

void info_destroy (void* info_id)
{
	free(info_id);
	info_progress = 0;
}

