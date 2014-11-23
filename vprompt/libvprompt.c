#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>

#include "vprompt.h"
#include "plextalk-constant.h"
#include "cld.h"
#include "plextalk-i18n.h"

static char buffer[VPROMPT_MAX_MSGSIZE];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static mqd_t mqd = (mqd_t)-1;
static int server_pid;
static int msg_id;

#define MAX_TTS_TEXT_LEN	(sizeof(buffer) - offsetof(struct vprompt_msg, tts.text))
#define MAX_MUSIC_PATH_LEN	(sizeof(buffer) - offsetof(struct vprompt_msg, music.path))

int voice_prompt_init(void)
{
	FILE* fp;

	mqd = mq_open(VPROMPT_MQ_PATH, O_WRONLY);
	if (mqd == (mqd_t)-1) {
		perror("Message queue open failed");
		return -1;
	}

	fp = popen("pidof "VPROMPT_SERVER, "r");
	fscanf(fp, "%d", &server_pid);
	fclose(fp);

	return 0;
}

void voice_prompt_uninit(void)
{
	if (mqd != (mqd_t)-1) {
		mq_close(mqd);
		mqd = (mqd_t)-1;
	}
}

static inline int voice_prompt_chk_cfg(struct voice_prompt_cfg *cfg)
{
#if 1
	if (cfg->volume < PLEXTALK_SOUND_VOLUME_MIN || cfg->volume > PLEXTALK_SOUND_VOLUME_MAX)
		return -1;
	if (cfg->speed < PLEXTALK_SOUND_SPEED_MIN || cfg->speed > PLEXTALK_SOUND_SPEED_MAX)
		return -1;
#endif
	return 0;
}

int voice_prompt_string_ex(int signal_at_end, enum plextalk_output_select output,
                           struct voice_prompt_cfg *cfg,
						   ivUInt32 codepage, ivUInt32 lang, ivUInt32 role,
						   ivUInt8 read_digit, ivUInt8 chinese_number_1,
						   const char* text, size_t size)
{
	struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
//	struct timespec timedout;
//	struct timeval curtime;
	int ret;

	if (mqd == (mqd_t)-1) {
		fprintf(stderr, "Voice prompt is not initialized!\n");
		return -1;
	}

	if (size > MAX_TTS_TEXT_LEN) {
		fprintf(stderr, "Text is too long!\n");
		return -1;
	}

	if (voice_prompt_chk_cfg(cfg)) {
		fprintf(stderr, "Voice prompt's config out of range!\n");
		return -1;
	}

	pthread_mutex_lock(&mutex);

	msg->stamp = voice_prompt_get_stamp();
	msg->mode = VPROMPT_MODE_TTS;
	memcpy(&msg->cfg, cfg, sizeof(*cfg));
	if (signal_at_end)
		msg->client_pid = getpid();
	else
		msg->client_pid = -1;
	msg->msg_id = msg_id;
	msg->output = output;

	msg->tts.codepage = codepage;
	msg->tts.lang = lang;
	msg->tts.role = role;
	msg->tts.read_digit = read_digit;
	msg->tts.chinese_number_1 = chinese_number_1;

	msg->tts.len = size;
	memcpy(msg->tts.text, text, size);

//	gettimeofdate(&curtime, NULL);
//	timedout.tv_sec = curtime.tv_sec + 1;
//	timedout.tv_nsec = curtime.tv_usec * 1000;
//	ret = mq_timedsend(mqd, buffer, offsetof(struct vprompt_msg, tts.text) + size, 0, &timedout);
	ret = mq_send(mqd, buffer, offsetof(struct vprompt_msg, tts.text) + size, 0);
	if (ret == 0) {
		ret = msg_id;
		msg_id++;
		if (msg_id < 0)
			msg_id = 0;
	}

	pthread_mutex_unlock(&mutex);

	return ret;
}

int voice_prompt_printf_ex(int signal_at_end, enum plextalk_output_select output,
                           struct voice_prompt_cfg *cfg,
						   ivUInt32 codepage, ivUInt32 lang, ivUInt32 role,
						   ivUInt8 read_digit, ivUInt8 chinese_number_1,
						   const char* text, ...)
{
	struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
	va_list args;
//	struct timespec timedout;
//	struct timeval curtime;
	int ret;

	if (mqd == (mqd_t)-1) {
		fprintf(stderr, "Voice prompt is not initialized!\n");
		return -1;
	}

	if (voice_prompt_chk_cfg(cfg)) {
		fprintf(stderr, "Voice prompt's config out of range!\n");
		return -1;
	}

	va_start(args, text);
	pthread_mutex_lock(&mutex);

	msg->stamp = voice_prompt_get_stamp();
	msg->mode = VPROMPT_MODE_TTS;
	memcpy(&msg->cfg, cfg, sizeof(*cfg));
	if (signal_at_end)
		msg->client_pid = getpid();
	else
		msg->client_pid = -1;
	msg->msg_id = msg_id;
	msg->output = output;

	msg->tts.codepage = codepage;
	msg->tts.lang = lang;
	msg->tts.role = role;
	msg->tts.read_digit = read_digit;
	msg->tts.chinese_number_1 = chinese_number_1;

	msg->tts.len = vsnprintf(msg->tts.text, MAX_TTS_TEXT_LEN, text, args);
	if (msg->tts.len <= MAX_TTS_TEXT_LEN) {
//		gettimeofdate(&curtime, NULL);
//		timedout.tv_sec = curtime.tv_sec + 1;
//		timedout.tv_nsec = curtime.tv_usec * 1000;
//		ret = mq_timedsend(mqd, buffer, offsetof(struct vprompt_msg, tts.text) + msg->tts.len, 0, &timedout);
		ret = mq_send(mqd, buffer, offsetof(struct vprompt_msg, tts.text) + msg->tts.len, 0);
		if (ret == 0) {
			ret = msg_id;
			msg_id++;
			if (msg_id < 0)
				msg_id = 0;
		}
	} else {
		fprintf(stderr, "Text is too long!\n");
		ret = -1;
	}

	pthread_mutex_unlock(&mutex);
	va_end(args);

	return ret;
}

int voice_prompt_music_ex(int signal_at_end, enum plextalk_output_select output,
                          struct voice_prompt_cfg *cfg,
						  const char* path, unsigned long start, unsigned long length)
{
	struct vprompt_msg *msg = (struct vprompt_msg*)buffer;
	int len;
//	struct timespec timedout;
//	struct timeval curtime;
	int ret;

	if (mqd == (mqd_t)-1) {
		fprintf(stderr, "Voice prompt is not initialized!\n");
		return -1;
	}

	len = strlen(path) + 1;
	if (len > MAX_MUSIC_PATH_LEN) {
		fprintf(stderr, "File's path is too long!\n");
		return -1;
	}

	if (voice_prompt_chk_cfg(cfg)) {
		fprintf(stderr, "Voice prompt's config out of range!\n");
		return -1;
	}

	pthread_mutex_lock(&mutex);

	msg->stamp = voice_prompt_get_stamp();
	// lhf start
	if( (0 == start) && ((unsigned long)-1 == length)){
		msg->mode = VPROMPT_MODE_AUDIO;
	}else{
		msg->mode = VPROMPT_MODE_MUSIC;
	}//lhf end
	memcpy(&msg->cfg, cfg, sizeof(*cfg));
	if (signal_at_end)
		msg->client_pid = getpid();
	else
		msg->client_pid = -1;
	msg->msg_id = msg_id;
	msg->output = output;

	msg->music.start = start;
	msg->music.length = length;
	memcpy(msg->music.path, path, len);

//	gettimeofdate(&curtime, NULL);
//	timedout.tv_sec = curtime.tv_sec + 1;
//	timedout.tv_nsec = curtime.tv_usec * 1000;
//	ret = mq_timedsend(mqd, buffer, offsetof(struct vprompt_msg, music.path) + len, 0, &timedout);
	ret = mq_send(mqd, buffer, offsetof(struct vprompt_msg, music.path) + len, 0);
	if (ret == 0) {
		ret = msg_id;
		msg_id++;
		if (msg_id < 0)
			msg_id = 0;
	}

	pthread_mutex_unlock(&mutex);

	return ret;
}

static int isEnglishString(const char* buffer, int buffer_length)
{
	int i, ret = 1;
	for(i=0; i<buffer_length; i++){
		if( (buffer[i]&0x80) != 0){
			ret = 0;
			break;
		}
	}
	return ret;
}

int VoicePromptDetectLanguage(const char* buffer, int buffer_length)
{
	if( isEnglishString(buffer, buffer_length) ){//all english string
		printf("-->all english char\n");
		return ivTTS_LANGUAGE_ENGLISH;
	}
	int lang = CldDetectLanguage(CHINESE, UTF8, buffer, buffer_length);
	printf("-->lang:%d\n",lang);
	switch (lang) {
	case ENGLISH:
		return ivTTS_LANGUAGE_ENGLISH;
	case CHINESE:
	case CHINESE_T:
		return ivTTS_LANGUAGE_CHINESE;
	case HINDI:
		return ivTTS_LANGUAGE_HINDI;
	}

	/* Oh, my god! I don't support the language now */
	return tts_lang;
}

int voice_prompt_abort(void)
{
	union sigval value;

	value.sival_int = voice_prompt_get_stamp();

	return sigqueue(server_pid, SIGVPABORT, value);
}

int voice_prompt_handle_fd(void)
{
	int fd;
	sigset_t mask;
	int ret;

	sigemptyset(&mask);
	sigaddset(&mask, SIGVPEND);

	/* block signals so that they aren't handled according
	 * to their default dispositions */
	ret = pthread_sigmask(SIG_BLOCK, &mask, NULL);
	if (ret != 0) {
		fprintf(stderr, "Could not block signals (%d).", ret);
		return -1;
	}

	return signalfd(-1, &mask, 0);
}
