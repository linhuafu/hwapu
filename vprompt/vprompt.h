#ifndef __VOICE_PROMPT_INTERNAL_H__
#define __VOICE_PROMPT_INTERNAL_H__

#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include <ivTTS.h>
#include "nxutils.h"
#include "libvprompt.h"

#define VPROMPT_MQ_PATH "/vprompt"
#define VPROMPT_SERVER	"vpromptd"

#define VPROMPT_MAX_MSGS	5
#define VPROMPT_MAX_MSGSIZE 2048

#define SIGVPABORT 34

struct vprompt_msg {
	unsigned int stamp;

	int mode;
#define VPROMPT_MODE_TTS	0
#define VPROMPT_MODE_MUSIC	1
#define VPROMPT_MODE_AUDIO	2	//add by lhf

	struct voice_prompt_cfg cfg;

	pid_t client_pid;
	int msg_id;
	enum plextalk_output_select output;

	union {
		struct {
			ivUInt32 codepage;
			ivUInt32 lang;
			ivUInt32 role;
			ivUInt8 read_digit;
			ivUInt8 chinese_number_1;
			size_t len;
			char text[0];
		} tts;

		struct {
			unsigned long start;
			unsigned long length;
			char path[0];
		} music;
	};
};

static inline unsigned int voice_prompt_get_stamp()
{
	struct timeval stamp;

	gettimeofday(&stamp, NULL);

	return (unsigned int)(stamp.tv_sec * 10000) + stamp.tv_usec / 100;
}

#endif	/* __VOICE_PROMPT_INTERNAL_H__ */
