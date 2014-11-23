#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "alarm_voice.h"
#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1

#define OSD_DBG_MSG
#include "nc-err.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#if DEBUG_PRINT
#define info_debug DBGMSG
#define info_err DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#define info_err(fmt,args...) do{}while(0)

#endif


//#if 1



struct AudioVoice{
	GstElement *pipeline;
	GstElement *conv;
	GstElement *sink;
	GstElement *src;
	GstElement *dec;
	GstElement *mp3_dec;
	GstElement *wav_dec;
	int			link;
	int gst_playing;
	
	GstElement *pEqualizer;
	GstElement *pSpeed;
	GstElement *pPitch;
	GstElement *pAmplify;

	GMainLoop *loop;
	guint sourceid;	/*ilde event source id*/
	guint timeout;
	pthread_t pid;

	pthread_mutex_t lock;
	pthread_cond_t	cond;

	float speed;
	double *bands;
	
	char *fname;
	int playing;
	int force_stop;
	AudioVoiceEndFunc callback;
	void				*ctx;
};


enum{
	AUDIO_VOICE_TYPE_NO_SUPPORT = -1,
	AUDIO_VOICE_TYPE_MP3= 0,
	AUDIO_VOICE_TYPE_WAV= 1,
		
};


/*let decode  audio linked*/
static void audio_cb_newpad (GstElement *decodebin, GstPad *pad, 
			   gboolean last, gpointer data)
{
	GstCaps *caps;	
	GstStructure *str;
	GstPad *audiopad;

	
	struct AudioVoice *thiz =(struct AudioVoice *)data;
	/* only link once */
	audiopad = gst_element_get_pad (thiz->sink, "sink");
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



int audio_voice_filter_file_type(char *fname)
{
	char *suffix = strrchr(fname, '.');

	info_debug("suffix=%s\n", suffix);
	if(suffix){
		if(strcasecmp(suffix, ".mp3") == 0){
			info_debug("type mp3\n");
			return AUDIO_VOICE_TYPE_MP3;
		}
		else if(strcasecmp(suffix, ".wav") == 0){
			info_debug("type wav\n");
			return AUDIO_VOICE_TYPE_WAV;
		}
		else{
			return AUDIO_VOICE_TYPE_NO_SUPPORT;
		}
	}

	return AUDIO_VOICE_TYPE_NO_SUPPORT;
}

static gboolean audio_voice_bus_callback (GstBus* bus, GstMessage *msg, 
	gpointer data)
{

	struct AudioVoice *thiz =(struct AudioVoice *)data;
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
		{
			GstState state;
			gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
			gst_element_get_state(thiz->pipeline,
				&state, NULL, GST_CLOCK_TIME_NONE);
			if(thiz->src){
				gst_bin_remove(GST_BIN(thiz->pipeline), thiz->src);
				thiz->src = 0;
			}
			info_debug("End of stream\n");

			thiz->playing = 0;
			if(thiz->force_stop == 0){
				if(thiz->callback && thiz->ctx){
					thiz->callback(thiz->ctx, 0);
				}
			}
		}
			break;
			
		case GST_MESSAGE_ERROR:
		{
			gchar* debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			info_debug("ERROR:%s\n", error->message);
			g_error_free(error);

			g_main_loop_quit(thiz->loop);
			break;
		}
		default:
			break;
	}
	
	return TRUE;

}

#if 0
static void
wav_add_pad (GstElement *element , GstPad *pad , gpointer data){
 
	gchar *name;
	GstElement *sink = (GstElement*)data;
 
	name = gst_pad_get_name(pad);
	gst_element_link_pads(element , name , sink , "sink");
	g_free(name);
}
#endif

void* audio_voice_thread(void *arg)
{
	GstBus *bus;
	//int type;

	struct AudioVoice *thiz = (struct AudioVoice *)arg;


	gst_init(NULL,NULL);
	//ps_check();
	thiz->loop = g_main_loop_new (NULL, FALSE);

	thiz->pipeline = gst_pipeline_new ("voice-audio-player");
	bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
	gst_bus_add_watch (bus, audio_voice_bus_callback, thiz);
	gst_object_unref (bus);


	/*factory make filesrc*/
	thiz->src = 0;
	//thiz->src = gst_element_factory_make ("filesrc", "source");
	/*
	g_object_set (G_OBJECT (src), "location", thiz->fname, NULL);
	*/
	
	thiz->sink = gst_element_factory_make("alsasink", "sink");
	if (!thiz->sink) 
		info_err("element sink error!\n");	

	thiz->dec = gst_element_factory_make ("decodebin", "decoder");
	if (!thiz->dec)
		info_err("Dec emelment decodebin error!\n");

	g_signal_connect (G_OBJECT (thiz->dec), "new-decoded-pad",
		G_CALLBACK (audio_cb_newpad), thiz);
	
	gst_bin_add_many(GST_BIN(thiz->pipeline), 
		thiz->dec,thiz->sink, NULL);
	

	/* run  loop*/
	gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);

	info_debug("thread signal\n");
	pthread_cond_signal(&thiz->cond);

	g_main_loop_run (thiz->loop);
	
	info_debug("exit loop\n");
	gst_element_set_state (thiz->pipeline, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT (thiz->pipeline));
	g_main_loop_unref (thiz->loop);

	//ps_check();
	/*
	thiz->playing = 0;
	if(thiz->force_stop == 0){
		if(thiz->callback && thiz->ctx){
			thiz->callback(thiz->ctx, 0);
		}
	}
	*/
	thiz->playing = 0;
	return (void*)0;
}


void audio_voice_handle_faile(struct AudioVoice *thiz)
{
	if(thiz){
		if(thiz->loop){
			g_main_loop_quit(thiz->loop);
		}
		
		while(audio_voice_isruning(thiz)){
			usleep(50);
		}

		if(thiz->pid){
			pthread_join(thiz->pid, NULL);
			thiz->pid = 0;
		}
		free(thiz);
		thiz = 0;
	}
}
static double voice_default_band[10] = {
	0,0,0,0,0,
	0,0,0,0,0
};


struct AudioVoice *audio_voice_create(void)
{
	int ret;
	struct AudioVoice *thiz = (struct AudioVoice*)malloc(
		sizeof(struct AudioVoice));
	if(!thiz){
		goto faile;
	}

	memset(thiz, 0, sizeof(struct AudioVoice));

	thiz->speed = 1.0;
	thiz->bands = voice_default_band;
	
	
	pthread_mutex_init(&thiz->lock, NULL);
	pthread_cond_init(&thiz->cond, NULL);


	thiz->callback = 0;
	thiz->ctx = 0;
	

	ret = pthread_create(&thiz->pid, NULL, audio_voice_thread, (void*)thiz);
	if(ret < 0){
		info_err("create thread err\n");
		thiz->pid = 0;
		goto faile;
	}
	
	pthread_mutex_lock(&thiz->lock);
	pthread_cond_wait(&thiz->cond, &thiz->lock);
	pthread_mutex_unlock(&thiz->lock);
	
	return thiz;
	
faile:
	audio_voice_handle_faile(thiz);
	return NULL;
}


int audio_voice_play_end(struct AudioVoice *thiz, 
	char *name, AudioVoiceEndFunc callback, void*ctx)
{
	//int type;
	info_debug("start\n");
	//GstState state;

	pthread_mutex_lock(&thiz->lock);
	
	thiz->callback = callback;
	thiz->ctx = ctx;
	thiz->fname = name;

	if(thiz->src){
		audio_voice_stop(thiz);
		thiz->src = 0;
	}
	
	thiz->src = gst_element_factory_make ("filesrc", "source");
	/*
	gst_element_get_state(thiz->pipeline,
		&state, NULL, GST_CLOCK_TIME_NONE);
	info_debug("state=%d\n", state);*/
	
	g_object_set (G_OBJECT (thiz->src), "location", 
		thiz->fname, NULL);

	gst_bin_add(GST_BIN(thiz->pipeline), thiz->src);
	gst_element_link_many (thiz->src, thiz->dec, 
			thiz->sink, NULL);

	gst_element_set_state (thiz->pipeline, GST_STATE_READY);
	gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
	//info_debug("playing ...\n");
	thiz->playing = 1;
	pthread_mutex_unlock(&thiz->lock);
	return 0;
}



int audio_voice_play(struct AudioVoice *thiz, char *name)
{
	//int type;
	//info_debug("start\n");
	pthread_mutex_lock(&thiz->lock);
	
	thiz->callback = 0;
	thiz->ctx = 0;
	thiz->fname = name;

	if(thiz->src){
		audio_voice_stop(thiz);
		thiz->src = 0;
	}
	
	thiz->src = gst_element_factory_make ("filesrc", "source");

	
	g_object_set (G_OBJECT (thiz->src), "location", 
		thiz->fname, NULL);

	gst_bin_add(GST_BIN(thiz->pipeline), thiz->src);
	gst_element_link_many (thiz->src, thiz->dec, 
		thiz->sink, NULL);
	
	
	gst_element_set_state (thiz->pipeline, GST_STATE_READY);
	gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
	//info_debug("playing ...\n");
	thiz->playing = 1;
	pthread_mutex_unlock(&thiz->lock);
	return 0;
}

int audio_voice_isruning(struct AudioVoice *thiz)
{
	return thiz->playing;
}

void audio_voice_stop(struct AudioVoice *thiz)
{
	info_debug("stop\n");
	pthread_mutex_lock(&thiz->lock);
	thiz->force_stop = 1;
	if(thiz->playing){
		//info_debug("force stop\n");
		GstState state;
		/*
		while((!thiz->gst_playing)&&(thiz->playing)){
			usleep(10);
		}*/
		gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
		gst_element_get_state(thiz->pipeline,
			&state, NULL, GST_CLOCK_TIME_NONE);
		info_debug("state=%d\n", state);
		if(thiz->src){
			gst_bin_remove(GST_BIN(thiz->pipeline), thiz->src);
			thiz->src = 0;
		}
		thiz->playing = 0;
		//info_debug("force stop sleep end\n");
	}
	thiz->force_stop = 0;
	pthread_mutex_unlock(&thiz->lock);
}


void audio_voice_destroy(struct AudioVoice *thiz)
{
	audio_voice_handle_faile(thiz);
}

//#endif

#if 0
struct AudioVoice{
	GstElement *pipeline;
	GstElement *conv;
	GstElement *sink;
	GstElement *audio;

	GstElement *pEqualizer;
	GstElement *pSpeed;
	GstElement *pPitch;
	GstElement *pAmplify;

	GMainLoop *loop;
	guint sourceid;	/*ilde event source id*/
	guint timeout;
	pthread_t pid;

	pthread_mutex_t lock;
	pthread_cond_t	cond;

	float speed;
	double *bands;
	
	char *fname;
	int playing;
	int force_stop;
	AudioVoiceEndFunc callback;
	void				*ctx;
};

enum{
	AUDIO_VOICE_TYPE_NO_SUPPORT = -1,
	AUDIO_VOICE_TYPE_MP3= 0,
	AUDIO_VOICE_TYPE_WAV= 1,
		
};

int audio_voice_filter_file_type(char *fname)
{
	char *suffix = strrchr(fname, '.');

	info_debug("suffix=%s\n", suffix);
	if(suffix){
		if(strcasecmp(suffix, ".mp3") == 0){
			info_debug("type mp3\n");
			return AUDIO_VOICE_TYPE_MP3;
		}
		else if(strcasecmp(suffix, ".wav") == 0){
			info_debug("type wav\n");
			return AUDIO_VOICE_TYPE_WAV;
		}
		else{
			return AUDIO_VOICE_TYPE_NO_SUPPORT;
		}
	}

	return AUDIO_VOICE_TYPE_NO_SUPPORT;
}

/*let decode  audio linked*/
static void audio_cb_newpad (GstElement *decodebin, GstPad *pad, 
			   gboolean last, gpointer data)
{
	GstCaps *caps;	
	GstStructure *str;
	GstPad *audiopad;

	
	struct AudioVoice *thiz =(struct AudioVoice *)data;
	/* only link once */
	audiopad = gst_element_get_pad (thiz->sink, "sink");
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


static gboolean audio_voice_bus_callback (GstBus* bus, GstMessage *msg, 
	gpointer data)
{
	GMainLoop* loop = (GMainLoop*) data;
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
			info_debug("End of stream\n");
			g_main_loop_quit(loop);
			break;
			
		case GST_MESSAGE_ERROR:
		{
			gchar* debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			info_err("ERROR:%s\n", error->message);
			g_error_free(error);

			g_main_loop_quit(loop);
			break;
		}
		default:
			break;
	}
	
	return TRUE;

}

void ps_check(void)
{
	char cmd[256];

	snprintf(cmd,256, "ps -aux | grep gmenu");
	system(cmd);
}

static void
wav_add_pad (GstElement *element , GstPad *pad , gpointer data){
 
	gchar *name;
	GstElement *sink = (GstElement*)data;
 
	name = gst_pad_get_name(pad);
	gst_element_link_pads(element , name , sink , "sink");
	g_free(name);
}

void* audio_voice_thread(void *arg)
{
	GstBus *bus;
	GstElement *src, *dec, *sink;
	int type;
	#if 0
	GstElement *conv1, *conv2;
	#endif
	
	struct AudioVoice *thiz = (struct AudioVoice *)arg;

	thiz->playing = 1;
	gst_init(NULL,NULL);
	//ps_check();
	thiz->loop = g_main_loop_new (NULL, FALSE);

	thiz->pipeline = gst_pipeline_new ("voice-audio-player");
	bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
	gst_bus_add_watch (bus, audio_voice_bus_callback, thiz->loop);
	gst_object_unref (bus);


	/*factory make filesrc*/
	src = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (src), "location", thiz->fname, NULL);

	sink = gst_element_factory_make("alsasink", "sink");
	if (!sink) 
		info_err("element sink error!\n");	
	thiz->sink = sink;

	dec = gst_element_factory_make ("decodebin", "decoder");
	g_signal_connect (G_OBJECT (dec), "new-decoded-pad",
		G_CALLBACK (audio_cb_newpad), thiz);

	gst_bin_add_many(GST_BIN(thiz->pipeline), src, dec, 
		sink, NULL);
	gst_element_link_many (src, dec, 
		sink, NULL);

	#if 0
	type = audio_voice_filter_file_type(thiz->fname);
	switch(type){
	case AUDIO_VOICE_TYPE_MP3:
	{
		dec = gst_element_factory_make ("mad", "madmp3dec");
		if (!dec)
			info_err("Dec emelment mad error!\n");
		
		gst_bin_add_many(GST_BIN(thiz->pipeline), src, dec, 
			sink, NULL);
		gst_element_link_many (src, dec, 
			sink, NULL);
	
	}
		break;
	case AUDIO_VOICE_TYPE_WAV:
	{
		dec = gst_element_factory_make ("wavparse", "wavdec");
		if (!dec)
			info_err("Dec emelment wav error!\n");
		gst_bin_add_many(GST_BIN(thiz->pipeline), src, dec, 
			sink, NULL);
		g_signal_connect(dec, "pad-added" , 
			G_CALLBACK(wav_add_pad), sink);
		
		gst_element_link_many (src, dec, 
			sink, NULL);
	}
		break;
	default:
		info_err("invalid file type\n");
		break;
	}
	
	#endif


	#if 0
	/*audioconvert*/
	conv1 = gst_element_factory_make ("audioconvert", "aconv");

	
	conv2 = gst_element_factory_make("audioconvert", "aconv1");

	
	thiz->pEqualizer = gst_element_factory_make("equalizer-10bands", 
			"pEqualizer");


	//thiz->pSpeed = gst_element_factory_make("speed", "pSpeed");    
	thiz->pPitch = gst_element_factory_make("pitch", "pPitch");  
	thiz->pAmplify = gst_element_factory_make("audioamplify", "pAmplify");


	gst_bin_add_many(GST_BIN(thiz->pipeline), src, dec, conv1, 
		thiz->pPitch, thiz->pEqualizer, conv2, sink, NULL);
	gst_element_link_many (src, dec, conv1, 
		thiz->pPitch, thiz->pEqualizer, conv2, sink, NULL);

	
	g_object_set(thiz->pPitch, 
		"tempo", thiz->speed, NULL);
	g_object_set(thiz->pEqualizer,
			"band0",	thiz->bands[0],
			"band1",	thiz->bands[1],
			"band2",	thiz->bands[2], 
			"band3",	thiz->bands[3],
			"band4",	thiz->bands[4],
			"band5",	thiz->bands[5],
			"band6",	thiz->bands[6],
			"band7",	thiz->bands[7],
			"band8",	thiz->bands[8],
			"band9",	thiz->bands[9],
			NULL);

	#endif

	/* run  loop*/
	gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);

	//thiz->state = STATE_RUNING;
	info_debug("thread signal\n");
	pthread_cond_signal(&thiz->cond);

	g_main_loop_run (thiz->loop);
	
	info_debug("exit loop\n");
	gst_element_set_state (thiz->pipeline, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT (thiz->pipeline));
	g_main_loop_unref (thiz->loop);

	//ps_check();
	thiz->playing = 0;
	if(thiz->force_stop == 0){
		if(thiz->callback && thiz->ctx){
			thiz->callback(thiz->ctx, 0);
		}
	}
	
	return (void*)0;
}


void audio_voice_stop(struct AudioVoice *thiz);

void audio_voice_handle_faile(struct AudioVoice *thiz)
{
	if(thiz){
		free(thiz);
		thiz = 0;
	}
}
static double voice_default_band[10] = {
	0,0,0,0,0,
	0,0,0,0,0
};


struct AudioVoice *audio_voice_create(void)
{

	struct AudioVoice *thiz = (struct AudioVoice*)malloc(
		sizeof(struct AudioVoice));
	if(!thiz){
		goto faile;
	}

	memset(thiz, 0, sizeof(struct AudioVoice));

	thiz->speed = 1.0;
	thiz->bands = voice_default_band;
	
	
	return thiz;
	
faile:
	audio_voice_handle_faile(thiz);
	return NULL;
}


int audio_voice_play_end(struct AudioVoice *thiz, 
	char *name, AudioVoiceEndFunc callback, void*ctx)
{
	int ret;
	
	pthread_mutex_init(&thiz->lock, NULL);
	pthread_cond_init(&thiz->cond, NULL);
	thiz->callback = callback;
	thiz->ctx = ctx;
	thiz->fname = name;

	if(thiz->pid){
		pthread_join(thiz->pid, NULL);
		thiz->pid = 0;
	}
	ret = pthread_create(&thiz->pid, NULL, audio_voice_thread, (void*)thiz);
	if(ret < 0){
		info_err("create thread err\n");
		return ret;
	}
	
	pthread_mutex_lock(&thiz->lock);
	pthread_cond_wait(&thiz->cond, &thiz->lock);
	pthread_mutex_unlock(&thiz->lock);

	return 0;
}


int audio_voice_play(struct AudioVoice *thiz, char *name)
{
	int ret;
	pthread_mutex_init(&thiz->lock, NULL);
	pthread_cond_init(&thiz->cond, NULL);

	thiz->fname = name;
	thiz->callback = 0;
	thiz->ctx = 0;
	
	if(thiz->pid){
		pthread_join(thiz->pid, NULL);
		thiz->pid = 0;
	}
	ret = pthread_create(&thiz->pid, NULL, audio_voice_thread, (void*)thiz);
	if(ret < 0){
		info_err("create thread err\n");
		return ret;
	}
	
	pthread_mutex_lock(&thiz->lock);
	pthread_cond_wait(&thiz->cond, &thiz->lock);
	pthread_mutex_unlock(&thiz->lock);

	return 0;
}

int audio_voice_isruning(struct AudioVoice *thiz)
{
	return thiz->playing;
}

void audio_voice_stop(struct AudioVoice *thiz)
{
	thiz->force_stop = 1;
	if(thiz->playing){
		
		if(thiz->loop){
			info_debug("wait quit\n");
			g_main_loop_quit(thiz->loop);
		}
		
		while(audio_voice_isruning(thiz)){
			usleep(50);
		}
		if(thiz->pid){
			pthread_join(thiz->pid, NULL);
			thiz->pid = 0;
		}
	}
	thiz->force_stop = 0;
}


void audio_voice_destroy(struct AudioVoice *thiz)
{
	audio_voice_handle_faile(thiz);
}

#endif


#if 0
void audio_voice_set_speed(struct AudioVoice *thiz, float speed)
{
	if(thiz){
		thiz->speed = speed;
		if(audio_voice_isruning(thiz)&&(thiz->pPitch)){
			g_object_set(thiz->pPitch, 
				"tempo", thiz->speed, NULL);
		}
		
	}
}

void audio_voice_set_band(struct AudioVoice *thiz, double *bands)
{
	if(thiz){
		thiz->bands = bands;
		if(audio_voice_isruning(thiz)&&(thiz->pEqualizer)){
			g_object_set(thiz->pEqualizer,
					"band0",	thiz->bands[0],
					"band1",	thiz->bands[1],
					"band2",	thiz->bands[2], 
					"band3",	thiz->bands[3],
					"band4",	thiz->bands[4],
					"band5",	thiz->bands[5],
					"band6",	thiz->bands[6],
					"band7",	thiz->bands[7],
					"band8",	thiz->bands[8],
					"band9",	thiz->bands[9],
					NULL);
		}
	}
}

#endif


struct SysVoice
{
	GstElement *pipeline;
	GstElement *src;
	GstElement *dec;
	GstElement *sink;
	GstElement *audio;
	int link;

	GMainLoop *loop;
	pthread_t pid;

	pthread_mutex_t lock;
	pthread_cond_t	cond;

	char *fname;
	int playing;
};

void sys_voice_handle_faile(struct SysVoice *thiz)
{
	if(thiz){
		info_debug("destroy\n");
		free(thiz);
		thiz = NULL;
	}
}



static gboolean sys_voice_bus_callback (GstBus* bus, GstMessage *msg, 
	gpointer data)
{
	struct SysVoice *thiz =(struct SysVoice *)data;
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
		{
			/*
			gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
			gst_element_unlink_many (thiz->src, thiz->dec, 
				thiz->sink, NULL);
			gst_bin_remove_many(GST_BIN(thiz->pipeline), thiz->src,  thiz->dec, 
				thiz->sink, NULL);
			
			thiz->pipeline = gst_pipeline_new ("sys-voice-pipe");
			bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
			gst_bus_add_watch (bus, sys_voice_bus_callback, thiz);
			gst_object_unref (bus);
			*/

			gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
			if(thiz->src){
				gst_bin_remove(GST_BIN(thiz->pipeline), thiz->src);
				thiz->src = 0;
			}
			info_debug("End of stream\n");
			thiz->playing = 0;
			//g_main_loop_quit(thiz->loop);
		}
			break;
			
		case GST_MESSAGE_ERROR:
		{
			gchar* debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			info_err("ERROR:%s\n", error->message);
			g_error_free(error);

			g_main_loop_quit(thiz->loop);
			break;
		}
		default:
			break;
	}
	
	return TRUE;

}

/*let decode  audio linked*/
static void cb_newpad (GstElement *decodebin, GstPad *pad, 
			   gboolean last, gpointer data)
{
	GstCaps *caps;	
	GstStructure *str;
	GstPad *audiopad;

	struct SysVoice *thiz = (struct SysVoice *)data;
	/* only link once */
	audiopad = gst_element_get_pad (thiz->sink, "sink");
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


void* sys_voice_thread(void *arg)
{
	GstBus *bus;
	//GstElement *dec, *sink;
	struct SysVoice *thiz =(struct SysVoice *)arg;
	info_debug("sys voice thread start\n");
	gst_init(NULL, NULL);

	thiz->playing = 1;
	//ps_check();
	thiz->loop = g_main_loop_new (NULL, FALSE);

	thiz->pipeline = gst_pipeline_new ("sys-voice-pipe");
	bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
	gst_bus_add_watch (bus, sys_voice_bus_callback, thiz);
	gst_object_unref (bus);



	/*factory make filesrc*/
	//thiz->src = gst_element_factory_make ("filesrc", "source");
	info_debug("create src\n");
	//g_object_set (G_OBJECT (thiz->src), "location", thiz->fname, NULL);
	thiz->src = 0;
	
	thiz->dec = gst_element_factory_make ("decodebin", "decoder");
	if (!thiz->dec)
		info_err("Dec emelment mad error!\n");

	thiz->sink = gst_element_factory_make("alsasink", "sink");
	if (!thiz->sink) 
		info_err("element sink error!\n");	

	#if 0

	#else

	g_signal_connect (G_OBJECT (thiz->dec), "new-decoded-pad",
		G_CALLBACK (cb_newpad), thiz);

	gst_bin_add_many(GST_BIN(thiz->pipeline), 
		 thiz->dec, thiz->sink, NULL);

	/*
	gst_element_link_many(
		thiz->dec, 
		thiz->sink, NULL);

	
	gst_element_link_many (thiz->dec, 
		thiz->sink, NULL);
		*/
	#endif

	/* run  loop*/
	gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);

	//thiz->state = STATE_RUNING;
	
	//pthread_cond_signal(&thiz->cond);
	info_debug("Paused loop\n");
	g_main_loop_run (thiz->loop);
	
	info_debug("exit loop\n");
	gst_element_set_state (thiz->pipeline, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT (thiz->pipeline));
	g_main_loop_unref (thiz->loop);
	
	info_debug("sys voice thread end\n");
	return (void*)0;
}

struct SysVoice *sys_voice_create(void)
{
	int ret;
	struct SysVoice *thiz = malloc(
		sizeof(struct SysVoice));
	if(!thiz){
		goto faile;
	}

	memset(thiz, 0, sizeof(struct SysVoice));

	info_debug("create\n");

	ret = pthread_create(&thiz->pid, NULL, 
			sys_voice_thread, (void*)thiz);
	if(ret < 0){
		info_err("sys voice thread err\n");
		goto faile;
	}



	
	return thiz;
faile:
	info_err("sys voice create err\n");
	sys_voice_handle_faile(thiz);
	return NULL;
}


void sys_voice_stop(struct SysVoice *thiz)
{
	if(thiz->playing){
		gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
		if(thiz->src){
			gst_bin_remove(GST_BIN(thiz->pipeline), thiz->src);
			thiz->src = 0;
		}
		thiz->playing = 0;
	}
	info_debug("stop end\n");
}


void sys_voice_start(struct SysVoice *thiz, char *fname)
{

	//GstPad *pad;
	info_debug("start\n");
	thiz->playing = 1;

	if(thiz->src){
		sys_voice_stop(thiz);
	}

	
	thiz->src = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (thiz->src), "location", fname, NULL);
	
	gst_bin_add(GST_BIN(thiz->pipeline), thiz->src);
	gst_element_link_many (thiz->src, thiz->dec, 
			thiz->sink, NULL);

	/*
	gst_bin_add_many(GST_BIN(thiz->pipeline), thiz->src,  thiz->dec, 
		thiz->sink, NULL);
	
	
	gst_element_link_many (thiz->src, thiz->dec, 
		thiz->sink, NULL);
		*/
	
	
	info_debug("set playing\n");
	/*
	pad = gst_element_get_static_pad (thiz->sink, "sink");
	gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_IDLE,
		(GstPadProbeCallback) cb_sink, NULL, NULL);
	*/
	gst_element_set_state (thiz->pipeline, GST_STATE_READY);
	gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
	info_debug("playing3 ...\n");
	
	
}



void sys_voice_destroy(struct SysVoice *thiz)
{
	sys_voice_handle_faile(thiz);
}

