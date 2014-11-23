#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
//#include "upgrader_tts.h"
#include "libvprompt.h"
#include<signal.h>

ivUInt32 tts_lang;
ivUInt32 tts_role;
static int tts_running = 0;

struct voice_prompt_cfg vprompt_cfg;

void upgrader_init_tts_prop()
{	
	tts_lang=ivTTS_LANGUAGE_ENGLISH;
	tts_role=ivTTS_ROLE_TERRY;
	/*vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed = g_config->setting.guide.speed;
//	vprompt_cfg.equalizer = g_config->setting.guide.equalizer;*/
	
	vprompt_cfg.volume = 10;
	vprompt_cfg.speed = 0;
//	vprompt_cfg.equalizer = 0;

}

void upgrader_tts_set_stopped (void)
{
	tts_running = 0;
}

int upgrader_tts_get_stopped(void)
{
	return !tts_running;
}

void upgrader_call(int  sig)
{
	upgrader_tts_set_stopped();
	printf("tts play over!\n");
}
void upgrader_tts_init ()
{	
	signal(SIGVPEND,upgrader_call);
	voice_prompt_init();
	voice_prompt_abort();
	upgrader_init_tts_prop();
}

void upgrader_tts_destroy (void)
{
	voice_prompt_abort();
	voice_prompt_uninit();
}

void upgrader_play_tts (char *data)
{

	voice_prompt_abort();
	tts_running=1;
	printf("start prompt!\n");
	voice_prompt_string(1, &vprompt_cfg, ivTTS_CODEPAGE_UTF8, tts_lang,
			tts_role, data, strlen(data)); 
	printf("over prompt!\n");
			
}

void upgrader_play_error_wav ()
{

	voice_prompt_abort();
	tts_running=1;
	printf("start prompt!\n");
	voice_prompt_music(1, &vprompt_cfg, "/opt/plextalk/data/sounds/audio_bu.wav"); 
	printf("over prompt!\n");
			
}
void Wait_tts_play_over()
{
	int i=0;
	while(tts_running==1)
	{
		sleep(1);
		i++;
		if(i==10)
		{
			tts_running=0;
		}
		printf("tts_runing %d\n",tts_running);
	}
}
/*
int tts_update(char *test1)
{
	char *test="Hello world";
	printf("%s\n",tts_text[Guide_VerUp_UpdaterFound]);
	upgrader_tts_init ();
	upgrader_play_tts(test1);
	Wait_tts_play_over();
	upgrader_play_tts(tts_text[Guide_VerUp_UpdaterFound]);
	Wait_tts_play_over();

	//sleep(3);
	printf("destory!\n");
	upgrader_tts_destroy();
}*/

