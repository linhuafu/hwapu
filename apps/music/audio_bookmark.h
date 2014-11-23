#ifndef _AUDIO_BOOKMARK_H__
#define _AUDIO_BOOKMARK_H__


typedef struct {

	int album_bmk_no;
	char* dir_path;
	const char* audio_path;
	int play_time;

}album_bmk_info;


typedef struct {

	int track_bmk_no;
	const char* audio_path;
	int play_time;
	
}track_bmk_info;


int set_album_bmk(album_bmk_info* bmk);
int get_album_bmk(album_bmk_info* bmk);
int set_track_bmk(track_bmk_info* bmk);
int get_track_bmk(track_bmk_info* bmk);

int get_track_bmk_maxnum(const char* track_path);
int get_album_bmk_maxnum(const char* album_path);

int del_album_bmk(const char* album_path, int bmk_no);
int del_track_bmk(const char* track_path, int bmk_no);

int del_all_album_bmk(const char* album_path);
int del_all_track_bmk(const char* track_path);

#endif
