#ifndef __VOICE_PROMPT_H__
#define __VOICE_PROMPT_H__

#include <sys/signalfd.h>
#include <ivTTS.h>
#include "amixer.h"

#define VPROMPT_MAX_TEXT_LENGTH 2000

#define SIGVPEND 35

int voice_prompt_init(void);

void voice_prompt_uninit(void);

struct voice_prompt_cfg {
	int volume;
	int speed;
};

extern struct voice_prompt_cfg vprompt_cfg;

/*
 * voice_prompt_xxxx return value:
 *     -1:     error
 *     others: current msg ID
 */
int voice_prompt_string_ex(int signal_at_end, enum plextalk_output_select output,
						struct voice_prompt_cfg *cfg,
						ivUInt32 codepage, ivUInt32 lang, ivUInt32 role,
						ivUInt8 read_digit, ivUInt8 chinese_number_1,
						const char* text, size_t size);

#define voice_prompt_string_ex2(signal_at_end, output, cfg, codepage, lang, role, text, size) \
	voice_prompt_string_ex(signal_at_end, output, cfg, codepage, lang, role, \
						ivTTS_READDIGIT_AUTO, ivTTS_CHNUM1_READ_YI, text, size)

#define voice_prompt_string2_ex2(signal_at_end, output, cfg, codepage, lang, role, text) \
	voice_prompt_string_ex2(signal_at_end, output, cfg, codepage, lang, role, text, strlen(text))

#define voice_prompt_string(signal_at_end, cfg, codepage, lang, role, text, size) \
	voice_prompt_string_ex2(signal_at_end, PLEXTALK_OUTPUT_SELECT_DAC, cfg, codepage, lang, role, text, size)

#define voice_prompt_string2(signal_at_end, cfg, codepage, lang, role, text) \
	voice_prompt_string2_ex2(signal_at_end, PLEXTALK_OUTPUT_SELECT_DAC, cfg, codepage, lang, role, text)

int voice_prompt_printf_ex(int signal_at_end, enum plextalk_output_select output,
						struct voice_prompt_cfg *cfg,
						ivUInt32 codepage, ivUInt32 lang, ivUInt32 role,
						ivUInt8 read_digit, ivUInt8 chinese_number_1,
						const char* text, ...);

#define voice_prompt_printf(signal_at_end, cfg, codepage, lang, role, format, ...) \
	voice_prompt_printf_ex(signal_at_end, PLEXTALK_OUTPUT_SELECT_DAC, cfg, codepage, lang, role, \
						ivTTS_READDIGIT_AUTO, ivTTS_CHNUM1_READ_YI, format, __VA_ARGS__)

int voice_prompt_music_ex(int signal_at_end, enum plextalk_output_select output,
						struct voice_prompt_cfg *cfg, const char* path,
						unsigned long start, unsigned long length);

#define voice_prompt_music(signal_at_end, cfg, path) \
	voice_prompt_music_ex(signal_at_end, PLEXTALK_OUTPUT_SELECT_DAC, cfg, path, 0, -1)

#define voice_prompt_music2(signal_at_end, output, cfg, path) \
	voice_prompt_music_ex(signal_at_end, output, cfg, path, 0, -1)

int voice_prompt_abort(void);

int VoicePromptDetectLanguage(const char* buffer, int buffer_length);

/*
 * 1: read
 *     struct signalfd_siginfo fdsi;
 *     ssize_t ret = read(fd, &fdsi, sizeof(fdsi));
 *     if (ret != sizeof(fdsi))
 *        handle_error();
 *     
 * 2: close
 *     close(fd);
 */
int voice_prompt_handle_fd(void);

#endif	/* __VOICE_PROMPT_H__ */
