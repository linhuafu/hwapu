#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <alloca.h>

#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>
#include <libdaemon/dfork.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "vprompt.h"
#include "amixer.h"
#include "plextalk-config.h"
#include "plextalk-constant.h"

#define ivTTS_RESOURCE_FILE_5_0  PLEXTALK_DATA_DIR "AiSound5/resource.irf"
#define ivTTS_RESOURCE_FILE_MTTS PLEXTALK_DATA_DIR "AiSound5/resourceMtts.irf"

#define ivTTS_HEAP_SIZE (512 * 1024)

struct tts_instance {
	/*TTS engine*/
	ivHTTS hTTS;
	ivPByte	pHeap;
	ivTResPackDescExt tResPackDesc;

	const char* resource;
	ivCStrA pUserID;

	ivTTSErrID ivCall (*CreateG)(
		ivHTTS ivPtr	phTTS,				/* [out] handle to an instance */
		ivPointer		pHeap,				/* [in] heap for instance */
		ivSize			nHeapSize,			/* [in] size of the heap */
		ivPointer		pCBParam,			/* [in] user callback parameter */
		ivPResPackDescExt	pResPackDesc,		/* [in] resource pack description array */
		ivSize			nResPackCount,		/* [in] resource pack count */
		ivPTTSUserInfo pUserInfo,			/* [in] TTS User Info */
	    ivCStrA        pUserID);            /* [in] SDK ID */

	/* destroy an instance */
	ivTTSErrID ivCall (*Destroy)(
		ivHTTS			hTTS );				/* [in] handle to an instance */

	/* get a parameter associated with an instance */
	ivTTSErrID ivCall (*GetParam)(
		ivHTTS			hTTS,				/* [in] handle to an instance */
		ivUInt32		nParamID,			/* [in] parameter ID */
		ivPUInt32		pnParamValue );		/* [out] buffer to receive the parameter value */

	/* set a parameter associated with an instance */
	ivTTSErrID ivCall (*SetParam)(
		ivHTTS			hTTS,				/* [in] handle to an instance */
		ivUInt32		nParamID,			/* [in] parameter ID */
		ivUInt32		nParamValue );		/* [in] parameter value */

	/* run an instance and hold current thread's control */
	ivTTSErrID ivCall (*Run)(
		ivHTTS			hTTS );				/* [in] handle to an instance */

	/* exit running of an instance and leave current thread's control */
	ivTTSErrID ivCall (*Exit)(
		ivHTTS			hTTS );				/* [in] handle to an instance */

	/* synthesize a buffer of text on an instance */
	ivTTSErrID ivCall (*SynthText)(
		ivHTTS			hTTS,				/* [in] handle to an instance */
		ivCPointer		pcData,				/* [in] pointer of text buffer data to be synthesized */
		ivSize			nSize );			/* [in] size of text buffer data to be synthesized */

	/* begin to synthesize from callback on an instance */
	ivTTSErrID ivCall (*SynthStart)(
		ivHTTS			hTTS );				/* [in] handle to an instance */
};

static struct voice_prompt {
	/*TTS engine*/
	struct tts_instance aisound5;
	struct tts_instance aisoundMtts;
	struct tts_instance *tts;

	/*gstreamer*/
	GstElement *pipeline;
	GMainLoop *loop;
	GstElement *appsrc;

	pthread_t gst_pth;
	pthread_t tts_pth;
	sem_t tts_trigger;
	sem_t tts_end;

	volatile int abort;
	int abort_pid;
	unsigned int abort_stamp;
	unsigned int bench;
	pthread_mutex_t lock;

	int audio_pid;		//add by lhf
} vprompt;

static char buffer[VPROMPT_MAX_MSGSIZE+1];
static mqd_t mqd = (mqd_t)-1;
static int quit;

static ivTTSErrID ivCall AudioOutputCB(ivPointer		pParameter,	/* [in] user callback parameter */
								  ivUInt16		nCode,		/* [in] output data code */
								  ivCPointer	pcData,		/* [in] output data buffer */
								  ivSize		nSize );

static ivBool ivCall ReadResCB( ivPointer		pParameter,	/* [in] user callback parameter */
								ivPointer		pBuffer,	/* [out] read resource buffer */
								ivResAddress	iPos,		/* [in] read start position */
								ivResSize		nSize )		/* [in] read size */
{
	FILE *fp = (FILE *) pParameter;
    int ret;

	fseek(fp, iPos, SEEK_SET);

	ret = fread(pBuffer, 1, nSize, fp);
    if ( ret == (int)nSize )
	    return ivTrue;
    else
        return ivFalse;
}

static int VoicePromptTTSInstanceInit(struct voice_prompt *vp, struct tts_instance *tts)
{
	ivTTSErrID ivReturn;
	FILE* fp;

	tts->pHeap = (ivPByte) malloc(ivTTS_HEAP_SIZE);
	if (tts->pHeap == NULL) {
		daemon_log(LOG_ERR, "Out of memory!\n");
		goto err1;
	}
	memset(tts->pHeap, 0, ivTTS_HEAP_SIZE);

	fp = fopen(tts->resource, "rb");
	if (fp == NULL) {
		daemon_log(LOG_ERR, "Open %s failed (%s)", tts->resource, strerror(errno));
		goto err2;
	}

	tts->tResPackDesc.pCBParam = fp;
	tts->tResPackDesc.pfnRead = ReadResCB;
	tts->tResPackDesc.pfnMap = NULL;
	tts->tResPackDesc.nSize = 0;

	tts->tResPackDesc.pCacheBlockIndex = NULL;
	tts->tResPackDesc.pCacheBuffer     = NULL;
	tts->tResPackDesc.nCacheBlockSize  = 0;
	tts->tResPackDesc.nCacheBlockCount = 0;
	tts->tResPackDesc.nCacheBlockExt   = 0;

	ivReturn = tts->CreateG(&tts->hTTS,
					tts->pHeap, ivTTS_HEAP_SIZE, vp,
					&tts->tResPackDesc, 1, NULL, tts->pUserID);
	if (ivReturn != ivTTS_ERR_OK) {
		daemon_log(LOG_ERR, "ivTTS_Create failed (%d)!\n", ivReturn);
		goto err3;
	}

	tts->SetParam(tts->hTTS, ivTTS_PARAM_OUTPUT_CALLBACK, (ivUInt32) AudioOutputCB);

	return 0;

err3:
	fclose(fp);
err2:
	free(tts->pHeap);
err1:
	return -1;
}

static void VoicePromptTTSInstanceUninit(struct tts_instance *tts)
{
	ivTTS_Destroy(tts->hTTS);

	if (tts->tResPackDesc.pCacheBlockIndex)
		free(tts->tResPackDesc.pCacheBlockIndex);
	if (tts->tResPackDesc.pCacheBuffer)
		free(tts->tResPackDesc.pCacheBuffer);

	fclose(tts->tResPackDesc.pCBParam);

	free(tts->pHeap);
}

static int VoicePromptTTSInit(struct voice_prompt *vp)
{
	int ret;

	vp->aisound5.resource = ivTTS_RESOURCE_FILE_5_0;
	vp->aisound5.pUserID = AISOUND_5_0_SDK_USERID;

	vp->aisound5.CreateG = ivTTS_5_0_CreateG;
	vp->aisound5.Destroy = ivTTS_5_0_Destroy;
	vp->aisound5.GetParam = ivTTS_5_0_GetParam;
	vp->aisound5.SetParam = ivTTS_5_0_SetParam;
	vp->aisound5.Run = ivTTS_5_0_Run;
	vp->aisound5.Exit = ivTTS_5_0_Exit;
	vp->aisound5.SynthText = ivTTS_5_0_SynthText;
	vp->aisound5.SynthStart = ivTTS_5_0_SynthStart;

	ret = VoicePromptTTSInstanceInit(vp, &vp->aisound5);
	if (ret < 0) {
		daemon_log(LOG_ERR, "aisound5 init failed!\n");
		return ret;
	}

	vp->aisoundMtts.resource = ivTTS_RESOURCE_FILE_MTTS;
	vp->aisoundMtts.pUserID = AISOUND_SDK_USERID;

	vp->aisoundMtts.CreateG = ivTTS_CreateG;
	vp->aisoundMtts.Destroy = ivTTS_Destroy;
	vp->aisoundMtts.GetParam = ivTTS_GetParam;
	vp->aisoundMtts.SetParam = ivTTS_SetParam;
	vp->aisoundMtts.Run = ivTTS_Run;
	vp->aisoundMtts.Exit = ivTTS_Exit;
	vp->aisoundMtts.SynthText = ivTTS_SynthText;
	vp->aisoundMtts.SynthStart = ivTTS_SynthStart;

	ret = VoicePromptTTSInstanceInit(vp, &vp->aisoundMtts);
	if (ret < 0) {
		VoicePromptTTSInstanceUninit(&vp->aisound5);
		daemon_log(LOG_ERR, "aisoundMtts init failed!\n");
		return ret;
	}

	return 0;
}

static void VoicePromptTTSUninit(struct voice_prompt *vp)
{
	VoicePromptTTSInstanceUninit(&vp->aisound5);
	VoicePromptTTSInstanceUninit(&vp->aisoundMtts);
}
/*
static void VoicePromptNotifyClientNoProtect(struct voice_prompt *vp, struct vprompt_msg *msg)
{
	if (msg->client_pid >= 0 && vp->abort_pid != msg->client_pid) {
		union sigval value;
		value.sival_int = msg->msg_id;
		sigqueue(msg->client_pid, SIGVPEND, value);
	}
}

static int VoicePromptAborted(struct voice_prompt *vp)
{
	int abort;

	pthread_mutex_lock(&vp->lock);
	abort = vp->abort;
	pthread_mutex_unlock(&vp->lock);

	return abort;
}
*/
static void VoicePromptNotifyClient(struct voice_prompt *vp, struct vprompt_msg *msg)
{
	int notify;

	if (msg->client_pid < 0)
		return;

	pthread_mutex_lock(&vp->lock);
	notify = !vp->abort || vp->abort_pid != msg->client_pid;
	pthread_mutex_unlock(&vp->lock);

	if (notify) {
		union sigval value;
		value.sival_int = msg->msg_id;
		sigqueue(msg->client_pid, SIGVPEND, value);
	}
}

//#define __TTS_SAMPLE

#ifdef __TTS_SAMPLE

static void write_buffer_to_file (const char *file, const char *buffer, int len)
{
	int fd = open(file, O_RDWR|O_CREAT|O_APPEND);

	write(fd, buffer, len);
	close(fd);
}

#endif

static ivTTSErrID ivCall AudioOutputCB(ivPointer		pParameter,	/* [in] user callback parameter */
								  ivUInt16		nCode,		/* [in] output data code */
								  ivCPointer	pcData,		/* [in] output data buffer */
								  ivSize		nSize )		/* [in] output data size */
{
	struct voice_prompt *vp = (struct voice_prompt *) pParameter;
	GstBuffer *gbuffer;
	GstFlowReturn ret;


#ifdef __TTS_SAMPLE
	write_buffer_to_file("/tmp/tts_sample", pcData, nSize);
	
#endif

	if (vp->abort)
		return ivTTS_ERR_EXIT;
//		return ivTTS_ERR_OK;

	gbuffer = gst_buffer_try_new_and_alloc(nSize);
	if (gbuffer == NULL) {
		daemon_log(LOG_WARNING, "AudioOutputCB alloc buffer failed");
//		return ivTTS_ERR_FAILED;
		return ivTTS_ERR_EXIT;
	}

	memcpy(GST_BUFFER_DATA(gbuffer), pcData, nSize);

	ret = gst_app_src_push_buffer(GST_APP_SRC(vp->appsrc), gbuffer);
	if (ret !=  GST_FLOW_OK) {
		gst_buffer_unref(gbuffer);
		daemon_log(LOG_WARNING, "AudioOutputCB push failed: %d\n", ret);
//		return ivTTS_ERR_FAILED;
		return ivTTS_ERR_EXIT;
	}

	return ivTTS_ERR_OK;
}

static gboolean VoicePromptBusCall (GstBus     *bus,
									GstMessage *msg,
									gpointer    data)
{
	struct voice_prompt *vp = (struct voice_prompt*)data;
	struct vprompt_msg *vp_msg = (struct vprompt_msg*)buffer;

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_EOS:
		if (!vp->abort && vp_msg->mode == VPROMPT_MODE_TTS) {
			/* wait buffer flush */
			int n = 25;
			do {
				usleep(10 * 1000);
			} while (--n && !vp->abort);
		}
		break;

	case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;

//			if (vp_msg->mode == VPROMPT_MODE_TTS)
//				vp->tts->Exit(vp->tts->hTTS);

			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);

			daemon_log(LOG_WARNING, "Error: %s\n", err->message);
			g_error_free (err);
		}
		break;

	default:
		return TRUE;
	}

	g_main_loop_quit (vp->loop);
	return TRUE;
}

static int VoicePromptCreatePostProc(GstElement **postproc, struct vprompt_msg *msg)
{
	int n = 0;

	postproc[0] = gst_element_factory_make("audioconvert", NULL);
	if (!postproc[0]) {
		daemon_log(LOG_ERR, "Could not create audioconvert");
		goto err1;
	}
	n++;

	if (msg->cfg.speed != 0) {
		postproc[n] = gst_element_factory_make("pitch", NULL);
		if (!postproc[n]) {
			daemon_log(LOG_ERR, "Could not create pitch");
			goto err1;
		}

		g_object_set(G_OBJECT(postproc[n]), "tempo", plextalk_sound_speed[msg->cfg.speed], NULL);
		n++;
	}

	postproc[n] = gst_element_factory_make ("alsasink", NULL);
	if (!postproc[n]) {
		daemon_log(LOG_ERR, "Could not create alsasink");
		goto err1;
	}
	n++;

	return n;

err1:
	while (n--) {
		if (!gst_element_get_parent(GST_OBJECT(postproc[n])))
			gst_object_unref(postproc[n]);
		postproc[n] = NULL;
	}
	return -1;
}

static void* VoicePromptTTSWorker(void *arg)
{
	struct voice_prompt *vp = (struct voice_prompt*)arg;
	struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
	struct tts_instance *tts;
	int ret;

//    pthread_detach(pthread_self());

	while (1) {
		sem_wait(&vp->tts_trigger);
		if (quit)
			break;

		tts = vp->tts;

		tts->SetParam(tts->hTTS, ivTTS_PARAM_INPUT_CODEPAGE, msg->tts.codepage);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_LANGUAGE, msg->tts.lang);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_ROLE, msg->tts.role);
//		tts->SetParam(tts->hTTS, ivTTS_PARAM_VOLUME, ivTTS_VOLUME_NORMAL);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_READ_DIGIT, msg->tts.read_digit);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_CHINESE_NUMBER_1, msg->tts.chinese_number_1);

		if (!vp->abort) {
			ret = tts->SynthText(tts->hTTS, (ivCPointer)msg->tts.text, msg->tts.len);
			if (ret != ivTTS_ERR_OK)
				daemon_log(LOG_ERR, "ivTTS_SynthText error: %d\n", ret);
		}
//		if (!vp->abort)
			gst_app_src_end_of_stream(GST_APP_SRC(vp->appsrc));

		sem_post(&vp->tts_end);
	}

	/* terminate the thread */
    pthread_exit(NULL);
}

static int VoicePromptTTS(struct voice_prompt *vp, struct vprompt_msg *msg)
{
	GstElement *pipeline = NULL;
	GstElement *appsrc = NULL;
	GstElement *postproc[3] = {};	// may be audioconv, pitch, audiosink
	int postproc_cnt;
	GstCaps* caps;
	GstBus *bus = NULL;

    pipeline = gst_pipeline_new ("pipeline");
    if (!pipeline) {
    	daemon_log(LOG_ERR, "Could not create pipeline");
    	goto err1;
    }

	appsrc = gst_element_factory_make ("appsrc", NULL);
	if (!appsrc) {
		daemon_log(LOG_ERR, "Could not create appsrc");
		goto err1;
	}

	caps = gst_caps_new_simple ("audio/x-raw-int",
		"width", G_TYPE_INT, 16,
		"depth", G_TYPE_INT, 16,
		"endianness", G_TYPE_INT, G_BYTE_ORDER,
		"rate", G_TYPE_INT, 16000,
		"signed", G_TYPE_BOOLEAN, TRUE,
		"channels", G_TYPE_INT, 1,
		NULL);
	gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
	gst_app_src_set_stream_type(GST_APP_SRC(appsrc), GST_APP_STREAM_TYPE_STREAM);
	g_object_set(G_OBJECT(appsrc), "block", TRUE, NULL);

	postproc_cnt = VoicePromptCreatePostProc(postproc, msg);
	if (postproc_cnt <= 0) {
		daemon_log(LOG_ERR, "Could not create post process elements");
		goto err1;
	}

	gst_bin_add_many (GST_BIN (pipeline), appsrc, postproc[0], postproc[1], postproc[2], NULL);
	if (!gst_element_link_many(appsrc, postproc[0], postproc[1], postproc[2], NULL)) {
		daemon_log(LOG_ERR, "Could not link appsrc, audioconv, pitch, audiosink");
		goto err1;
	}

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, VoicePromptBusCall, &vprompt);
	gst_object_unref (bus);

	pthread_mutex_lock(&vp->lock);
	if (vp->abort) {
		pthread_mutex_unlock(&vp->lock);
		daemon_log(LOG_INFO, "Abort by client");
		goto err3;
	}
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	vp->pipeline = pipeline;
	pthread_mutex_unlock(&vp->lock);

	vp->appsrc = appsrc;
	if (msg->tts.lang == ivTTS_LANGUAGE_HINDI)
		vp->tts = &vp->aisoundMtts;
	else
		vp->tts = &vp->aisound5;
	sem_post(&vp->tts_trigger);

	g_main_loop_run (vp->loop);
	sem_wait(&vp->tts_end);

	pthread_mutex_lock(&vp->lock);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	vp->pipeline = NULL;
	pthread_mutex_unlock(&vp->lock);

	vp->appsrc = NULL;
	gst_object_unref (GST_OBJECT (pipeline));

	return 0;

err3:
	gst_element_set_state (pipeline, GST_STATE_NULL);
//err2:
	gst_object_unref (GST_OBJECT (pipeline));
	return -1;

err1:
	if (appsrc && !gst_element_get_parent(GST_OBJECT(appsrc)))
		gst_object_unref(appsrc);
	while (postproc_cnt--) {
		if (!gst_element_get_parent(GST_OBJECT(postproc[postproc_cnt])))
			gst_object_unref(postproc[postproc_cnt]);
	}
	gst_object_unref (GST_OBJECT (pipeline));
	return -1;
}

static void VoicePromptNewPad (GstElement *decodebin, GstPad *pad,
			   gboolean last, gpointer data)
{
	GstCaps *caps;
	GstStructure *str;
	GstPad *audiopad;

	/* only link once */
	audiopad = gst_element_get_static_pad ((GstElement *)data, "sink");
//	if (GST_PAD_IS_LINKED (audiopad)) {
//		g_object_unref (audiopad);
//		return;
//	}

	/* check media type */
	caps = gst_pad_get_caps (pad);
	str = gst_caps_get_structure (caps, 0);
	if (g_strrstr (gst_structure_get_name (str), "audio"))
		gst_pad_link (pad, audiopad);
	gst_caps_unref (caps);

	gst_object_unref (audiopad);
}

//add by lhf start
static int musicSeek(struct voice_prompt *vp,struct vprompt_msg *msg)
{
	GstClockTime seek_beg_time;
	GTimeVal val;
	gboolean ret_bool;
	GstStateChangeReturn stateRet;
	GstState state;
	unsigned long start;
	
	if(0 == msg->music.start){
		gst_element_set_state (vp->pipeline, GST_STATE_PLAYING);
		return 0;
	}

	start = msg->music.start / plextalk_sound_speed[msg->cfg.speed];
	val.tv_sec	= start / 1000;
	val.tv_usec = (start % 1000)*1000;
	seek_beg_time = GST_TIMEVAL_TO_TIME(val);

	stateRet = gst_element_set_state (vp->pipeline, GST_STATE_PAUSED);
	stateRet = gst_element_get_state (vp->pipeline, &state, NULL,GST_CLOCK_TIME_NONE);
	daemon_log(LOG_INFO, "stateRet=%d!\n",stateRet);
	ret_bool = gst_element_seek (vp->pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
			GST_SEEK_TYPE_SET, seek_beg_time, 
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	daemon_log(LOG_INFO, "ret_bool=%d!\n",ret_bool);
	stateRet = gst_element_set_state (vp->pipeline, GST_STATE_PLAYING);
	return 0;
}
//add by lhf end
//add by lhf start
static gboolean cb_timeout (struct voice_prompt *vp)
{
	daemon_log(LOG_INFO, "TIMER OUT!\n");
	g_main_loop_quit (vp->loop);
	return FALSE;
}
//add by lhf end

static int VoicePromptMusic(struct voice_prompt *vp, struct vprompt_msg *msg)
{
	GstElement *pipeline = NULL;
	GstElement *filesrc = NULL;
	GstElement *decodebin = NULL;
	GstElement *postproc[3] = {};	// may be audioconv, pitch, audiosink
	int postproc_cnt;
	GstBus *bus = NULL;
	gint timer_tag = 0;		//add by lhf
	
    pipeline = gst_pipeline_new ("pipeline");
    if (!pipeline) {
    	daemon_log(LOG_ERR, "Could not create pipeline");
    	goto err1;
    }

	postproc_cnt = VoicePromptCreatePostProc(postproc, msg);
	if (postproc_cnt <= 0) {
		daemon_log(LOG_ERR, "Could not create post process elements");
		goto err1;
	}

	filesrc = gst_element_factory_make("filesrc", NULL);
	if (!filesrc) {
		daemon_log(LOG_ERR, "Could not create filesrc");
		goto err1;
	}

	g_object_set(G_OBJECT(filesrc), "location", msg->music.path, NULL);

	decodebin = gst_element_factory_make("decodebin2", NULL);
	if (!decodebin) {
		daemon_log(LOG_ERR, "Could not create decodebin");
		goto err1;
	}

	g_signal_connect (G_OBJECT (decodebin), "new-decoded-pad",
					G_CALLBACK (VoicePromptNewPad), postproc[0]);

	gst_bin_add_many (GST_BIN (pipeline), filesrc, decodebin, postproc[0], postproc[1], postproc[2], NULL);
	if (!gst_element_link (filesrc, decodebin)) {
		daemon_log(LOG_ERR, "Could not link decodebin to filesrc");
		goto err1;
	}
	if (!gst_element_link_many(postproc[0], postproc[1], postproc[2], NULL)) {
		daemon_log(LOG_ERR, "Could not link audioconv, pitch, audiosink");
		goto err1;
	}

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, VoicePromptBusCall, &vprompt);
	gst_object_unref (bus);

	pthread_mutex_lock(&vp->lock);
	if (vp->abort) {
		pthread_mutex_unlock(&vp->lock);
		daemon_log(LOG_INFO, "Abort by client");
		goto err3;
	}
	//gst_element_set_state (pipeline, GST_STATE_PLAYING);	//rm by lhf
	vp->pipeline = pipeline;
	musicSeek(vp, msg);		//add by lhf
	pthread_mutex_unlock(&vp->lock);

	if((unsigned long)-1!= msg->music.length ){//add by lhf
		guint interval = msg->music.length*plextalk_sound_speed[msg->cfg.speed];
		timer_tag = g_timeout_add (interval, (GSourceFunc)cb_timeout, vp);
	}//lhf end
	
	g_main_loop_run (vp->loop);

	if(timer_tag){//add by lhf
		g_source_remove(timer_tag);
	}//lhf end

	pthread_mutex_lock(&vp->lock);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	vp->pipeline = NULL;
	pthread_mutex_unlock(&vp->lock);

	gst_object_unref (GST_OBJECT (pipeline));

	return 0;

err3:
	gst_element_set_state (pipeline, GST_STATE_NULL);
//err2:
	gst_object_unref (GST_OBJECT (pipeline));
	return -1;

err1:
	if (filesrc && !gst_element_get_parent(GST_OBJECT(filesrc)))
		gst_object_unref(filesrc);
	if (decodebin && !gst_element_get_parent(GST_OBJECT(decodebin)))
		gst_object_unref(decodebin);
	while (postproc_cnt--) {
		if (!gst_element_get_parent(GST_OBJECT(postproc[postproc_cnt])))
			gst_object_unref(postproc[postproc_cnt]);
	}
	gst_object_unref (GST_OBJECT (pipeline));
	return -1;
}

//add by lhf
static int VoicePromptAudio(struct voice_prompt *vp, struct vprompt_msg *msg)
{
	pid_t pid;
	int status;

	pthread_mutex_lock(&vp->lock);
	if (vp->abort) {
		pthread_mutex_unlock(&vp->lock);
		daemon_log(LOG_INFO, "Abort by client");
		return -1;
	}
	pthread_mutex_unlock(&vp->lock);
	
	if((pid = fork())< 0) {
		status = -1;
		daemon_log(LOG_INFO, "Could not fork");
	} else if (pid == 0) {
		char* args[] = {
			"madplay",
			"-S",
			"-q",
			msg->music.path,
			NULL,
		};
		execvp(args[0], args);
		_exit(127);
	} else {
		pthread_mutex_lock(&vp->lock);
		vp->audio_pid = pid;
		pthread_mutex_unlock(&vp->lock);
		daemon_log(LOG_INFO, "!!!!!!!!!!!!!!wait:%d\n", pid);
		while (waitpid(pid, &status, 0) < 0){
			if(errno != EINTR){
				status = -1;
				break;
			}
		}
	}
	daemon_log(LOG_INFO, "exit\n");
	pthread_mutex_lock(&vp->lock);
	vp->audio_pid = 0;
	pthread_mutex_unlock(&vp->lock);
}
//add by lhf end

static long master_vol, dac_vol, mute;
static int change_vol;
static int last_vp_vol;

static const char* output_mux[] = {
	[PLEXTALK_OUTPUT_SELECT_SIDETONE] = PLEXTALK_OUTPUT_SIDETONE,
	[PLEXTALK_OUTPUT_SELECT_BYPASS] = PLEXTALK_OUTPUT_BYPASS,
	[PLEXTALK_OUTPUT_SELECT_DAC] = PLEXTALK_OUTPUT_DAC,
};

static int VoicePromptSaveVolume(enum plextalk_output_select output, int vol)
{
	snd_mixer_t *handle;

	handle = amixer_open(NULL);

//	amixer_get_playback_volume(handle, PLEXTALK_PLAYBACK_VOL_MASTER, 0, &master_vol, NULL);
//	amixer_get_playback_volume(handle, PLEXTALK_PLAYBACK_VOL_DAC, 0, &dac_vol, NULL);
//
//	mute = amixer_get_playback_switch(handle, PLEXTALK_PLAYBACK_MUTE, 0);
//	if (!mute)
//		amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);
//
//	if (master_vol != plextalk_sound_volume[vol].master)
//		amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, plextalk_sound_volume[vol].master);
//	if (dac_vol != plextalk_sound_volume[vol].DAC)
//		amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_DAC, plextalk_sound_volume[vol].DAC);

	amixer_sset_str(handle, VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);
//	amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);

	amixer_close(handle);

//	last_vp_vol = vol;
	return 0;
}

static int VoicePromptRestoreVolume(enum plextalk_output_select output)
{
	snd_mixer_t *handle;

	handle = amixer_open(NULL);

//	amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_ON);

//	if (master_vol != plextalk_sound_volume[last_vp_vol].master)
//		amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_MASTER, master_vol);
//	if (dac_vol != plextalk_sound_volume[last_vp_vol].DAC)
//		amixer_sset_integer(handle, VOL_RAW, PLEXTALK_PLAYBACK_VOL_DAC, dac_vol);

	if (output != PLEXTALK_OUTPUT_SELECT_DAC)
		amixer_sset_str(handle, VOL_RAW, PLEXTALK_OUTPUT_MUX, output_mux[output]);
//	if (!mute)
//		amixer_sset_str(handle, VOL_RAW, PLEXTALK_PLAYBACK_MUTE, PLEXTALK_CTRL_OFF);

	amixer_close(handle);

	return 0;
}

static void* VoicePromptWorker(struct voice_prompt *vp)
{
//    pthread_detach(pthread_self());

	if (VoicePromptTTSInit(vp) < 0) {
		quit = 1;
		goto err1;
	}

	vp->loop = g_main_loop_new (NULL, TRUE);
	if (!vp->loop) {
		quit = 1;
		daemon_log(LOG_ERR, "Could not create main loop");
		goto err2;
	}

    while (!quit) {
    	struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
		ssize_t msg_len;
		unsigned msg_prio;
//		struct mq_attr attr;

		msg_len = mq_receive(mqd, buffer, VPROMPT_MAX_MSGSIZE, &msg_prio);
		if (quit)
			break;
		if (msg_len < 0) {
			daemon_log(LOG_ERR, "mq_receive failed (%s)", strerror(errno));
			continue;
		}
		buffer[msg_len] = '\0';
/*
		if (msg->mode == VPROMPT_MODE_TTS) {
			if (offsetof(struct vprompt_msg, tts.text) + msg->tts.len != msg_len) {
				daemon_log(LOG_WARNING, "Corrupt tts message");
				continue;
			}
		} else {
			if (offsetof(struct vprompt_msg, music.path) + strlen(msg->music.path) + 1 != msg_len) {
				daemon_log(LOG_WARNING, "Corrupt music message");
				continue;
			}
		}
*/
		pthread_mutex_lock(&vp->lock);
		if (vp->abort &&
			(unsigned int)(msg->stamp - vp->bench) >=
			(unsigned int)(vp->abort_stamp - vp->bench)) {
			vp->abort = 0;
		}
		if (vp->abort) {
			pthread_mutex_unlock(&vp->lock);
		} else {
			pthread_mutex_unlock(&vp->lock);

			VoicePromptSaveVolume(msg->output, msg->cfg.volume);

			if (msg->mode == VPROMPT_MODE_TTS){
				VoicePromptTTS(vp, msg);
			}else if (msg->mode == VPROMPT_MODE_MUSIC){	//lhf
				VoicePromptMusic(vp, msg);
			}else{
				VoicePromptAudio(vp, msg);
			}
					//lhf
			VoicePromptRestoreVolume(msg->output);
		}

		vp->bench = msg->stamp;
		VoicePromptNotifyClient(vp, msg);
	}

	g_main_loop_unref (vp->loop);
err2:
	VoicePromptTTSUninit(vp);
err1:
	/* terminate the thread */
    pthread_exit(NULL);
}

static int VoicePromptInit(int argc, char *argv[])
{
	struct mq_attr attr = { .mq_maxmsg = VPROMPT_MAX_MSGS, .mq_msgsize = VPROMPT_MAX_MSGSIZE, };
	int ret;

	gst_init(&argc, &argv);

	mqd = mq_open(VPROMPT_MQ_PATH, O_CREAT | /*O_EXCL |*/ O_RDONLY, 0666, &attr);
	if (mqd == (mqd_t)-1) {
		daemon_log(LOG_ERR, "Cretae Message queue failed (%s)", strerror(errno));
		return -1;
	}

	pthread_mutex_init(&vprompt.lock, NULL);

    if (pthread_create(&vprompt.gst_pth, NULL, VoicePromptWorker, &vprompt) < 0) {
        daemon_log(LOG_ERR, "Create VoicePromptWorker failed (%s)", strerror(errno));
		goto err1;
	}

	sem_init(&vprompt.tts_trigger, 0, 0);
	sem_init(&vprompt.tts_end, 0, 0);

	if (pthread_create(&vprompt.tts_pth, NULL, VoicePromptTTSWorker, &vprompt) < 0) {
		daemon_log(LOG_ERR, "Create VoicePromptTTSWorker failed (%s)", strerror(errno));
		goto err2;
	}

	return 0;

	/* cleanup */
err2:
	sem_destroy(&vprompt.tts_trigger);
	sem_destroy(&vprompt.tts_end);
	pthread_join(vprompt.gst_pth, NULL);
err1:
	pthread_mutex_destroy(&vprompt.lock);
	mq_close(mqd);
	mq_unlink(VPROMPT_MQ_PATH);
	return -1;
}

static void VoicePromptUninit(void)
{
	pthread_mutex_lock(&vprompt.lock);
	vprompt.abort = 1;
	vprompt.abort_pid = getpid();
	vprompt.abort_stamp = voice_prompt_get_stamp();
	if (!vprompt.pipeline)
		pthread_mutex_unlock(&vprompt.lock);
	else {
		struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
		pthread_mutex_unlock(&vprompt.lock);
//		if (msg->mode == VPROMPT_MODE_TTS)
//			vprompt.tts->Exit(vprompt.tts->hTTS);
		g_main_loop_quit (vprompt.loop);
	}

	sem_post(&vprompt.tts_trigger);
	sem_post(&vprompt.tts_end);

	pthread_kill(vprompt.tts_pth, SIGTERM);
	pthread_join(vprompt.tts_pth, NULL);

	sem_destroy(&vprompt.tts_trigger);
	sem_destroy(&vprompt.tts_end);

	pthread_kill(vprompt.gst_pth, SIGTERM);
	pthread_join(vprompt.gst_pth, NULL);

	pthread_mutex_destroy(&vprompt.lock);

	/* cleanup */
	mq_close(mqd);
	mq_unlink(VPROMPT_MQ_PATH);
}

const char *pid_file_proc()
{
    return "/var/run/" VPROMPT_SERVER ".pid";
}

int main(int argc, char *argv[])
{
    pid_t pid;

	daemon_pid_file_proc = pid_file_proc;

    /* Set indetification string for the daemon for both syslog and PID file */
    daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);

    /* Check if we are called with -k parameter */
    if (argc >= 2 && !strcmp(argv[1], "-k")) {
        int ret;

        /* Kill daemon with SIGINT */

        /* Check if the new function daemon_pid_file_kill_wait() is available, if it is, use it. */
#ifdef DAEMON_PID_FILE_KILL_WAIT_AVAILABLE
        if ((ret = daemon_pid_file_kill_wait(SIGINT, 5)) < 0)
#else
        if ((ret = daemon_pid_file_kill(SIGINT)) < 0)
#endif
            daemon_log(LOG_WARNING, "Failed to kill daemon");

        return ret < 0 ? 1 : 0;
    }

    /* Check that the daemon is not rung twice a the same time */
    if ((pid = daemon_pid_file_is_running()) >= 0) {
        daemon_log(LOG_ERR, "Daemon already running on PID file %u", pid);
        return 1;
    }

    /* Prepare for return value passing from the initialization procedure of the daemon process */
    daemon_retval_init();

    /* Do the fork */
    if ((pid = daemon_fork()) < 0) {

        /* Exit on error */
        daemon_retval_done();
        return 1;

    } else if (pid) { /* The parent */
        int ret;

        /* Wait for 20 seconds for the return value passed from the daemon process */
        if ((ret = daemon_retval_wait(20)) < 0) {
            daemon_log(LOG_ERR, "Could not recieve return value from daemon process.");
            return 255;
        }

        daemon_log(ret != 0 ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value.", ret);
        return ret;

    } else { /* The daemon */
        int fd;
        sigset_t mask;
		int ret;

        /* Create the PID file */
        if (daemon_pid_file_create() < 0) {
            daemon_log(LOG_ERR, "Could not create PID file (%s).", strerror(errno));

            /* Send the error condition to the parent process */
            daemon_retval_send(1);
            goto finish;
        }

		sigemptyset(&mask);
		sigaddset(&mask, SIGINT);
		sigaddset(&mask, SIGQUIT);
//		sigaddset(&mask, SIGHUP);
		sigaddset(&mask, SIGVPABORT);

		/* block signals so that they aren't handled according
		 * to their default dispositions */
		ret = pthread_sigmask(SIG_BLOCK, &mask, NULL);
		if (ret != 0) {
            daemon_log(LOG_ERR, "Could not block signals (%d).", ret);

            daemon_retval_send(2);
            goto finish;
		}

		fd = signalfd(-1, &mask, 0);
		if (fd == -1) {
			daemon_log(LOG_ERR, "Could not register signal handlers (%s).", strerror(errno));

            daemon_retval_send(3);
            goto finish;
		}

        /*... do some further init work here */
        if (VoicePromptInit(argc, argv) < 0) {
			daemon_log(LOG_ERR, "Could not init voice-prompt (%s).", strerror(errno));

            daemon_retval_send(4);
            goto finish1;
        }

        /* Send OK to parent process */
        daemon_retval_send(0);

        daemon_log(LOG_INFO, "Sucessfully started");

        while (!quit) {
        	struct signalfd_siginfo fdsi;

			ret = read(fd, &fdsi, sizeof(fdsi));
			if (ret != sizeof(fdsi)) {
				daemon_log(LOG_ERR, "Read siginfo failed.");
                break;
            }

            /* Dispatch signal */
            switch (fdsi.ssi_signo) {

            case SIGINT:
            case SIGQUIT:
                daemon_log(LOG_WARNING, "Got SIGINT or SIGQUIT");
                quit = 1;
                break;

//          case SIGHUP:
//              daemon_log(LOG_INFO, "Got a HUP");
//              daemon_exec("/", NULL, "/bin/ls", "ls", (char*) NULL);
//              break;

			case SIGVPABORT:
				pthread_mutex_lock(&vprompt.lock);
				vprompt.abort = 1;
				vprompt.abort_pid = fdsi.ssi_pid;
				vprompt.abort_stamp = fdsi.ssi_int;
				//lhf
				struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
				if (msg->mode == VPROMPT_MODE_TTS || msg->mode == VPROMPT_MODE_MUSIC ){
					if (vprompt.pipeline) {
						pthread_mutex_unlock(&vprompt.lock);
						g_main_loop_quit (vprompt.loop);
						break;
					}
					pthread_mutex_unlock(&vprompt.lock);
				} else {// VPROMPT_MODE_AUDIO
					if(0 != vprompt.audio_pid){
						pthread_mutex_unlock(&vprompt.lock);
						kill(vprompt.audio_pid, SIGTERM);
						break;
					}
					pthread_mutex_unlock(&vprompt.lock);
				}//lhf end
				break;

			default:
				daemon_log(LOG_INFO, "Got a unhandled %d", fdsi.ssi_signo);
				break;
            }
        }

        /* Do a cleanup */
        VoicePromptUninit();

finish1:
		close(fd);
finish:
        daemon_log(LOG_INFO, "Exiting...");
        daemon_pid_file_remove();
        return 0;
    }
}
