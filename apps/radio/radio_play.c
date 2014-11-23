#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <gst/gst.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "radio_play.h"
#include "radio_rec_extern.h"
#include "fmlib.h"
#include "radio_type.h"
#include "xml-helper.h"
#include "plextalk-config.h"
//#define OSD_DBG_MSG 1
#include "nc-err.h"
#include "amixer.h"

extern int radio_auto_scan_mode_active;
extern int radio_auto_scan_end_flag;
extern int radio_mute_state;


static struct tuner fm_tuner;

extern int radio_auto_scan_first_seek;

static double current_fm_mhz;

typedef struct {

	double fm_range_max;
	double fm_range_min;

	double fm_search_max;
	double fm_search_min;
	double fm_search_spacing;
} FM_RANGE_ARG;

static FM_RANGE_ARG FmRangeConfig; 


#define RAIOD_RANGE_TABLE_SIZE 				3
#define RADIO_RANGE_TABLE_DEFAULT_SELECT	0
#define RADIO_DEFAULT_SEARCH_SPACING		0.1

static const double const RadioSearchRangeTable[RAIOD_RANGE_TABLE_SIZE][2] = {
	{87.5, 108.0},
	{76.0, 90.0},
	{76,0, 108.0},
};

static const double const RadioRangeTable[RAIOD_RANGE_TABLE_SIZE][2] = {
	{87.0, 108.0},
	{76.0, 90.0},
	{76,0, 108.0},
};


#define EQUAL_BASE	0.001

static int
f_equal (double x, double y)
{
	return (fabs(x - y) <= EQUAL_BASE);
}


static int 
radio_range_check_and_fix (double *min, double *max)
{
	int i = 0;

	for (i; i < RAIOD_RANGE_TABLE_SIZE; i++) {
		if (f_equal(*min, RadioRangeTable[i][0]) && (f_equal(*max, RadioRangeTable[i][1]))) {
			DBGMSG("Radio range table matched!!!\n");
			return i;
		}
	}

	DBGMSG("Radio range table not matched, use default!\n");
	*min = RadioRangeTable[RADIO_RANGE_TABLE_DEFAULT_SELECT][0];
	*max = RadioRangeTable[RADIO_RANGE_TABLE_DEFAULT_SELECT][1];

	return 0;
}


void radio_set_region (void)
{
	double range_min, range_max;
	int search_index;

	CoolShmReadLock(g_config_lock);
	range_min = g_config->radio_low;
	range_max = g_config->radio_hi;
	CoolShmReadUnlock(g_config_lock);

	DBGMSG("Read from g_config range_min = %f\n", range_min);
	DBGMSG("Read from g_config range_max = %f\n", range_max);

	search_index = radio_range_check_and_fix(&range_min, &range_max);

	FmRangeConfig.fm_range_min = range_min;	
	FmRangeConfig.fm_range_max = range_max;
	FmRangeConfig.fm_search_min = RadioSearchRangeTable[search_index][0];
	FmRangeConfig.fm_search_max = RadioSearchRangeTable[search_index][1];
	FmRangeConfig.fm_search_spacing = RADIO_DEFAULT_SEARCH_SPACING;

	DBGMSG("FmRangeConfig.fm_range_min = %f\n", FmRangeConfig.fm_range_min);
	DBGMSG("FmRangeConfig.fm_range_max = %f\n", FmRangeConfig.fm_range_max);
	DBGMSG("FmRangeConfig.fm_search_min = %f\n", FmRangeConfig.fm_search_min);
	DBGMSG("FmRangeConfig.fm_search_max = %f\n", FmRangeConfig.fm_search_max);	
}


double 
radio_get_fmMax(void)
{
	return FmRangeConfig.fm_range_max;
}


double 
radio_get_fmMin(void)
{
	return FmRangeConfig.fm_range_min;
}


void 
radio_uninit (void)
{
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_OFF);
	tuner_close(&fm_tuner);
}


int 
radio_start (void)
{		
	const char* device = NULL;
	int index = 0;
	
	tuner_open (&fm_tuner, device, index);
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND, PLEXTALK_CTRL_ON);

	/* in case other app close this!!! */
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_FM_RADIO, PLEXTALK_CTRL_ON);
	
	return 0;
}


/*weather the radio is muted or not*/
int
radio_is_muted (void)
{
//	bool bmuted = false;
	
//	tuner_is_muted(&fm_tuner, &bmuted);

//	return bmuted;
	return radio_mute_state;
}


/*set  radio mute or unmute*/
int 
radio_set_mute (bool mute, int immediately)
{
	amixer_direct_sset_str(VOL_RAW, PLEXTALK_ANALOG_PATH_IGNORE_SUSPEND,mute ? "Off": "On");
//	tuner_set_mute(&fm_tuner, mute);	

	if (mute) {
		if (immediately)
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_DAC);
		radio_mute_state = 1;
	} else {
		if (immediately)
			amixer_direct_sset_str(VOL_RAW, PLEXTALK_OUTPUT_MUX, PLEXTALK_OUTPUT_BYPASS);
		radio_mute_state = 0;
	}

	return 0;
}


/*set radio frequency*/
int 
radio_set_freq (double mhz)
{
	long long int freq = mhz * 16000 + 0.5;
	bool override = false;

	current_fm_mhz = mhz;
	tuner_set_freq(&fm_tuner, freq, override);

	return 0;	
}


/* Get radio current frequency */
double 
radio_get_freq (void)
{
	int freq = tuner_get_freq(&fm_tuner);

	current_fm_mhz = ((double)freq / 16000.0);
	
	return current_fm_mhz;
}


/*radio manual scan, scan forward or backward via from orient*/
int 
radio_manual_scan (int orient)
{
	if (orient == SCAN_FORWARD) {
		current_fm_mhz += FmRangeConfig.fm_search_spacing;
		if (current_fm_mhz >  FmRangeConfig.fm_range_max)
			current_fm_mhz = FmRangeConfig.fm_range_min;
	} else {
		current_fm_mhz -= FmRangeConfig.fm_search_spacing;
		if (current_fm_mhz < FmRangeConfig.fm_range_min)
			current_fm_mhz = FmRangeConfig.fm_range_max;
	}
	radio_set_freq (current_fm_mhz);

	return 0;
}



static double before_seek_freq ;

static void fix_freq (void)
{
	double base = radio_get_freq() * 10;
	double new = (double)lround(base)/ (double)10;

	//for BUG: after seek end not in 87.0 frequency

	double adjust_freq = FmRangeConfig.fm_search_min + FmRangeConfig.fm_search_spacing * 2;

	if (f_equal(before_seek_freq, FmRangeConfig.fm_range_min) && f_equal(new, adjust_freq))
		new = FmRangeConfig.fm_search_min;
	
	radio_set_freq(new);
}


void radio_seek (void *arg)
{
	DBGMSG("Radio Seek function!\n");
		
	int dir, ret;
	double near;
	long long int search_low, search_high;
	time_t before_time, after_time, elapse_time;	
	double ref_frq = radio_get_freq();
	double before_freq, after_freq;
	struct seek_freq* seek = (struct seek_freq*)arg;

	radio_auto_scan_mode_active = 1;

	before_seek_freq = radio_get_freq();

	/* Handler for bug (87.0 allways seek to 87.5) */
	if (radio_get_freq() < FmRangeConfig.fm_search_min) {
		DBGMSG("Radio freq less than fmsearch min!\n");
//		radio_set_freq(FmRangeConfig.fm_search_min + FmRangeConfig.fm_search_spacing);
		radio_set_freq(FmRangeConfig.fm_search_min + FmRangeConfig.fm_search_spacing * 2);
	}
		
	dir = (seek->orient == SCAN_FORWARD) ? 1 : 0;	
	search_low = (long long int)(FmRangeConfig.fm_search_min * 16000);
	search_high = (long long int)(FmRangeConfig.fm_search_max * 16000);

	DBGMSG("Before in tuner hw freq seek!\n");
	//int spac = FmRangeConfig.fm_search_spacing * 1000;
	//tuner_hw_freq_seek(&fm_tuner, dir, 1, spac, search_low, search_high);

#if 0
	/* Get time and freq before do seek */
	before_time = time(NULL);
	before_freq = radio_get_freq();

	printf("before hw seek!!!\n");
#endif

	ret = tuner_hw_freq_seek(&fm_tuner, dir, 1, 0, search_low, search_high);
	
#if 0
	printf("after hw seeki!!!\n");
	
	/* Get time and freq after do seek */
	after_freq = radio_get_freq();
	after_time = time(NULL);

	elapse_time = after_time - before_time;

	if ((elapse_time <= 1) && f_equal(before_freq, after_freq)) {
		printf("Bad Radio Seek !!!\n");
		printf("Do seek repeatly!\n");
		ret = tuner_hw_freq_seek(&fm_tuner, dir, 1, 0, search_low, search_high);
	} else 
		printf("Good Radio Seek!\n");
#endif

	if (ret < 0) 
		DBGMSG("Error in tuner hw freq seek!\n");
	DBGMSG("After tuner hw freq seek!\n");

	fix_freq();	
	radio_auto_scan_end_flag = 1;
	radio_auto_scan_mode_active = 0;

	pthread_exit(0);
}

