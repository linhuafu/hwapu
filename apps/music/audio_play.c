#include <stdio.h>
#include <gst/gst.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "audio_ui.h"
#include "audio_play.h"
#include "audio_type.h"
#include "audio_extern.h"
#include "audio_tts.h"
#define OSD_DBG_MSG
#include "nc-err.h"
#include "plextalk-config.h"

#include "sysfs-helper.h"
#include "audio_type.h"
#include "plextalk-constant.h"
#include "plextalk-statusbar.h"

#define __USE_AUDIO_PLAY__
#ifdef __USE_AUDIO_PLAY__

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

static pthread_t pid;

struct _audio_stat audio_stat = {

	.pthread_flag      = 0,
	.total_play_time   = 0,
	.current_play_time = 0,
	.pause_flag        = 0,
	.stop_flag         = 0,
	.error_flag = 0,
};

static GstElement *pipeline;
static GMainLoop  *loop;
static GstElement *pEqualizer;
static GstElement *pPitch;
static GstElement *pScaletempo;
static GstElement *conv1;
static GstElement *conv2;


static GstElement *audio;				

extern char fre_min[8];
extern char fre_max[8];
extern struct audio_player player;


//double  speed_rate[11]= {0.5, 0.75, 1, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0};    //{0.5,0.75,1,1.2,1.4,1.6,1.7,1.8,1.9,2.0,2.1};

 extern void*
Track_play (void* arg);

/*seek to the position to playback, 
  *after seek, start_seek_time_flag will be set to 0,
  *units ms
  */
static int start_seek_time;
static int start_seek_time_flag;

void* audio_play (void* arg);


void audio_join()
{
	pthread_join(pid, NULL);
}

/*****************************************
GST_STATE_PENDING
GST_STATE_NULL
GST_STATE_READY
GST_STATE_PAUSED
GST_STATE_PLAYING

******************************************/
void Get_Music_State(struct audio_player* player)
{
 
	GstState state;

	if(audio_stat.stop_flag==1)
	{
		player->play_info.play_state=STOPED;
		return ;
	}
	

	if(pipeline)
	{
//		printf("  Enter Get_Music_State!\n");
		
		gst_element_get_state(pipeline,&state, NULL, GST_CLOCK_TIME_NONE);

//		printf("  Enter Get_Music_State state =%d!\n",state);
		
		if(state==GST_STATE_PLAYING)
		{
			player->play_info.play_state=PLAYING;
			audio_stat.pause_flag=0;
			audio_stat.stop_flag=0;
			
		}
		else if(state==GST_STATE_PAUSED)
		{
			player->play_info.play_state=PAUSED;
			audio_stat.pause_flag=1;
			audio_stat.stop_flag=0;
		}
		else
		{
			player->play_info.play_state=STOPED;
			audio_stat.stop_flag=1;
			audio_stat.pause_flag=0;
		}
	}
	else
	{

		player->play_info.play_state=NO_START;
		audio_stat.pause_flag=0;
		audio_stat.stop_flag=0;
	}


		return ;	

}

int Music_IsPaused()
{

	GstState state;
	if(pipeline)
	{
		gst_element_get_state(pipeline,&state, NULL, GST_CLOCK_TIME_NONE);

	//DBGMSG("pipeline state is %d!\n",state);
	
		if(state==GST_STATE_PLAYING)
		{
			//DBGMSG("pipeline  is playing!\n");
			
			return 0;
		}
		else if(state==GST_STATE_PAUSED)
		{
			//DBGMSG("pipeline  is pause!\n");
			return 1;
		}
		else
		{
			//DBGMSG("pipeline  is stop!\n");
			return 0;
		}
		
	}
	return 0;

}

int Music_IsPlaying()
{

	GstState state;
	if(pipeline)
	{
		gst_element_get_state(pipeline,&state, NULL, GST_CLOCK_TIME_NONE);

	//DBGMSG("pipeline state is %d!\n",state);
	
		if(state==GST_STATE_PLAYING)
		{
			//DBGMSG("pipeline  is playing!\n");
			
			return 1;
		}
		else if(state==GST_STATE_PAUSED)
		{
			//DBGMSG("pipeline  is pause!\n");
			return 0;
		}
		else
		{
			//DBGMSG("pipeline  is stop!\n");
			return 0;
		}
		
	}
	return 0;

}


/*
  *create a new audio thread, it create thread only when there is no audio thread
  *it will do noting when there is a audio thread
  */
int 
audio_start (struct start* start_place)
{

	int res = 0;
	printf("AUDIO_START FUNCTION!\n");


	audio_stat.suspendflag=0;

	DBGMSG("Set cpu  freq max=%s \n",fre_max);
	
	sysfs_write(PLEXTALK_SCALING_SETSPEED, fre_max );

	if (!audio_stat.pthread_flag) 
	{
	//	audio_join();

		printf("AUDIO_START FUNCTION_111111111111111!\n");
		music_setConditionIcon(  SBAR_CONDITION_ICON_PLAY); 

		if (StringStartWith(start_place->path, "/media/cdda"))
			audio_stat.trackflag = 1;
		else
			audio_stat.trackflag = 0;
	
		//这个线程有可能创建失败，这里
		if(audio_stat.trackflag)
		{
			res = pthread_create(&pid, NULL, Track_play, (void*)start_place);		
		}
		else
		{
			res = pthread_create(&pid, NULL, audio_play, (void*)start_place);
		}

		if (res != 0) 
		{
			return -1;
		}
		else 
		{
			audio_stat.stop_flag = 0;
			player.play_info.breakflg=0;

			pthread_mutex_lock(&lock);
			pthread_cond_wait(&cond, &lock);
			pthread_mutex_unlock(&lock);

			return 0;
		}
	}
	else
	{
		return 0;
	}

	return 0;
}






void
audio_pause (int tts, int set_cpu,int filelist )
{	
	DBGMSG("Enter audio_pause fun=%d .\n",audio_stat.stop_flag);

	if(audio_stat.stop_flag==0 && pipeline)
	{//只在在music 没有停止的情况下，才可以pause music 

		DBGMSG("Enter audio_pause _1111111111 .\n");
		
		gst_element_set_state(pipeline, GST_STATE_PAUSED);
		DBGMSG("Enter audio_pause _22222222222222 .\n");
		
		if(filelist)
		{
				DBGMSG(" audio_pause   in filelist!.\n");
			
				gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
				usleep(300000);
		}

		DBGMSG("Enter audio_pause _3333333333333333.\n");
		audio_stat.pause_flag = 1;		

		audio_stat.suspendflag=1;
		
		if (set_cpu)
		{//cpu_freq_set_min();
			DBGMSG("Set cpu  freq min=%s\n",fre_min);	
			sysfs_write(PLEXTALK_SCALING_SETSPEED, fre_min );
		}
		
		if (tts)
		{
			audio_tts(TTS_NORMAL_BEEP, 0);
		}
		DBGMSG("Enter audio_pause _444444444444444444\n");
		music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE);

	}
}



/*recover the paused audio thread*/
void 
audio_recover (int tts)
{	
	if (tts) 
	{
		audio_tts(TTS_NORMAL_BEEP, 0);
		
	}


	if(pipeline )
	{

		
			player.play_info.resumeflg=0;
			player.play_info.breakflg=0;
			
			DBGMSG("Set cpu  freq max_1111111111111=%s \n",fre_max);
			sysfs_write(PLEXTALK_SCALING_SETSPEED, fre_max );
			
			
			DBGMSG("audio_recover =%p\n",pipeline);
			
			gst_element_set_state(pipeline, GST_STATE_PLAYING);
			gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
			//usleep (10000);

			audio_stat.suspendflag=0;
			audio_stat.pause_flag = 0;	

			music_setConditionIcon(  SBAR_CONDITION_ICON_PLAY);
	}
}


/*seek audio, with ms*/
int 
audio_seek (int time)
{
	//gst_element_set_state(pipeline, GST_STATE_PAUSED);

	DBGMSG("[enter seek]\n");
	DBGMSG("[seek]:time = %d\n", time);

	DBGMSG("DEBUG: speed = %f\n", plextalk_sound_speed[player.speed]);

	DBGMSG("time = %d\n", time);
	int real_time = time / plextalk_sound_speed[player.speed];
	DBGMSG("real_time = %d\n", real_time);
	
	struct GTimeVal {

	  //hopeless
	  char reserved[32];
	  glong tv_sec;
	  glong tv_usec;
	};
	
	struct GTimeVal val = {
		.tv_sec  = real_time / 1000,
		.tv_usec = real_time % 1000,
	};
	
	GstClockTime seek_time = GST_TIMEVAL_TO_TIME(val);

	DBGMSG("audio_seek pipeline=%p!\n",pipeline);

	//gst_element_set_state(pipeline, GST_STATE_PAUSED);			
	//gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	
	int res = gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
			GST_SEEK_TYPE_SET, seek_time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	
	//gst_element_set_state(pipeline, GST_STATE_PLAYING);			
	//gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

	if (!res) 
	{
		DBGMSG("Seek failed!\n");
		return -1;
	}

	DBGMSG("[exit seek]\n");
	return 0;	
}


/*stop the audio thread, it will returned untill audio thead is over
  *but if there is no thread, it will do nothing
  */
int 
audio_stop (void)
{
	DBGMSG("AUDIO_STOP FUNCTION.\n");

	if (audio_stat.pthread_flag) 
	{
		DBGMSG("do quit loop!\n");
		g_main_loop_quit(loop);
			
		pthread_join(pid, NULL);

		audio_stat.suspendflag=1;
	}
	
	DBGMSG("audio_stop returned!\n");
	return 0;
}


static int link_flag;
static int equlink_flag;


int 
audio_speed_set (float old_speed, float new_speed)
{

	DBGMSG("new_speed = %f,old_speed =%f\n", new_speed,old_speed);

//	g_object_set(pPitch, "tempo", new_speed, NULL);

	gint64 pos;
	GstFormat fmt = GST_FORMAT_TIME;
	
	gst_element_query_position (pipeline, &fmt, &pos); 


	gint64 seek_time = pos * (old_speed / new_speed);


	DBGMSG("seek_time = %f, pos=%d\n ", seek_time,pos);

	g_object_set(pPitch, "tempo", new_speed, NULL);
	gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
		GST_SEEK_TYPE_SET, seek_time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

#if 0
	if (link_flag) 
	{ //之前速度不为0
	
		if (new_speed == 1.0)
		{
//			printf("DEBUG: Need Unlink!\n");
			gst_element_set_state(pipeline, GST_STATE_PAUSED);		
			gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);		// 要断开pPitch  元件

			gst_element_unlink(pPitch, conv2);
			gst_element_unlink(conv1, pPitch);
			gst_element_link(conv1, conv2);
			gst_element_sync_state_with_parent(conv2);
				
			gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
				GST_SEEK_TYPE_SET, seek_time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
		
			//gst_element_set_state(pipeline, GST_STATE_PLAYING);			
			//gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
			link_flag = 0;
		}
		else 
		{
//			printf("DEBUG: Set new speed!\n");
			g_object_set(pPitch, "tempo", new_speed, NULL);
			gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
				GST_SEEK_TYPE_SET, seek_time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

			link_flag = 1;
			

		}			

	} 
	else 
	{//之前速度为0
	
		if (new_speed == 1.0) 
		{
//			printf("DEBUG: Return directly!\n");
			return 0;
		} 
		else
		{
//			printf("DEBUG: Need link and set speed!\n");
			gst_element_set_state(pipeline, GST_STATE_PAUSED);
			gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

			gst_element_unlink(conv1, conv2);
			gst_element_link(conv1, pPitch);
			gst_element_link(pPitch, conv2);
			gst_element_sync_state_with_parent(pPitch);
			g_object_set(pPitch, "tempo", new_speed, NULL);
			gst_element_sync_state_with_parent(pPitch);
			
			gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
				GST_SEEK_TYPE_SET, seek_time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

			//gst_element_set_state(pipeline, GST_STATE_PLAYING);			
		//	gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
			link_flag = 1;
		}
	}
#endif
	return 0;
}


/*audio set equalizer*/
int 
audio_equalizer_set (double* band)
{
	printf("set equalizer=%p!\n",pEqualizer);
	
	int i;
	for (i = 0; i< 10; i++) 
	{

		printf("band[%d] = %f", i, band[i]);

	}

	if(pEqualizer)
	{
		

	g_object_set(pEqualizer, 
		 				  "band0", band[0], 
						  "band1", band[1], 
						  "band2", band[2], 
						  "band3", band[3], 
						  "band4", band[4], 
						  "band5", band[5], 
						  "band6", band[6], 
						  "band7", band[7], 
						  "band8", band[8], 
						  "band9", band[9], 
						  NULL);
	}

	return 0;
}


/*set audio stat of current time*/
static void 
set_audio_current_time (GstElement* pipeline)
{
	gint64 pos;

	double time;
	
	GstFormat fmt = GST_FORMAT_TIME;
	if (gst_element_query_position(pipeline, &fmt, &pos)) 
	{
		time= GST_TIME_AS_MSECONDS(pos);
	//	printf("set_audio_currentl_time=%f, %f \n",time,player.speed);	
		audio_stat.current_play_time = plextalk_sound_speed[player.speed] * time;
		//audio_stat.current_play_time = time;
	} 
	else 
	{
		//DBGMSG("set audio current time failure!\n");
	}
}


/*set audio stat of total time*/
static  void 
set_audio_total_time(GstElement* pipeline)
{
	gint64 len;
	double time;

	GstFormat fmt = GST_FORMAT_TIME;
	if (gst_element_query_duration(pipeline, &fmt, &len)) 
	{

		time= GST_TIME_AS_MSECONDS(len);
	//	printf("set_audio_total_time=%f, %f \n",time,player.speed);
		audio_stat.total_play_time = plextalk_sound_speed[player.speed] * time;
	//	audio_stat.total_play_time = time;
	} 
	else
	{
		//DBGMSG("set audio total time failure!\n");
	}
}


/*g_timer callback function*/
static gboolean 
cb_timeout_action (GstElement* pipeline)
{
	set_audio_current_time(pipeline);
	set_audio_total_time(pipeline);

	return TRUE;
}



static void
add_pad (GstElement *element , GstPad *pad , gpointer data)
{ 
	gchar *name;
	GstElement *sink = (GstElement*)data;
 
	name = gst_pad_get_name(pad);
	gst_element_link_pads(element , name , sink , "sink");
	g_free(name);
}


/*let decode  audio linked*/
static void
cb_newpad (GstElement *decodebin, GstPad *pad, 
			   gboolean last, gpointer data)
{
	GstCaps *caps;	
	GstStructure *str;
	GstPad *audiopad;

	/* only link once */
	audiopad = gst_element_get_pad (audio, "sink");
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


/*bus message handler*/
static gboolean 
bus_callback (GstBus* bus, GstMessage *msg, gpointer data)
{
	GMainLoop* loop = (GMainLoop*) data;
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
			DBGMSG("End of stream\n");
			audio_stat.stop_flag = 1;
			
			g_main_loop_quit(loop);
			break;
			
		case GST_MESSAGE_ERROR:
		{
			gchar* debug;
			GError *error;

			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			DBGMSG("ERROR:%s\n", error->message);
			g_error_free(error);
			audio_stat.stop_flag = 1;
			audio_stat.error_flag = 1;
			g_main_loop_quit(loop);
			break;
		}
		default:
			break;
	}
	
	return TRUE;
}


/*init global value*/
static 
void init_global_var (struct start* this)
{
	start_seek_time = this->start_time;

	start_seek_time_flag 		 = 1;
	
	audio_stat.pthread_flag 	 = 1;
	audio_stat.pause_flag 		 = 0;
	audio_stat.current_play_time = 0;
	audio_stat.total_play_time 	 = 0;

	audio_stat.error_flag = 0;
}


void*
Track_play (void* arg)
{


	 DBGMSG("Track_play  thread begin!\n");
	 
	 GstElement *src, *dec,  *sink;
	 GstPad *audiopad;
	 GstBus *bus;
	 gint timer_tag;
	 int number;
	 
	 struct start* this = arg;
	 
	

	// pthread_detach(pthread_self());

	 
	 init_global_var(this);
	 
	 gst_init (NULL, NULL);
	 loop = g_main_loop_new (NULL, FALSE);
	 
	 /* setup */
	 pipeline = gst_pipeline_new ("pipeline");

	 DBGMSG("Track_play pipeline=%p!\n",pipeline);	
	 
	 bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	 gst_bus_add_watch (bus, bus_callback, loop);
	 gst_object_unref (bus);
	 number=this->trackindex;

	  DBGMSG("Track_play  trackindex =%d !\n" ,this->trackindex);
	 
#if 0
	 /* factory make filesrc */
	 src = gst_element_factory_make ("cddasrc", "source");
	 assert(src);
	 g_object_set (G_OBJECT (src), "track", number, NULL);
	 
	 /* factory make decodebin */
	 dec = gst_element_factory_make ("decodebin", "decoder");
	 assert(dec);
	 g_signal_connect (dec, "new-decoded-pad", G_CALLBACK (cb_newpad), NULL);
	 
	 /* link src and dec in pipeline */
	 gst_bin_add_many (GST_BIN (pipeline), src, dec, NULL);
	 gst_element_link (src, dec);
	 
	 /* create audiobin for output(just like alsasink) */
	 audio = gst_bin_new ("audiobin");
	 assert(audio);
	 
	 /* cconv1 will be the first element in audio(BIN) */
	 conv1       = gst_element_factory_make("audioconvert", "aconv1");
	 assert(conv1);
	 audiopad = gst_element_get_pad (conv1, "sink");
#endif
	 

	 /* factory make other element */ 
	 src = gst_element_factory_make ("cdiocddasrc", "source");


	DBGMSG("track is=%p!   number=%d \n",src,number);
	
	 g_object_set (G_OBJECT (src), "track", number, NULL);
	 
	 sink        = gst_element_factory_make("alsasink", "sink");
	 conv1       = gst_element_factory_make("audioconvert", "aconv1");
	 pPitch      = gst_element_factory_make("pitch", "pitch");
	 
	 conv2       = gst_element_factory_make("audioconvert", "aconv2");
	 pEqualizer  = gst_element_factory_make("equalizer-10bands", "pEqualizer");
	g_object_set(pEqualizer, 
	 				  "band0", this->bands[0], 
					  "band1", this->bands[1], 
					  "band2", this->bands[2], 
					  "band3", this->bands[3], 
					  "band4", this->bands[4], 
					  "band5", this->bands[5], 
					  "band6", this->bands[6], 
					  "band7", this->bands[7], 
					  "band8", this->bands[8], 
					  "band9", this->bands[9], 
					  NULL);


	 
	 assert(sink && pPitch  && conv2 && pEqualizer);
#if 0
	 /* put this element into audio(BIN) */
	 gst_bin_add_many (GST_BIN (audio), conv1, pPitch, conv2, pEqualizer, sink, NULL);
	 gst_element_link_many (conv1, pPitch, conv2, pEqualizer, sink, NULL);
	 
	 /* Add a ghost on aduio(BIN) */
	 gst_element_add_pad (audio,gst_ghost_pad_new ("sink", audiopad));
	 gst_object_unref (audiopad);
#endif 


	
	 gst_bin_add_many (GST_BIN (pipeline), src,conv1,  pPitch, conv2, pEqualizer, sink, NULL);

#if 0
	if (player.speed == 0) 
	{
		 gst_element_link_many (src,  conv1, conv2, pEqualizer, sink, NULL);
		link_flag = 0;
	}
	else
	
	{
		 g_object_set(pPitch, "tempo", this->speed, NULL);

		 gst_element_link_many (src,  conv1,pPitch, conv2, pEqualizer, sink, NULL);
		 link_flag = 1;
	}
#endif
	 g_object_set(pPitch, "tempo", this->speed, NULL);

	 gst_element_link_many (src,  conv1,pPitch, conv2, pEqualizer, sink, NULL);
	 link_flag = 1;


	 
	// /*decode and audio will be link in the cb_newpad function*/
	// gst_bin_add (GST_BIN (pipeline), audio);
	  
	 /* Add timer for set audio time  */
	 timer_tag = g_timeout_add (100, (GSourceFunc)cb_timeout_action, pipeline);
	 
	 /* run  loop*/
	 gst_element_set_state (pipeline, GST_STATE_PLAYING);
	  
	 pthread_cond_signal(&cond);
	 
	 g_main_loop_run (loop);
	 
	 /* cleanup */
	 g_source_remove(timer_tag);
	 gst_element_set_state (pipeline, GST_STATE_NULL);
	 gst_object_unref (GST_OBJECT (pipeline));
	 g_main_loop_unref (loop);
	 
	 audio_stat.pthread_flag = 0;
	 DBGMSG("audio thread returned!\n");

	 pthread_exit(0);
	 
	 return 0;
}


void*
audio_play (void* arg)
{
	DBGMSG("audio thread begin!\n");
	GstElement *src, *dec, *sink;
	GstPad *audiopad;
	GstBus *bus;
	gint timer_tag;

	struct start* this = arg;


	
	init_global_var(this);

	gst_init (NULL, NULL);
	loop = g_main_loop_new (NULL, FALSE);

	/* setup */
	pipeline = gst_pipeline_new ("pipeline");

	DBGMSG("audio_play pipeline=%p!\n",pipeline);		
	
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, bus_callback, loop);
	gst_object_unref (bus);

	/* factory make filesrc */
	src = gst_element_factory_make ("filesrc", "source");
	assert(src);
	g_object_set (G_OBJECT (src), "location", this->path, NULL);   //设置数据源的location 属性 

	/* factory make decodebin */
	dec = gst_element_factory_make ("decodebin", "decoder");
	assert(dec);
	g_signal_connect (dec, "new-decoded-pad", G_CALLBACK (cb_newpad), NULL);

	/* link src and dec in pipeline */
	gst_bin_add_many (GST_BIN (pipeline), src, dec, NULL);
	gst_element_link (src, dec);

	/* create audiobin for output(just like alsasink) */
	audio = gst_bin_new ("audiobin");
	assert(audio);

	/* cconv1 will be the first element in audio(BIN) */
	conv1       = gst_element_factory_make("audioconvert", "aconv1");
	assert(conv1);
	audiopad = gst_element_get_pad (conv1, "sink");

	/* factory make other element */ 
	sink        = gst_element_factory_make("alsasink", "sink");
	
	pPitch 	    = gst_element_factory_make("pitch", "pitch");
	
	conv2       = gst_element_factory_make("audioconvert", "aconv2");
	pEqualizer  = gst_element_factory_make("equalizer-10bands", "pEqualizer");

	DBGMSG("audio_play pEqualizer=%p!\n",pEqualizer);	
	
	//
	g_object_set(pEqualizer, 
	 				  "band0", this->bands[0], 
					  "band1", this->bands[1], 
					  "band2", this->bands[2], 
					  "band3", this->bands[3], 
					  "band4", this->bands[4], 
					  "band5", this->bands[5], 
					  "band6", this->bands[6], 
					  "band7", this->bands[7], 
					  "band8", this->bands[8], 
					  "band9", this->bands[9], 
					  NULL);

	
	assert(sink && pPitch  && conv2 && pEqualizer);

	gst_bin_add_many (GST_BIN (audio), conv1, pPitch, conv2, pEqualizer, sink, NULL);



#if 0
	/* put this element into audio(BIN) */
	//CoolShmReadLock(g_config_lock);
	if (player.speed == 0) 
	{


		gst_element_link_many (conv1, conv2, pEqualizer, sink, NULL);

		if(player.effect_value==0)
		{
			equlink_flag=0;
		
		}
		else
		{
			equlink_flag=1;
			
		}

		
		link_flag = 0;
	}
	else 
	{
		//速度不为0
		
		g_object_set(pPitch, "tempo", this->speed, NULL);
		gst_element_link_many (conv1, pPitch, conv2, pEqualizer, sink, NULL);

		if(player.effect_value==0)
		{
			
			equlink_flag=0;
		
		}
		else
		{
			equlink_flag=1;
		}
		link_flag = 1;
	}
#endif
		g_object_set(pPitch, "tempo", this->speed, NULL);
		gst_element_link_many (conv1, pPitch, conv2, pEqualizer, sink, NULL);

		if(player.effect_value==0)
		{
			
			equlink_flag=0;
		
		}
		else
		{
			equlink_flag=1;
		}
		link_flag = 1;

	
	//CoolShmReadUnlock(g_config_lock);


	
	
	/* Add a ghost on aduio(BIN) */
	gst_element_add_pad (audio,gst_ghost_pad_new ("sink", audiopad));
	gst_object_unref (audiopad);
		
	/*decode and audio will be link in the cb_newpad function*/
	gst_bin_add (GST_BIN (pipeline), audio);
		
	/* Add timer for set audio time  */
	timer_tag = g_timeout_add (100, (GSourceFunc)cb_timeout_action, pipeline);

	/* run  loop*/
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
		
	pthread_cond_signal(&cond);
	
	g_main_loop_run (loop);
	
	/* cleanup */
	
	g_source_remove(timer_tag);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (pipeline));
	g_main_loop_unref (loop);

	audio_stat.pthread_flag = 0;
	DBGMSG("audio thread returned!\n");


	pthread_exit(0);

	return 0;
}

#endif
