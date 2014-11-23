#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <glib.h>
#include "audio_dir.h"
#include "audio_bookmark.h"
#include "audio_extern.h"
#include "audio_draw.h"
#include <dirent.h>
#define OSD_DBG_MSG
#include "nc-err.h"
#include "audio_play.h"
#include "file_cursor.h"

//use audio_box
//#include "audio_box.h"


extern struct nano_data nano;

/* Function: set player infomation
 * Set player dir infomation, offset infomation 
 * Set player bmk infomation.
 */


extern int sys_set_music_total_bookmark(int vol);



/******************************************
type==1  表示是文件夹play
type==0 表示是单曲play

path 表示分别传的是文件路径，还是文件夹路径


*********************************************/
int set_player_info (const char * path, enum file_type type,struct audio_player * player)

{			
		if((audio_stat.trackflag==1) ||StringStartWith(path, "/media/cdda"))
		{
			return set_CDDA_player_info(path,type,player);
			
		}
		else 
		{
			return set_memory_player_info(path,type,player);	
		}

		
}

int set_memory_player_info (const char * path, enum file_type type,struct audio_player * player)
{


	char *fpath;
	char *temppath;
	int pos=0;

	DBGMSG("enter set_player_info  path: %s \n", path);	

	
	if(player->filecur!=NULL)
	{

		DBGMSG("set_memory_player_info destory  player->filecur\n");	
		
		 file_cursor_destroy (player->filecur);
		 player->filecur=NULL;
	}


	if (type == AUDIO_TRACK) 
	{//单曲play(用于进行文件的选择

		fpath=parse_father_dir (path);
		DBGMSG(" AUDIO_TRACK  father dir=%s!\n",fpath);
		if (fpath==NULL)   
			return  0;
		strcpy(player->start_place.dirpath,fpath);
		player->play_mode 	= PLAY_TRACK;
		player->filecur=file_cursor_create (player->start_place.dirpath, MODE_MUX_LEVEL);		//只search 第一层的文件夹
		if(player->filecur==NULL)   return 0;	

		DBGMSG(" AUDIO_TRACK  file_cursor_create finish!\n");
		while(1)
		{//找到与player->start_place.path相同的文件进行play 

			temppath= file_cursor_next_track (player->filecur,&pos,0);
			DBGMSG(" AUDIO_TRACK_111111111111\n");
			if(temppath==NULL) 
			{//第一层没有mp3
				  DBGMSG(" AUDIO_TRACK  file_cursor_destroy!\n");
				 file_cursor_destroy (player->filecur);
				  player->filecur=NULL;
				return 0;	
			}
			else
			{
				if(strcmp(temppath,path)==0)
				{
						DBGMSG(" AUDIO_TRACK  play path =%s!\n",temppath);
						strcpy(player->start_place.path,temppath);
						return 1;
				}
				else
				{
					continue;
				}
					
			}
			

		}
		
	}
	else if (type == AUDIO_ALBUM) 
	{//album play (用于进行文件的选择)

		
		player->play_mode = PLAY_ALBUM;
		DBGMSG(" AUDIO_ALBUM  father dir=%s!\n",path);
		strcpy(player->start_place.dirpath,path);
		player->filecur=file_cursor_create (player->start_place.dirpath, MODE_MUX_LEVEL);		//research 所有的文件夹
		if(player->filecur==NULL)   return 0;
		DBGMSG(" AUDIO_ALBUM_111111111111\n");
		temppath= file_cursor_next_track (player->filecur,&pos,1);
		DBGMSG(" AUDIO_ALBUM_222222222222\n");
		if(temppath==NULL)   
		{
			 file_cursor_destroy (player->filecur);
			 player->filecur=NULL;
			return 0;
		}

		
		

		DBGMSG(" AUDIO_ALBUM  play path =%s!\n",temppath);
		strcpy(player->start_place.path,temppath);
		
		
	}
	else 
	{//用于上电时要进行album play 的恢复
	
		player->play_mode = PLAY_ALBUM;
		DBGMSG(" AUDIO_ALBUM  father dir=%s  player->start_place.path =%s !\n",path,player->start_place.path);
		strcpy(player->start_place.dirpath,path);
		DBGMSG(" AUDIO_ALBUM _333333333333333333\n");
		player->filecur=file_cursor_create (player->start_place.dirpath, MODE_MUX_LEVEL);		//research 所有的文件夹
		DBGMSG(" AUDIO_ALBUM _444444444444444444444\n");
		if(player->filecur==NULL)  
		{
			DBGMSG("AUDIO_ALBUM   file_cursor_create failed!\n");
			return 0;
		}

		DBGMSG(" AUDIO_ALBUM _5555555555555555555555\n");
		
		while(1)
		{//找到与player->start_place.path相同的文件进行play 

			temppath= file_cursor_next_track (player->filecur,&pos,1);

			DBGMSG("while  temppath=%s\n",temppath);
			if(temppath==NULL) 
			{
				DBGMSG(" AUDIO_ALBUM _666666666666666666666666\n");
				 file_cursor_destroy (player->filecur);
				 player->filecur=NULL;
				return 0;	
			}
			else if (pos==POS_END)
				{
					DBGMSG(" AUDIO_ALBUM _888888888888888888\n");
					strcpy(player->start_place.path,temppath);
					return 1;
				}
			else
			{
				if(strcmp(temppath,player->start_place.path)==0)
				{
						DBGMSG(" AUDIO_ALBUM _77777777777777777777777777\n");
						return 1;
				}
				else
				{
					continue;
				}
					
			}
			

		}
		
	
		
	
		
	}

	return 1;
	
		
}



/*********************************
将CDDA的track1-->track0  ,track 2-->track 1

/media/cdda/Track 1

*********************************/

int   Conver_Cdda_TrackPath(struct audio_player* player,int step)
{
	char *pstr=player->start_place.path;
	
	int len=0;
	int num;



	pstr=strstr(pstr,"Track");
	if(pstr==NULL)   return 0 ;

	pstr=pstr+6;		//指向后面的数字Track 1, Track 2....

	len=strlen(pstr);
	if(len==0)   return 0;

	num=atoi(pstr);
	DBGMSG(" Conver_Cdda_TrackPath num=%d !\n",num);	

	
	
	num=num+step;

	if(num<1)  num=player->track_num;

	if(num>player->track_num)   num=1;
		
	
	snprintf(pstr, 20, "%d", num);
	
	player->start_place.trackindex=num;
	
	DBGMSG(" Conver_Cdda_TrackPath num=%s ,trackindex=%d !\n",player->start_place.path,num);	
	return 1;

}



/***************************

在打开Cdda时进行文件路径的转换

*****************************/

int set_CDDA_player_info (const char * path, enum file_type type,struct audio_player * player)
{
	char *pstr=player->start_place.path;

	char *fpath;

	strcpy(player->start_place.path,path);

	fpath=parse_father_dir (path);

	if(fpath==NULL)   
		return  0;
 
	strcpy(player->start_place.dirpath,fpath);		
	
	DBGMSG("Enter set_CDDA_player_info =%s   dir_path=%s ,type=%d!\n",player->start_place.path,player->start_place.dirpath,type);	

	if(type==AUDIO_TRACK)
	{//表示pstr打开的是文件

		player->play_mode 	= PLAY_TRACK;
		return Conver_Cdda_TrackPath(player,0);    

	}
	else
	{//表示打开的是文件夹
		player->play_mode 	= PLAY_ALBUM;
		strcpy(player->start_place.dirpath,"/media/cdda");
		strcpy(player->start_place.path,"/media/cdda/Track 1");
		player->start_place.trackindex=1;
		DBGMSG("set_CDDA_player_info =%s!\n",player->start_place.path);	
		return 1;
	}
	
}


int load_cdda_file (enum LOAD_MODE mode, struct audio_player* player,int flag)
{

	char* next_album = NULL;
	char* pre_album = NULL;		
	int step;
	int retint =1;


	DBGMSG("Enter load_cd_file fun!\n");	
	
	switch (mode)
	{
		case NEXT_TRACK:
			{
				
				retint=Conver_Cdda_TrackPath(player,1);
				player->start_place.start_time = 0;			
			
				break;
		  }
				
		case PRE_TRACK:
			{
				
				retint=Conver_Cdda_TrackPath(player,-1);
				player->start_place.start_time = 0;
				break;
			} 
			
			
				
		case SHUFFLE_TRACK:
			 {
			
				step= (rand()%(player->track_num+1) );
				retint=Conver_Cdda_TrackPath(player,step);
				player->start_place.start_time = 0;
				break;
				
				
				} 
			
		case NEXT_ALBUM:
	
				
			 
			
			break;
	
		case PRE_ALBUM:
		
			
				
			break;				
		}
	
		
		return retint;
}



/****************************************

fun: 得到当前层的下一首，或下一首的歌曲

*****************************************/
int  Load_CurLevel_File(enum LOAD_MODE mode, struct audio_player* player)
{

	DBGMSG("Enter Load_CurLevel_File fun!\n");

	char *tempath;
	int retint=1;


	if (StringStartWith(player->start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;

	
	if(audio_stat.trackflag)
	{
		return load_cdda_file (mode, player,0);
	}
	else
	{


	 if(mode==NEXT_TRACK)
	 {//得到当前层的下一首歌曲

		tempath=CurLevel_Cursor_next();	
		
		if(tempath!=NULL)
		{
			strcpy(player->start_place.path,tempath);
	   	      	player->start_place.start_time = 0;	
	   	      			
		}
		else
		{
			retint=0;
		}
		
	 }
	 else 
	 {//得到当前层的上一首歌曲

		tempath=CurLevel_Cursor_prev();	
		if(tempath!=NULL)
		{
			strcpy(player->start_place.path,tempath);
	   	      	player->start_place.start_time = 0;	
	   	      			
		}
		else
		{
			retint=0;
		}
		
		
	  }

	 return retint;
	
	
	
}

}



/******************************************
flag==0  表示只在第一层目录找.mp3或.wav
flag==1  表示 包含子文件夹进行找.mp3 或.wav

******************************************/
int load_file(enum LOAD_MODE mode, struct audio_player* player,int flag,int *pos)
{

	DBGMSG("Enter load_file fun!\n");

	if (StringStartWith(player->start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;

	
	if(audio_stat.trackflag)
	{					
		return load_cdda_file (mode, player,flag);
	}
	else
	{		

		return load_memory_file(mode ,player,flag,pos);		
	}

	
}	

int   load_memory_file (enum LOAD_MODE mode, struct audio_player* player,int flag,int *pos)
{


	char *tempath;
	int retint=1;
	int step;
	char *fpath;
	int i;

	static   int old_step=0;
	
	printf("Enter load_memory_file fun mode =%d!\n",mode);

	
	switch (mode)
	{
		case NEXT_TRACK:
		{
				
				
				tempath=file_cursor_next_track(player->filecur,pos	,flag);	
				printf("load_memory_file    NEXT_TRACK tempath=%s\n",tempath);
				if(tempath!=NULL)
				{
					strcpy(player->start_place.path,tempath);
		   	      		player->start_place.start_time = 0;	
		   	      			
				}
				else
				{
					retint=0;
				}
				
			
			break;
		}
				
		case PRE_TRACK:
			{

				tempath=file_cursor_prev_track(player->filecur,pos	,flag);	
				printf("load_memory_file  PRE_TRACK  tempath=%s\n",tempath);
				if(tempath!=NULL)
				{
					strcpy(player->start_place.path,tempath);
		   	      		player->start_place.start_time = 0;	
		   	      			
				}
				else
				{
					retint=0;
				}
			
			break;
			}
				
		case SHUFFLE_TRACK:
			 {



				int total_num=0;
				int n=20;
				
				while(n--)
				{
					tempath=file_cursor_next_track(player->filecur,pos,flag);
					if(*pos==POS_END)
					{
						
						break;
						
					}
				}

				if(n)
				{//表示上面扫描后，music 少于20首 

					while(1)
					{
					
						tempath=file_cursor_next_track(player->filecur,pos,flag);
						
						DBGMSG(" seek path=%s\n",tempath);
						
						if(*pos==POS_END)
						{
							total_num++;
							break;
							
						}
						total_num++;
						if(total_num>=20)   break;

					}

				
				}
				else
				{
					total_num=50;
					
				}

			
				
				
				
				printf("load_memory_file  SHUFFLE_TRACK total  =%d !\n",total_num);

				while(1)
				{
					
						step= rand()%total_num;				//得到随机数
						if(step!=old_step)
						{
							old_step=step;
							break;
						}
						
				}

				

				printf("load_memory_file  SHUFFLE_TRACK =%d !\n",step);

			
				
				for(i=0;i<step;i++)
				{

					printf("load_memory_file  SHUFFLE_TRACK_22222222222222!\n");
					tempath=file_cursor_next_track(player->filecur,pos,flag);	
					if(tempath==NULL)
					{
						DBGMSG("load_memory_file  SHUFFLE_TRACK_333333333333333 !\n");
						retint=0;
						break;
					}
			   	      			
					printf("load_memory_file  SHUFFLE_TRACK path=%s !\n",tempath);
						
				}

				printf("load_memory_file  SHUFFLE_TRACK i=%d step=%d  path=%s !\n",i,step,tempath);
				if(i==step)
				{
					strcpy(player->start_place.path,tempath);
			   	      	player->start_place.start_time = 0;	
				}
					



			break;
			} 
			
		case NEXT_ALBUM:
			{

			 	tempath= file_cursor_next_album (player->filecur);	
			 	printf("load_memory_file  NEXT_ALBUM  tempath=%s\n",tempath);
				if(tempath!=NULL)
				{
					strcpy(player->start_place.path,tempath);
		   	      		player->start_place.start_time = 0;	
		   	      		player->bmk_no.current_bmk_no=-1;
		   	      			
				}
				else
				{
					retint=0;
				}
				break;
			}
	
		case PRE_ALBUM:
			{
				tempath= file_cursor_prev_album (player->filecur);	
			 	printf("load_memory_file  NEXT_ALBUM  tempath=%s\n",tempath);
				if(tempath!=NULL)
				{
					strcpy(player->start_place.path,tempath);
		   	      		player->start_place.start_time = 0;	
		   	      		player->bmk_no.current_bmk_no=-1;
		   	      			
				}
				else
				{
					retint=0;
				}
			break;	
			}
		}
	

		if(player->start_place.path!=NULL)
		{
			
				fpath=parse_father_dir (player->start_place.path);

				if(fpath!=NULL)
				strcpy(player->start_place.dirpath,fpath);
		}

		
		return retint;
}




