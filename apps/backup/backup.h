#ifndef __BACKUP_H__
#define __BACKUP_H__

#include "widgets.h"


enum {

	BACKUP_INTERNER = 0,
	BACKUP_SDCARD,
	BACKUP_USB,	
};

enum {
	No_error = 0,
	CDDA_Remove,
	SD_card_Remove,
	Stroage_medium_Full,
	Faile_to_backup,
};

struct CopyInfo{
	FILE *fp;
	int fd;
	float	 prog_val;
	int		report;
	char   *strinfo;
};

struct backup_nano{

/****************/
	FORM* formMain;
	LABEL* label;
	TIMER *timer;
	struct CopyInfo *thiz;
	char *source;                  //备份源文件
	char *dest;                    //备份源文件
	char dirpath[PATH_MAX];                 //目录文件
	int iscdda;                    //cdda
	int fd;                         //TTS发音
	int getting_size;              //获取得到总文件大小
	int ret_pathsize;              //获取得到文件总大小
	int err_exit;                  //异常退出
	int wait_state;                //更新等待中
	int binfo;
	int bmusic;                    //是否拷贝音乐文件
//	int type;                //是哪种类型的存储器
	int media;               //源是哪种类型
	int remove_type;        // 1--SRC, 2--DST
	int	path_report;
	int path_fd;
	int		src_depth;
	int		dest_depth;
//	int key_locked;
	int bgsound;                   //背景音
	void *info_id;                 //帮助信息
	FILE *path_fp;
	unsigned long long freedir;    //磁盘所剩余总空间
	unsigned long long pathtotal;  //拷贝文件的总空间

};



#endif
