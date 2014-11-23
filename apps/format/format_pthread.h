#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>


//使用U盘的一个文件来保存系统设置
#define USE_UDISK_PLEXTAL_SETTING	0



int format_prepare (const char* curMedia);
int start_format(const char* curMedia);
int stop_format(void);
void wait_format_finish();
int format_pthread_exist();


