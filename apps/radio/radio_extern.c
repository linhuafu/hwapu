#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "sysfs-helper.h"


int check_keylocedstatus(void)
{
	char buf[16];
	int res;
	int keylock = 0;

	memset(buf, 0, sizeof(buf));
	res = sysfs_read("/sys/devices/platform/gpio-keys/on_switches", buf, sizeof(buf));
	if (!strcmp(buf, "0"))
		keylock = 1;
	else
		keylock = 0;
	
	return keylock;
}


char* radio_get_freq_str (double freq)
{
	static char str[16];

	memset (str, 0, 16);
	sprintf (str, "%.1fMHz", freq); 

	return str;
}


