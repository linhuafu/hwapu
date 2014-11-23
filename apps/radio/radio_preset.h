#ifndef __RADIO_PRESET_H__
#define __RADIO_PRESET_H__

#define MAX_PRE_NUM 16		

#define NEXT_STATION		0x01
#define PRE_STATION			0x02


enum preset_ret{

	PRESET_SUCCESS = 0,		
	PRESET_MAX_NUMS,		
	PRESET_ALREADY,				
};


int preset_init (void);
int preset_close (void);

enum preset_ret set_preset_station (double freq);
double get_preset_station (int orient);

int get_cur_station_no (void);
int get_total_station_no (void);

int del_cur_preset_station (void);
int del_all_preset_station (void);

void write_preset_tmppath(int bChanel);

void recreate_station_file(void);

#endif
