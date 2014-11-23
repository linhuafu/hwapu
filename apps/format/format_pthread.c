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
#include <sys/vfs.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "storage.h"
#include "Nxutils.h"
#include "format_pthread.h"


static pthread_cond_t  cond;
static pthread_mutex_t lock;
static GstElement *pipeline;
static GMainLoop  *loop;
static pthread_t pid = -1;

extern void
format_restore_setting_outer();
extern void
format_save_setting_outer();


static void*
format_pipeline_create (void* arg);


//check the format pthread is exist
int format_pthread_exist()
{
	if(pid != -1) {
		return !pthread_kill(pid,0);
	}
	return 0;
}

static int format_media_enum(struct mntent *mount_entry, void* user_data)
{
	//char **ptr = user_data;

	if (StringStartWith(mount_entry->mnt_fsname, user_data)) {
		//*ptr = strdup(mount_entry->mnt_fsname);
		return 1;
	}

	return 0;
}

int format_media_check(char *device)
{
	return CoolStorageEnumerate(format_media_enum, device);
}



int format_prepare (const char* curMedia)
{

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
	
	if (pthread_create(&pid, NULL, format_pipeline_create, (void*)curMedia)) {
		DBGMSG("format_prepare start pthread failure!\n");
		return -1;
	}
	//pthread_mutex_lock(&lock);
	//pthread_cond_wait(&cond, &lock);
	//pthread_mutex_unlock(&lock);

	return 0;
}


static void*
format_pipeline_create (void* arg)
{	
	
	const char* value = arg;
	DBGMSG("Enter the pipeline create :%s.\n",value);
	//GstElement *src,*enc, *out_file;
	
	//gst_init(NULL, NULL);
		
	//loop = g_main_loop_new(NULL, FALSE);
	//if (!loop) {
	//	DBGMSG("g_main_loop_new error!\n"); 
	//	return NULL;
	//}
	
	//pipeline = gst_pipeline_new ("fpipeline");
	//if (!pipeline) {
	//	DBGMSG("gst_pipeline_new error!\n");
	//	return NULL;
	//}
	
	//src 	 = gst_element_factory_make("alsasrc", "alsasrc");
	//enc 	 = gst_element_factory_make("shinemp3enc", "mp3_encoder");
	//out_file = gst_element_factory_make("filesink", "filesink");

	//if (!src || !enc || !out_file) {
	//	DBGMSG("Gst_element_factory_make failure!\n");
	//	return NULL;
	//}
	
	//g_object_set(G_OBJECT(enc), "bitrate", 128, NULL);

	#if 0
	g_object_set(G_OBJECT(enc), "mode", 1, NULL);		//stereo
	#endif

	//g_object_set(G_OBJECT(out_file), "location", f_path, NULL);

	//gst_bin_add_many(GST_BIN(pipeline), src, enc, out_file, NULL);
	//gst_element_link_many(src, enc, out_file, NULL);

	//gst_element_set_state(pipeline, GST_STATE_READY);

	//pthread_cond_signal(&cond);
	//recorder_stat = PREPARE;
	
	//g_main_loop_run(loop);

	//gst_element_set_state (pipeline, GST_STATE_NULL);
	//gst_object_unref (GST_OBJECT (pipeline));
	//g_main_loop_unref (loop);

	//保存系统设置文件
	//
	//更换成了"/opt/plextalk/usr_setting/"所以不需要保存设置
#if USE_UDISK_PLEXTAL_SETTING	
	format_save_setting_outer();
#endif
	//end

	char cmd[1024];

	if(format_media_check(value) == 1) {
		memset(cmd,0x00,1024);
		snprintf(cmd,1024, "umount -l %s", value);//umount
		DBGMSG("cmd = %s\n", cmd);
		system(cmd);
		DBGMSG("Enter umount.\n");
	} else {
		DBGMSG("no need umount\n");
	}

	memset(cmd,0x00,1024);
	snprintf(cmd,1024, "mkfs.vfat %s", value);//format
	system(cmd);
	DBGMSG("Enter format.\n");

	memset(cmd,0x00,1024);
	snprintf(cmd,1024, "pmount --sync -c utf8 --noatime --exec %s 2>/dev/null", value);//remount
	system(cmd);
	DBGMSG("Enter remount.\n");

	//恢复系统文件
	//更换成了"/opt/plextalk/usr_setting/"所以不需要保存设置
#if USE_UDISK_PLEXTAL_SETTING	
	format_restore_setting_outer();
#endif	
	pthread_exit(0);
}


int start_format(const char* curMedia)
{

	//gst_element_set_state(pipeline, GST_STATE_PLAYING);
	//gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	//recorder_stat = RECORDING;
	DBGMSG("start_format \n");
	format_prepare(curMedia);

	return 0;
}

void wait_format_finish()
{
	if(pid != -1)
	{
		pthread_join(pid, NULL);
	}
}

int stop_format(void)
{
	//g_main_loop_quit(loop);
	if(pid != -1)
	{
		pthread_kill(pid,0);
		pthread_join(pid, NULL);
		pid = -1;
	}


	return 0;
}


#if 0
int radio_recorder_pause (void)
{

	gst_element_set_state(pipeline, GST_STATE_PAUSED);
	recorder_stat = PAUSE;

	return 0;
}
#endif

