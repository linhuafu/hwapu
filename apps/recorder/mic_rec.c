#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#define  DEBUG_PRINT 1
//#include "info_print.h"
#include "mic_rec.h"
#include "rec_extern.h"
#include "rec_tts.h"
//#include "p_mixer.h"
#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "sysfs-helper.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


enum _monitor_stat{
	MONITOR_ON = 0,
	MONITOR_OFF,
};


static pthread_cond_t  cond;
static pthread_mutex_t lock;
static pthread_t pid = -1;


static enum _monitor_stat monitor_stat = MONITOR_OFF;			//init as monitor off

static GstElement *pipeline, *src, *enc, *out_file,*rate;
static GMainLoop  *loop;

static char* file_path = NULL;

void* mic_prepare(void* arg);


void init_monitor_state (void)
{
	if (mic_headphone_on())
		monitor_stat = MONITOR_ON;
	else
		monitor_stat = MONITOR_OFF;
}

int 
mic_rec_prepare(const char* filename)
{
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
	
	int res = pthread_create(&pid, NULL, mic_prepare, (void*)filename);
	if (res != 0) {
		info_debug("Create mic_rec pthread error!\n");
		return -1;
	}

	pthread_mutex_lock(&lock);
	pthread_cond_wait(&cond, &lock);
	pthread_mutex_unlock(&lock);
	return 0;
}


void* 
mic_prepare (void* arg)
{
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");

	GstCaps* caps;
	GError *error=NULL;
	char cmdBuf[512];

	gst_init(NULL, NULL);

#if 0
	file_path = (char*)arg;

	if (!file_path) {
		info_debug("get file path error!\n");
		info_debug("exit the thread!\n");
		return NULL;
	}
#endif	

	loop = g_main_loop_new(NULL, FALSE);
	if (!loop) {
		info_debug("g_main_loop_new error!\n"); 
		return NULL;
	}

#if 0	

	pipeline = gst_pipeline_new ("pipeline");
	if (!pipeline) {
		info_debug("gst_pipeline_new error!\n");
		return NULL;
	}
	
	src   	 = gst_element_factory_make("alsasrc", "alsasrc");
	enc 	 = gst_element_factory_make("shinemp3enc", "mp3_encoder");
	out_file = gst_element_factory_make("filesink", "filesink");
	//rate   	 = gst_element_factory_make("rate", "rate");

	if (!src || !enc || !out_file) {
		info_debug("Gst_element_factory_make failure!\n");
		return NULL;
	}

	g_object_set(G_OBJECT(enc), "bitrate", 128, NULL);//BUG 193266 128kbps->32kbps
	//g_object_set(G_OBJECT(enc), "mode", 3, NULL);	//BUG 193266 stereo->monaural

	g_object_set(G_OBJECT(out_file), "location", file_path, NULL);

	gst_bin_add_many(GST_BIN(pipeline), src, enc, out_file, NULL);
	gst_element_link_many(src, enc, out_file, NULL);
#else
	memset(cmdBuf,0x00,512);
	strcpy(cmdBuf, 
	"alsasrc ! queue ! audio/x-raw-int,endianess=1234,signed=true,width=16,depth=16,rate=44100,channels=2 ! queue ! shinemp3enc bitrate=128 ! filesink name=my_filesrc");
//	strcpy(cmdBuf, 
//	"alsasrc ! queue ! audio/x-raw-int,endianess=1234,signed=true,width=16,depth=16,rate=22050,channels=2 ! queue ! shinemp3enc bitrate=64 ! filesink name=my_filesrc");

	pipeline = gst_parse_launch(cmdBuf,&error);
	if (!pipeline) {
		info_debug("gst_pipeline_new error!\n");
		return NULL;
	}
	//mic_rec_set_path(file_path);
#endif	

	//gst_element_set_state(pipeline, GST_STATE_PAUSED);
	//gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);


	pthread_cond_signal(&cond);
	
	g_main_loop_run(loop);

	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (pipeline));
	g_main_loop_unref (loop);
	
	return NULL;

}

int mic_rec_set_path(char *filepath)
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


/*This to be some bugs for delay.otherwise, pause and recover will be no effect*/
int 
mic_rec_recording (char *filepath,int needsetfile)
{
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");

	if(needsetfile)
		mic_rec_set_path(filepath);
		
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

	info_debug("Set to mic recording!\n");
	return 0;
}


int 
mic_rec_pause (void)
{

	gst_element_set_state(pipeline, GST_STATE_PAUSED);
	gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

	info_debug("Set to mic paused!\n");
	return 0;
}

int mic_rec_get_status()
{
	
}


int 
mic_rec_get_time (void)
{
#if 0
	struct stat info;
	stat(file_path, &info);
	return (info.st_size * 8) / 32000;	
#endif
	gint64 pos;
	int time;
	
	GstFormat fmt = GST_FORMAT_TIME;
	if (gst_element_query_position(pipeline, &fmt, &pos)) {
		time = GST_TIME_AS_MSECONDS(pos);

		if(time < 2000 && time >1800)
		{//码率问题 处理第二秒不显示问题
		     time = 2000;
		     info_debug("2 second!!!\n");
		}
	} else {
	      time = 0;                  //最开始的值为0
		info_debug("set audio current time failure!\n");
	}

	return time/1000;
}


int 
mic_rec_stop (void)
{	
	if(pid != -1) {
		g_main_loop_quit(loop);
		pthread_join(pid, NULL);
		pid = -1;
	}

	return 0;
}


int 
mic_rec_monitor_on (void)
{
	
	DBGLOG("mic_rec_monitor_on!\n");
	//system("amixer sset 'Output Mux' 'Sidetone1'");
	p_mixer_set(OUTPUT_SIDETONE, 0);
	monitor_stat = MONITOR_ON;
	DBGLOG("mic_rec_monitor_on out!\n");
	return 0;
}


int 
mic_rec_monitor_off (void)
{	
	//system("amixer sset 'Output Mux' 'DAC'");
	p_mixer_set(OUTPUT_DAC, 0);
	monitor_stat = MONITOR_OFF;
	DBGLOG("mic_rec_monitor_off!\n");
	return 0;
}


int 
mic_rec_switch_monitor(void)
{
	if (monitor_stat == MONITOR_ON) {
		mic_rec_monitor_off();
		rec_tts(TTS_SWITCH_MONITOR_OFF);
	} else {
		mic_rec_monitor_on();
		rec_tts(TTS_SWITCH_MONITOR_ON);
		//wait_tts();
		//mic_rec_monitor_on();
	}
	return 0;
}


int 
get_mic_rec_monitor_state(void)
{
	if (monitor_stat == MONITOR_ON)
		return 1;
	else 
		return 0;
}


