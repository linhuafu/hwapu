#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "radio_rec_extern.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"


/* For record file to storage */
#define REC_STORE_PATH_INNER	"/media/mmcblk0p2/RadioRecording/"
#define REC_STORE_PATH_SDCRD	"/media/mmcblk1p1/RadioRecording/"
#define REC_STORE_PATH_SDCRD1	"/media/mmcblk1/RadioRecording/"

/* For get remain memory */
#define DISK_SDCRD				"/media/mmcblk1p1"
#define DISK_SDCRD1			"/media/mmcblk1"
#define DISK_INNER				"/media/mmcblk0p2"


/* Reserved memory */
#define MIN_STORE_RESERVED	100 * 1024 * 1024


/* Get recorder filename thought current time */
char* 
radio_rec_get_filename (void)
{	
	static char  file_name[32];

	time_t now = time (NULL);
	struct tm* now_tm = localtime (&now);
	int sec  = now_tm->tm_sec;
	int min  = now_tm->tm_min;
	int hour = now_tm->tm_hour;			
	int day  = now_tm->tm_mday;
	int mon  = now_tm->tm_mon+1;
	int year = now_tm->tm_year + 1900;
	year=year%100;          //年取后两位

	memset (file_name, 0, 32);
	snprintf (file_name, 32, "%02d%02d%02d_%02d%02d%02d.mp3",
		year, mon, day, hour, min, sec);
	
	return file_name;
}

/* Get disk free memory */
unsigned long long 
radio_rec_get_storage (enum store_type type)
{
	struct statfs diskinfo;
	const  char*  disk_path = NULL;
	
	switch (type) {
		case REC_STORE_INTER_MEM:
			disk_path = DISK_INNER;
			break;

		case REC_STORE_SD_CARD:
		{
			DIR* t = opendir(DISK_SDCRD);
			if (!t) {
				disk_path = DISK_SDCRD1;
			}
			else{
				disk_path = DISK_SDCRD;
				closedir(t);
			}
		}
		break;
	}
	
	statfs (disk_path, &diskinfo);
	unsigned long long freedisk 
		= diskinfo.f_bfree * diskinfo.f_bsize;

	if (freedisk <= MIN_STORE_RESERVED) {
//		printf ("Disk storage not enough!\n");
		return 0;
	}

	return freedisk;
}

int radio_rec_get_remaintime(enum store_type type)
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
	rec_remaintime = (int)(freedisk*8/(long long)128000); //录音

	DBGMSG("remaintime:%d\n",rec_remaintime);

	return rec_remaintime;//最在100H*3600
	
}

int radio_rec_get_fake_remaintime(enum store_type type)
{
	if (type)
	{
		return radio_rec_get_remaintime(REC_STORE_INTER_MEM);
	}
	else {
		if(check_sd_exist()) {
			return radio_rec_get_remaintime(REC_STORE_SD_CARD);
		} else {
			return 0;
		}
	}
	
}
unsigned long long radio_rec_get_freespace(enum store_type type)
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



#if 0
/* Conver time to time_string*/
char* 
radio_rec_time_to_string (int time)
{
	static char buf[16];
	
	memset(buf, 0, 16);
	time_t thistime = time;
	struct tm* now = localtime (&thistime);
	time_t hour = now->tm_hour;
	
	sprintf (buf, "%02d:%02d:%02d", (int) hour, 
		(int) now->tm_min, (int) now->tm_sec);	

	return buf;	
}
#else
char*  
radio_rec_time_to_string (int time)
{
	int hour = 0;
	int min = 0;
	int sec = 0;
	static char buf[16];
	memset(buf, 0, 16);

	hour = ((int)time/3600)%100;
	min  = ((int)(time-hour*3600)/60)%60;
	sec  = (int)time%60;

	
	sprintf(buf, "%02d:%02d:%02d", (int)hour, (int)min, (int)sec);	
	return buf;
}

#endif


/* Get recording time */
int radio_rec_get_time(char* file_path)
{
	struct stat64 info;
	
	if (stat64 (file_path, &info) == -1)
		return 0;
	
	return (info.st_size * 8) / 128000;		
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
		DBGMSG("fatal error!\n");
		return NULL;
	}

	if (store_type)
		store_path = REC_STORE_PATH_INNER;
	else
	{
		DIR* t = opendir(DISK_SDCRD);
		if (!t) {
			store_path = REC_STORE_PATH_SDCRD1;
		}
		else{
			store_path = REC_STORE_PATH_SDCRD;
			closedir(t);
		}
	}

	DBGMSG("store_path = %s\n", store_path);
	t_dir = opendir(store_path);
	
	if (!t_dir) {
		DBGMSG("No such directory , build it!\n");
		
		if (mkdir(store_path, O_CREAT) == -1) {
			DBGMSG("build error!\n");
			return NULL;
		} else {
			DBGMSG("build new directory success!\n");
		}
	}

	memset(path, 0, 256);
	snprintf(path, 256, "%s%s", store_path, file_name);

	return path;
}


/* If record file is less than 1s, this file will be delete */
void
check_record_file(char* filepath)
{
	struct stat64 info;
	stat64(filepath, &info);

	if (info.st_size <= 0)	
		remove(filepath);
	
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
			DBGMSG("SD card not exist\n");
			return 0;
		}
	}

	closedir(t);
	return 1;
}

int radio_rec_file_uplimit (const char* path)
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
	return (size >= 2*1024*1024*1024UL);

}
