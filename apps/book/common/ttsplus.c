#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "ttsplus.h"
#include "cld.h"
#include "plextalk-i18n.h"
#include "plextalk-config.h"
#include "Plextalk-constant.h"
#include "typedef.h"
#include "dbmalloc.h"

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

struct TtsObj{
	/*TTS engine*/
	struct tts_instance aisound5;
	struct tts_instance aisoundMtts;
	struct tts_instance *tts;

	ivUInt32 role;
	ivUInt32 lang;
	ivCPointer buff;
	ivSize     size;
	ivSize	   codepage;

	/* Audio Output */
	GstElement *pipeline;
	GstAppSrc *appsrc;
	GstElement *sink;
	GMainLoop  *loop;
	GstElement *pEqualizer;
	GstElement *pPitch;
	GstElement *pAmplify;
	GstElement *speed;

	/* Pthread ID and Lock/cond */
	pthread_t apid;
	pthread_t tpid;

	sem_t sem_tts;
	sem_t tts_trigger;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	
	/* Thread status */
	volatile int playing;
	volatile int abort;
	volatile int tts_wait;
	int		thread_live;
	int		tts_synth;
	
	int tts_speed;
	int tts_band;
};


static void 
auto_detect_lang(ivSize codepage, const char* buff, int size, ivUInt32* role, ivUInt32* lang)
{	
	int encoding_hint;
	DBGMSG("codepage:%d\n",codepage);
	if(ivTTS_CODEPAGE_UTF8 == codepage){
		encoding_hint = UTF8;
	}else if(ivTTS_CODEPAGE_UTF16LE == codepage){
		encoding_hint = UTF16LE;
	}else if(ivTTS_CODEPAGE_GBK == codepage){
		encoding_hint = GBK;
	}else {
		DBGMSG("unknow\n");
		encoding_hint = UNKNOWN_ENCODING;
	}
	
	enum Language ret_lang =  CldDetectLanguage(CHINESE, encoding_hint, buff, size);

	switch (ret_lang) {
		case ENGLISH:
			*lang = ivTTS_LANGUAGE_ENGLISH;
			break;
		
		case HINDI:
			*lang = ivTTS_LANGUAGE_HINDI;
			break;

		case CHINESE:
		case CHINESE_T:
			*lang = ivTTS_LANGUAGE_CHINESE;
			break;

		default:
			//*mode = NORMAL_TTS;
			*lang = tts_lang;
			break;
	}

	*role = plextalk_get_tts_role(*lang);

}

static int quit = 0;

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

/* progress callback */
static ivTTSErrID ivCall ProgressCB(
		ivPointer       pParameter,		/* [in] user callback parameter */
		ivUInt32		iProcPos,		/* [in] current processing position */
		ivUInt32		nProcLen )		/* [in] current processing length */
{
	return ivTTS_ERR_OK;
}


static int VoicePromptTTSInstanceInit(struct TtsObj *vp, struct tts_instance *tts)
{
	ivTTSErrID ivReturn;
	FILE* fp;

	tts->pHeap = (ivPByte) app_malloc(ivTTS_HEAP_SIZE);
	if (tts->pHeap == NULL) {
		DBGMSG("Out of memory!\n");
		goto err1;
	}
	memset(tts->pHeap, 0, ivTTS_HEAP_SIZE);

	fp = fopen(tts->resource, "rb");
	if (fp == NULL) {
		DBGMSG("Open %s failed (%s)", tts->resource, strerror(errno));
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
		DBGMSG("ivTTS_Create failed (%d)!\n", ivReturn);
		goto err3;
	}

	tts->SetParam(tts->hTTS, ivTTS_PARAM_OUTPUT_CALLBACK, (ivUInt32) AudioOutputCB);
	tts->SetParam(tts->hTTS, ivTTS_PARAM_PROGRESS_CALLBACK, (ivUInt32)ProgressCB);
	return 0;

err3:
	fclose(fp);
err2:
	app_free(tts->pHeap);
err1:
	return -1;
}

static void VoicePromptTTSInstanceUninit(struct tts_instance *tts)
{
	DBGMSG("ivtts:%d\n", (int)tts->hTTS);
	if(0 != tts->hTTS){
		tts->Destroy(tts->hTTS);
		tts->hTTS = 0;
	}

	if (tts->tResPackDesc.pCacheBlockIndex){
		app_free(tts->tResPackDesc.pCacheBlockIndex);
		tts->tResPackDesc.pCacheBlockIndex = 0;
	}
	if (tts->tResPackDesc.pCacheBuffer){
		app_free(tts->tResPackDesc.pCacheBuffer);
		tts->tResPackDesc.pCacheBuffer = 0;
	}

	if(tts->tResPackDesc.pCBParam){
		fclose(tts->tResPackDesc.pCBParam);
		tts->tResPackDesc.pCBParam = 0;
	}

	if(tts->pHeap){
		app_free(tts->pHeap);
		tts->pHeap = 0;
	}
}

static int VoicePromptTTSInit(struct TtsObj *vp)
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
		DBGMSG("aisound5 init failed!\n");
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
		DBGMSG("aisoundMtts init failed!\n");
		return ret;
	}
	vp->tts = &vp->aisound5;
	return 0;
}

static void VoicePromptTTSUninit(struct TtsObj *vp)
{
	VoicePromptTTSInstanceUninit(&vp->aisound5);
	VoicePromptTTSInstanceUninit(&vp->aisoundMtts);
}


static ivTTSErrID ivCall AudioOutputCB(ivPointer		pParameter,	/* [in] user callback parameter */
								  ivUInt16		nCode,		/* [in] output data code */
								  ivCPointer	pcData,		/* [in] output data buffer */
								  ivSize		nSize )		/* [in] output data size */
{
	struct TtsObj *vp = (struct TtsObj *) pParameter;
	GstBuffer *gbuffer;
	GstFlowReturn ret;

	if (vp->abort)
		return ivTTS_ERR_FAILED;

	gbuffer = gst_buffer_try_new_and_alloc(nSize);
	if (gbuffer == NULL) {
		DBGMSG("AudioOutputCB alloc buffer failed");
		return ivTTS_ERR_EXIT;
	}

	memcpy(GST_BUFFER_DATA(gbuffer), pcData, nSize);

	ret = gst_app_src_push_buffer(vp->appsrc, gbuffer);
	if (ret !=  GST_FLOW_OK) {
		gst_buffer_unref(gbuffer);
		DBGMSG( "AudioOutputCB push failed: %d\n", ret);
		return ivTTS_ERR_EXIT;
	}

	return ivTTS_ERR_OK;
}

#if 0
static int WriteAudioData(struct TtsObj *thiz, const char *buffer, int nSize)
{
	GstBuffer *gbuffer;
	guint8 *ptr;
	GstFlowReturn ret;

	ptr = g_malloc(nSize);
	g_assert(ptr);
	
	memcpy(ptr, buffer, nSize);
	
	gbuffer = gst_buffer_new();
	
	GST_BUFFER_MALLOCDATA(gbuffer) = ptr;
	GST_BUFFER_SIZE(gbuffer) = nSize;
	GST_BUFFER_DATA(gbuffer) = GST_BUFFER_MALLOCDATA(gbuffer);

	ret = gst_app_src_push_buffer(thiz->appsrc, gbuffer);
	if (ret !=  GST_FLOW_OK) {
		DBGMSG("push data error\n");
		return 0;
	}

	return nSize;
}

static ivTTSErrID ivCall AudioOutputCB(ivPointer		pParameter,	/* [in] user callback parameter */
								  ivUInt16		nCode,		/* [in] output data code */
								  ivCPointer	pcData,		/* [in] output data buffer */
								  ivSize		nSize )		/* [in] output data size */
{
	struct TtsObj *thiz = (struct TtsObj *)pParameter;
	int num;
	int ret;

	if(thiz->abort){
		return ivTTS_ERR_OK;
	}

	num = WriteAudioData(thiz, pcData, nSize);
	if (num != nSize){
		DBGMSG("write data %d (must be %d) \n", num, nSize);
		ret = ivTTS_ERR_FAILED;
	} else {
		ret = ivTTS_ERR_OK;
	}

	return ret;
}

#endif

static gboolean
tts_obj_bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	struct TtsObj *thiz = (struct TtsObj *)data;

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_EOS:
		{
			DBGMSG ("stream end\n");
			pthread_mutex_lock(&thiz->lock);
			gst_element_set_state ((GstElement*)thiz->pipeline, GST_STATE_NULL);
			thiz->playing = 0;
			pthread_cond_signal(&thiz->cond);
			pthread_mutex_unlock(&thiz->lock);
			DBGMSG ("ret\n");
		}
		break;
	case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;

			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);
			DBGMSG ("Error: %s\n", err->message);
			g_error_free (err);
			g_main_loop_quit (thiz->loop);
			break;
		}
    default:
      break;
  }

	return TRUE;
}


/*thread output*/
static void* tts_obj_audio_thread (void *arg)
{
	GstBus *bus;
	struct TtsObj *thiz = (struct TtsObj *)arg;

	gst_init(NULL, NULL);

	thiz->thread_live = 1;	
	thiz->loop = g_main_loop_new (NULL, FALSE);
	thiz->pipeline = gst_pipeline_new ("audio-player");
	thiz->appsrc = (GstAppSrc*)gst_element_factory_make ("appsrc", "myappsrc");
	thiz->sink = gst_element_factory_make ("alsasink", "alsa-output");

	/*TTS sample rate=16000,   16bit per sample*/
	GstCaps *caps;
	caps = gst_caps_new_simple ("audio/x-raw-int",
		"width", G_TYPE_INT, 16,
		"depth", G_TYPE_INT, 16,
		"endianness", G_TYPE_INT, G_BYTE_ORDER,
		"rate",G_TYPE_INT, 16000,
		"signed", G_TYPE_BOOLEAN, TRUE,
		"channels",G_TYPE_INT, 1,
		NULL);
	gst_app_src_set_caps(GST_APP_SRC(thiz->appsrc), caps);
	g_object_set(G_OBJECT(thiz->appsrc), "block", TRUE, NULL);

//	GstAppStreamType stype = gst_app_src_get_stream_type(thiz->appsrc);

	bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
	gst_bus_add_watch (bus, tts_obj_bus_call, thiz);
	gst_object_unref (bus);

	/*equalizer-3bands, equalizer-10bands*/
	thiz->pEqualizer = gst_element_factory_make("equalizer-3bands", "pEqualizer");
	thiz->pPitch = gst_element_factory_make ("pitch", "pPitch");

	g_object_set(thiz->pPitch, "tempo", plextalk_sound_speed[thiz->tts_speed], NULL);
	g_object_set(thiz->pEqualizer,
			"band0",	plextalk_sound_equalizer[thiz->tts_band][0],
			"band1",	plextalk_sound_equalizer[thiz->tts_band][1],
			"band2",	plextalk_sound_equalizer[thiz->tts_band][2],
			NULL);

	gst_bin_add_many (GST_BIN (thiz->pipeline), (GstElement *)thiz->appsrc, thiz->pPitch, 
			thiz->pEqualizer, thiz->sink, NULL);
	gst_element_link_many((GstElement *)thiz->appsrc,thiz->pPitch, thiz->pEqualizer, 
			thiz->sink, NULL);

	gst_element_set_state (thiz->pipeline, GST_STATE_PAUSED);
	
	sem_post(&thiz->sem_tts);
	g_main_loop_run (thiz->loop);

	/* Clean Up */
	gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (thiz->pipeline));
	g_main_loop_unref (thiz->loop);

	pthread_mutex_lock(&thiz->lock);
	thiz->playing = 0;
	thiz->thread_live = 0;
	pthread_mutex_unlock(&thiz->lock);
	DBGMSG("gst exit\n");
	pthread_exit(0);
}

static void* tts_thread(void *arg)
{
	ivTTSErrID ivReturn;
	struct TtsObj *thiz = (struct TtsObj *)arg;
	struct tts_instance *tts;
	
	pthread_mutex_lock(&thiz->lock);
	thiz->tts_synth = 1;
	pthread_mutex_unlock(&thiz->lock);

	while(1){
		pthread_mutex_lock(&thiz->lock);
		thiz->tts_wait = 1;
		pthread_mutex_unlock(&thiz->lock);
		pthread_cond_signal(&thiz->cond);
		
		sem_wait(&thiz->tts_trigger);
		
		pthread_mutex_lock(&thiz->lock);
		thiz->tts_wait = 0;
		pthread_mutex_unlock(&thiz->lock);
		pthread_cond_signal(&thiz->cond);
		
		pthread_mutex_lock(&thiz->lock);
		if(quit){
			pthread_mutex_unlock(&thiz->lock);
			break;
		}
		pthread_mutex_unlock(&thiz->lock);
		
		tts = thiz->tts;
		tts->SetParam(tts->hTTS, ivTTS_PARAM_INPUT_CODEPAGE, thiz->codepage);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_LANGUAGE, thiz->lang);
		tts->SetParam(tts->hTTS, ivTTS_PARAM_ROLE, thiz->role);
		
		if(!thiz->abort){
			//gst_element_set_state (thiz->pipeline, GST_STATE_READY);
			gst_element_set_state (thiz->pipeline, GST_STATE_PLAYING);
			
			printf("synth text\n");
			pthread_mutex_lock(&thiz->lock);
			thiz->playing = 1;
			pthread_mutex_unlock(&thiz->lock);
			pthread_cond_signal(&thiz->cond);
			
			ivReturn = tts->SynthText(tts->hTTS, (ivCPointer)thiz->buff, 
							thiz->size);
			printf("synth end\n");
			if(ivReturn != ivTTS_ERR_OK){
				printf("ivTTS_SynthText err\n");
			}
			gst_app_src_end_of_stream(thiz->appsrc);
		}
		//synth end
		//pthread_mutex_lock(&thiz->lock);
		//if(!thiz->abort){
		//	gst_app_src_end_of_stream(thiz->appsrc);
		//}
		//pthread_mutex_unlock(&thiz->lock);
		
	}
	pthread_mutex_lock(&thiz->lock);
	thiz->tts_synth = 0;
	pthread_mutex_unlock(&thiz->lock);
	DBGMSG("ttspth exit\n");
	pthread_exit(0);
}

void tts_obj_destroy(struct TtsObj *thiz)
{
//	struct TtsObj *thiz = ttsObj;
	if (thiz) {
		DBGMSG("in\n");
		pthread_mutex_lock(&thiz->lock);
		if(thiz->tts_synth){
			quit = 1;
			thiz->abort = 1;
			pthread_mutex_unlock(&thiz->lock);
			sem_post(&thiz->tts_trigger);
			pthread_join(thiz->tpid, NULL);
			thiz->tpid = 0;
		}else{
			pthread_mutex_unlock(&thiz->lock);
		}

		pthread_mutex_lock(&thiz->lock);
		if(thiz->thread_live){
			pthread_mutex_unlock(&thiz->lock);
			g_main_loop_quit (thiz->loop);
			pthread_join(thiz->apid, NULL);
			thiz->apid = 0;
		}else{
			pthread_mutex_unlock(&thiz->lock);
		}
		DBGMSG("dest\n");
		pthread_mutex_destroy(&thiz->lock);
		pthread_cond_destroy(&thiz->cond);
		sem_destroy(&thiz->sem_tts);
		sem_destroy(&thiz->tts_trigger);
		DBGMSG("tts un\n");
		VoicePromptTTSUninit(thiz);
		app_free(thiz);
		thiz = 0;
		DBGMSG("out\n");
	}
}

struct TtsObj *tts_obj_create(void)
{
	int ret;
	struct TtsObj *thiz;

	thiz = app_malloc(sizeof(struct TtsObj));
	if(!thiz){
		DBGMSG("malloc err\n");
		goto faile;
	}
	memset(thiz, 0, sizeof(struct TtsObj));
	if(VoicePromptTTSInit(thiz)<0){
		goto faile;
	}
	
	thiz->tts_speed = 0;
	thiz->tts_band = 0;
	
	sem_init(&thiz->sem_tts, 0, 0);
	sem_init(&thiz->tts_trigger, 0, 0);
	pthread_mutex_init(&thiz->lock, NULL);
	pthread_cond_init(&thiz->cond, NULL);
	
	quit = 0;
	DBGMSG("gst thread\n");
	ret = pthread_create(&thiz->apid, NULL, tts_obj_audio_thread, thiz);
	if(ret < 0){
		DBGMSG("thread create err\n");
		goto faile;
	}
	sem_wait(&thiz->sem_tts);
	DBGMSG("tts thread\n");
	ret = pthread_create(&thiz->tpid, NULL, tts_thread, thiz);
	if(ret < 0){
		DBGMSG("thread create err\n");
		goto faile;
	}
	DBGMSG("ret\n");
	return thiz;
faile:
	DBGMSG("fail\n");
	tts_obj_destroy(thiz);
	return NULL;
}


void tts_obj_stop(struct TtsObj *thiz)
{	
//	struct TtsObj *thiz = ttsObj;
	return_if_fail(NULL!=thiz);

	DBGMSG("stop:%d\n",thiz->playing);
	pthread_mutex_lock(&thiz->lock);
	if (thiz->playing) {
		thiz->abort = 1;
		pthread_mutex_unlock(&thiz->lock);

		//thiz->tts->Exit(thiz->tts->hTTS);
		DBGMSG("wait stop\n");
		pthread_mutex_lock(&thiz->lock);
		while(!thiz->tts_wait){
			pthread_cond_wait(&thiz->cond, &thiz->lock);
		}
		pthread_mutex_unlock(&thiz->lock);
		DBGMSG("11\n");
		pthread_mutex_lock(&thiz->lock);
		gst_element_set_state (thiz->pipeline, GST_STATE_NULL);
		thiz->playing = 0;
		pthread_mutex_unlock(&thiz->lock);
		DBGMSG("ret\n");
		return;
	}
	pthread_mutex_unlock(&thiz->lock);
}


void tts_obj_start (struct TtsObj *thiz,char *buff, ivSize size, ivSize codepage)
{
	int ret;
	ivUInt32 role, lang;
//	struct TtsObj *thiz = ttsObj;
	
	return_if_fail(NULL!=thiz);

	tts_obj_stop(thiz);
	DBGMSG("start in\n");

	auto_detect_lang(codepage, buff, size, &role, &lang);
	
	pthread_mutex_lock(&thiz->lock);
	thiz->buff = buff;
	thiz->size = size;
	thiz->codepage = codepage;
	thiz->role = role;
	thiz->lang = lang;
	thiz->abort = 0;
	thiz->tts = &thiz->aisound5;
	if(ivTTS_LANGUAGE_HINDI==lang){
		thiz->tts = &thiz->aisoundMtts;
	}
	pthread_mutex_unlock(&thiz->lock);
	
	DBGMSG("lang:%d role:%d\n",(int)lang, (int)role);

	sem_post(&thiz->tts_trigger);

	DBGMSG("wait play\n");
	pthread_mutex_lock(&thiz->lock);
	while(!thiz->playing){
		pthread_cond_wait(&thiz->cond, &thiz->lock);
	}
	pthread_mutex_unlock(&thiz->lock);
	DBGMSG("start ret\n");
}


int tts_obj_isruning(struct TtsObj *thiz)
{
	return_val_if_fail(NULL!=thiz, 0);
	return thiz->playing;
}

void tts_obj_set_band(struct TtsObj *thiz,int band)
{
//	struct TtsObj *thiz = ttsObj;
	return_if_fail(NULL!=thiz);
	
	thiz->tts_band = band;
	if((thiz->pEqualizer)){
		DBGMSG("set obj band\n");
		g_object_set(thiz->pEqualizer,
				"band0",	plextalk_sound_equalizer[band][0],
				"band1",	plextalk_sound_equalizer[band][1],
				"band2",	plextalk_sound_equalizer[band][2], 
				NULL);
	}
}

void tts_obj_set_speed(struct TtsObj *thiz,int speed)
{
//	struct TtsObj *thiz = ttsObj;
	return_if_fail(NULL!=thiz);
	
	thiz->tts_speed = speed;
	if (thiz->pPitch) {
		DBGMSG("set speed obj\n");
		g_object_set(thiz->pPitch, "tempo", plextalk_sound_speed[speed], NULL);
	}
}

