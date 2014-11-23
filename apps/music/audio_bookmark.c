#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "audio_bookmark.h"
#include "audio_type.h"
#include "application-ipc.h"
#include "audio_play.h"
//#include "audio_box.h"

#include "nxutils.h"

#define OSD_DBG_MSG
#include "nc-err.h"

//#include "dmalloc.h"

#define    MAX_BOOKMARK_NUM			1024

struct _album_bmk{
	int  no;
	int  time;
	char audio_path[256];	
};


struct _track_bmk{
	int no;
	int time;
};


static char album_bmk_name[512];
static char track_bmk_name[512];
 
static struct _album_bmk album_bookmark; 
static struct _track_bmk track_bookmark;


extern int sys_set_music_number_bookmark(int vol);
extern struct audio_player player;


static void
set_album_bmk_name (const char* dir_path)
{

	DBGMSG("set_album_bmk_name =%s \n",dir_path);	
	
	char* p = strrchr(dir_path, '/');
	int size = p - dir_path;
	char name[512];
	
	memset(name, 0, 512);
	strcpy(name, dir_path);
	//name[size] = '_';
	memset(album_bmk_name, 0, 512);
	sprintf(album_bmk_name, "%s%s", name, "f.bmk");

	DBGMSG("album_bmk_name =%s \n",album_bmk_name);	
}


static void
set_track_bmk_name (const char* track_path)
{

	DBGMSG("set_track_bmk_name =%s \n",track_path);
	char* p = strrchr(track_path, '.');
	int size = p - track_path;
	char name[512];
	
	memset(name, 0, 512);
	strcpy(name, track_path);
	name[size] = '_';
	memset(track_bmk_name, 0, 512);
	sprintf(track_bmk_name, "%s.%s", name, "bmk");
}


//the unexpected bug happens here
int 
set_album_bmk (album_bmk_info* bmk)
{

	int intret=0;
	int index_bookmark=0;
	 struct _album_bmk album_temp; 

	DBGMSG("Enter  set_album_bmk_111111111111111111\n");

	memset(&album_temp, 0, sizeof(album_temp));
	
	DBGMSG("Enter  set_album_bmk_22222222222222222 =%s \n",bmk->dir_path);
	

	
	
		
	strncpy(album_temp.audio_path, bmk->audio_path, 256);
	album_temp.no   = bmk->album_bmk_no;
	album_temp.time = bmk->play_time;
	
	DBGMSG("Enter  set_album_bmk_44444444444444444444 audio_path= %s \n",bmk->audio_path);

	if(StringStartWith(bmk->audio_path,"/media/sr")||StringStartWith(bmk->audio_path,"/media/cdda"))
	{
		DBGMSG("Enter  set_album_bmk___aaaaaaaaaaaaa\n");
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}

	else if(isMediaReadOnly(bmk->audio_path))
	{//写保护
		DBGMSG("Enter  set_album_bmk__bbbbbbbbbbbbbbbbb\n");
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}
	else if(index_bookmark=IsExist_BookMark(& player,audio_stat.current_play_time))
	{//此书签己存在
		DBGMSG("Enter  set_album_bmk__ccccccccccccccccccccc\n");
		NhelperBookMarkResult(APP_BOOKMARK_EXIST_ERR,index_bookmark);  
		return -1;
	}
	else if(IsMax_Album_BookMark(bmk->dir_path))
	{//己达到最大的书签值

		DBGMSG("Enter  set_album_bmk__ddddddddddddddddddddddddddd\n");
		NhelperBookMarkResult(APP_BOOKMARK_UPPER_LIMIT_ERR,0);  
		return -1;
	}
	else
	{
		DBGMSG("Enter  set_album_bmk_55555555555555555555=%s\n",album_bmk_name);

		set_album_bmk_name(bmk->dir_path);

		DBGMSG("Enter  set_album_bmk_333333333333333333=%s\n",album_bmk_name);
		
		int fd = open(album_bmk_name, O_RDWR|O_CREAT|O_APPEND, 444);
		if (fd < 0) 
		{
			DBGMSG("Error : open %s failure!\n", album_bmk_name);
			perror("failure");
			NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);  	
			return -1;
		}
		DBGMSG("Enter  set_album_bmk_666666666666666666666666\n");
		int res = write(fd, &album_temp, sizeof(album_temp));
		if (res < 0) 
		{
			DBGMSG("Error : write bookmark failure!\n");
			NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);  	
			 
			close(fd);
			return -1;
		} 
		DBGMSG("Enter  set_album_bmk_777777777777777777777777777\n");
		NhelperBookMarkResult(APP_BOOKMARK_SET_OK,0);  	
		close(fd);
		return 0;

	}
	
 
	
}


int
set_track_bmk (track_bmk_info* bmk)
{

	int index_bookmark=0;

	struct _track_bmk track_temp;

	DBGMSG("enter  set_track_bmk fun_1111111111111 ==%s \n",bmk->audio_path);
	
	set_track_bmk_name(bmk->audio_path);
	
	DBGMSG("enter  set_track_bmk fun_222222222222222222\n");
	
	memset(&track_temp, 0, sizeof(track_temp));
	track_temp.no   = bmk->track_bmk_no;
	track_temp.time = bmk->play_time;

//	printf("[track_bmk]: no = %d\n", track_temp.no);
//	printf("[track_bmk]: time = %d\n", track_temp.time);
	
	DBGMSG("Bookmark name = %s    audio_path=%s \n", track_bmk_name,bmk->audio_path);

	if(StringStartWith(bmk->audio_path,"/media/sr")||StringStartWith(bmk->audio_path,"/media/cdda"))
	{
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}
	else if(isMediaReadOnly(bmk->audio_path))
	{//写保护
	
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}
	else if(index_bookmark=IsExist_BookMark(& player,audio_stat.current_play_time))
	{//此书签己存在
		
		NhelperBookMarkResult(APP_BOOKMARK_EXIST_ERR,index_bookmark);  
		return -1;
	}
	else if(IsMax_Track_BookMark(bmk->audio_path))
	{//己达到最大的书签值
		
		NhelperBookMarkResult(APP_BOOKMARK_UPPER_LIMIT_ERR,0);  
		return -1;
	}

	else
	{
		set_track_bmk_name(bmk->audio_path);
		
		int fd = open(track_bmk_name, O_RDWR|O_CREAT|O_APPEND, 444);
		if (fd < 0)
		{
			DBGMSG("Error : open %s failure!\n", track_bmk_name);
			NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);  
			return -1;
		}

		DBGMSG("wite  bookmark : no=%d,time=%d\n",track_temp.no, track_temp.time);
		int res = write(fd, &track_temp, sizeof(track_temp));
		if (res < 0)
		{
			DBGMSG("Error : write bookmark failure!\n");
			NhelperBookMarkResult(APP_BOOKMARK_SET_ERR,0);  
			close(fd);
			return -1;
		} 
		DBGMSG("write ok!\n");
		NhelperBookMarkResult(APP_BOOKMARK_SET_OK,0);  	
		close(fd);
		return 0;
	}
	


}


int
get_album_bmk (album_bmk_info* bmk)
{
	
	DBGMSG("get_album_bmk  =%s\n",bmk->dir_path);
		
	set_album_bmk_name(bmk->dir_path);

	DBGMSG("get_album_bmk  after  =%s\n",album_bmk_name);
	
	int fd = open(album_bmk_name, O_RDONLY);
	if (fd < 0) {
		DBGMSG("Error : open %s failure!\n", album_bmk_name);
		return -1;
	}
	
	/*find the audio_path and time with no*/
	int seek_size = bmk->album_bmk_no * sizeof(struct _album_bmk);
	int res = lseek(fd, seek_size, SEEK_SET);
	if (res == -1) {
		DBGMSG("Error : lseek to %d album bookmark failure!\n", bmk->album_bmk_no);
		goto err;
	}
	/* read it into the album_bookmark*/
	memset(&album_bookmark, 0, sizeof(struct _album_bmk));
	res = read(fd, &album_bookmark, sizeof(struct _album_bmk));
	if (res != sizeof(struct _album_bmk)) {
		DBGMSG("Error : read %d album bookmark failure!\n", bmk->album_bmk_no);
		goto err;
	}

	bmk->audio_path = album_bookmark.audio_path;
	bmk->play_time  = album_bookmark.time;

	DBGMSG("D:after get audio_path = %s\n", bmk->audio_path);
	DBGMSG("D:after get play_time = %d\n", bmk->play_time);
	
	close(fd);
	return 0;
	
err:
	close(fd);
	return -1;

}


int
get_track_bmk (track_bmk_info * bmk)
{
	DBGMSG("[get track bmk]\n");

	set_track_bmk_name(bmk->audio_path);
	int fd = open(track_bmk_name, O_RDONLY);
	if (fd < 0) {
		DBGMSG("Error : open %s failure!\n", track_bmk_name);
		return -1;
	}
	int seek_size = bmk->track_bmk_no * sizeof(struct _track_bmk);
	int res = lseek(fd, seek_size, SEEK_SET);
	if (res == -1) {
		DBGMSG("Error : lseek to %d track bookmark failure!\n", 
			bmk->track_bmk_no);
		goto err;
	}

	memset(&track_bookmark, 0, sizeof(struct _track_bmk));
	res = read(fd, &track_bookmark, sizeof(struct _track_bmk));
	if (res != sizeof(struct _track_bmk)) {
		DBGMSG("Error : read %d track bookmark failure!\n", 
			bmk->track_bmk_no);
		goto err;
	}

	bmk->play_time = track_bookmark.time;
	close(fd);	

	
	DBGMSG("[get bmk]:track_bookmark.time = %d\n", track_bookmark.time);
	DBGMSG("[exit track bmk]\n");

	return 0;

err:
	close(fd);
	return -1;
}


int   IsMax_Track_BookMark(const char* track_path)
{
		int num=0;

		num=get_track_bmk_maxnum (track_path);
		if(num>=MAX_BOOKMARK_NUM)
		{

			return 1;
		}
		else
		{
			return 0;
		
		}
}






int   IsMax_Album_BookMark(const char* track_path)
{
		int num=0;

		num=get_album_bmk_maxnum (track_path);
		if(num>=MAX_BOOKMARK_NUM)
		{

			return 1;
		}
		else
		{
			return 0;
		
		}
}


int 
get_track_bmk_maxnum (const char* track_path)
{
	struct stat buf;
	set_track_bmk_name(track_path);
	if (stat(track_bmk_name, &buf) < 0) {
		return 0;
	}
	return (buf.st_size / sizeof(struct _track_bmk));
}


int 
get_album_bmk_maxnum (const char* album_path)
{
	struct stat buf;
	set_album_bmk_name(album_path);
	if (stat(album_bmk_name, &buf) < 0) {
		return 0;
	}
	return (buf.st_size / sizeof(struct _album_bmk));
}


static int 
del_content_from_file (int file_fd, int file_size, int offset, int del_size)
{
	void* buffer_1 = malloc(file_size - offset);
	if (!buffer_1)
		return -1;
	memset(buffer_1, 0, file_size - offset);

	void* buffer_2 = malloc(file_size - offset);
	if (!buffer_2)
		return -1;
	memset(buffer_2, 0, file_size - offset);

	int res = lseek(file_fd, offset, SEEK_SET);
	if (res == -1) {
		DBGMSG("Error : lseek to %d  failure!\n", offset);
		goto d_err;
	}

	res = read(file_fd, buffer_1, file_size - offset);
	if (res == -1) {
		DBGMSG("Error : read file to buffer_1 error!\n");
		goto d_err;
	}

	memcpy(buffer_2, buffer_1 + del_size, file_size - offset - del_size);

	res = lseek(file_fd, offset, SEEK_SET);

	res = write(file_fd, buffer_2, file_size - offset);
	if (res == -1){
		DBGMSG("write content to buffer_2 error!\n");
		goto d_err;
	}

	free(buffer_2);
	free(buffer_1);
	return 0;
	
d_err:
	free(buffer_2);
	free(buffer_1);
	return -1;
	
}


int 
del_album_bmk (const char* album_path, int bmk_no)
{	
	struct stat info;

	set_album_bmk_name(album_path);

	if(StringStartWith(album_path,"/media/sr")||StringStartWith(album_path,"/media/cdda"))
	{
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}
	else if(isMediaReadOnly(album_path))
	{//写保护
	
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}
	else if(IsMayDel_BookMark(& player,audio_stat.current_play_time)==0)
	{//当前书签非法
		
		NhelperBookMarkResult(APP_BOOKMARK_NOT_EXIST_ERR,0);  
		return -1;
	}
	else 
	{
	
		int fd = open(album_bmk_name, O_RDWR);
		if (fd < 0) 
		{
			DBGMSG("Error : open %s failure!\n", album_bmk_name);
			NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
			return -1;
		}
		/*find the audio_path and time with no*/
		if(bmk_no<0) 
		{
			DBGMSG("del_album_bmk  bmk_no <0!\n");
			bmk_no=0;
		}
		int seek_size = bmk_no * sizeof(struct _album_bmk);

		stat(album_bmk_name, &info);
		
		int res = del_content_from_file(fd, info.st_size,
					seek_size, sizeof(struct _album_bmk));
		if (res == -1) 
		{
			DBGMSG("del_content_from_file error!\n");
			NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
			close(fd);
			return -1;
		}

		close(fd);

		if ((info.st_size - sizeof(struct _album_bmk)) <= 0) 
		{
			DBGMSG("There is no bmk in this file, delete this file!\n");
			remove(album_bmk_name);
		} 
		else 
		{
			res = truncate(album_bmk_name, info.st_size - sizeof(struct _album_bmk));
			if (res == -1) 
			{
				NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);
				perror("truncate error!");
				return -1;
			}
		}

		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);  
		return 0;
	}

}


int 
del_track_bmk (const char* track_path, int bmk_no)
{
	struct stat info;
	
	set_track_bmk_name(track_path);





	if(StringStartWith(track_path,"/media/sr")||StringStartWith(track_path,"/media/cdda"))
	{
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}
	
	else if(isMediaReadOnly(track_path))
	{//写保护
	
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}
	else if(IsMayDel_BookMark(& player,audio_stat.current_play_time)==0)
	{//当前书签非法
		
		NhelperBookMarkResult(APP_BOOKMARK_NOT_EXIST_ERR,0);  
		return -1;
	}
	else 
	{
	
		int fd = open(track_bmk_name, O_RDWR);
		if (fd < 0) 
		{
			DBGMSG("Error : open %s failure!\n", track_bmk_name);
			NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);  
			return -1;
		}

		if(bmk_no<0) 
		{
			DBGMSG("del_track_bmk  bmk_no <0!\n");
			bmk_no=0;
		}
		
		int seek_size = bmk_no * sizeof(struct _track_bmk);
		
		stat(track_bmk_name, &info);
			
		int res = del_content_from_file(fd, info.st_size, seek_size, sizeof( struct _track_bmk));
					
		if (res == -1) 
		{
			DBGMSG("del content from file error!\n");
			NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);  
			close(fd);
			return -1;
		}
		
		close(fd);

		if ((info.st_size - sizeof(struct _track_bmk)) <= 0) 
		{
			DBGMSG("There is no bmk in this file, delete this file!\n");
			remove(track_bmk_name);
		} 
		else
		{
			res = truncate(track_bmk_name, info.st_size - sizeof(struct _track_bmk));
			if (res == -1) 
			{ 	NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);  
				perror("truncate error!");
				
				return -1;
			}
		}

		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);  
		return 0;
	}

	
}


//may be "system("rm -rf albumname")" will be better
int 
del_all_album_bmk (const char* album_path)
{
	set_album_bmk_name(album_path);
	
	if(StringStartWith(album_path,"/media/sr")||StringStartWith(album_path,"/media/cdda"))
	{
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}
	

	else if(isMediaReadOnly(album_path))
	{//写保护
	
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}	
	
	else if (remove(album_bmk_name) == -1)
		
	{
		DBGMSG("remove album_bmk error!\n");
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);  
		return -1;
	}
	else
	{	
		NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);  
		return 0;
	}
}


int 
del_all_track_bmk (const char* track_path)
{
	set_track_bmk_name(track_path);

	if(StringStartWith(track_path,"/media/sr")||StringStartWith(track_path,"/media/cdda"))
	{
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;
	}
	
	else if(isMediaReadOnly(track_path))
	{//写保护
	
		NhelperBookMarkResult(APP_BOOKMARK_MEDIA_LOCKED_ERR,0);     
		return -1;

	}	
	
	if (remove(track_bmk_name) == -1)
	{
		DBGMSG("remove track_bmk error!\n");
		NhelperBookMarkResult(APP_BOOKMARK_DEL_ERR,0);  
		return -1;
	}
	else
	{	NhelperBookMarkResult(APP_BOOKMARK_DEL_OK,0);  
		return 0;
	}
	
	
}



/****************************************
FUN:  判断此书签是否可del 

1:  表示可del
0:  表示不可del 
*****************************************/
int  IsMayDel_BookMark (struct audio_player* player,  int  urrent_play_time )
{

		int  save_bookmark_no=0;
		
		DBGMSG("Enter  IsMayDel_BookMark  fun =%d!\n",player->bmk_no.current_bmk_no);

			

		if(player->play_mode==PLAY_TRACK) 
		{

			player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);
			DBGMSG("Enter  IsMayDel_BookMark fun_1111111111=%d    %d !\n",player->bmk_no.total_bmk_no,player->bmk_no.current_bmk_no);
			
		
		}

		else
		{

			player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
			
			DBGMSG("Enter  IsMayDel_BookMark _22222222222222222222=%d %d \n", player->bmk_no.total_bmk_no,player->bmk_no.current_bmk_no);
		}


		if(player->bmk_no.total_bmk_no<=0)
		{//总书签为0，不可del 
			return  0;		
		}
		if(player->bmk_no.total_bmk_no>0)
		{
			return 1;			
		}
		
		if(player->bmk_no.current_bmk_no<player->bmk_no.total_bmk_no)
		{//可del 
			return 1;
			
		}
		else
		{//不可del 
			return 0;
		}
		

	
		
		//sys_set_music_number_bookmark(player->bmk_no.current_bmk_no+1);
		//sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);

}


/***********************************
fun:  检查某一秒的bookmark是否存在(urrent_play_time)

return :  0表示不存在。
           (1,2,3,....)表示存在，并为当前的bookmark index

*************************************/
int   IsExist_BookMark (struct audio_player* player,  int  urrent_play_time )
{

#if 0

	int  save_bookmark_no=0;
		
		DBGMSG("Enter  IsExist_BookMark  fun =%d!\n",player->bmk_no.current_bmk_no);

			

		if(player->play_mode==PLAY_TRACK) 
		{

			player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);
			
			if(player->bmk_no.current_bmk_no>=player->bmk_no.total_bmk_no)
			{
				player->bmk_no.current_bmk_no=0;
			
			}
			DBGMSG("Enter  IsExist_BookMark fun_1111111111=%d    %d !\n",player->bmk_no.total_bmk_no,player->bmk_no.current_bmk_no);
		
		}

		else
		{

			player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
			
			if(player->bmk_no.current_bmk_no>=player->bmk_no.total_bmk_no)
			{
				player->bmk_no.current_bmk_no=0;
			
			}
			
			DBGMSG("Enter  IsExist_BookMark_22222222222222222=%d %d \n", player->bmk_no.total_bmk_no,player->bmk_no.current_bmk_no);
		}


		save_bookmark_no= sys_get_music_number_bookmark();
		DBGMSG("Read no is =%d \n",save_bookmark_no);

		if(save_bookmark_no<=0)
		{
			
			return 0;
			
		}
		
		if(save_bookmark_no==player->bmk_no.current_bmk_no+1)
		{//书签存在
			
			return save_bookmark_no;
		
		}
		else 
		{

			return 0;
		}
		
		//sys_set_music_number_bookmark(player->bmk_no.current_bmk_no+1);
		//sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);


#else


	track_bmk_info track_bmk;
	album_bmk_info album_bmk;
	int timer1,timer2;
	

	
	int flag=0;
	int i=0;

	//audio_stat.current_play_time

	DBGMSG("Enter  IsExist_BookMark  fun!\n");


	timer1=urrent_play_time/1000;
	
	
	if(player->play_mode==PLAY_TRACK) 
		
	{

			player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);
		
			DBGMSG("Enter IsExist_BookMark_11111111111111111 fun=%d   !\n",player->bmk_no.total_bmk_no);
			
			track_bmk.audio_path   = player->start_place.path;	
			
			for(i=1;i<=player->bmk_no.total_bmk_no  ;i++)
			{

				track_bmk.track_bmk_no =i-1 ;
				if (get_track_bmk(&track_bmk) == -1) 
				{//fail 
						flag=0;

						DBGMSG("Enter  IsExist_BookMark_222222222222222222fun!\n");
						break;
					
				}
			     else
			     	{	

			     			DBGMSG("urrent_play_time=%d  track_bmk.play_time=%d !\n",urrent_play_time,track_bmk.play_time);
			     			timer2=track_bmk.play_time/1000;

			     			DBGMSG("timer1=%d  timer2=%d !\n",timer1, timer2);
			     			
						if(timer1==timer2)
						{//写文件
							DBGMSG("Enter  IsExist_BookMark_333333333333333333  un!\n");
							flag=1;
							 break;

						}
			     	}

				
			}
	

			DBGMSG("Enter  IsExist_BookMark_444444444444444444444  un!\n");
			if(flag==0)
			{
				DBGMSG("Enter  IsExist_BookMark_aaaaaaaaaaaaaaaaaaaaa  un!\n");
				i=0;
					
			}
			DBGMSG("Enter   IsExist_BookMark iiiiiiiiiiiii=%d !\n",i);

			return    i;
			
			//sys_set_music_number_bookmark(i);
			//sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);
		
		
	}
	else
	{
			DBGMSG("Enter Enter   IsExist_BookMark_xxxxxxxxxxxxxx fun=%s   !\n",player->start_place.dirpath);
			
			player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
			
			DBGMSG("Enter   IsExist_BookMark_555555555555555=%d.\n", player->bmk_no.total_bmk_no);
			
			album_bmk.dir_path     = player->start_place.dirpath;
			
			for(i=1;i<=player->bmk_no.total_bmk_no  ;i++)
			{


						album_bmk.album_bmk_no = i-1;
						
						
						if (get_album_bmk(&album_bmk) == -1) 
						{
							DBGMSG("Enter   IsExist_BookMark_6666666666666666.\n");
							flag=0;
							break;
						}
						else
						{
							DBGMSG("urrent_play_time=%d  track_bmk.play_time=%d !\n",urrent_play_time,album_bmk.play_time);

							timer2=album_bmk.play_time/1000;

							DBGMSG("timer1=%d  timer2=%d !\n",timer1, timer2);
						
							if(timer1==timer2 &&  (!strcmp(album_bmk.audio_path,player->start_place.path)))
							{//写文件
									DBGMSG("Enter   IsExist_BookMark_77777777777777777777.\n");
									flag=1;
								   	break;

							}
						}
								
				}


				DBGMSG("Enter   IsExist_BookMark_88888888888888888888888.\n");
				if(flag==0)
				{		
					DBGMSG("Enter   IsExist_BookMark_bbbbbbbbbbbbbbbbb un!\n");
						i=0;
						
				}
			DBGMSG("Enter   IsExist_BookMark iiiiiiiiiiii=%d !\n",i);
		//	sys_set_music_number_bookmark(i);
			//sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);

			return i;
																
	}
			
#endif

						

}







/***********************************
fun:当menu启用时，music要计算当前正在play的文件在此刻是否有bookmark

*************************************/
void CheckBookMarkAtMenuPop (struct audio_player* player,  int  urrent_play_time )
{


#if 1

		DBGMSG("Enter  CheckBookMarkAtMenuPop  fun =%d!\n",player->bmk_no.current_bmk_no);

			

		if(player->play_mode==PLAY_TRACK) 
		{

			player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);
			
		
			
		
		}

		else
		{

			player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
		
	
		}




		DBGMSG("Enter  CheckBookMarkAtMenuPop_11111111111111111 fun=%d    %d !\n",player->bmk_no.total_bmk_no,player->bmk_no.current_bmk_no);

		if(player->bmk_no.total_bmk_no>0)
		{

			if(player->bmk_no.current_bmk_no<0)
			{
				sys_set_music_number_bookmark(1);

			}
			else
			{
				sys_set_music_number_bookmark(player->bmk_no.current_bmk_no+1);	
			}
					
		}
		
		
		
		sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);


#else





	track_bmk_info track_bmk;
	album_bmk_info album_bmk;
	int timer1,timer2;
	

	
	int flag=0;
	int i=0;

	//audio_stat.current_play_time

	DBGMSG("Enter  CheckBookMarkAtMenuPop  fun!\n");


	timer1=urrent_play_time/1000;
	
	
	if(player->play_mode==PLAY_TRACK) 
		
	{

			player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);
		
			DBGMSG("Enter  CheckBookMarkAtMenuPop_11111111111111111 fun=%d   !\n",player->bmk_no.total_bmk_no);
			
			track_bmk.audio_path   = player->start_place.path;	
			
			for(i=1;i<=player->bmk_no.total_bmk_no  ;i++)
			{

				track_bmk.track_bmk_no =i-1 ;
				if (get_track_bmk(&track_bmk) == -1) 
				{//fail 
						flag=0;

						DBGMSG("Enter  CheckBookMarkAtMenuPop_222222222222222222fun!\n");
						break;
					
				}
			     else
			     	{	

			     			DBGMSG("urrent_play_time=%d  track_bmk.play_time=%d !\n",urrent_play_time,track_bmk.play_time);
			     			timer2=track_bmk.play_time/1000;

			     			DBGMSG("timer1=%d  timer2=%d !\n",timer1, timer2);
			     			
						if(timer1==timer2)
						{//写文件
								DBGMSG("Enter  CheckBookMarkAtMenuPop_333333333333333333  un!\n");
								flag=1;
							   	break;

						}
			     	}

				
			}
	

			DBGMSG("Enter  CheckBookMarkAtMenuPop_444444444444444444444  un!\n");
			if(flag==0)
			{
				DBGMSG("Enter  CheckBookMarkAtMenuPop_aaaaaaaaaaaaaaaaaaaaa  un!\n");
				i=0;
					
			}
			DBGMSG("Enter  CheckBookMarkAtMenuPop  iiiiiiiiiiiii=%d !\n",i);
			sys_set_music_number_bookmark(i);
			sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);
		
		
	}
	else
	{

			player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
			
			DBGMSG("Enter  CheckBookMarkAtMenuPop_555555555555555=%d.\n", player->bmk_no.total_bmk_no);
			
			album_bmk.dir_path     = player->start_place.dirpath;
			
			for(i=1;i<=player->bmk_no.total_bmk_no  ;i++)
			{


						album_bmk.album_bmk_no = i-1;
						
						
						if (get_album_bmk(&album_bmk) == -1) 
						{
							DBGMSG("Enter  CheckBookMarkAtMenuPop_6666666666666666.\n");
							flag=0;
							break;
						}
						else
						{
							DBGMSG("urrent_play_time=%d  track_bmk.play_time=%d !\n",urrent_play_time,album_bmk.play_time);

							timer2=album_bmk.play_time/1000;

							DBGMSG("timer1=%d  timer2=%d !\n",timer1, timer2);
						
							if(timer1==timer2 &&  (!strcmp(album_bmk.audio_path,player->start_place.path)))
							{//写文件
									DBGMSG("Enter  CheckBookMarkAtMenuPop_77777777777777777777.\n");
									flag=1;
								   	break;

							}
						}
								
				}


				DBGMSG("Enter  CheckBookMarkAtMenuPop_88888888888888888888888.\n");
				if(flag==0)
				{		
					DBGMSG("Enter  CheckBookMarkAtMenuPop_bbbbbbbbbbbbbbbbb un!\n");
						i=0;
						
				}
			DBGMSG("Enter  CheckBookMarkAtMenuPop  iiiiiiiiiiii=%d !\n",i);
			sys_set_music_number_bookmark(i);
			sys_set_music_total_bookmark(player->bmk_no.total_bmk_no);
																
	}
			
			
						
#endif

}


