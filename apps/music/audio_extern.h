#ifndef __AUDIO_EXTERN_H__
#define __AUDIO_EXTERN_H__
#include <time.h>
#include <dirent.h>
#include "audio_type.h"


int folder_contain_music (const char* fpath);

void time_to_string (unsigned long time, char* buf, size_t length);

char* parse_album_name (char* dir_path);

char* parse_father_dir (char* dir_path);


int check_vaild_path (const char* path);

/*it will return 0 if this is not a mp3 or wav file*/
int check_audio_file (const char* path);

/*it will return 1 if this is the last track of this album*/

int check_ret_file(const char* path, int type);

int r_check_ret_file (const char* path, int type, char* buf, int size);

int headphone_on (void);

//int keylock_locked (void);



#endif
