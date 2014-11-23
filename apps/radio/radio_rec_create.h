#ifndef __RADIO_REC_CREATE_H__
#define __RADIO_REC_CREATE_H__


enum recorder_stat {

	PREPARE = 0,
	RECORDING,
	PAUSE,
};


int radio_recorder_prepare (const char* f_path);

int radio_recorder_recording (char *filepath,int needsetfile);

int radio_recorder_pause (void);

/* return the time of recorder */
int radio_recorder_time (void);	

int radio_recorder_exit (void);

enum recorder_stat radio_recorder_stat (void);


#endif
