#ifndef __RADIO_REC_EXTERN_H__
#define __RADIO_REC_EXTERN_H__


enum store_type{

	REC_STORE_SD_CARD = 0,
	REC_STORE_INTER_MEM,
};


/*it will return 0 if storage is not enough*/
unsigned long long radio_rec_get_storage(enum store_type type);

int radio_rec_get_remaintime(enum store_type type);

int radio_rec_get_time(char* filename);

char* radio_rec_get_filename(void);

char* radio_rec_time_to_string(int time);

char* get_store_path (const char* filename, int store_type);

void check_record_file(char* filepath);

int check_sd_exist (void);

unsigned long long radio_rec_get_freespace(enum store_type type);


#endif
