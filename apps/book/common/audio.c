#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

#include "audio.h"
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1
#include "info_print.h"
#include "Plextalk-constant.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "typedef.h"

#include "dbmalloc.h"

enum{
	AUDIO_STATE_END = 0,
	AUDIO_STATE_RUN=1
};

struct BookAudio{
	GstElement *pipeline;
	GstElement *conv1;
	GstElement *sink;
	GstElement *audio;

	GstElement *pEqualizer;
	GstElement *pPitch;
	GstElement *pAmplify;

	int speed;
	int band;
	
	GMainLoop *loop;
	guint sourceid;	/*ilde event source id*/
	guint timeout;
	pthread_t pid;
	const char *fname;
	int seek_flag;
	unsigned long beg_time;
	
	sem_t sem;
	pthread_mutex_t lock;
	
	int state;
	int pause;
	unsigned long time;
};


	
static void book_audio_handle_faile(struct BookAudio *thiz)
{
	if(thiz){
		pthread_mutex_destroy(&thiz->lock);
		sem_destroy(&thiz->sem);
		app_free(thiz);
		thiz = 0;
	}
}

struct BookAudio *book_audio_create(void)
{
	struct BookAudio *thiz = (struct BookAudio *)app_malloc(
		sizeof(struct BookAudio));
	if(!thiz){
		info_debug("init err\n");
		goto faile;
	}
	memset(thiz, 0, sizeof(struct BookAudio));
	thiz->speed = 0;
	thiz->band = 0;
	sem_init(&thiz->sem, 0, 0);
	pthread_mutex_init(&thiz->lock, NULL);

//ztz .loop create only once!!
	gst_init(NULL, NULL);
	thiz->loop = g_main_loop_new (NULL, FALSE);

	return thiz;
faile:
	book_audio_handle_faile(thiz);
	return NULL;
}


static int inline pipeline_active (struct BookAudio *thiz)
{
	int active;

	pthread_mutex_lock(&thiz->lock);
	active = thiz->pipeline ? 1 : 0;
	pthread_mutex_unlock(&thiz->lock);

	return active;
}

static gboolean book_audio_bus_callback (GstBus* bus, GstMessage *msg, 
	gpointer data)
{

	struct BookAudio *thiz = (struct BookAudio *)data;
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
			printf("--->End of stream\n");	
			break;
			
		case GST_MESSAGE_ERROR:
		{
			gchar* debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			printf("---->Gst Message Error!\n");
			info_debug("ERROR:%s\n", error->message);
			g_error_free(error);

			break;
		}
		default:
			return TRUE;
	}

	printf("%s, %d:Eos, do g_main_loop_quit!!\n", __func__, __LINE__);
	g_main_loop_quit(thiz->loop);

	return TRUE;
}


/*let decode  audio linked*/
static void cb_newpad (GstElement *decodebin, GstPad *pad, 
			   gboolean last, gpointer data)
{
	GstCaps *caps;	
	GstStructure *str;
	GstPad *audiopad;

	struct BookAudio *thiz = (struct BookAudio *)data;
	/* only link once */
	audiopad = gst_element_get_pad (thiz->audio, "sink");
	if (GST_PAD_IS_LINKED (audiopad)) {
		g_object_unref (audiopad);
		return;
	}

	/* check media type */
	caps = gst_pad_get_caps (pad);
	str = gst_caps_get_structure (caps, 0);
	if (!g_strrstr (gst_structure_get_name (str), "audio")) {
		gst_caps_unref (caps);
		gst_object_unref (audiopad);
		return;
	}
	gst_caps_unref (caps);

	/* link'n'play */
	gst_pad_link (pad, audiopad);
}


unsigned long book_audio_get_total_time(struct BookAudio *thiz)
{
	gint64 len;
	unsigned long total;
	GstFormat fmt = GST_FORMAT_TIME;
	return_val_if_fail(NULL != thiz, 0);
	if (gst_element_query_duration(thiz->pipeline, &fmt, &len)) {
		total = GST_TIME_AS_MSECONDS(len);
		info_debug("get total=%ld\n", total);
	} else {
		total = 0;
		info_debug("invalid total=%ld\n", total);
	}
	return total;
}


/*seek audio, with ms*/
int book_audio_seek (struct BookAudio *thiz, 
	unsigned long beg_time, unsigned long end_time)
{
	GstSeekType stop_type;
	GstClockTime seek_beg_time;
	GTimeVal val;
	gboolean ret_bool;
	GstStateChangeReturn stateRet;
	GstState state;

	if(!thiz->pipeline){
		info_debug("NULL\n");
		return -1;
	}
	
	info_debug("begtime=%ld\n", beg_time);
	beg_time = beg_time/plextalk_sound_speed[thiz->speed];
	info_debug("!!begtime=%ld\n", beg_time);
	val.tv_sec  = beg_time / 1000;
	val.tv_usec = (beg_time % 1000)*1000;
	seek_beg_time = GST_TIMEVAL_TO_TIME(val);

	stateRet = gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);
	info_debug("ret:%d\n", stateRet);
	while(GST_STATE_CHANGE_ASYNC==stateRet ){
		stateRet = gst_element_get_state(thiz->pipeline,&state,NULL,GST_CLOCK_TIME_NONE);
		info_debug("ret:%d state:%d\n", stateRet,state);
	}

	if(GST_STATE_CHANGE_FAILURE==stateRet){
		info_debug("!!!!!!!!!!!FAILED ret:%d\n", stateRet);
		//g_main_loop_quit(thiz->loop);
		return -1;
	}
	
	ret_bool = gst_element_seek (thiz->pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
			GST_SEEK_TYPE_SET, seek_beg_time,
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	
	info_debug("seek ret:%d\n",ret_bool);

//
	printf("%s, %d :Do audio seek!!!\n", __func__, __LINE__);
	info_debug("thiz->pipeline = %s\n", thiz->pipeline);
	
	stateRet = gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
	stateRet = gst_element_get_state(thiz->pipeline,&state,NULL,GST_CLOCK_TIME_NONE);
	info_debug("ret:%d state:%d\n", stateRet,state);
	return 0;	
}

int book_audio_isruning(struct BookAudio *thiz)
{
	return_val_if_fail(NULL != thiz, 0);

	return pipeline_active(thiz);
}

enum{
	BOOK_AUDIO_TYPE_NO_SUPPORT = -1,
	BOOK_AUDIO_TYPE_MP3= 0,
	BOOK_AUDIO_TYPE_WAV= 1,	
};

static int book_audio_filter_file_type(const char *fname)
{
	char *suffix = strrchr(fname, '.');

	info_debug("suffix=%s\n", suffix);
	if(suffix){
		if(strcasecmp(suffix, ".mp3") == 0){
			info_debug("type mp3\n");
			return BOOK_AUDIO_TYPE_MP3;
		}
		else if(strcasecmp(suffix, ".wav") == 0){
			info_debug("type wav\n");
			return BOOK_AUDIO_TYPE_WAV;
		}
		else{
			return BOOK_AUDIO_TYPE_NO_SUPPORT;
		}
	}

	return BOOK_AUDIO_TYPE_NO_SUPPORT;
}

#if 0
static void
awav_add_pad (GstElement *element , GstPad *pad , gpointer data){
 
	gchar *name;
	GstElement *sink = (GstElement*)data;
 
	name = gst_pad_get_name(pad);
	gst_element_link_pads(element , name , sink , "sink");
	g_free(name);
}
#endif

static void* audio_thread(void *arg)
{
	GstBus *bus;
	GstElement *src, *dec, *conv1, *conv2, *sink;
	GstPad *audiopad;

	printf("%s, %d : Audio thread start!!\n", __func__, __LINE__);
	
	struct BookAudio *thiz = (struct BookAudio *)arg;

	thiz->pipeline = gst_pipeline_new ("book-audio-player");

	printf("gst_pipeline_new !!!. thiz->pipeline = %x\n", thiz->pipeline);
	
	bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
	gst_bus_add_watch (bus, book_audio_bus_callback, thiz);
	gst_object_unref (bus);

	info_debug("%s, %d : fname=%s\n",__func__, __LINE__, thiz->fname);
	/*factory make filesrc*/
	src = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (src), "location", thiz->fname, NULL);
	
	dec = gst_element_factory_make ("decodebin2", "decoder3");
	g_signal_connect(dec, "new-decoded-pad" , 
		G_CALLBACK(cb_newpad), thiz);

	/*bin add*/
	gst_bin_add_many (GST_BIN (thiz->pipeline), src, dec, NULL);
	gst_element_link (src, dec);
	
	/* create audio output ,include audioconvert and alsasink*/
	thiz->audio = gst_bin_new ("audiobin");

	/*audioconvert*/
	
	conv1 = gst_element_factory_make ("audioconvert", "aconv");

	audiopad = gst_element_get_pad (conv1, "sink");
	
	conv2 = gst_element_factory_make("audioconvert", "aconv1");

	
	thiz->pEqualizer = gst_element_factory_make("equalizer-3bands", 
			"pEqualizer");
	thiz->pPitch = gst_element_factory_make("pitch", "pPitch"); 
	if(thiz->pEqualizer){
		//DBGMSG("set obj band\n");
		g_object_set(thiz->pEqualizer,
				"band0",	plextalk_sound_equalizer[thiz->band][0],
				"band1",	plextalk_sound_equalizer[thiz->band][1],
				"band2",	plextalk_sound_equalizer[thiz->band][2], 
				NULL);
	}
	
	if (thiz->pPitch) {
		g_object_set(thiz->pPitch, "tempo", plextalk_sound_speed[thiz->speed], NULL);
	}
	//thiz->pAmplify = gst_element_factory_make("audioamplify", "pAmplify");
	
	sink = gst_element_factory_make("alsasink", "sink");

	gst_bin_add_many (GST_BIN (thiz->audio), conv1, 
		thiz->pPitch, conv2,thiz->pEqualizer,
		 sink, NULL);
	gst_element_link_many (conv1, 
		thiz->pPitch, conv2,thiz->pEqualizer,
		 sink, NULL);
	
	
	gst_element_add_pad (thiz->audio,gst_ghost_pad_new ("sink", audiopad));
	gst_object_unref (audiopad);

	/*audio add in pipeline*/
	/*decode and audio will be link in the cb_newpad function*/
	gst_bin_add (GST_BIN (thiz->pipeline), thiz->audio);
	
	/* run  loop*/
	gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);
	
	sem_post(&thiz->sem);

	printf("g_main_loop run!!\n");
	
	g_main_loop_run (thiz->loop);

	printf("%s, %d : thread start exit!\n", __func__, __LINE__);

	gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
	
	pthread_mutex_lock(&thiz->lock);
	gst_object_unref (GST_OBJECT (thiz->pipeline));
	thiz->pipeline = NULL;
	pthread_mutex_unlock(&thiz->lock);

	printf("%s, %d : thread end exit!\n", __func__, __LINE__);

	pthread_exit(0);
}



void book_audio_play(struct BookAudio *thiz, const char *fname, unsigned long beg_time,
	unsigned long end_time)
{
	int ret;
	GstStateChangeReturn stateRet;
	GstState state;
	
	return_if_fail(NULL != thiz);

	printf("%s, %d : Start do book audio play!\n", __func__, __LINE__);
	
	book_audio_stop(thiz);
	
	thiz->fname = fname;
	printf("%s, %d : before do pthread create!!\n", __func__, __LINE__);
	ret = pthread_create(&thiz->pid, NULL, audio_thread, (void*)thiz);
	printf("%s, %d : !!fname = %s\n", __func__, __LINE__, fname);
	printf("%s, %d : pthread_create!!!!!, thiz->pid = %d\n", __func__, __LINE__, thiz->pid);
	if(ret < 0){
		info_err("thread create err\n");
	}
	thiz->time = 0;
	sem_wait(&thiz->sem);

	printf("%s, %d : before do audio seek!\n", __func__, __LINE__);
	ret = book_audio_seek(thiz, beg_time, end_time);
	printf("%s, %d : after do audio seek!\n", __func__, __LINE__);

	if(ret < 0){
		info_debug("$play error\n");
		book_audio_stop(thiz);
	}
	info_debug("play ret\n");
}

void book_audio_stop(struct BookAudio *thiz)
{
	return_if_fail(NULL != thiz);

	int end_flag;

	printf("%s, %d : book audio stop called!!!\n", __func__, __LINE__);

	if (pipeline_active(thiz)) {
		printf("%s, %d : Book audio stop call quit loop!\n", __func__, __LINE__);
		g_main_loop_quit(thiz->loop); 
		printf("%s, %d : Book audio stop call qiut loop end!\n", __func__ ,__LINE__);
	}

	if (thiz->pid != 0) {
		printf("%s, %d : pthread_join = %d\n", __func__, __LINE__, thiz->pid);
		pthread_join(thiz->pid, NULL);
		printf("%s, %d : after pthread_join!!\n", __func__, __LINE__);
		thiz->pid = 0;
	}

	printf("%s, %d : qiut & join end!\n", __func__, __LINE__);
	thiz->pause = 0;
	printf("book_audio_stop exit!!\n");
}

int book_audio_pause(struct BookAudio *thiz)
{
	return_val_if_fail(NULL != thiz, 0);
	GstStateChangeReturn stateRet;
	GstState state;

	if (!pipeline_active(thiz))
		return 0;

	stateRet = gst_element_get_state(thiz->pipeline,&state,NULL,GST_CLOCK_TIME_NONE);
	info_debug("ret:%d state:%d\n", stateRet,state);
	if(thiz->pause){
		thiz->pause = 0;
		gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
	}
	else{
		thiz->pause = 1;
		gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);
	}

	return 0;
}

int book_get_pause(struct BookAudio *thiz)
{
	return_val_if_fail(NULL != thiz, 0);
	return thiz->pause;
}

unsigned long book_audio_get_time(struct BookAudio *thiz)
{
	gint64 pos;
	GstFormat fmt = GST_FORMAT_TIME;
	unsigned long time = 0;
	return_val_if_fail(NULL != thiz, 0);
	
	if(pipeline_active(thiz)){
		if (gst_element_query_position(thiz->pipeline, &fmt, &pos)) {
			time = GST_TIME_AS_MSECONDS(pos);
			time = (double)time * plextalk_sound_speed[thiz->speed];
		}
	}

	return time;
}

void book_audio_destroy(struct BookAudio *thiz)
{
	book_audio_handle_faile(thiz);
}

void book_audio_set_band(struct BookAudio *thiz, int band)
{
	return_if_fail(NULL != thiz);
	thiz->band = band;

	int stop_flag;

	if (!pipeline_active(thiz))
		return;

	if(thiz->pEqualizer){
		g_object_set(thiz->pEqualizer,
				"band0",	plextalk_sound_equalizer[band][0],
				"band1",	plextalk_sound_equalizer[band][1],
				"band2",	plextalk_sound_equalizer[band][2], 
				NULL);
	}
}


void book_audio_set_speed(struct BookAudio *thiz, int speed)
{
	return_if_fail(NULL != thiz);
	thiz->speed = speed;

	if (!pipeline_active(thiz))
		return;
	
	if (thiz->pPitch) {
		g_object_set(thiz->pPitch, 
			"tempo", plextalk_sound_speed[speed], NULL);
	}
}

