#ifndef __MIC_RECORDER_H__
#define __MIC_RECORDER_H__

enum {

	OUTPUT_BYPASS = 0,
	OUTPUT_DAC,
	OUTPUT_SIDETONE,

	INPUT_MIC1,
	INPUT_LINEIN,

	MIC1_BOOST,
	CAPTURE,
	AGC_ON,
	
	SPEAKER_ON,
	SPEAKER_OFF,

	HEADPHONE_ON,
	HEADPHONE_OFF,

	VOLUME_SET,
	ANALOG_PATH_IGNORE_SUSPEND,
	
	
};

void p_mixer_set (int choice, int val);

void init_monitor_state (void);

int mic_rec_prepare(const char* filename);
int mic_rec_recording(char *filepath,int needsetfile);
int mic_rec_pause(void);
int mic_rec_stop(void);
int mic_rec_get_time(void);

int mic_rec_monitor_on(void);
int mic_rec_monitor_off(void);
int mic_rec_switch_monitor(void);

/* get monitor state, 1 means monitor on
 *  return 0, monitor off
 */
int get_mic_rec_monitor_state(void);

#endif
