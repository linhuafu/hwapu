#ifndef __REC_EXTERN_H__
#define __REC_EXTERN_H__
#include "rec_bool.h"

enum store_type{
	REC_STORE_SD_CARD = 0,
	REC_STORE_INTER_MEM = 1,
};


/*it will return -1 if storage is not enough*/
unsigned long long rec_get_storage(enum store_type type);

// check for storage
bool check_store(enum store_type type);

char* rec_get_filename(void);

char* time_to_string(int time);

int mic_headphone_on(void);

char* get_store_path(const char* file_name, int store_type);

void check_record_file (const char* file_name);

int check_sd_exist (void);

int keylock_locked (void);

int rec_get_remaintime(enum store_type type);

unsigned long long rec_get_freespace(enum store_type type);


#endif
