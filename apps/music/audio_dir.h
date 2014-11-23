#ifndef __AUDIO_DIR_H__
#define __AUDIO_DIR_H__
#include "audio_type.h"


enum file_type{

	AUDIO_ALBUM = 0,
	AUDIO_TRACK,
	AUDIO_ALBUM_RECOVER,		//表示上电的恢复  album play

};


enum LOAD_MODE{

	NEXT_TRACK = 0,
	PRE_TRACK	  ,
	NEXT_ALBUM    ,
	PRE_ALBUM	  ,
	SHUFFLE_TRACK,				
};


enum SEEK_BMK_MODE{

	NEXT_BOOKMARK = 0,
	PRE_BOOKMARK,
};


int set_player_info(const char* path, enum file_type type, struct audio_player* player);

int load_file (enum LOAD_MODE mode, struct audio_player* player,int flag,int *pos);


#endif
