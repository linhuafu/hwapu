#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include "rec_extern.h"
#define DEBUG_PRINT   1
//#include "info_print.h"
//#include "sys_config.h"
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "plextalk-config.h"
#include "Sysfs-helper.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


/*reserved memory , 100M for debug*/
#define MIN_STORE_RESERVED	100 * 1024 * 1024

#define REC_STORE_PATH_INNER	"/media/mmcblk0p2/MicrophoneRecording/"
#define REC_STORE_PATH_SDCRD	"/media/mmcblk1p1/MicrophoneRecording/"
#define REC_STORE_PATH_SDCRD1	"/media/mmcblk1/MicrophoneRecording/"//"/media/mmcblk1p1/MicrophoneRecording/"


/* For get remain memory */
#define DISK_SDCRD				"/media/mmcblk1p1"
#define DISK_SDCRD1				"/media/mmcblk1"//"/media/mmcblk1p1"
#define DISK_INNER				"/media/mmcblk0p2"


bool 
check_store (enum store_type type)
{
	if (rec_get_storage(type) <= MIN_STORE_RESERVED)
		return false;

	return true;
}

/* Get disk free memory */
unsigned long long 
rec_get_storage (enum store_type type)
{
	struct statfs diskinfo;
	const  char*  disk_path = NULL;
	DIR* t ;//= opendir(DISK_SDCRD);
	
	switch (type) {
		case REC_STORE_INTER_MEM:
			disk_path = DISK_INNER;
			break;

		case REC_STORE_SD_CARD:
			disk_path = DISK_SDCRD;	
			if(!opendir(disk_path))
			{
				disk_path = DISK_SDCRD1;	
			}
			break;

	}
	
	statfs (disk_path, &diskinfo);
	unsigned long long freedisk 
		= diskinfo.f_bfree * diskinfo.f_bsize;

	if (freedisk <= MIN_STORE_RESERVED) {
		printf ("Disk storage not enough!\n");
		return 0;
	}

	return freedisk;
}

static int 
check_file_name(const char* filepath)
{
	struct stat info;

	stat(filepath, &info);
	if (S_ISREG(info.st_mode)) {
		info_debug("This filename is not a new name!\n");
		return 0;
	} else {
		info_debug("This filename is a new name!\n");
		return 1;
	}
}


/*Need to handle if there this filename already exits*/
char* 
rec_get_filename (void)
{
	int i = 1;
	time_t now = time(NULL);
	struct tm* now_tm = localtime(&now);
	int sec  = now_tm->tm_sec;
	int min  = now_tm->tm_min;
	int hour = now_tm->tm_hour;
	int day  = now_tm->tm_mday;
	int mon  = now_tm->tm_mon+1;
	int year = now_tm->tm_year + 1900;
	year=year%100;
	static char  file_name[32];
	memset(file_name, 0, 32);

	snprintf(file_name, 32, "%02d%02d%02d_%02d%02d%02d.mp3", 
		year, mon, day, hour, min, sec);

	//it will set file_name until this is a new file name
	while (!check_file_name(file_name)) {
		snprintf(file_name, 32, "%02d%02d%02d_%02d%02d%02d_%d.mp3", 
			year, mon, day, hour, min, sec, i);
		i++;
	}

	return file_name;
}

#if 0//到24小时后会wrap
char*  
time_to_string (int time)
{
	static char buf[16];
	memset(buf, 0, 16);
	time_t thistime = time;
	struct tm* now = localtime(&thistime);

	time_t hour = now->tm_hour;

	sprintf(buf, "%02d:%02d:%02d", (int)hour, (int)now->tm_min, (int)now->tm_sec);	
	return buf;
}
#else
char*  
time_to_string (int time)
{
	int hour = 0;
	int min = 0;
	int sec = 0;
	static char buf[16];
	memset(buf, 0, 16);

	hour = ((int)time/3600)%1000;
	min  = ((int)(time-hour*3600)/60)%60;
	sec  = (int)time%60;

	
	sprintf(buf, "%03d:%02d:%02d", (int)hour, (int)min, (int)sec);	
	return buf;
}

#endif

#define HEADPHONE_JACK	"/sys/devices/platform/jz-codec/headphone_jack"

int 
mic_headphone_on (void)
{
	int hp_online;

	sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);
	
	info_debug("mic_headphone hp_online :%d",hp_online);
	
	return hp_online;
}


/* Get the recording file path via rec_media */
char* 
get_store_path (const char* file_name, int store_type)
{
	static char path[256];
	DIR* t_dir;
	const char* store_path = NULL;

	/* fix sotre_type */
	if ((store_type != 0) && (store_type != 1)) {
		info_debug("fatal error!\n");
		return NULL;
	}

	if (store_type)
		store_path = REC_STORE_PATH_INNER;
	else
		store_path = REC_STORE_PATH_SDCRD;
	
	t_dir = opendir(store_path);
	
	if (!t_dir) 
	{
		info_debug("No such directory , build it!\n");
		
		if (mkdir(store_path, 0755) == -1) 
		{
			info_debug("build %s error!\n",store_path);

			//SD卡需要判断两次路径
			if(!store_type)
			{
				store_path = REC_STORE_PATH_SDCRD1;
				t_dir = opendir(store_path);
				if (!t_dir)
				{
					info_debug(" 2 No such directory , build it!\n");

					if (mkdir(store_path, 0755) == -1) 
					{
						info_debug("2 build %s error!\n",store_path);
						return NULL;			
					}
				}
			}
		}
	}
	info_debug("get_store_path path:%s \n",store_path);
	memset(path, 0, 256);
	snprintf(path, 256, "%s%s", store_path, file_name);

	return path;
}

//get the file path but not really create the folder in the target media
char* 
get_store_fake_path (const char* file_name, int store_type)
{
	static char path[256];
	DIR* t_dir;
	const char* store_path = NULL;

	/* fix sotre_type */
	if ((store_type != 0) && (store_type != 1)) {
		info_debug("fatal error!\n");
		return NULL;
	}

	if (store_type)
		store_path = REC_STORE_PATH_INNER;
	else
		store_path = REC_STORE_PATH_SDCRD;
	
	info_debug("get_store_path path:%s \n",store_path);
	memset(path, 0, 256);
	snprintf(path, 256, "%s%s", store_path, file_name);

	return path;
}




/* If record file is less than 1s, this file will be delete */
void
check_record_file (const char* filepath)
{
	struct stat64 info;

	if(!filepath){
		return;
	}
	
	info_debug("enter check_record_file:%s!\n",filepath);
	
	if(!stat64(filepath, &info))
	{

		info_debug("The file size is: %d \n",info.st_size);

		if (info.st_size <= 0)	
		remove(filepath);
	}

	//fflush the cache
	system("sync");

	
}


/* it will return 1 if sd_card exist, or it will return 0 */
int check_sd_exist (void)
{
	DIR* t = opendir(DISK_SDCRD);

	if (!t) {

		t = opendir(DISK_SDCRD1);
		if(!t)
		{
			info_debug("SD card not exist\n");
			return 0;
		}
		
	}

	closedir(t);
	return 1;
}


#define KEYLOCK  "/sys/devices/platform/gpio-keys/on_switches"

int 
keylock_locked (void)
{
	char res;
	
	int fd = open(KEYLOCK, O_RDONLY);

	if (fd == -1) {
		printf("open key lock error!\n");
		return 0;
	}
	
	if ((read(fd, &res, 1)) != 1) {
		printf("read key_lock error!\n");
		close(fd);
		return 0;
	}

	close(fd);

	if (res == '0') {
		printf("[keypad]:locked\n");
		return 1;
	} else {
		printf("[keypad]:unlocked\n");
		return 0;
	}
}


#define RECORD_BIT_RATE	128000

int rec_get_remaintime(enum store_type type)
{
	struct statfs diskinfo;
	const char* filepath = NULL;
	unsigned long long freedisk;
	int rec_remaintime = 0;

	if (type)
	{
		filepath = DISK_INNER;
	}
	else{
		DIR* t = opendir(DISK_SDCRD);
		if (!t) {
			filepath = DISK_SDCRD1;
		}
		else{
			filepath = DISK_SDCRD;
			closedir(t);
		}
	}
	statfs (filepath, &diskinfo);
	freedisk = (long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize;

	//只显示2G范围
	if(freedisk > (2*1024*1024*1024UL))
		freedisk = (2*1024*1024*1024UL);

	
	//计算剩余时间
	rec_remaintime = (int)(freedisk*8/(long long)RECORD_BIT_RATE); //录音
	DBGMSG("remaintime:%d\n",rec_remaintime);

	return rec_remaintime;
	
}

unsigned long long rec_get_freespace(enum store_type type)
{
	struct statfs diskinfo;
	const char* filepath = NULL;
	unsigned long long freedisk;
	int rec_remaintime = 0;

	if (type)
	{
		filepath = DISK_INNER;
	}
	else{
		DIR* t = opendir(DISK_SDCRD);
		if (!t) {
			filepath = DISK_SDCRD1;
		}
		else{
			filepath = DISK_SDCRD;
			closedir(t);
		}
	}
	statfs (filepath, &diskinfo);
	freedisk = (long long)diskinfo.f_bfree * (long long)diskinfo.f_bsize;
	DBGMSG("freedisk:%lld\n",freedisk);

	return freedisk;
	
}




int rec_get_fake_remaintime(enum store_type type)
{
	if (type)
	{
		return rec_get_remaintime(REC_STORE_INTER_MEM);
	}
	else {
		if(check_sd_exist()) {
			return rec_get_remaintime(REC_STORE_SD_CARD);
		} else {
			return 0;
		}
	}
	
}


int 
rec_file_uplimit (const char* path)
{
	unsigned int size;
	struct stat64 buf;
	if(!path)
		return 0;
	//if(strlen(path) <= 0)
	//	return 0;
	//DBGMSG("rec_file_uplimit %s\n",path);
	if (stat64(path, &buf) < 0) {
		return 0;
	}
	size = buf.st_size;
	
	//DBGMSG("rec_file_uplimit size:%d\n",size);
	return (size >= (2*1024 - 3)*1024*1024UL);
}



