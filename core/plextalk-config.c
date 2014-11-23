
#include "nc-err.h"
#include "plextalk-config.h"

//wgp  --start--
#include <libxml/parser.h>
#include <libxml/tree.h>

//#define  DEVICE_PATH  "/media/mmcblk0p2/.plextalk/DeviceConfigurationFile.xml"

#define  DEVICE_PATH  "/opt/plextalk/data/config/DeviceConfigurationFile.xml"

struct plextalk_global_config* g_config;
shm_lock_t* g_config_lock;



void Get_Device_Setting_Xml()
{


	xmlDocPtr doc=NULL;
	xmlNodePtr    cur=NULL;
	char* name=NULL;
	char* value=NULL;

	xmlKeepBlanksDefault (0); 


	
	 printf("enter Get_Device_Setting_Xml .\n");
	
	//create Dom tree
	doc=xmlParseFile(DEVICE_PATH);
	if(doc==NULL)
	{
	   printf("ERROR: Loading xml file failed.\n");
	   return;
	}

	// get root node
	cur=xmlDocGetRootElement(doc);
	if(cur==NULL)
	{
	   printf("ERROR: empty file\n");
		
		g_config->auto_poweroff_time=15;
		g_config->time_format=0;
		g_config->hour_system=0;
		g_config->radio_hi=108.0;
		g_config->radio_low=87.0;
		g_config->langsetting=0;
		g_config->tts_setting=0;

		
	   xmlFreeDoc(doc); 
	    return;
	}

	//walk the tree 
	cur=cur->xmlChildrenNode; //get sub node
	cur=cur->xmlChildrenNode; //get sub node
	while(cur !=NULL)
	{
	   name=(char*)(cur->name); 
	      
	   xmlChar* szAttr = xmlGetProp(cur,BAD_CAST "select");
	   printf("DEBUG: name is:%s, value is: %s\n",name,(char *)szAttr);
	   if(!strcmp(name,"AutoPowerOFFTime"))
	   {	
			g_config->auto_poweroff_time=atoi(szAttr);
			
			 printf("auto_poweroff_time:%d\n",g_config->auto_poweroff_time);
	   }
	   else if(!strcmp(name,"DayControl"))
	   {
			g_config->time_format=atoi(szAttr);
		
		 printf("time_format:%d\n",g_config->time_format);
	   }
	   
	    else if(!strcmp(name,"HourFormat"))
	   {
			g_config->hour_system=atoi(szAttr);
		
		 printf("hour_system:%d\n",g_config->hour_system);
	   }

	   else if(!strcmp(name,"MaximumFrequencyRangeControl"))
	   {
			g_config->radio_hi=atof(szAttr);
		
		 printf("radio_hi:%f\n",g_config->radio_hi);
	   }

	    else if(!strcmp(name,"MinimumFrequencyRangeControl"))
	   {
			g_config->radio_low=atof(szAttr);
		
		 printf("radio_low:%f\n",g_config->radio_low);
	   }

	   else if(!strcmp(name,"LanguageSetControl"))
	   {
			g_config->langsetting=atoi(szAttr);
		
		 printf("langsetting:%d\n",g_config->langsetting);
	   }
	 else if(!strcmp(name,"TTSSettingMenuControl"))
	   {
			g_config->tts_setting=atoi(szAttr);
		
		 printf("tts_setting:%d\n",g_config->tts_setting);
	   }

	   
	   xmlFree(szAttr); 
	   cur=cur->next;
	}

	 if(g_config->tts_setting==1)
	{
		g_config->setting.tts.chinese = 0;		
	}
	// release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return ; 

}

//wgp --end--


#if 0
	//下面这个函数是为升级时候的一个bug
#endif
static int
need_correct_lang_setting (void)
{
	int ret;

	int Device_lang_setting;
	const char *usr_setting_lang;
	
	CoolShmReadLock(g_config_lock);
	Device_lang_setting = g_config->langsetting;
	usr_setting_lang = g_config->setting.lang.lang;
	CoolShmReadUnlock(g_config_lock);

	printf("%s, %d: Device_lang_setting = %d, usr_setting_lang = %s\n", __func__, 
		__LINE__, Device_lang_setting, usr_setting_lang);

	if (Device_lang_setting == 0) {	 //  3 : 0-english, 1- zh_CN, 2 - zh_TW
		if (!strcmp(usr_setting_lang, "hi_IN")) {
			printf("err: usr_setting_lang hi_IN, but current no support hi_IN(chinese)\n");
			return 1;
		} else
			return 0;
	} else if (Device_lang_setting == 1){		// 2 : 1->english 2->hi_CN
		if (!strcmp(usr_setting_lang, "zh_CN") || !strcmp(usr_setting_lang, "zh_TW")) {
			printf("err: usr_setting_lang zh_CN or zh_TW, but current no support this(hi_IN)\n");
			return 1;
		} else
			return 0;
	} else {
		//all suport
		return 0;
	}
}


int plextalk_global_config_init(void)
{
	int first;
	int need_fix = 0;

	g_config = CoolShmGet(PLEXTALK_GLOBAL_CONFIG_SHM_ID, sizeof(struct plextalk_global_config),
	                      &first, &g_config_lock);
	if (!g_config || !first)
	{
		WARNLOG("Unable to initialize memory, failed to initialize global config.\n");
		return -1;
	}

	memset(g_config, 0, sizeof(*g_config));
//reserved here ...(should be deleted)	
	memset(g_config, 0, sizeof(struct plextalk_global_config)); 

#if 0
	if (plextalk_setting_read_xml(PLEXTALK_SETTING_DIR "setting.xml") != 0) {
		need_fix = 1;	//if read setting xml err!, need to set lang with Device_Setting_xml
		plextalk_setting_read_xml(PLEXTALK_CONFIG_DIR "setting.xml");
	}
	
	if (plextalk_volume_read_xml(PLEXTALK_SETTING_DIR "volume.xml") != 0)
		plextalk_volume_read_xml(PLEXTALK_CONFIG_DIR "volume.xml");

	 Get_Device_Setting_Xml();	//wgp
#endif

	Get_Device_Setting_Xml();	//wgp

	if (plextalk_volume_read_xml(PLEXTALK_SETTING_DIR "volume.xml") != 0)
		plextalk_volume_read_xml(PLEXTALK_CONFIG_DIR "volume.xml");

	if (plextalk_setting_read_xml(PLEXTALK_SETTING_DIR "setting.xml") != 0) {
		need_fix = 1;	//if read setting xml err!, need to set lang with Device_Setting_xml
		plextalk_setting_read_xml(PLEXTALK_CONFIG_DIR "setting.xml");
	} else 
		need_fix = need_correct_lang_setting();		//need check usr_setting between dev_setting
	
//ztz , ---start---			

/* fix (g_config->setting.lang.lang) with Device_Setting_Xml */
	static char* DeviceConfigLangStr[] = {
		"zh_CN",
		"hi_IN",
		"en_US",			//to be considered...
	};

	printf("need_fix = %d\n", need_fix);
	
	if (need_fix) {
		CoolShmWriteLock(g_config_lock);
		strlcpy(g_config->setting.lang.lang, DeviceConfigLangStr[g_config->langsetting], 
			sizeof(g_config->setting.lang.lang));
		CoolShmWriteUnlock(g_config_lock);
	}
//ztz, ---end---
	
	//plextalk_time_system_init(); //wgp

	return 0;
}


int plextalk_global_config_open(void)
{
	int first;

	g_config = CoolShmGet(PLEXTALK_GLOBAL_CONFIG_SHM_ID, sizeof(struct plextalk_global_config),
	                      &first, &g_config_lock);
	if (!g_config)
	{
		WARNLOG("Unable to initialize memory, failed to initialize global config.\n");
		goto bail_no_shm;
	}
	if (first)
	{
		WARNLOG("Global config shared memory in wrong state.\n");
		goto bail;
	}

	return 0;

bail:
	CoolShmPut(PLEXTALK_GLOBAL_CONFIG_SHM_ID);
bail_no_shm:
	return -1;
}

void plextalk_global_config_close(void)
{
	CoolShmPut(PLEXTALK_GLOBAL_CONFIG_SHM_ID);
}

#define PLEXTALK_LOCK_FUNC(name) \
int plextalk_ ## name ## _lock(void) \
{ \
	int lock; \
 \
	CoolShmWriteLock(g_config_lock); \
	lock = g_config->name ## _lock++; \
	CoolShmWriteUnlock(g_config_lock); \
 \
	return lock; \
} \
int plextalk_ ## name ## _unlock(void) \
{ \
	int lock; \
 \
	CoolShmWriteLock(g_config_lock); \
	lock = g_config->name ## _lock; \
	if (lock > 0) \
		g_config->name ## _lock--; \
	CoolShmWriteUnlock(g_config_lock); \
 \
	return lock; \
} \
int plextalk_ ## name ## _is_locked(void) \
{ \
	int lock; \
 \
	CoolShmWriteLock(g_config_lock); \
	lock = g_config->name ## _lock; \
	CoolShmWriteUnlock(g_config_lock); \
 \
	return lock; \
}

PLEXTALK_LOCK_FUNC(suspend)
PLEXTALK_LOCK_FUNC(schedule)
PLEXTALK_LOCK_FUNC(sleep_timer)

unsigned int plextalk_get_timer_countdown(int index)
{
	unsigned int countdown;

	CoolShmReadLock(g_config_lock);
	countdown = g_config->timer_countdown[index];
	CoolShmReadUnlock(g_config_lock);

	return countdown;
}

#define PLEXTALK_ACCESS_FUNC(memb) \
int plextalk_get_ ## memb(void) \
{ \
	int value; \
 \
	CoolShmReadLock(g_config_lock); \
	value = g_config->memb; \
	CoolShmReadUnlock(g_config_lock); \
 \
	return value; \
} \
void plextalk_set_ ## memb(int value) \
{ \
	CoolShmWriteLock(g_config_lock); \
	g_config->memb = value; \
	CoolShmWriteUnlock(g_config_lock); \
}

PLEXTALK_ACCESS_FUNC(statusbar_category_icon)
PLEXTALK_ACCESS_FUNC(alarm_disable)
PLEXTALK_ACCESS_FUNC(menu_disable)

PLEXTALK_ACCESS_FUNC(help_disable)
PLEXTALK_ACCESS_FUNC(hotplug_voice_disable)//add by km
PLEXTALK_ACCESS_FUNC(SD_hotplug_voice_disable)//add by km
PLEXTALK_ACCESS_FUNC(offtimer_disable)//add by km
PLEXTALK_ACCESS_FUNC(volume_vprompt_disable)
PLEXTALK_ACCESS_FUNC(auto_off_disable)