#ifndef __FILE_CURSOR_H__
#define __FILE_CURSOR_H__

struct file_cursor;
typedef struct file_cursor FileCursor;


enum { MODE_ONE_LEVEL = 0, MODE_MUX_LEVEL };

FileCursor* file_cursor_create (const char* folder, int mode);

enum {POS_BEGIN = 0, POS_CUR, POS_END};

const char* file_cursor_next_track (FileCursor* fc, int* pos_flag, int mode);
const char* file_cursor_prev_track (FileCursor* fc, int* pos_flag, int mode);

const char* file_cursor_next_album (FileCursor* fc);
const char* file_cursor_prev_album (FileCursor* fc);


int file_cursor_curt_count (FileCursor* fc,int mode);
int file_cursor_max_count(FileCursor* fc,int mode);

void file_cursor_destroy (FileCursor* fc);


#endif

