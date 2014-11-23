#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

#include "file-helper.h"

#define  MAX_PATH    512




struct  m_node;


typedef struct   PlayList{

		dir_node_t  head;			//每一层的文件列表的信息
		int      folderindex;		//文件夹的开始序号
		char  curpath[512];		//本层结点的所在文件夹
		struct  m_node *   pfirstfolder;    //指向本层文件夹的首指针
		struct  m_node *  pfather;	 //指向上一层的文件夹
		
			
	
}PlayList_T;




typedef   struct  m_node					//文件夹结点
{	
		char  name[512];					//文件夹的名称
		struct  m_node* next;				//指向平行的下一个文件夹
		struct  m_node* prev;				
 		PlayList_T *   pson;				//指向的文件列表
 		
 	
 }M_node;			


typedef   struct  m_music_info
{
	int all_album_num;			//所有album的歌曲数
	int toplevel_num;				//第一层文件夹的歌曲数
	int  all_album_cur_index;		
	int  toplevel_cur_index;
	
	
	char * pathlist;
	

}MUSIC_INFO;


PlayList_T  *   playlist;

MUSIC_INFO   music_info;

M_node*   Malloc_Node_Init(dir_node_t *  phead);



static int total_mp3=0;
char *pmusic_path=NULL;

static int copy_index=0;

/***********************************************
fun:     根据父文件夹结点得到其下面的结点

input: dirname   此层的父路径(用于scandir)
	 pmode      

*************************************************/
int  Get_SubFile_Node(char *dirname,  M_node *pmode)
{

	int ret;
	int i;

	//printf("Enter Get_SubFile_Node  fun!\n");
	
	pmode->pson=(PlayList_T * )malloc(sizeof(PlayList_T ));
	if(pmode->pson==NULL)    return -1;

	//printf("Malloc sub playlist=%p !\n",pmode->pson);

	memset(pmode->pson,0x00,sizeof(PlayList_T));

	pmode->pson->pfather=pmode;
	
	strcpy(pmode->pson->curpath,dirname);
	ret = MusicScanDir(dirname,&(pmode->pson->head), PlextalkFilterMusic);
	//printf("Enter Get_SubFile_Node_11111111111!\n");
	if (ret < 0 || (pmode->pson->head.num <= 0)) 	
	{
		return -1;
	}
	//printf("Enter Get_SubFile_Node_222222222222222!\n");
	pmode->pson->folderindex=pmode->pson->head.num-pmode->pson->head.dnum;	//此层的文件夹的开始序号
	//printf("Enter Get_SubFile_Node_33333333333333333333!\n");
	if(pmode->pson->head.dnum)
	{//有文件夹则创建结点
		//printf("Enter Get_SubFile_Node_44444444444444444444!\n");
		pmode->pson->pfirstfolder= (M_node*   )Malloc_Node_Init(&(pmode->pson->head));    //创建pmode文件夹，下一层的文件夹结点
		
				
	}
	


}



int  Freepnode(M_node *  pnode,int num)
{
		int i;
		M_node *  pnext;
		M_node *tempnode;
		
		//printf("Enter freepnode   !\n");

		tempnode=pnode;
	
		for(i=0;i<num;i++)
		{
			
			pnext=tempnode->next;
			free(tempnode);
			tempnode=pnext;
		}

		return 0;
		
}




/***************************************
Fun:   free当前层的文件家结点
input :  pfirstnode 是当前层  某一个文件夹结点
	
*****************************************/
void FreeFolderNode(M_node *pnode,PlayList_T * plist)
{

	M_node *tempnode;
	M_node *nodebak;
	int i,k;
	int  dnum=0;
	int  num=0;

	struct dirent ** name;
	



	//printf("Enter  FreeFolderNode!\n");

	if(pnode->pson->head.dnum==0)
	{//表示此文件夹下一层没有文件夹

		//printf("Enter  FreeFolderNode   pnode->pson->head.dnum==0   playlist =%p,pnode=%p!\n",pnode->pson,pnode);


		//printf("plist=%p! dnum=%d    num=%d \n",plist,plist->head.dnum,pnode->pson->head.num);
		
		plist->head.dnum--;


		
		num=pnode->pson->head.num;
		name=pnode->pson->head.namelist;
		pnode->pson->head.namelist= NULL;

		
		if (name) 
		{
			while ( num-- ) 
			{
				if (name[num])
				{
					free(name[num]);
				}
			}
			
			free(name);
		}

	

		//printf("Enter  FreeFolderNode_1111111111111!\n");
		free(pnode->pson);
		//printf("Enter  FreeFolderNode_22222222222222222!\n");
		pnode->pson=NULL;
		
		//printf("Enter  FreeFolderNode_333333333333333333!\n");
		
		//printf("Enter  FreeFolderNode   pnode->pson->head.dnum==0   pnode =%p!\n",pnode);
	
		free(pnode);
		pnode=NULL;
		
		
		
		return ;  
	}

	plist=pnode->pson;
	
	tempnode=plist->pfirstfolder;

	//printf(" FreeFolderNode =%d !\n",plist->head.dnum);

	dnum=plist->head.dnum;
	for(i=0;i<dnum;i++)
	{
		nodebak=tempnode->next;
		//printf(" FreeFolderNode path =%s !\n",tempnode->name);
		FreeFolderNode(tempnode,plist);	
		tempnode=nodebak;
	}
	

	
		
}

int  FreePlayList(PlayList_T * playlist)
{

	PlayList_T  *plist=playlist;		
	PlayList_T *pfather;
	M_node * pnode=playlist->pfirstfolder;
	M_node* nodebak;
	int dnum=0;
	int num=0;
	int i;
	struct dirent ** name;

	//printf("Enter FreePlayList Fun!\n");

	if(plist->head.dnum==0)
	{//没有文件夹

		//printf("Freeplaylist dum ==0!\n");
		pfather=plist->pfather;
		free(plist);
		return 0;
		
	}


    //printf("FreePlayList plist->head.dnum =%d!\n",plist->head.dnum);

    dnum=plist->head.dnum;
    
     for(i=0;i<dnum;i++)				//第一层文件列表的文件夹结点数
    	{//先free文件夹结点

    		 //printf("FreePlayList plist  for free!\n");
    		 
		nodebak=pnode->next;
		FreeFolderNode(pnode,plist);	
		pnode=nodebak;
			
     	}



	//printf(" Finish   FreePlayList      plist->head->num=%d\n",plist->head.num);

	num = plist->head.num;


	
	

		num=plist->head.num;
		name=plist->head.namelist;
		plist->head.namelist= NULL;

		
		if (name) 
		{
			while ( num-- ) 
			{
				if (name[num])
				{
					free(name[num]);
				}
			}
			
			free(name);
		}


	
	
	//printf(" Finish   FreePlayList    free=%p\n",plist);			
	
  	free(plist);
       
	
		

}



void AddNode(M_node *headpnode, M_node *tempnode)
{
		M_node * next=headpnode;
		
	
			
		while(next)
		{
			headpnode=next;
			next=headpnode->next;
		}

		headpnode->next=tempnode;
		tempnode->prev=headpnode;
		tempnode->next=NULL;
		

		

}
/*****************************
fun:  分配结点及初始化
********************************/
M_node*   Malloc_Node_Init(dir_node_t *  phead)
{

	int i;
	int total=phead->dnum;
	M_node*   tempnode=NULL;	
	M_node*  headpnode=NULL;
	int index;


	//printf("Enter Malloc_Node_Init fun=%d !\n", total);
	index=phead->num-phead->dnum;



	
	for(i=0;i<total;i++)
	{

		tempnode=(M_node*) malloc(sizeof(M_node));
		if(tempnode)
		{
			//printf("Malloc node =%p !\n", tempnode);
			
			memset(tempnode,0x00,sizeof(M_node));
			strcpy(tempnode->name,phead->namelist[index]->d_name);
			if(headpnode==NULL)   
			{
				headpnode=tempnode;	
				headpnode->prev=NULL;
			}
			else
			{
				AddNode(headpnode,tempnode);
			}
			index++;
			
		}
		else
		{
			//printf("Enter Malloc_Node_Init   freepnode !\n");
			Freepnode(headpnode,i);
			return NULL;
		}

	}


	return headpnode;
	
	
}


int Get_Top_LevelNode(const char *dirname,PlayList_T  *   playlist)
{
	int ret;
	int i;
	

	//printf("Enter Get_Top_LevelNode  fun=%p!\n",&(playlist->head));
	
	ret = MusicScanDir(dirname,&(playlist->head), PlextalkFilterMusic);

	
	if (ret < 0 || (playlist->head.num <= 0)) 	
	{
		return -1;
	}

	music_info.toplevel_num=playlist->head.num-playlist->head.dnum;	//得到第一层的mp3的个数
	
	playlist->folderindex=playlist->head.num-playlist->head.dnum;	//此层的文件夹的开始序号

	//printf("playlist->folderindex =%d\n",playlist->folderindex);
	
	if(playlist->head.dnum)
	{//有文件夹则创建结点
	
		playlist->pfirstfolder= (M_node*   ) Malloc_Node_Init(&(playlist->head));
				
	}


	

	
}


/*********************************************
Fun:       根据己知的文件夹结点数，来search各结点下的结点

input :num    表示当前层的文件夹的个数
        pnode   指向文件夹的首指针(如果有文件夹，有待分配结点)
        fatherpath: 此层的父路径

***********************************************/

void Get_Next_LevelData(int num, M_node *pfirstnode,char *fatherpath)
{
	int i;
	char path[512];
	M_node *pnode=pfirstnode;

	
	//printf(" Enter Get_Next_LevelData fun =%s  %p num=%d!\n",fatherpath,pnode,num);

	memset(path,0x00,sizeof(path));
	
	for(i=0;i<num;i++)
	{
	
	
		strcpy(path,fatherpath);
	
		strcat(path,"/");
	
		strcat(path,pnode->name);
		//printf(" Enter Get_Next_LevelData_7777777777777777=%s\n",pnode->name);
		Get_SubFile_Node(path,  pnode);			//创建文件夹结点下的文件列表(pnode->pson)
	
		if(pnode->pson->head.dnum)
		{
	
			Get_Next_LevelData(pnode->pson->head.dnum,pnode->pson->pfirstfolder,pnode->pson->curpath);
			
		}
		
		total_mp3+=pnode->pson->head.num-pnode->pson->head.dnum;
	
		pnode=pnode->next;			//下一个文件夹结点
	
		
		
	}
	
}


int CopyLevelPath(PlayList_T  *playlist)
{

	int i=0;
	int k=0;
	int songs;
	int node_cnt=0;
	M_node *pnode;
	char path[MAX_PATH];

	if(playlist==NULL)   return -1;

	//printf("Enter CopyLevelPath fun!\n");
	songs=playlist->head.num-playlist->head.dnum;

	pnode=playlist->pfirstfolder;
	memset(path,0x00,MAX_PATH);

	//printf("CopyLevelPath fun=%d!\n",songs);
	
	for(i=0;i<songs;i++)
	{
		
		strcpy(pmusic_path+(MAX_PATH*copy_index),playlist->curpath);
		strcat(pmusic_path+(MAX_PATH*copy_index),"/");
		strcat(pmusic_path+(MAX_PATH*copy_index),playlist->head.namelist[i]->d_name);
		
		memcpy(path,pmusic_path+(MAX_PATH*copy_index),MAX_PATH);

		//printf("copy path=%s\n",path); 

		copy_index++;
	}

	node_cnt=playlist->head.dnum;

	if(node_cnt==0)  return 0;


	for(k=0;k<node_cnt;k++)
	{
	
		playlist=pnode->pson;
		CopyLevelPath(playlist);
		pnode=pnode->next;				
	}

	return  0;

}


	


int  CopyToPlayList(PlayList_T *playlist)
{

	//printf("Enter CopyToPlayList =%d!\n",total_mp3);
	
	pmusic_path=(char *)malloc(total_mp3*MAX_PATH);
	
	if(pmusic_path==NULL)
	{
		//printf(" CopyToPlayList  failed!\n");
		return -1;
       }

	memset(pmusic_path,0x00,total_mp3*MAX_PATH);



	//printf("Enter CopyToPlayList _111111111111111!\n");
	return CopyLevelPath(playlist);

	

	
}


/***********************************

Fun:  得到第所有文件夹的歌曲

**********************************/
int  Get_All_Album_Num()
{




		if(music_info.all_album_num>=0)  return music_info.all_album_num;
		  		else   return 0;

		
	
}
/***********************************

Fun:  得到第一层文件夹mp3 的个数

**********************************/

int   Get_TopLevel_Num()
{

	if(music_info.toplevel_num>=0)   return music_info.toplevel_num;
	else   return 0;
}





char * Get_All_Album_PathName(int index)
{

	 int    music_index=index;

	  if(music_index>=music_info.all_album_num-1)
	  {

	  	music_index=music_index%(music_info.all_album_num);
	  }

	return   (music_info.pathlist+MAX_PATH*music_index);
	
}









char * Get_TopLevel_PathName(int index)
{

	 int    music_index=index;

	  if(music_index>music_info.toplevel_num)
	  {

	  	music_index=music_index%music_info.toplevel_num;
	  }




	return   (music_info.pathlist+MAX_PATH*music_index);
	
}



char * Get_All_Album_Next_Path()
{


		music_info.all_album_cur_index++;

		if(music_info.all_album_cur_index>music_info.all_album_num-1)
		{

				music_info.all_album_cur_index=0;
				

		}
		

		return Get_All_Album_PathName(music_info.all_album_cur_index);	

		
}

char * Get_All_Album_Prev_Path()
{



		music_info.all_album_cur_index--;

		if(music_info.all_album_cur_index<0)
		{

				
			music_info.all_album_cur_index=music_info.all_album_num-1;

		}
		


		return Get_All_Album_PathName(music_info.all_album_cur_index);	

		
}




char * Get_TopLevel_Next_Path()
{


		music_info.toplevel_cur_index++;

		if(music_info.toplevel_cur_index>music_info.toplevel_num-1)
		{

				music_info.toplevel_cur_index=0;
				

		}
		

		return Get_All_Album_PathName(music_info.toplevel_cur_index);	

		
}

char * Get_TopLevel_Prev_Path()
{



		music_info.toplevel_cur_index--;

		if(music_info.toplevel_cur_index<0)
		{

				
			music_info.toplevel_cur_index=music_info.toplevel_num-1;

		}
		


		return Get_All_Album_PathName(music_info.toplevel_cur_index);	

		
}

/****************************************
Fun: 得到所有文夹的随机歌曲
******************************************/
char *  Get_All_Album_Rand_Index()
{		

	int n;
	
      n= rand()%music_info.all_album_num;



	return   (music_info.pathlist+MAX_PATH*n);

		
	
}


/****************************************
Fun: 得到所有文夹的随机歌曲
******************************************/
char *  Get_TopLevel_Rand_Index()
{		

	int n;
	
      n= rand()%music_info.toplevel_num;


	return   (music_info.pathlist+MAX_PATH*n);

		
	
}




int   Get_All_Album_CurIndex()
{

		return   music_info.all_album_cur_index;
		
}

/**********************************
input:传进来的是打开的文件夹或文件的父目录

return : 0 正确，-1 错误 

***********************************/

int  Creat_PlayList(const char *dirname)
{
	int ret;
	int i;

	dir_node_t node;
	M_node *pfirstfolder;

	

	//printf("Enter Creat_PlayList Fun==%s !\n",dirname);
	copy_index=0;


	memset(&music_info,0x00,sizeof(music_info));

	music_info.all_album_cur_index=-1;
	music_info.toplevel_cur_index=-1;
	
	music_info.pathlist=pmusic_path;
	
	if(pmusic_path)
	{

		//printf("Free  pmusic_path Fun!\n");
		
		free(pmusic_path);
		pmusic_path=NULL;
	}

	playlist=(PlayList_T  * )malloc(sizeof(PlayList_T));
	if(playlist==NULL)   
	{
		//printf("  malloc failed!\n");
		return -1;
	}

	//printf("malloc  playlist  =%p\n",playlist);
	memset(playlist,0x00,sizeof(PlayList_T));
	playlist->pfather=NULL;
	strcpy(playlist->curpath,dirname);
	Get_Top_LevelNode(dirname,playlist);
	pfirstfolder=playlist->pfirstfolder;
	total_mp3=playlist->head.num-playlist->head.dnum;
	Get_Next_LevelData(playlist->head.dnum,pfirstfolder,playlist->curpath);

	//printf("Finish  seek music =%d\n",total_mp3);
	
	CopyToPlayList(playlist);
	FreePlayList(playlist);
	playlist=NULL;
	music_info.pathlist=pmusic_path;

	music_info.all_album_num=total_mp3;
	

//	printf("Finish   music path copy!\n");

	//for(i=0;i<total_mp3;i++)
//	{
	
	//	printf(" i=%d path =%s\n",i,pmusic_path+(i*MAX_PATH));	
//	}
	
	



}
