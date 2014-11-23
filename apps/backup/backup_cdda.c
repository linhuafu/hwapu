#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <backup.h>
#include "file-helper.h"
//#define OSD_DBG_MSG 0
#include "nc-err.h"
struct CopyCDDA{
	GMainLoop *loop;
	float precent;
	float precent_file;
	char  *dest_path;
	int   backup_error;
	int   isdir;
	int   track_num;
};

struct CopyCDDA copycdda;

int Backup_cdda_error();
float Return_precnet();

//定义消息处理函数
static gboolean bus_call(GstBus *bus,GstMessage *msg,gpointer data)
{
   //这个是主循环的指针，在接受EOS消息时退出循环
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(copycdda.loop);
            break;
        case GST_MESSAGE_ERROR:
        {
             gchar *debug;
             GError *error;
		Backup_cdda_error();
             gst_message_parse_error(msg,&error,&debug);
             g_free(debug);
             g_printerr("ERROR:%s\n",error->message);
             g_error_free(error);
             g_main_loop_quit(copycdda.loop);
             break;
        }
        default:
             break;
    }
    return TRUE;
}

int get_player_time(GstElement *PipeLine)
{
    gint64 len;
    time_t total_time,pos;
    int Total_Time=0;
    int Pos=0;
    GstFormat fmt=GST_FORMAT_TIME;
    if(gst_element_query_duration(PipeLine,&fmt,&len))
    {
				total_time=GST_TIME_AS_SECONDS(len);
    }
    
    if(gst_element_query_position(PipeLine,&fmt,&len))
    {
				pos=GST_TIME_AS_SECONDS(len);
    }
    DBGMSG("total_time:%d\n",(int)total_time);
    DBGMSG("pos_time:%d\n",(int)pos);
    if(total_time==pos)
    {
    	gst_element_set_state(PipeLine,GST_STATE_NULL);
    	g_print("time over\n");
    	g_main_loop_quit(copycdda.loop);
    }
    Total_Time=(int)total_time;
    Pos=(int)pos;
    if(Pos > Total_Time)
	copycdda.precent=0;
   else
    	copycdda.precent=Pos*100.0/Total_Time;
    DBGMSG("precent:%f\n",copycdda.precent);
    //Return_precnet();
    return 1;	
}

//备份一首歌
int Backup_one_track(int num,char *backup)
{

    GstElement *pipeline,*source,*decoder,*sink;//定义组件
    GstBus *bus;
    gint time_tag;

    gst_init(NULL,NULL);
    copycdda.loop = g_main_loop_new(NULL,FALSE);//创建主循环，在执行 g_main_loop_run后正式开始循环

    //创建管道和组件
    pipeline = gst_pipeline_new("cdda-backup");
    
    source = gst_element_factory_make("cdiocddasrc","file-source");
    decoder = gst_element_factory_make("shinemp3enc","mp3-encoder");
    sink = gst_element_factory_make("filesink","file-output");

    if(!pipeline||!source||!decoder||!sink){
        g_printerr("One element could not be created.Exiting.\n");
        return -1;
    }
    //设置 source的location 参数。即 文件地址.
    g_object_set(G_OBJECT(source),"mode",1,NULL);
    DBGMSG("track: %d\n",num);
    g_object_set(G_OBJECT(source),"track",num,NULL);
    //g_object_set(G_OBJECT(source),"read-speed",100,NULL);
    
    g_object_set(G_OBJECT(decoder),"bitrate",128,NULL);
    
    g_object_set(G_OBJECT(sink),"location",backup,NULL);
   
    //得到管道的消息总线
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
   //添加消息监视器
    gst_bus_add_watch(bus,bus_call,copycdda.loop);
    gst_object_unref(bus);
    
    //把组件添加到管道中.管道是一个特殊的组件，可以更好的让数据流动
    gst_bin_add_many(GST_BIN(pipeline),source,decoder,sink,NULL);
   //依次连接组件
    gst_element_link_many(source,decoder,sink,NULL);
   
    time_tag=g_timeout_add(1,(void *)get_player_time,pipeline);
    //开始播放
    gst_element_set_state(pipeline,GST_STATE_PLAYING);
    g_print("Running\n");
    //开始循环
    g_main_loop_run(copycdda.loop);
    g_print("Returned,stopping playback\n");
    g_source_remove(time_tag);
    gst_element_set_state(pipeline,GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    return 1;
}

int get_track_id(char *path)
{
	if(path==NULL)
	{
		DBGMSG("get_track_id():source path is NULL!\n");
		return -1;
	}
	char *ptr;
	int track_id;
	ptr=strstr(path,"Track");
	ptr+=5;
	track_id=atoi(ptr);
	DBGMSG("track_id:%d",track_id);
	return track_id;
}

int create_dest_path(char *dest)
{
	char path[PATH_MAX];
	char cmd[PATH_MAX];
	char *ptr;

	if(dest==NULL)
	{
		DBGMSG(" dest path error!\n");
		return -1;
	} 
	strcpy(path,dest);
	ptr=strstr(path,"Track");
	*ptr='\0';
	DBGMSG("dest_path:%s\n",path);
	sprintf(cmd,"mkdir -p \"%s\"",path);
	if(system(cmd)!=0)
	{
		DBGMSG("create %s error!\n",path);
		return -1;
	}else
	{
		DBGMSG("create %s success!\n",path);
	}
	return 0;
}
int get_track_num()
{
	FILE *fp;
	int count=0;
	if((fp=popen("/sbin/cdrom_id /dev/sr0 | /bin/grep ID_CDROM_MEDIA_TRACK_COUNT_AUDIO","r"))==NULL)
	{
		DBGMSG("open dev/sr0 error!\n");
		return -1;
	}
	fscanf(fp,"ID_CDROM_MEDIA_TRACK_COUNT_AUDIO=%d",&count);
	DBGMSG("count:%d\n",count);
	pclose(fp);
	return count;
}

int return_backup_error()
{
	return copycdda.backup_error;
}
int Backup_cdda_error()
{
	char cmd[PATH_MAX];
	char path[PATH_MAX];
	FILE *fp;
	char *ptr;
	int count=0;
	struct stat64 buf;
	struct statfs diskInfo;
	int backup_path_is_sd=-1;
	enum{mmcblk0p2,mmcblk1,mmcblk1p1};
	
	copycdda.backup_error=-1;
	sprintf(cmd,"rm -rf \"%s\"",copycdda.dest_path);
	if(system(cmd)!=0)
	{
		DBGMSG("rm dest error!\n");
	}else
	{
		DBGMSG("rm dest ok\n");
	}

	//cdda remove
	if((fp=popen("/sbin/cdrom_id /dev/sr0 | /bin/grep ID_CDROM_MEDIA_CD","r"))==NULL)
	{
		DBGMSG("open dev/sr0 error!\n");
	}else{
		fscanf(fp,"ID_CDROM_MEDIA_CD=%d",&count);
		DBGMSG("count:%d\n",count);
		pclose(fp);
		if(count != 1)
		{
			copycdda.backup_error=CDDA_Remove;
			DBGMSG("CDDA_Remove\n");
			return copycdda.backup_error;
		}
	}


	strncpy(path,copycdda.dest_path,20);
	path[20]='\0';
	DBGMSG("path:%s\n",path);
	if((ptr=strstr(path,"/media/mmcblk0p2"))!=NULL)
	{
		statfs("/media/mmcblk0p2", &diskInfo);
		backup_path_is_sd=mmcblk0p2;
	}else if((ptr=strstr(path,"/media/mmcblk1"))!=NULL)
	{
		statfs("/media/mmcblk1", &diskInfo);
		backup_path_is_sd=mmcblk1;
	}else if((ptr=strstr(path,"/media/mmcblk1p1"))!=NULL)
	{
		statfs("/media/mmcblk1p1", &diskInfo);
		backup_path_is_sd=mmcblk1p1;
	}
	usleep(500000);
	//sd card remove
	if(backup_path_is_sd==mmcblk1)
	{
		if(stat64("/dev/mmcblk1", &buf) != 0){
			copycdda.backup_error=SD_card_Remove;
			DBGMSG("SD_card_Remove\n");
			return copycdda.backup_error;
		}
	}
	if(backup_path_is_sd==mmcblk1p1)
	{
		if(stat64("/dev/mmcblk1p1", &buf) != 0){
			copycdda.backup_error=SD_card_Remove;
			DBGMSG("SD_card_Remove\n");
			return copycdda.backup_error;
		}
	}
	
	//stroage medium full
	unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
  	unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
  	DBGMSG("free = %llu",freeDisk);
	if(freeDisk<=10)
	{
		DBGMSG("Not enough space on the target media ,Processing is interrupted !");
		copycdda.backup_error=Stroage_medium_Full;
		return copycdda.backup_error;
	}
	copycdda.backup_error=Faile_to_backup;
	return copycdda.backup_error;
}
//备份整张光盘
int Backup_all_track(char *path,int num)
{
	if(num <= 0)
	{
		DBGMSG("error:backup 0 track !\n");
		return 0;
	}
	char dest[PATH_MAX];
	char Number[3];
	char *p;
	int len,i;
	struct stat64 buf;
	if(stat64(path,&buf)!=0)
	{
		sprintf(dest,"mkdir -p \"%s\"",path);
		if(system(dest)!=0)
		{
			DBGMSG("mkdir error!\n");
			return -1;
		}else
			DBGMSG("mkdir -p ok\n");
	}
	memset(dest,0,PATH_MAX);
	for(i=0;i<num;i++)
	{
		p=dest;
		
		len=strlen(path);
		strcpy(dest,path);
		p=p+len;
		
		strcpy(p,"Track");
		p=p+5;
		sprintf(Number,"%d",i+1);
		len=strlen(Number);
		if(len==1)
		{
			strcpy(p,"0");
			p++;
		}
		strcpy(p,Number);
		p=p+len;
		strcpy(p,".mp3");
		DBGMSG("%s\n",dest);
		Backup_one_track(i+1,dest);
		copycdda.precent_file=(i+1)*100.0/num;
	}
	return 1;
}

int is_dir(char *path)
{
	int len;
	len=strlen(path);
	DBGMSG("len :%d",len);
	if(*(path+len-1)=='/')
		return 1;
	else
		return 0;
}
float Return_precnet()
{
	if(copycdda.isdir)
	{
		DBGMSG("precent_file:%f\n",copycdda.precent_file+copycdda.precent/copycdda.track_num);
		return copycdda.precent_file+copycdda.precent/copycdda.track_num;
	}else{
		DBGMSG("precent_one track:%f\n",copycdda.precent);
		return copycdda.precent;
	}
}

int Backup_cdda(void* arg )
{
	struct backup_nano *cdda=(struct backup_nano *)arg;
	int track_id;
	if(cdda->source==NULL||cdda->dest==NULL)
	{
		DBGMSG("source or dest path error!\n");
		return -1;
	} 
	if(strlen(cdda->source)>PATH_MAX||strlen(cdda->dest) > PATH_MAX)
	{
		DBGMSG("source or dest path too long!\n");
		return -1;
	} 
	DBGMSG("source:%s\n",cdda->source);
	DBGMSG("dest:%s\n",cdda->dest);
	copycdda.dest_path=cdda->dest;
	copycdda.backup_error=0;
	if(is_dir(cdda->source))
	{
		copycdda.isdir = 1;
		copycdda.track_num=get_track_num();
		Backup_all_track(copycdda.dest_path,copycdda.track_num);
		DBGMSG("is a dir\n");
	 }else
	 {
	    copycdda.isdir = 0;
	    create_dest_path(copycdda.dest_path);
	    track_id=get_track_id(cdda->source);
	    Backup_one_track(track_id,copycdda.dest_path);
	    DBGMSG("is a file!\n");
	  }
  return 1;
}
//备份光盘
/*
int Backup_cdda(char *source,char *dest)
{
	int track_id;
	if(source==NULL||dest==NULL)
	{
		DBGMSG("source or dest path error!\n");
		return -1;
	} 
	if(strlen(source)>PATH_MAX||strlen(dest) > PATH_MAX)
	{
		DBGMSG("source or dest path too long!\n");
		return -1;
	} 
	DBGMSG("source:%s\n",source);
	DBGMSG("dest:%s\n",dest);
	copycdda.dest_path=dest;
	copycdda.backup_error=0;
	if(is_dir(source))
	{
		copycdda.isdir = 1;
		copycdda.track_num=get_track_num();
		Backup_all_track(dest,copycdda.track_num);
		DBGMSG("is a dir\n");
	 }else
	 {
	    copycdda.isdir = 0;
	    create_dest_path(dest);
	    track_id=get_track_id(source);
	    Backup_one_track(track_id,dest);
	    DBGMSG("is a file!\n");
	  }
  return 1;
}*/


