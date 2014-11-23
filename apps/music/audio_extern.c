#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <limits.h>
#include <ctype.h>
#define __USE_XOPEN_EXTENDED
#define __USE_GNU
#include <ftw.h>
#include "audio_extern.h"
#include "resource.h"
#include "audio_type.h"
#include "audio_play.h"
#include "file_cursor.h"
#include "file-helper.h"
#include "nxutils.h"


//#define OSD_DBG_MSG
#include "nc-err.h"

//#include "dmalloc.h"

extern int sys_set_music_backup_path(char *path);
extern struct audio_player player;

void
time_to_string (unsigned long  time, char* buf, size_t length)
{
	int hour;
	int mins;
	int secs;

	if (time < 0) {
		DBGMSG("tim < 0\n");
		DBGMSG("This may cause an error!!\n");
		return ;
	}
	
	secs = (unsigned long)time % 60;
	time /= 60;
	mins = (unsigned long)time % 60;
	time /= 60;
	hour = (unsigned long)time % 100;

	snprintf(buf, length, "%02d:%02d:%02d", hour, mins, secs);
		
}


/*handler for parse album name*/ 
char* 
parse_album_name (char* dir_path)
//这个好像有点问题????
//将dir_path + size 改为 +size -1
{
	static char album_name[256];

	memset(album_name, 0, 256);
	int size = strlen(dir_path);
	if (*(dir_path + size ) == '/') {
		*(dir_path + size ) = 0;
	}
	char* p = strrchr(dir_path, '/');
	int dir_size = p - dir_path + 1;
	int name_size = size - dir_size;
	strncpy(album_name, p + 1, name_size);

	return album_name;
}


/*it will return 0 if the path is not vaild*/
int 
check_vaild_path (const char* path)
{
	/* Maybe the path is NULL */
	
	DBGMSG("check_vaild_path  = %s\n", path);
	
	if (!path)
	{
		DBGMSG("check_vaild_path -111111111111111\n", path);
		return 0;

	}
	else if(!strstr(path,"/media/"))
	{
			DBGMSG("check_vaild_path -55555555555555555\n", path);
			return 0;	
	}

	DBGMSG("check_vaild_path -22222222222222222\n", path);
	//这里增加一个判断文件大小的,如果文件小于某个，返回非法文件
	
	struct stat64 info;

	if (StringStartWith(path, "/media/cdda")) {
		return 1;
	} 
	stat64(path, &info);

	DBGMSG("file size=%d\n",info.st_size);
	if(info.st_size<1000)
	{
		return 0;
	}
		
	
	if (S_ISREG(info.st_mode)) 
	{
		DBGMSG("check_vaild_path -333333333333333333333\n", path);
		return 1;
	} 
	else 
	{	DBGMSG("check_vaild_path -444444444444444444\n", path);
		return 0;
	}
}



/******************************************************
Fun:  检查音频文件的扩展名对不对 

*******************************************************/
int 
check_audio_file (const char* path)
{	
	char* p = strrchr(path, '.');		

	if(strstr(path,"/media/cdda/Track"))
	{
		return 1;	
	}
	else if (p == NULL) 
	{
		return 0;
	} 
	else if (!(strncmp(p, ".mp3", 4)) || !(strncmp(p, ".wav", 4))) 
	{
		return 1;
	} 
	else
	{
		return 0;	
	}
}


/*handler for album dir_path  find father dir*/
char* 
parse_father_dir (char* dir_path)
{
	DBGMSG("dir_path = %s\n", dir_path);

	static char father_dir[512];

	char buf[512];
	memset(buf, 0, 512);

	memset(father_dir, 0, 512);
	/*there are two '/' in album dir_path*/
	int size = strlen(dir_path);

	memcpy(buf, dir_path, size);
	if (*(buf + size - 1) == '/') 
	{
		*(buf + size - 1) = 0;
	}

	char* p = strrchr(buf, '/');
	int copy_size = p - buf;

	memcpy(father_dir, buf, copy_size);
	
	return father_dir;
}


void 
audio_back_up (struct audio_player* player  )
{
	char path[512]={0};
	char buf[1024]={0};
	char *pstr;
	char* f_path=player->start_place.path;

	DBGMSG("audio_back_up  =%s \n",f_path);

	strcpy(path,f_path);
	pstr=path+strlen(path)-1;


 	DBGMSG("Enter  audio_back_up fun!\n");
	sys_set_music_resume_path((char*)player->start_place.path);			//set start place

     sys_get_music_backupfloder_path(player->start_place.backupfloder);

		
	

 if(player->play_mode==PLAY_TRACK)
 {//单曲播放


 		DBGMSG(" PLAY_TRACK  backup =%s\n",path);	
 		sys_set_music_backup_path(path);
		
 }
  else
  {//专辑play 




	      DBGMSG(" PLAY_ALBUM backup =%s\n",player->start_place.backupfloder);	
	      sys_set_music_backup_path(player->start_place.backupfloder);

		
  }
 
	int b_len = strlen(BACKUP_PATH) + strlen("echo") + 32;
	int m_len = strlen(f_path) + b_len ;
	
	snprintf(buf, m_len, "%s%s%s%s", "echo ", f_path, " > ", BACKUP_PATH);
	system(buf);
	sys_set_global_backup(2);
}


/* 
 * Test if this folder contain music file(WAV, MP3)
 */
#define DEPTH_MAX_LEVEL	10
static int contain_audio_file;

static int inline 
get_file_depth (const char *fpath)
{
	int size = strlen(fpath);
	int count = 0;
	int i;

	for (i = 0; i < size; i++)
		if (fpath[i] == '/')
			count ++;
	
	return count;
}

static int
ftw_cb(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
	int absolute_depth = get_file_depth(fpath);

	if (absolute_depth > DEPTH_MAX_LEVEL) {
		DBGMSG("Max Depth level has reached!!, return FTW_SKIP_SIBLINGS!\n");
		return FTW_SKIP_SIBLINGS;
	}

	if ((tflag == FTW_D) && PlextalkIsDsiayBook(fpath)) {
		DBGMSG("Daisy folder , omit it!\n");
		return FTW_SKIP_SUBTREE;
	} else if ((tflag == FTW_F) && (PlextalkIsMusicFile(fpath))) {
		DBGMSG("Regular file, and music file!\n");
		contain_audio_file = 1;
		return FTW_STOP;
	} else {
		DBGMSG("Other's , do ftw continue!\n");
		return FTW_CONTINUE;
	}		
}

int folder_contain_music (const char* fpath)
{
	int ret;
	int flags = 0;
	contain_audio_file = 0;

	if (StringStartWith(fpath, "/media/cdda"))
		return 1;
	
	flags |= FTW_PHYS;
	flags |= FTW_ACTIONRETVAL;

	if (PlextalkIsDirectory(fpath)) {
		nftw(fpath, ftw_cb, 20, flags);
		return contain_audio_file;
	} else {
		DBGMSG("fpath is a regular file!\n");
		return 1;
	}
}
