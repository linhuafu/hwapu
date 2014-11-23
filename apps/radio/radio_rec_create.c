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
#include "radio_rec_create.h"
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "sysfs-helper.h"

static pthread_cond_t  cond;
static pthread_mutex_t lock;
static GstElement *pipeline;
static GMainLoop  *loop;
static pthread_t pid = -1;
static GstElement *out_file;



static int recorder_stat = PREPARE;

static char cpu_freq_max[8];

void*
recorder_pipeline_create (void* arg)
{	
	DBGMSG("Enter the pipeline create.\n");

//set cpu to max
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");
	
	const char* f_path = arg;
 	GstElement *src,*enc, *out_file;
	GError *error=NULL;
	char cmdBuf[512];
	
	gst_init(NULL, NULL);
		
	loop = g_main_loop_new(NULL, FALSE);
	if (!loop) {
		DBGMSG("g_main_loop_new error!\n"); 
		return NULL;
	}

#if 0	
	pipeline = gst_pipeline_new ("pipeline");
	if (!pipeline) {
		DBGMSG("gst_pipeline_new error!\n");
		return NULL;
	}
	
	src   	 = gst_element_factory_make("alsasrc", "alsasrc");
	enc 	 = gst_element_factory_make("shinemp3enc", "mp3_encoder");
	out_file = gst_element_factory_make("filesink", "filesink");

	if (!src || !enc || !out_file) {
		DBGMSG("Gst_element_factory_make failure!\n");
		return NULL;
	}
	
	g_object_set(G_OBJECT(enc), "bitrate", 128, NULL);

	#if 0
	g_object_set(G_OBJECT(enc), "mode", 1, NULL);		//stereo
	#endif

	g_object_set(G_OBJECT(out_file), "location", f_path, NULL);

	gst_bin_add_many(GST_BIN(pipeline), src, enc, out_file, NULL);
	gst_element_link_many(src, enc, out_file, NULL);
#else
	memset(cmdBuf,0x00,512);
	strcpy(cmdBuf, 
	"alsasrc ! queue ! audio/x-raw-int,endianess=1234,signed=true,width=16,depth=16,rate=44100,channels=2 ! queue ! shinemp3enc bitrate=128 mode=1 ! filesink name=my_filesrc");
	pipeline = gst_parse_launch(cmdBuf,&error);
	if (!pipeline) {
		DBGMSG("gst_pipeline_new error!\n");
		return NULL;
	}

#endif	

	//gst_element_set_state(pipeline, GST_STATE_READY);

	pthread_cond_signal(&cond);
	recorder_stat = PREPARE;
	
	g_main_loop_run(loop);
	return NULL;
}


int radio_recorder_exit (void)
{	
	//set cpu freq to on demand
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "ondemand");
	
	if(pid != -1) {
		gst_element_set_state (pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (pipeline));
		g_main_loop_unref (loop);
	}

	return 0;
}


int radio_recorder_prepare (const char* f_path)
{
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
	
	if (pthread_create(&pid, NULL, recorder_pipeline_create, (void*)f_path)) {
		DBGMSG("Radio start pthread failure!\n");
		return -1;
	}
	pthread_mutex_lock(&lock);
	pthread_cond_wait(&cond, &lock);
	pthread_mutex_unlock(&lock);

	return 0;
}


int radio_rec_set_path(char *filepath)
{
	if(!filepath) {
		return -1;
	}
	
	if(pid != -1 && pipeline) {
		out_file = gst_bin_get_by_name(GST_BIN(pipeline),"my_filesrc");
		g_object_set(G_OBJECT(out_file), "location", filepath, NULL);
		DBGMSG("set path\n");
	}
}



int radio_recorder_recording (char *filepath,int needsetfile)
{
//set cpu freq

	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");

	if(needsetfile)
		radio_rec_set_path(filepath);
	
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	recorder_stat = RECORDING;

	return 0;
}


int radio_recorder_pause (void)
{
	gst_element_set_state(pipeline, GST_STATE_PAUSED);
	recorder_stat = PAUSE;

	return 0;
}


/* return the time of recorder */
int radio_recorder_time (void)	
{
	gint64 pos;
	int time;
	
	GstFormat fmt = GST_FORMAT_TIME;
	if (gst_element_query_position(pipeline, &fmt, &pos)) {
		time = GST_TIME_AS_MSECONDS(pos);
	} else {
	      time = 0;                  //最开始的值为0
		DBGMSG("set audio current time failure!\n");
	}

	return time/1000;
}


/* get recorder state */
enum recorder_stat radio_recorder_stat (void)
{
	return recorder_stat;
}

