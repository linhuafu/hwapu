//#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#define MWINCLUDECOLORS
#include "microwin/nano-X.h"
#include "microwin/device.h"
#include "efont.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <locale.h>
#include <libintl.h>
#include <glib.h>
#include <gst/gst.h>
#include "volume_ctrl.h"    


#include "audio_play.h"
//#include "explorer.h"
#include "load_preset.h"

#include "audio_bookmark.h"
//#include "menu_notify.h"
#include "audio_ui.h"
#include "audio_tts.h"
#include "audio_draw.h"
#include "audio_extern.h"
#include "audio_type.h"
#include "audio_dir.h"
//=======================
#include "amixer.h"
#include "application.h"
#include "neux.h"
#include "widgets.h"
#include "desktop-ipc.h"
#include "application-ipc.h"
#include "plextalk-i18n.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "sysfs-helper.h"
#include "plextalk-statusbar.h"
#include "plextalk-titlebar.h"
#include "plextalk-keys.h"
#include "plextalk-config.h"
#include "plextalk-ui.h"
#include "plextalk-theme.h"
#include "plextalk-constant.h"
#include "libvprompt.h"
#include "file_cursor.h"
#include "playlist.h"
#include "vprompt-defines.h"
#include "storage.h"
#include <mntent.h>

//use audio_box 
//#include "audio_box.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#define OSD_DBG_MSG
#include "nc-err.h"

#if 0
//this define is for hotplug
#define MWHOTPLUG_DEV_MMCBLK1	0
#define MWHOTPLUG_DEV_SDA		1
#define MWHOTPLUG_DEV_BATTERY	2
#define MWHOTPLUG_DEV_VBUS		3
#define MWHOTPLUG_DEV_ACIN		4

#define MWHOTPLUG_ACTION_ADD	0
#define MWHOTPLUG_ACTION_REMOVE	1
#define MWHOTPLUG_ACTION_CHANGE	2
#endif


#define    AUTO_RESTORE_COUNT  30

#define TITLEBAR_SCROLL_TIME 500
#define   DISTANCE_Y        0

#define   MAX_FOLDER_LEVEL   8

//#define  MUSIC_INI       "/media/mmcblk0p2/.plextalk/music.ini"
#define 	MUSIC_INI      		PLEXTALK_SETTING_DIR"music.ini"





#define PLEXTALK_SETTING_DIR  "/opt/plextalk/usr_setting/"




#define  MUSIC_SAVEFILE    "/tmp/.music"


#define PACKAGE "music"
#define _(S) 	gettext(S)			

#define TIMER_PERIOD 		150			//0.15s flush screen 
#define SEEK_TIME_FORWARD	0x01
#define SEEK_TIME_BACK		0x02




static FORM *  gmusic_formMain;

  char    fre_min[8];
  char    fre_max[8];


int g_endflg=0;
static int last_app;	// we only record "book, music and radio"
static int active_app;
static int   old_suspendflag=0;

//struct plextalk_language_all all_langs;

//static int udc_online;
//static int acin_online;
//static int vbus_online;
//static int hp_online;
//static int key_lock;


static int vprompt_end_recover_audio;


extern  void  normal_beep (void);

struct audio_player player;

/* structure for signal handler */
struct Signalset {
	
	void* player;
	void* nano;
};

static struct Signalset sig_set;

struct nano_data nano;


//static TIMER *key_timer;
static int long_pressed_key = -1;


static   char g_albumname[256]={0};
static int g_info_flag;
static int  enter_filelist=0;


static float speed_table[] = {0.5, 0.75, 1, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0};// {0.5,0.75,1,1.2,1.4,1.6,1.7,1.8,1.9,2.0,2.1};   

static int key_music_handler (struct nano_data* nano, struct audio_player* player,char *path);
// static void  Music_OnTimerClockTimer(WID__ wid);
 static void Music_show_wait (struct nano_data* nano);
 void RegttsSingal (void);

extern void set_CDDA_player_info(struct audio_player * player , int ret_type);
extern void  Conver_Cdda_TrackPath(struct audio_player* player,int step);
static void Music_onHotplug(WID__ wid, int device, int index, int action);
static void Music_onAppMsg(const char * src, neux_msg_t * msg);
extern void  normal_beep (void);
extern  void volume_error_beep (void);

void Start_Music_From_Stop();

static void sig_set_equ_handler(struct audio_player* player, struct nano_data* nano,int issound);
extern  struct voice_prompt_cfg vprompt_cfg;;
extern int  Load_CurLevel_File(enum LOAD_MODE mode, struct audio_player* player);
//============================================
int music_setConditionIcon(int state)
{
	NhelperStatusBarSetIcon(SRQST_SET_CONDITION_ICON, state);
	if(SBAR_CONDITION_ICON_PLAY == state){
		plextalk_set_volume_vprompt_disable(1);
	}else{
		plextalk_set_volume_vprompt_disable(0);
	}
	return 1;
}

static int MusicExist(const char *fpath)
{
	if (StringStartWith(fpath, "/media/cdda"))
		return 1;
	else
		return PlextalkIsFileExist(fpath);
}


static int readonly;

static int storage_enum(struct mntent *mount_entry, void* user_data)
{
	DBGMSG("mnt_dir=%s\n",mount_entry->mnt_dir);
	char *pStart = strstr(mount_entry->mnt_dir ,PLEXTALK_MOUNT_ROOT_STR);
	if(NULL != pStart)
	{
		DBGMSG("pStart=%s\n",pStart);
	
		if(!strncmp(pStart, user_data, strlen(pStart)))
		{
			readonly = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
			return 1;
		}
	
	}
	return 0;
}

int isMediaReadOnly(const char *path)
{
	int ret = CoolStorageEnumerate(storage_enum ,path);
	if(1 == ret)
	{
		return readonly;
	}
	else
	{
		return 0;
	}
}

void  sys_get_music_backup_path(char *path)
{
	
	
	

	CoolGetCharValue(MUSIC_SAVEFILE, "backup","backup_path", 512);
	
        
}



int sys_set_music_backup_path(char *path)
{


    CoolSetCharValue(MUSIC_SAVEFILE,"backup","backup_path",path);
   return 0;
	
}



int sys_get_music_total_bookmark(void)
{

	int val;
	char str[20];
	CoolGetCharValue(MUSIC_SAVEFILE, "bookmark","count", str, 4);
   	val=atoi(str);
       return val;
	

}


int sys_set_music_total_bookmark(int vol)
{


	char str[20];
	snprintf(str, 20, "%d", vol);
	CoolSetCharValue(MUSIC_SAVEFILE,"bookmark","count",str);
	return 0;
	
}




int sys_get_music_number_bookmark(void)
{

	int val;
	char str[20];
	CoolGetCharValue(MUSIC_SAVEFILE, "bookmark","current", str, 4);
   	val=atoi(str);
       return val;
	

}


int sys_set_music_number_bookmark(int vol)
{


	char str[20];
	snprintf(str, 20, "%d", vol);
	CoolSetCharValue(MUSIC_SAVEFILE,"bookmark","current",str);
	return 0;
	
}

//================================================

/*****************************
 Fun: 要等TTS发音完后才进行下面的语句
 
******************************/
static void  Wait_TTSContinue(WID__ wid)
{

 	
		

  

}

/**********************************************


inmenu:  表示进入menu时的中断 
filelist :表示进入file list 的中断
***********************************************/
void Break_Music( struct audio_player* player,int inmenu,int filelist)
{


	DBGMSG("Enter  Break_Music  Fun!\n");

	Get_Music_State(player);
		
	if(inmenu)
	{//将要进入meun中打断

		DBGMSG("Break_Music   go menu!\n");
	
		voice_prompt_abort();
		if(player->play_info.breakflg)
		{
			
		}
		else
		{

				if(player->play_info.play_state==PLAYING)
				{
					player->play_info.breakflg=1;	
					audio_pause(0,0,inmenu);
					player->play_info.resumeback=audio_recover;

				}
		}
	

		return ;
	

	}

	DBGMSG("Break_Music _1111111111111!\n");
	if(player->play_info.breakflg)
	{//如果己处在打断的过程中则不响应再次打断

		return ;
	}


	
	DBGMSG("Break_Music _22222222222222!\n");
	
	if(player->play_info.play_state==PLAYING)
	{

		DBGMSG("music is playing !\n");
		player->countforsound=AUTO_RESTORE_COUNT;	
		player->play_info.breakflg=1;	
		audio_pause(0,0,filelist);
		player->play_info.resumeback=audio_recover;
		
	}
	else
	{
		DBGMSG("music is not  playing !\n");
		player->play_info.resumeback=NULL;
		player->countforsound=-1;
		player->play_info.breakflg=0;
		player->play_info.resumeflg=0;
		
	}
	
	
}


 

 


int sys_get_music_speaker_volume(void)
{

	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","speaker_volume", str, 2);
   	val=atoi(str);
       return val;
	

}





int sys_set_music_speaker_volume(int vol)
{


	char str[20];
	snprintf(str, 20, "%d", vol);
	CoolSetCharValue(MUSIC_INI,"history","speaker_volume",str);
	return 0;
	
}


int sys_set_music_play_mode(int  play_mode)
{

	char str[20];
	snprintf(str, 20, "%d", play_mode);
	CoolSetCharValue(MUSIC_INI,"history","play_mode",str);
	return 0;

}
	

int sys_get_music_play_mode(void)
{
	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","play_mode", str, 2);
   	val=atoi(str);
       return val;

	
}




int sys_get_music_headphone_volume(void)
{
	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","headphone_volume", str, 2);
   	val=atoi(str);
       return val;
	
	
}

int sys_set_music_headphone_volume(int vol)
{
	
	char str[20];
	snprintf(str, 20, "%d", vol);
	CoolSetCharValue(MUSIC_INI,"history","headphone_volume",str);
	return 0;
	
}

int sys_get_music_speed(void)
{

	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","music_speed", str, 2);
   	val=atoi(str);
       return val;
	
}	


int  sys_get_music_navi(void)
{

	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","music_navi", str, 2);
   	val=atoi(str);

	if(val>4 ||val<0 )   
	{
		val=0;
	}
	
   	
       return val;
	

}



int  sys_set_music_navi(int navi)
{

	char str[20];
	snprintf(str, 20, "%d", navi);
	CoolSetCharValue(MUSIC_INI,"history","music_navi",str);
	return 0;

}


int sys_set_music_speed(int speed)
{

	char str[20];
	snprintf(str, 20, "%d", speed);
	CoolSetCharValue(MUSIC_INI,"history","music_speed",str);
	return 0;
}

int sys_get_music_repeat(void)
{
	int val;
	char str[20];
	DBGMSG("Enter sys_get_music_repeat fun!\n");
	CoolGetCharValue(MUSIC_INI, "history","music_repeat", str, 2);
	DBGMSG("music_repeat is=%s\n",str);
   	val=atoi(str);
	DBGMSG("sys_get_music_repeat 11111111111=%d\n",val);
       return val;
}

void  sys_set_music_repeat(int val)
{
	char str[20];
	DBGMSG("Enter sys_set_music_repeat  fun!\n");
	
	snprintf(str, 20, "%d", val);
	DBGMSG("Enter sys_set_music_repeat  fun =%s !\n",str);
	CoolSetCharValue(MUSIC_INI,"history","music_repeat",str);
	
}

int sys_get_music_equalizer(void)
{
	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","music_equalizer", str, 2);
   	val=atoi(str);
       return val;
}
int sys_set_music_equalizer(int val)
{
	char str[20];
	snprintf(str, 20, "%d", val);
	CoolSetCharValue(MUSIC_INI,"history","music_equalizer",str);
	return 0;
}

int sys_get_music_resume_time(void)
{
	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","resume_time", str, 10);
   	val=atoi(str);
       return val;
}

int sys_set_music_resume_time(int val)
{
	char str[20];
	snprintf(str, 20, "%d", val);
	CoolSetCharValue(MUSIC_INI,"history","resume_time",str);
	return 0;
}





void  sys_get_music_resume_path(char *path)
{
	
	
	

	CoolGetCharValue(MUSIC_INI, "history","resume_path", path, 512);
	
        
}



int sys_set_music_resume_path(char *path)
{


    CoolSetCharValue(MUSIC_INI,"history","resume_path",path);
	return 0;
	
}
//================================



void  sys_get_music_backupfloder_path(char *path)
{
	
	
	

	CoolGetCharValue(MUSIC_INI, "history","backupfloder_path", path, 512);
	
        
}



int sys_set_music_backupfloder_path(char *path)
{


    CoolSetCharValue(MUSIC_INI,"history","backupfloder_path",path);
	return 0;
	
}


//========================================

char* sys_get_music_browser(void)
{
	char str[100];
	
	memset(str,0,sizeof(str));
	CoolGetCharValue(MUSIC_INI, "history","resume_browser", str, 100);
        return str;
}


int sys_set_music_browser(char *path)
{


    CoolSetCharValue(MUSIC_INI,"history","resume_browser",path);
	return 0;
}





int sys_get_global_backup(void)
{
	int val;
	char str[20];
	CoolGetCharValue(MUSIC_INI, "history","backup", str, 2);
   	val=atoi(str);
       return val;
	
}


int sys_set_global_backup(int val)
{
	char str[20];
	snprintf(str, 20, "%d", val);
	CoolSetCharValue(MUSIC_INI,"history","backup",str);
	return 0;
}

void sys_set_music_Initialize()
{
	
}

void  Music_Default_Set(struct audio_player* player)
{

	player->repeat_mode=0;
	sys_set_music_repeat(player->repeat_mode);
				
	player->effect_value=0;
	sys_set_music_equalizer(player->effect_value);
	
	sig_set_equ_handler(player,&nano,0);
				
	player->speed=0;
//暂时这样给
	audio_speed_set(1.0 ,speed_table[player->speed+2]);
	sys_set_music_speed(player->speed);	

	player->navi_value=NAVI_SEEK_BOOKMARK;
	sys_set_music_navi(player->navi_value);
	
	

	
}



/* 回调函数 */
void fd_read(void *pdata)
{
   struct signalfd_siginfo fdsi;
   
 DBGMSG("enter fd_read Fun !\n");
	
   ssize_t ret = read(nano.fd, &fdsi, sizeof(fdsi));

   if (ret != sizeof(fdsi))
     {
     		//handle_error();

     		DBGMSG("fd_read  error!!\n");
   	}

	//...自己的逻辑部分


	DBGMSG("fd_read    g_info_flag =%d    %d  %d\n",g_info_flag,nano.menuflag,player.play_info.breakflg);
	player.is_running=0;
	player.countforsound=-1;



	DBGMSG("fd_read %d , %d\n",player.manual_pause,player.in_file_select);


	if(g_info_flag ||nano.menuflag==1||player.in_file_select)
	{//表示在infomation  or menu ,filelist  

		DBGMSG("In information or  in menu !\n");
		return ;
	}

#if 1

      if(player.play_info.breakflg)
      	{
      		if(player.play_info.resumeback)
      		{
      			DBGMSG("fd_read  resume  music !\n");
      			player.play_info.resumeback(0);
      		}
      	}

	

#else		

	if(audio_autostopped())
	{// music 自然play结束

		DBGMSG("fd_read music audio_autostopped!\n");
		return ;
	}


	if((nano.recover_flag  ||nano.playingflag) && player.manual_pause==0 &&  player.in_file_select==0)
	{
			DBGMSG("fd_read audio_recover!\n");
			audio_recover(0);							
			nano.recover_flag =0;
			nano.playingflag=0;
	}


#endif

	if (vprompt_end_recover_audio == 1) {
		audio_recover(0);
		vprompt_end_recover_audio = 0;
	}


	
  }


					



/*****************************************
fun :让所有的控件确认是否可见

******************************************/

void   ShowAllControl(int show)
{

	 

		if(show)//图片控件需要重新加载绘制,注意要在Show之前SepPic否则会刷不出来
		{
			audio_draw_equalizer(&nano, player.effect_value);
			NeuxThemeIconSetPic(nano.psymbol, THEME_ICON_MUSIC_SYMBOL);
			NeuxThemeIconSetPic(nano.pprocess, THEME_ICON_MUSIC_POLE);
			//NeuxThemeIconSetPic(nano.pcell, THEME_ICON_MUSIC_CELL);
			audio_draw_repeat_mode(&nano, player.repeat_mode);	

			audio_draw_current_time(&nano, audio_stat.current_play_time / 1000);			//当前时间
			audio_draw_total_time(&nano, audio_stat.total_play_time / 1000);				//总时间 
			DBGMSG("total time=%d,cur time=%d\n",audio_stat.total_play_time,audio_stat.current_play_time );
			audio_draw_progress_bar(&nano, audio_stat.current_play_time, audio_stat.total_play_time, 50);
		
			
			
		}
		NeuxWidgetShow(nano.pprocess,show);
		NeuxWidgetShow(nano.pcell, show);
		NeuxWidgetShow(nano.prepeat_mode, show);
		NeuxWidgetShow(nano.lplay_mode, show);

		NeuxWidgetShow(nano.psymbol, show);
		NeuxWidgetShow(nano.lsong_name, show);
		NeuxWidgetShow(nano.lcur_time, show);
		NeuxWidgetShow(nano.ltotal_time, show);
	
		
	
	
		

}


int  Get_DirLevel(char *path)
{
	
	int level=0;
	char *pstr=path;

	while(*pstr)
	{
		if(*pstr=='/')
		{
			level++;	
		}

		pstr++;
		
	}


	return level;
	
}


char * GetAlbum_Name(char *path)
{

	int len=0;
	int level=0;
	char *p;
	char *p1;
	
	DBGMSG("enter GetAlbum_Name Fun %s!\n",path);

	len=strlen(path);

	p=path+len-1;
	p1=p;

	level=Get_DirLevel(path);

	while(len--)
		{

			if(*p=='/')
			{
				
				break;
			}
			p--;
		}


	DBGMSG("thd album name is %s!\n",p+1);
	
	if(p<=p1)
	{

		DBGMSG("thd album name is ok %s!\n",p+1);

		
		strcpy(g_albumname,p+1);

		if(level<=2)
		{
			if(!strcmp(g_albumname,"mmcblk0p2"))
			{

				strcpy(g_albumname,_("Internal Memory"));
			}
			else if (strstr(g_albumname,"mmcblk1"))
			{
				
				strcpy(g_albumname,_("SD Card"));	
			}
			else if(strstr(g_albumname,"sd"))
			{

				strcpy(g_albumname,_("USB Memory"));	
			}
			else 
			{
				strcpy(g_albumname,_("Music"));	
			}

				
		}
		

		DBGMSG("last  path is:%s  \n",g_albumname);
		return g_albumname;
	}
	else
	{
		DBGMSG("thd album name is  error \n");
		strcpy(g_albumname,_("Music"));

		DBGMSG("last1111111111  path is:%s  \n",g_albumname);
		
		return g_albumname;
	}


	
	

}


/* Music Grab key */
static void
music_grab_key (struct nano_data* nano)
{

	//NeuxWidgetGrabKey(nano->form, VKEY_MUSIC,        NX_GRAB_HOTKEY_EXCLUSIVE);
	//NeuxWidgetGrabKey(nano->form, MWKEY_ENTER,        NX_GRAB_HOTKEY_EXCLUSIVE);


}



static void audio_tts_CDDA (enum tts_mode mode, char * path)
{


	char *pstr=path;
	
	int len=0;
	int num;

	pstr=strstr(pstr,"Track");
	if(pstr==NULL)   return ;

	pstr=pstr+6;		//指向后面的数字Track 1, Track 2....

	len=strlen(pstr);
	if(len==0)   return;

	num=atoi(pstr);
	

	
	num++;
	
	
	snprintf(pstr, 20, "%d", num);
	
	audio_tts(TTS_SEEK_TRACK, (int)path);	

}


static void 
ui_audio_start (struct audio_player* player,int issound)
{	
	/*Create theared until it success*/
	int retint=0;
	int  loopcnt=20;
	char  *fpath;
	struct stat64 info;

	DBGMSG("enter ui_audio_start Fun =%s   %s! time=%d \n",player->start_place.path,player->start_place.dirpath,player->start_place.start_time);
	g_endflg=0;

	if (StringStartWith(player->start_place.path, "/media/cdda")) {

	} else {
		stat64(player->start_place.path, &info);

		DBGMSG("ui_audio_start file size=%d\n",info.st_size);
		if(info.st_size<1000)
		{
			audio_stat.error_flag=1;
			music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
			return 0;
		}
	}
		

	
	player->start_place.speed = speed_table[player->speed+2];
	load_preset(player);
	player->start_place.bands = player->equ.gains;
	
	while(audio_start(&player->start_place) == -1)
	{	
		DBGMSG("[fatal]:audio_start error!\n");
		usleep(1000);	//wait and create again
	}


		fpath=parse_father_dir (player->start_place.path);
	
		if(fpath)   
		{
			strcpy(player->start_place.dirpath,fpath);
		}


		
	NhelperTitleBarSetContent(GetAlbum_Name(player->start_place.dirpath));
	audio_draw_title(&nano, player->start_place.path);

	if (StringStartWith(player->start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;
	
	if(audio_stat.trackflag)
	{
		
			player->track_num=CoolCDDAGetTrackCount();
	}
	
	player->check_flag=0;

	/*Avoid for tts repeately*/

	
	DBGMSG("read call ui_audio_start _1111111111111111 player->exp =%d  ==%s\n",player->exp,player->start_place.path);



	if (!player->exp) 
	{
			if(issound)
			{
				audio_tts(TTS_SEEK_TRACK, (int)player->start_place.path);     
			}
		
	}
	else
	{
		player->exp = 0;
	}




#if 1

	DBGMSG("read call ui_audio_start _22222222222222 ==%s\n",player->start_place.path);
	audio_back_up(player);


	if (player->start_place.start_time)
	{
		DBGMSG("audio_seek is:%d !\n",player->start_place.start_time);

		while(loopcnt--)
		{
			DBGMSG("Setting   seek time!\n");
			retint=audio_seek(player->start_place.start_time);
			usleep(200);
			if(retint==0)
			{
				break;
			}

		}
	
			
	}


//当为PLAY_ALBUM play 时，由于在进行album的书签跳转时，要调用此函数，故player->bmk_no.current_bmk_no=-1;没有放到下面
	if(player->play_mode==PLAY_TRACK) 
	{
		player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);		
		player->bmk_no.current_bmk_no=-1;

		
	}
	else
	{
		player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
		
	}
	

	


	DBGMSG("audio play is :%d !\n",player->bmk_no.total_bmk_no);


#endif

	
}



void  SetMedaiIcon(const char *path)
{

// why???
//	audio_stat.trackflag=0;
		
	if(StringStartWith(path,"/media/mmcblk1"))
	{//打开tf 卡的内容
		DBGMSG(" open  tf content !\n");
	//	menu_notify_cmd("desktop", MN_NOTIFY_DESKTOP_MEDIA_SDCARD);	
	
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_SD_CARD);
	}
      else if(strstr(path,"/media/mmcblk0p2"))
      	{//打开内卡的内容
      		DBGMSG(" open  inner momory content  !\n");
      		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
      	}
      	 else if(StringStartWith(path,"/media/sd"))
      	 {//打开usb的内容

      	 	DBGMSG(" open   USB  content \n");
      	 	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
      	 }
      	  else if(StringStartWith(path,"/media/cdda")||StringStartWith(path,"/media/sr"))
      	 {//打开CD上的内容

//		audio_stat.trackflag=1;
		
      	 	DBGMSG(" open  CDROM  content!\n");
      	 	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_USB_MEM);
      	 	
      	 }
      	 else
      	 {
		NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM);
      	 }

 }



static int
audio_player_init (struct nano_data* nano, struct audio_player* player)
{
	/* get last config */

	
	char path[512] ;
	
	int start_time ;

	DBGMSG("Enter audio_player_init fun!\n");
	
	//sys_set_music_repeat(ALBUM_REPEAT);		//for test 
	
	DBGMSG("Enter audio_player_init fun_111111111111111!\n");
	
	player->repeat_mode  = sys_get_music_repeat();

	DBGMSG(" Play mode=%d !\n",player->repeat_mode );
	
	player->speed        = sys_get_music_speed();	
	
	DBGMSG("Enter audio_player_init  fun  player->speed  =%d!\n",player->speed );
	
	player->effect_value = sys_get_music_equalizer();

	DBGMSG("Enter audio_player_init  fun  player->effect_value  =%d!\n",player->effect_value );
	
	player->volume		 = sys_get_music_speaker_volume();

	DBGMSG("Enter audio_player_init  fun  player->volume=%d!\n",player->volume);
	
	player->play_mode =sys_get_music_play_mode();

	DBGMSG("Enter audio_player_init  fun  player->play_mode =%d!\n",player->play_mode);


	player->navi_value=sys_get_music_navi();
	
	player->adjust		 = player->volume;

	player->manual_pause = 0; 		/*No manual pause */
	player->help   	   	 = 0;		/*help  is not in progress*/
	player->exp    	  	 = 0;		/*if the music is selected form key_music*/
	player->filecur=NULL;
	player->countforsound=-1;
	player->loadflag=1;
	player->deltitleok=0;

	/* Init the audio signal flag */
	player->del_track 		= 0;
	player->del_album 		= 0;
	player->set_speed 		= 0;
	player->set_repeat		= 0;
	player->set_bmk 		= 0;
	player->del_cur_bmk 	= 0;
	player->del_all_bmk 	= 0;
	player->set_equ 		= 0;
	player->set_guid_volume = 0;
	player->set_guid_speed 	= 0;;
	player->set_font_size 	= 0;
	player->set_font_color 	= 0;
	player->set_language 	= 0;
	player->set_tts 		= 0;
	player->patherror=0;
	nano->menuflag=0;
	audio_stat.stop_flag = 1;
	audio_stat.suspendflag=0;
	
	plextalk_set_menu_disable(0);
	/* not in file selection */
	player->in_file_select = 0;
	
	/* load  equalizer value */
	memset(player->equ.gains, 0, sizeof(double) * 10);
	
	plextalk_suspend_lock();
	
	

	memset(path,0,sizeof(path));
	
	sys_get_music_resume_path(path);

	sys_get_music_backupfloder_path(player->start_place.backupfloder);
	
	DBGMSG(" Music path is=%s !\n",path);
		
	start_time = sys_get_music_resume_time();


	DBGMSG("start_time=%d\n",start_time);

	player->bmk_no.current_bmk_no=-1;

	Get_Music_FormMenu(player);
	

	if (!check_vaild_path(path)) 
	{//当文件名不合法时，要重新选文件
		
		
		DBGMSG(" Music path is invaild! \n");
		player->patherror=1;
		//enter_filelist=1;                                                    //等待发LOGO_TTS发音完，再从PAINT中进入  key_music_handler
		sleep(2);
		
		key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);
	
	} 
	else 
	{

			
		SetMedaiIcon(path);
		
		audio_draw_repeat_mode(nano, player->repeat_mode);
		audio_draw_equalizer(nano, player->effect_value);

		

	
		DBGMSG(" Music path is ok! \n");
		
//		printf("[DEBUG]:%s,%d\n", __func__, __LINE__);

	
		
		if(player->play_mode==PLAY_TRACK)
		{//进行单曲的播放

			DBGMSG(" set  play mode as PLAY_TRACK=%s !\n",path);

			strcpy(player->start_place.path,path);
			
			if(set_player_info(path, AUDIO_TRACK, player)==0)	//这个也许可以放在ui-audio的后面
			{
				key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);
				
							
			}
			else 
			{	
				DBGMSG("player->start_place.start_time =%d start_time=%d\n",player->start_place.start_time,start_time);
				player->start_place.start_time = start_time;
				DBGMSG(" set  play mode as PLAY_TRACK1111111111111=%s !\n",path);
			}
			


			
		}
		else
		{// album play 
		
			DBGMSG(" set  play mode as  PLAY_ALBUM !\n");

			char *fpath=parse_father_dir (path);

			DBGMSG(" Fathpath=%s !\n",fpath);
			strcpy(player->start_place.path,path);
			
			if(set_player_info(fpath, AUDIO_ALBUM_RECOVER, player)==0)
			{
				key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);	
			}
			else
			{
				player->start_place.start_time = start_time;
			}
			
		}

		DBGMSG("enter audio_player_init------888888888888888888!\n");
		ui_audio_start(player,1);
//		printf("[DEBUG]:%s,%d\n", __func__, __LINE__);

	}

	return 0;
}


static void 
sig_del_track_handler (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("sig delete track handler!\n");

	if(strstr(player->start_place.path,"/media/cdda"))
	{
		DBGMSG("sig delete track handler_44444444444444!\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);    
		return; 
	}

	
	 if(isMediaReadOnly(player->start_place.path))
	 {//写保护

	 	DBGMSG("sig delete track handler_1111111111!\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);    
		return ;
	 }

	
	
	 if(remove(player->start_place.path)==-1)
	{

		DBGMSG("sig delete track handler_2222222222222!\n");
		NhelperDeleteResult(APP_DELETE_ERR);                //删除成功
		
		return ;

	}

	 audio_stop();
	DBGMSG("sig delete track handler_33333333333333333!\n");
	NhelperDeleteResult(APP_DELETE_OK); 
	


	player->deltitleok=1;
	
	player->del_track = 0;
}


static void
sig_del_album_handler (struct audio_player* player, struct nano_data* nano)
{
	
	char cmd[512];
	
	DBGMSG("sig_del_album handler!\n");

	if(strstr(player->start_place.path,"/media/cdda"))
	{
		DBGMSG("sig_del_album_handler_111111111111111!\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);    
		return; 
	}
	
	 if(isMediaReadOnly(player->start_place.path))
	 {//写保护
	 
		DBGMSG("sig_del_album_handler_222222222222222!\n");
		NhelperDeleteResult(APP_DELETE_MEDIA_LOCKED_ERR);	
		return ;
	 }

	audio_stop();

	memset(cmd,0x00,sizeof(cmd));

	sprintf(cmd, "rm -rf %s", player->start_place.dirpath);
	DBGMSG("cmd=%s\n",cmd);
	system(cmd);
//	audio_tts(TTS_DELETE_ALBUM, 0);
	NhelperDeleteResult(APP_DELETE_OK);



	player->deltitleok=1;
	player->del_album = 0;
}





static void
sig_set_bmk_handler(struct audio_player* player, struct nano_data* nano)
{
	album_bmk_info album_bookmark;	
	track_bmk_info track_bookmark;
	int res;

	 DBGMSG("Enter  sig_set_bmk_handler fun %d\n",player->play_mode);
	 
	switch (player->play_mode)
		{
		case PLAY_ALBUM:

			DBGMSG("Enter  sig_set_bmk_handler fun_11111111111111\n");
			
			album_bookmark.album_bmk_no = player->bmk_no.total_bmk_no ;
			album_bookmark.audio_path = player->start_place.path;
			album_bookmark.dir_path   = player->start_place.dirpath;
			album_bookmark.play_time  = audio_stat.current_play_time;

			DBGMSG("Enter  sig_set_bmk_handler fun_22222222222222222\n");
			
			res = set_album_bmk(&album_bookmark);	

			DBGMSG("Enter  sig_set_bmk_handler fun_33333333333333333\n");
	
			if (res == -1)
			{
				//audio_tts(TTS_SET_BMK, 1);		//set bmk failure
				DBGMSG("Enter  set_album_bmk fun  set failed!\n");
			}
			else 
			{
				//audio_tts(TTS_SET_BMK, 0);		//set bmk success
				
				player->bmk_no.total_bmk_no ++;

				DBGMSG("Enter  set_album_bmk fun  set ok =%d!\n",player->bmk_no.total_bmk_no);
			}
			DBGMSG("Enter  sig_set_bmk_handler fun_444444444444444444444\n");
			break;
	
		case PLAY_TRACK:	
			DBGMSG("Enter  sig_set_bmk_handler fun_555555555555555=%s \n",player->start_place.path);
			track_bookmark.track_bmk_no = player->bmk_no.total_bmk_no;
			track_bookmark.audio_path	= player->start_place.path;
			track_bookmark.play_time	= audio_stat.current_play_time;

			DBGMSG("Enter  sig_set_bmk_handler fun_6666666666666666 =%s \n",track_bookmark.audio_path);
			
			res = set_track_bmk(&track_bookmark);
			
			DBGMSG("Enter  sig_set_bmk_handler fun_777777777777777777777\n");
			if (res == -1) 
			{
				DBGMSG("Enter  sig_set_bmk_handler fun  set failed!\n");
				//audio_tts(TTS_SET_BMK, 1);		//set bmk failure
			}
			else 
			{
				//audio_tts(TTS_SET_BMK, 0);		//set bmk success
				
				
				player->bmk_no.total_bmk_no ++;

				DBGMSG("Enter  set_track_bmk fun  set ok =%d!\n",player->bmk_no.total_bmk_no);
			}
			DBGMSG("Enter  sig_set_bmk_handler fun_888888888888888888888n");
			break;	
	}	
	player->set_bmk = 0;
}



static void 
sig_del_cur_bmk_handler(struct audio_player* player, struct nano_data* nano)
{
	int res;

	switch (player->play_mode) 
		{
		case PLAY_TRACK:
			
			res = del_track_bmk(player->start_place.path, player->bmk_no.current_bmk_no);
							
				
			if (res == -1) 
			{
				//audio_tts(TTS_DELETE_CUR_BMK, -1);		//del cur_tra_bmk failure
			} 
			else
			{
				DBGMSG("%d bookmark removed!\n", player->bmk_no.current_bmk_no);
				//audio_tts(TTS_DELETE_CUR_BMK, player->bmk_no.current_bmk_no);		//del cur_tra_bmk success

				if(player->bmk_no.current_bmk_no>=0)
				{
						player->bmk_no.current_bmk_no -= 1;
				}

				player->bmk_no.total_bmk_no -= 1;
				
			}
			break;
	
		case PLAY_ALBUM:
			
			res = del_album_bmk(player->start_place.dirpath, player->bmk_no.current_bmk_no);
				
			if (res == -1) 
			{
				//audio_tts(TTS_DELETE_CUR_BMK, -1);		//del cur_alb_bmk failure
			} 
			else 
			{
				//audio_tts(TTS_DELETE_CUR_BMK, player->bmk_no.current_bmk_no);		//del cur_alb_bmk success
			
				player->bmk_no.total_bmk_no -= 1;
				
				if(player->bmk_no.current_bmk_no>=0)
				{
						player->bmk_no.current_bmk_no -= 1;
				}
				DBGMSG("%d bookmark removed!\n", player->bmk_no.current_bmk_no);
			}
			break;
	}
	player->del_cur_bmk = 0;
}


static void
sig_del_all_bmk_handler(struct audio_player* player, struct nano_data* nano)
{
	int res;

	switch (player->play_mode) 
		{
		case PLAY_TRACK:
			
		res = del_all_track_bmk(player->start_place.path);
		if (res == -1) 
		{
			//audio_tts(TTS_DELETE_ALL_BMK, -1);		//del bmk failure
		} 
		else 
		{
			//audio_tts(TTS_DELETE_ALL_BMK, 0);		//del bmk success
			
		}
		break;

		case PLAY_ALBUM:
		res = del_all_album_bmk(player->start_place.dirpath);
		if (res == -1)
		{
			//audio_tts(TTS_DELETE_ALL_BMK, -1);		//del all_bmk failure
		} 
		else 
		{
			//audio_tts(TTS_DELETE_ALL_BMK, 0);		//del all_bmk success					//wait for tts
		}
		break;
	}
	player->del_all_bmk = 0;
}

void load_preset(struct audio_player* player)
{

	char qeustr[17][20]={ "Flat","Bass",  "Classical",  "Club",   "Dance ",   "Flat",   " Hall ",   "Headphones",  "Live",   " Party",   "Pop",  " Reggae ",     " Rock",   "Ska",   "Soft",     "Treble",  " Techno"};
				                            
	char preset[10];
	int i;

	DBGMSG(" load_preset  player->effect_value=%d\n",player->effect_value);

	if(player->effect_value<0)  player->effect_value=0;
	if(player->effect_value>16)  player->effect_value=16;
	
	strcpy(preset,qeustr[player->effect_value]);
	
	DBGMSG(" load_preset   sig_set_equ_handler =%s!\n",preset);


	xml_read_equalizer_10bands(preset, &(player->equ));

	for(i=0;i<10;i++)
	{
		DBGMSG("%d =%f\n",i,player->equ.gains[i]);			
	}	
				
}


static void
sig_set_equ_handler(struct audio_player* player, struct nano_data* nano,int issound)
{


	
	DBGMSG("recivev signal of set equalizer=%d!\n", player->effect_value);
	

	load_preset(player);
	
	if(issound)
	{
		audio_tts(TTS_SET_EQU, player->effect_value);
	}
	
	audio_equalizer_set(player->equ.gains);

	player->set_equ = 0;
}


#if 0

static void
sig_set_guid_speed (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("signal handler for set guid speed!\n");

	player->set_guid_speed = 0;
}

static void
sig_set_guid_volume (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("signal handler for set guid volume!\n");
	audio_tts_change_volume();
	player->set_guid_volume = 0;
}


static void
sig_set_font_size (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("signal handler for set font size!\n");

	player->set_font_size = 0;
}


/***********************************
Fun:  当在menu 中修改系统语言时，要重新设置标题栏

************************************/
static void
sig_set_language (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("signal handler for set language!\n");

	sys_set_cur_lang_env("music");//change the current thread language enviroment

//	scroll_text_set_text(nano->title_text, _("My Music Files"));//change special text display content



	
	player->set_language = 0;
}



static void
sig_set_tts (struct audio_player* player, struct nano_data* nano)
{
	DBGMSG("signal handler for signal set tts!\n");
	
	player->set_tts = 0;
}


#endif

/*bookmark handler*/
static void
ui_bmk_handler (enum SEEK_BMK_MODE mode, struct audio_player* player)
{
	track_bmk_info track_bmk;
	album_bmk_info album_bmk;

	int   cur_bookmark_bak=0;
	
	switch (mode) {
		case NEXT_BOOKMARK:
			{	
				
				DBGMSG("ui_bmk_handler =%d,   %d\n",player->bmk_no.current_bmk_no,player->bmk_no.total_bmk_no);
				
				player->bmk_no.current_bmk_no++;	
				if (player->bmk_no.current_bmk_no > player->bmk_no.total_bmk_no - 1) 
				{
					DBGMSG("bookmark rollback!\n");
					player->bmk_no.current_bmk_no = 0;
				} 
			
				switch (player->play_mode) 
				{
					case PLAY_TRACK:
						track_bmk.track_bmk_no = player->bmk_no.current_bmk_no ;	
						DBGMSG("Seek PLAY_TRACK  bookmark %d.\n", player->bmk_no.current_bmk_no);
						track_bmk.audio_path   = player->start_place.path;
						if (get_track_bmk(&track_bmk) == -1) 
						{
							audio_tts(TTS_SEEK_BOOKMARK, -1);	//seek failure
																//no wait
							return;
						}

						DBGMSG("PLAY_TRACK bookmark time=%d\b",track_bmk.play_time);
						
						if (audio_autostopped()) 
						{//音乐己停止
							cur_bookmark_bak=player->bmk_no.current_bmk_no;
							ui_audio_start(player,1);
							audio_seek(track_bmk.play_time);

							player->bmk_no.current_bmk_no=cur_bookmark_bak;
						}
						else 
						{
							audio_seek(track_bmk.play_time);
						}

						
						audio_tts(TTS_SEEK_BOOKMARK, player->bmk_no.current_bmk_no+1);		//seek success
				
						break;

					case PLAY_ALBUM:
						DBGMSG("Seek PLAY_ALBUM bookmark %d.\n", player->bmk_no.current_bmk_no);
						album_bmk.album_bmk_no = player->bmk_no.current_bmk_no ;   
						album_bmk.dir_path     =player->start_place.dirpath;
						
						if (get_album_bmk(&album_bmk) == -1) 
						{
							audio_tts(TTS_SEEK_BOOKMARK, -1);	// seek failure
																//no wait
							return;
						}

						if (!check_vaild_path(album_bmk.audio_path))
						{
							DBGMSG("This is not a vaild bookmark.\n");
							sig_del_cur_bmk_handler(player, (struct nano_data*)(sig_set.nano));					
						} 
						else 
						{						
							strcpy(player->start_place.path , album_bmk.audio_path);
							player->start_place.start_time = album_bmk.play_time;
							DBGMSG("PLAY_TRACK bookmark time=%d,%s \n", album_bmk.play_time,player->start_place.path);
							NeuxTimerSetEnabled(nano.tid,FALSE);
							audio_stop();
							ui_audio_start(player,1);
							audio_seek(player->start_place.start_time);						//seek  play time 		
							NeuxTimerSetEnabled(nano.tid,TRUE);
							audio_tts(TTS_SEEK_BOOKMARK, player->bmk_no.current_bmk_no+1);	//seek success
						
						}
						break;
				}
			}
			break;

		case PRE_BOOKMARK:
			{
				player->bmk_no.current_bmk_no--;
				
				if (player->bmk_no.current_bmk_no < 0) 
				{
					DBGMSG("current_bmk_no < 0, rollback!\n");
					player->bmk_no.current_bmk_no = player->bmk_no.total_bmk_no - 1;
				}
				
				switch (player->play_mode)
					{
					case PLAY_TRACK:
						track_bmk.track_bmk_no = player->bmk_no.current_bmk_no;
						track_bmk.audio_path   = player->start_place.path;
						if (get_track_bmk(&track_bmk) == -1)
						{
							audio_tts(TTS_SEEK_BOOKMARK, -1);		//seek failure
							return;
						}

						if (audio_autostopped()) 
						{//音乐己停止
							cur_bookmark_bak=player->bmk_no.current_bmk_no;
							ui_audio_start(player,1);
							audio_seek(track_bmk.play_time);

							player->bmk_no.current_bmk_no=cur_bookmark_bak;
						}
						else 
						{
							audio_seek(track_bmk.play_time);
						}
												
						audio_tts(TTS_SEEK_BOOKMARK, player->bmk_no.current_bmk_no+1);			//seek success
			
						break;

					case PLAY_ALBUM:
						album_bmk.album_bmk_no = player->bmk_no.current_bmk_no ;
						album_bmk.dir_path     =player->start_place.dirpath;
						if (get_album_bmk(&album_bmk) == -1) {
							audio_tts(TTS_SEEK_BOOKMARK, -1);		//seek filure
							return;
						}
						strcpy(player->start_place.path ,album_bmk.audio_path);
						player->start_place.start_time = album_bmk.play_time;
						DBGMSG("Ready  call  audio_stop--1111111111!\n");
						NeuxTimerSetEnabled(nano.tid,FALSE);
						audio_stop();
						ui_audio_start(player,1);
						audio_seek(player->start_place.start_time);						//seek  play time 	
						NeuxTimerSetEnabled(nano.tid,TRUE);
						audio_tts(TTS_SEEK_BOOKMARK, player->bmk_no.current_bmk_no+1);			//seek success
					
						break;
				}
			}
			break;
	}
}


/*seek_time , it will pause when seek to the begining*/
 int 
ui_seek_time (int time, int flags, struct audio_player* player)
{
	struct nano_data* nano = sig_set.nano;
	int  pos;

	//???
	time = time;//speed_rate[player->speed+2];
	
	DBGMSG("Enter  ui_seek_time  Fun!\n");


	if (flags== SEEK_TIME_FORWARD) 
	{						
		DBGMSG("step!\n");
		if ((audio_stat.current_play_time + time) >= audio_stat.total_play_time) 
		{//向前到最后 
			DBGMSG("step!\n");

			//这里销毁那个timer
			if (nano->l_key.l_flag) 			//如果有的话
			{	
				DBGMSG("step!\n");

				DBGMSG("Enter  ui_seek_time  Fun_aaaaaaaaaaaaaaa!\n");
				NeuxTimerSetEnabled(nano->l_key.lid,FALSE);
				nano->l_key.l_flag = 0;
			}
			
			DBGMSG("step!\n");

			if(player->play_mode==PLAY_TRACK)
			{//单曲play 
			
				DBGMSG("step!\n");
				if(player->repeat_mode==STANDARD_PLAY)
				{//标准play 

					DBGMSG("step!\n");
					if (audio_autostopped()) 
					{//停止
						DBGMSG("step!\n");
						DBGMSG("Enter  ui_seek_time   end of music!\n");
						audio_tts(TTS_END_MUSIC, 0);
						music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
						player->start_place.start_time=0;
						
					}
					else
					{	
						DBGMSG("step!\n");
						DBGMSG("Enter  ui_seek_time  Fun_1111111111111!\n");
						audio_seek(audio_stat.total_play_time);	

					
						DBGMSG("step!\n");
						if(Music_IsPaused())
						{
							
							audio_recover(0);
							DBGMSG("step!\n");
						}
						DBGMSG("step!\n");
						
					}
					DBGMSG("step!\n");
				}
				else
				{//重新play 

						DBGMSG("step!\n");
						DBGMSG("Enter  ui_seek_time  Fun_aaaaaaaaaaaaaaaaa!\n");
						audio_seek(audio_stat.total_play_time);	

						DBGMSG("step!\n");

						if(Music_IsPaused())
						{
							DBGMSG("step!\n");

							audio_tts(TTS_END_MUSIC, 0);
							music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
							
						}
						DBGMSG("step!\n");

				}
			
				DBGMSG("step!\n");
			}
			else
			{//album  play 
				DBGMSG("step!\n");
				if(player->repeat_mode==STANDARD_PLAY)
				{//标准play 
#if 1
						DBGMSG("step!\n");
						DBGMSG("Ready  call  audio_stop--22222222222222222!\n");

					if(Music_IsPaused())
					{
						DBGMSG("step!\n");
						audio_seek(audio_stat.total_play_time);		
						audio_tts(TTS_END_MUSIC, 0);	
					}
					else
					{
						DBGMSG("step!\n");
						g_endflg=0;
						audio_tts(TTS_NORMAL_BEEP, 0);
						DBGMSG("Go to next track!\n");
						load_file(NEXT_TRACK, player,1,&pos);
						if(pos==POS_END)
						{
							DBGMSG("step!\n");
							DBGMSG("ui seek time  in  standard play  to  last !\n");
							audio_seek(audio_stat.total_play_time);		
							g_endflg=1;
						}
						else
						{
							DBGMSG("step!\n");
							player->loadflag=0;
							audio_stop();
							player->start_place.start_time=0;
							ui_audio_start(player,1);
							DBGMSG("step!\n");
						}
					}
					

#else 
						if (audio_autostopped()) 
						{//停止
							
							audio_tts(TTS_END_MUSIC, 0);
							player->start_place.start_time=0;
						
						}
						
						else if(IsTopLevel_End())
						{//正在play一个loop 中的最后一首歌曲
						
							DBGMSG("Go to stop!\n");
							audio_seek(audio_stat.total_play_time);	
							
							if(Music_IsPaused())
							{
								
								audio_recover(0);
							
							}
							
						}
						else 
						{//下首 
							if(Music_IsPaused())
							{
								audio_seek(audio_stat.total_play_time);		
								audio_tts(TTS_END_MUSIC, 0);
								
							}
							else
							{
								DBGMSG("Go to next track!\n");
								audio_tts(TTS_NORMAL_BEEP, 0);
								audio_stop();
								load_file(NEXT_TRACK, player,0,&pos);
								player->start_place.start_time=0;
								
								ui_audio_start(player,1);	
							}
						}

#endif					
					
				}
				else  if(player->repeat_mode==TRACK_REPEAT)
				{//要track repeat play 

				
					DBGMSG("step!\n");
					audio_seek(audio_stat.total_play_time);	
					DBGMSG("Ready  call  audio_stop--333333333333333!\n");
					if(Music_IsPaused())
					{
						DBGMSG("step!\n");
						audio_tts(TTS_END_MUSIC, 0);
						DBGMSG("step!\n");		
					}
					
					DBGMSG("step!\n");
						
				}
				else  if(player->repeat_mode==ALBUM_REPEAT)
				{//专辑重复

					DBGMSG("step!\n");
					
					if(Music_IsPaused())
					{
						DBGMSG("step!\n");
						audio_seek(audio_stat.total_play_time);		
						audio_tts(TTS_END_MUSIC, 0);
								
					}
					else
					{
						DBGMSG("step!\n");
						audio_tts(TTS_NORMAL_BEEP, 0);
						DBGMSG("Ready  call  audio_stop--444444444444444444444!\n");
						audio_stop();
						DBGMSG("Go to next track!\n");
#if 1					
						if(Have_Muisc_InTopLevel())
						{//第一层有mp3文件

							DBGMSG("step!\n");
							DBGMSG("have  mp3 file in top level! \n");	
							load_file(NEXT_TRACK, player,0,&pos);
							player->loadflag=0;
							ui_audio_start(player,1);
							
							

						}
						else
						{//第一层没有mp3文件

						  	DBGMSG("step!\n");
							DBGMSG("have  no mp3 file in top level! \n");	
							
						
							if(Load_CurLevel_File(NEXT_TRACK, player))
							{
								player->loadflag=0;
								ui_audio_start(player,1);		
							}
						}


#else					
				
						
						
						
						load_file(NEXT_TRACK, player,0,&pos);
						player->start_place.start_time=0;
						ui_audio_start(player,1);	


#endif						
					}
				}
				else 
				{//所有 专辑重复
					DBGMSG("step!\n");
					if(Music_IsPaused())
					{
						DBGMSG("step!\n");
						audio_seek(audio_stat.total_play_time);		
						audio_tts(TTS_END_MUSIC, 0);
								
					}
					else
					{
						DBGMSG("step!\n");
						audio_tts(TTS_NORMAL_BEEP, 0);
						DBGMSG("Ready  call  audio_stop--55555555555555555!\n");
						audio_stop();
						DBGMSG("Go to next track!\n");
						load_file(NEXT_TRACK, player,1,&pos);
						player->start_place.start_time=0;
						player->loadflag=0;
						ui_audio_start(player,1);	
					}

				}
				
				
				
			}
				
		
		}
		else 
		{
			DBGMSG("step!\n");
			audio_tts(TTS_NORMAL_BEEP, 0);
			audio_seek(audio_stat.current_play_time + time);
		}
		return 0;
	} 
	else 
	{//向后倒退

		DBGMSG("step!\n");
		DBGMSG("Enter  ui_seek_time  Fun_11111111111111111111!\n");

		if ((audio_stat.current_play_time - time) < 0) 
		{
			DBGMSG("step!\n");
			if (nano->l_key.l_flag) 
			{
				NeuxTimerSetEnabled(nano->l_key.lid,FALSE);
				nano->l_key.l_flag = 0;
			}
			DBGMSG("step!\n");
			DBGMSG("Enter  ui_seek_time  Fun_2222222222222222222222!\n");
			player->start_place.start_time=0;
			if (audio_autostopped()) 
			{
				DBGMSG("step!\n");
				audio_seek(player->start_place.start_time=0);	
				ui_audio_start(player,0);
				audio_tts(TTS_BEGIN_MUSIC, 0);
			}
			else
			{
				DBGMSG("step!\n");
				audio_seek(player->start_place.start_time=0);	
				audio_tts(TTS_BEGIN_MUSIC, 0);
				ui_audio_start(player,0);
			}
				
			
		} 
		else 
		{//有时间退
				DBGMSG("step!\n");
				if (audio_autostopped()) 
				{

					DBGMSG("step!\n");
					DBGMSG("Enter  ui_seek_time  Fun_444444444444444444444!\n");
					player->start_place.start_time=0;
					ui_audio_start(player,0);
					audio_tts(TTS_BEGIN_MUSIC, 0);
					
					
						
						
				}
				else
				{	
						DBGMSG("step!\n");
						audio_tts(TTS_NORMAL_BEEP, 0);
						DBGMSG("Enter  ui_seek_time  Fun_55555555555555555555555555!\n");
						audio_seek(audio_stat.current_play_time - time);	
						
				}
		
			
		}
	}	
	
	return 0;
}
	



/* Recover Icon for Music when come to music again */
static void
recover_music_icon (struct nano_data* nano)
{



	DBGMSG("enter recover_music_icon fun!\n");
	
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MUSIC);



	if(audio_stat.stop_flag)
	{// music play 己结束

		DBGMSG("music have stop!\n");

		music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE);
	}
	else
	{//没有结束

		if (!audio_stat.pause_flag) 
		{//没有暂停

			DBGMSG("music play !\n");
			
			music_setConditionIcon( SBAR_CONDITION_ICON_PLAY);
		} 
		else 
		{//暂停

			DBGMSG("music have pause!\n");
			music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE);
		}	
	
	}

		

}

void DesktopScreenSaver(GR_BOOL activate)
{
 
}




/* Signal handler to tell music, TTS(app) has stopped */
void music_tip_voice_sig(int sig)
{
#if 0
	audio_set_tts_stopped();
#endif
	printf("music tip voice signal recieve!\n");

	struct audio_player* player = sig_set.player;

//	printf("[debug]:%s,%d\n", __func__, __LINE__);

	player->is_running = 0;

//	printf("[debug]:%s,%d\n", __func__, __LINE__);

}


void 
music_term_sig (int sig)
{
	DBGMSG("music term sig function handler!\n");
	
	struct audio_player* player = sig_set.player;



	sys_set_music_speaker_volume(player->volume);						//set volume
	sys_set_music_speed(player->speed);									//set speed
	sys_set_music_repeat(player->repeat_mode);							//set repeat mode
	sys_set_music_resume_path((char*)player->start_place.path);			//set start place
	sys_set_music_backupfloder_path((char*)player->start_place.backupfloder);
	sys_set_music_resume_time(audio_stat.current_play_time);			//set start time
	sys_set_music_play_mode(player->play_mode);						//save  play mode

	//set volume of speaker
	#if 0
	int sys_get_music_speaker_volume(void);
	int sys_set_music_speaker_volume(int vol);
	int sys_get_music_headphone_volume(void);
	int sys_set_music_headphone_volume(int vol);
	#endif
	/* Copy config from /tmp to /opt/config */
	//sys_save_tmp_to_disk_music();		
	audio_tts_destroy();
	DBGMSG("Ready  call  audio_stop--3333333333333333!\n");
	audio_stop();

	exit(0);
}


int 
audio_autostopped (void)
{
	if (audio_stat.stop_flag == 1 
		&& audio_stat.pthread_flag == 0 && audio_stat.error_flag==0)
		return 1;
	else 
		return 0;
}


static void 
audio_auto_check (struct audio_player* player)
{
	int pos=0;

		
	if (audio_autostopped()) 
	{// mp3 播放己结束
		audio_join();
		
		
		if(player->check_flag)
		{
			return ;
		}
	
		DBGMSG("audio auto check.play_mode=%d   repeat_mode=%d \n",player->play_mode,player->repeat_mode);


		if(player->play_mode==PLAY_TRACK)
		{//单曲play
		
			switch (player->repeat_mode)
			{
				case STANDARD_PLAY:
					
					if(player->check_flag)
					{
						break;
					}
					player->check_flag=1;
					DBGMSG("audio_auto_check  in PLAY_TRACK \n");
					music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE); 
					DBGMSG("Read  call  audio_pause-333333333333333333333!\n");
					audio_pause(0, 1,0);
					audio_tts(TTS_END_MUSIC, 0);
					player->start_place.start_time=0;
					break;
							

				case TRACK_REPEAT:
				case ALBUM_REPEAT:	
				case ALL_ALBUM_REPEAT:
				case SHUFFLE_PLAY:
					player->start_place.start_time=0;	
					player->loadflag=0;
					ui_audio_start(player,1);
					break;
			}
				
	

		}
		else
		{//Album play

		
		switch (player->repeat_mode)
		{
			case STANDARD_PLAY:
				{// 文件夹的标准play 
#if 1
			

					if(g_endflg)
					{
						
						DBGMSG("STANDARD_PLAY  in PLAY_ALBUM   LOOP END! \n");
						player->check_flag=1;
						audio_tts(TTS_END_MUSIC, 0);
						music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
						player->start_place.start_time=0;
						g_endflg=0;
							
					}
					else
					{

						load_file(NEXT_TRACK, player,1,&pos);
						DBGMSG("STANDARD_PLAY  in PLAY_ALBUM=%d \n",pos);
						if(POS_END==pos)
						{
							DBGMSG("STANDARD_PLAY  in PLAY_ALBUM   LOOP END! \n");
							player->check_flag=1;
							audio_tts(TTS_END_MUSIC, 0);
							music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
							player->start_place.start_time=0;
							
						}
						else 
						{
							
							DBGMSG("STANDARD_PLAY  in PLAY_ALBUM   Next  song! \n");
							player->loadflag=0;
							ui_audio_start(player,1);
						}

						
					}
					
					
					
					


#else 
			
					if(player->check_flag)
					{
						break;
					}

					
					if(IsTopLevel_End())
					{//第一次的mp3己全部play 结束

						DBGMSG("STANDARD_PLAY last track! \n");
						player->check_flag=1;
						music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE); 		  //设置icon(左边第二个)
						audio_tts(TTS_END_MUSIC, 0);
						player->start_place.start_time=0;
						 TopLevel_toFirst();
						load_file(NEXT_TRACK, player,0,&pos);			//seek track forward

							
						
					}
					else
					{
						
						 load_file(NEXT_TRACK, player,0,&pos);			//seek track forward
						 ui_audio_start(player,1);

					}
					 
				
#endif

					
					break;
					
				}

			case TRACK_REPEAT:
				DBGMSG("TRACK_REPEAT  in PLAY_ALBUM \n");
				player->start_place.start_time=0;	
				player->loadflag=0;
				ui_audio_start(player,1);
				break;

			case ALBUM_REPEAT:
				{
						//文件夹重复	
					
					DBGMSG("ALBUM_REPEAT  in PLAY_ALBUM \n");
#if 1					
					if(Have_Muisc_InTopLevel())
					{//第一层有mp3文件

						DBGMSG("have  mp3 file in top level! \n");	
						load_file(NEXT_TRACK, player,0,&pos);
						player->loadflag=0;
						ui_audio_start(player,1);	
						

					}
					else
					{//第一层没有mp3文件

					  	
						DBGMSG("have  no mp3 file in top level! \n");	
						
					
						if(Load_CurLevel_File(NEXT_TRACK, player))
						{
							player->loadflag=0;
							ui_audio_start(player,1);		
						}
						else
						{
							player->check_flag=1;
							audio_tts(TTS_END_MUSIC, 0);
							music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);
							player->start_place.start_time=0;
						}
							
							
						
						
					}

#else 						
					if(load_file(NEXT_TRACK, player,0,&pos)==0)					    //seek track forward
					{//表示没有找到合适的下一首歌曲(只适合选择没有文件的文件夹进行play)

						DBGMSG("ALBUM_REPEAT last track! \n");
						player->check_flag=1;
						music_setConditionIcon( SBAR_CONDITION_ICON_PAUSE); 		  //设置icon(左边第二个)
						audio_tts(TTS_END_MUSIC, 0);
						player->start_place.start_time=0;
					
					}
					else 
					{
						
						ui_audio_start(player,1);	
					}
#endif					
			
					break;
				}

			case ALL_ALBUM_REPEAT:
				{
					//所有文件夹重复
					
					DBGMSG("ALL_ALBUM_REPEAT  in PLAY_ALBUM \n");
					load_file(NEXT_TRACK, player,1,&pos);		
					player->loadflag=0;
					ui_audio_start(player,1);
					break;
				}

			case SHUFFLE_PLAY:

					
					DBGMSG("SHUFFLE_TRACK  in PLAY_ALBUM \n");
					load_file(SHUFFLE_TRACK, player,1,&pos);						//seek track forward
					player->loadflag=0;
					ui_audio_start(player,1);
				break;
			
		}
			

		}
		
		

	}
}


/***************************************

Fun:   按左右键长按，会进行快退，快进.

****************************************/
static void
timer_seek_handler (struct nano_data* nano, struct audio_player* player)
{
	DBGMSG("Timer seek handler\n");

	ui_seek_time(5* 1000, nano->l_key.orient, player);


	nano->l_key.s_flag = 1;			//意味着执行过seek,这里有这么复杂?
}




/***********************************************

Fun:   在mp3模块下按down key ，以切换导航单元 
input:   player 
output:  player->navi_value  导航索引

************************************************/
static int 
key_up_handler(struct audio_player* player)
{
	switch (player->play_mode)
		{
		case PLAY_TRACK:
			{
				if (player->navi_value >= 3)
				{
					player->navi_value = 0;
				} 
				else 
				{
					player->navi_value ++;
				}			
				break;
			}

		case PLAY_ALBUM:
			{
				if (player->navi_value >= 4) 
				{
					player->navi_value = 0;
				}
				else 
				{
					player->navi_value ++;
				}	

// why this?
#if 0
				if(audio_stat.trackflag)
				{//play cdda 的内容
					
					if(player->navi_value>=4)
						player->navi_value=0;

				}
#endif
				
				break;
			}
	}

	printf("TTS_NAVIGATION =%d \n",player->navi_value);
	audio_tts(TTS_NAVIGATION, player->navi_value);		//no need for wait

	sys_set_music_navi(player->navi_value);
	
	return 0;
}


/***********************************************

Fun:   在mp3模块下按down key ，以切换导航单元 
input:   player 
output:  player->navi_value  导航索引

************************************************/
static int 
key_down_handler (struct audio_player* player)
{
	switch (player->play_mode)
		{
		case PLAY_TRACK:
			{
				if (player->navi_value <=0)
				{
	 				player->navi_value = 3;
				} 
				else
				{
	 				player->navi_value --;
				}
				break;
			}

		case PLAY_ALBUM:
				{
					if (player->navi_value <=0) 
					{
						player->navi_value = 4;
					} 
					else 
					{
						player->navi_value--;
					}

// why this ???
#if 0
					if(audio_stat.trackflag)
					{//play cdda 的内容
						
						if(player->navi_value<=0)
							player->navi_value=3;

					}
#endif
					break;
				}
	}

	printf("TTS_NAVIGATION =%d \n",player->navi_value);
	audio_tts(TTS_NAVIGATION, player->navi_value);

	sys_set_music_navi(player->navi_value);
	
	return 0;
}


/********************************************************

Fun:   在mp3模块下，按右键进行不同导航单元 的跳转


*******************************************************/

static int
key_right_handler(struct audio_player* player)
{
	char *path;
	int pos;

	if (player->play_mode == PLAY_TRACK) {//单曲play 
	
		printf("%s, %d:play_mode == PLAY_TRACK!\n", __func__, __LINE__);
		switch (player->navi_value) 
		{
		case NAVI_SEEK_BOOKMARK:
			printf("handler for next bookmark\n");
			ui_bmk_handler(NEXT_BOOKMARK, player);
			break;
		
		case NAVI_SEEK_SECONDS:
			if (audio_autostopped())	//music have stopped 	
				Start_Music_From_Stop();		
			else
				ui_seek_time(30 * 1000, SEEK_TIME_FORWARD, player);
			break;
			
		case NAVI_SEEK_MINUTES:
			if (audio_autostopped()) //music have stopped 	
				Start_Music_From_Stop();
			else
				ui_seek_time(10 * 60 * 1000, SEEK_TIME_FORWARD, player);
			break;

		case NAVI_SEEK_TRACK:
			audio_tts(TTS_NORMAL_BEEP, 0);
			player->loadflag = 0;
			if (player->repeat_mode==ALL_ALBUM_REPEAT||player->repeat_mode==STANDARD_PLAY) {//所有文件夹循环(含子文件夹)
				printf("key_right_handler_NAVI_SEEK_TRACK!\n");	
				load_file(NEXT_TRACK ,player,1,&pos);				//seek track forward	
			} else {//只在第一层的文件中进行切换(loop)
				printf("key_right_handler_NAVI_SEEK_TRACK!\n");	
				load_file(NEXT_TRACK ,player,0,&pos);				//seek track forward
			}
			audio_stop();
			ui_audio_start(player,1);
			break;
	     }
	 }else {//文件夹play 
		printf("%s, %d:play_mode == PLAY_ALAUM!\n", __func__, __LINE__);
		switch (player->navi_value)  {
		case NAVI_SEEK_BOOKMARK:
			printf("handler for next bookmark\n");
			ui_bmk_handler(NEXT_BOOKMARK, player);
			break;
		
		case NAVI_SEEK_SECONDS:
			if (audio_autostopped()) //music have stopped 	
				Start_Music_From_Stop();
			else
				ui_seek_time(30 * 1000, SEEK_TIME_FORWARD, player);
			break;
			
		case NAVI_SEEK_MINUTES:
			if (audio_autostopped()) //music have stopped 	
				Start_Music_From_Stop();
			else
				ui_seek_time(10 * 60 * 1000, SEEK_TIME_FORWARD, player);
			break;

		case NAVI_SEEK_TRACK:
			{//下一首
			
			printf("%s, %d: seek track!\n", __func__, __LINE__);
			audio_tts(TTS_NORMAL_BEEP, 0);
			player->loadflag=0;
		
			if (player->repeat_mode==ALL_ALBUM_REPEAT||player->repeat_mode==STANDARD_PLAY)
			{//所有文件夹循环(含子文件夹)

					printf("repeatmode, all album repeat or standard play!\n");
					printf("%s, %d. step\n!", __func__, __LINE__);
					load_file(NEXT_TRACK ,player,1,&pos);				//seek track forward
					audio_stop();
					ui_audio_start(player,1);
			}
			else if(player->repeat_mode==SHUFFLE_PLAY)
			{
					printf("%s, %d. step\n!", __func__, __LINE__);

					//load_file(SHUFFLE_TRACK, player,1,&pos);	
					load_file(NEXT_TRACK ,player,1,&pos);				//seek track forward	
					audio_stop();
					ui_audio_start(player,1);
			}
			else 
			{//album repeat 
					printf("repeat mode, album repeat!\n");
					printf("%s, %d. step\n!", __func__, __LINE__);
						if(Have_Muisc_InTopLevel())
						{//第一层有mp3文件
							printf("%s, %d. step\n!", __func__, __LINE__);
							printf("have  mp3 file in top level! \n");	
							load_file(NEXT_TRACK, player,0,&pos);
							audio_stop();
							ui_audio_start(player,1);	
						}
						else
						{//第一层没有mp3文件
							printf("%s, %d. step\n!", __func__, __LINE__);
							printf("have  no mp3 file in top level! \n");	
							if(Load_CurLevel_File(NEXT_TRACK, player))
							{
						  		printf("%s, %d. step\n!", __func__, __LINE__);
								audio_stop();
								ui_audio_start(player,1);	
							}
						}
				}	
				break;
			}
		case NAVI_SEEK_ALBUM:
			printf("%s, %d. step\n!", __func__, __LINE__);

			player->loadflag=0;
			audio_tts(TTS_NORMAL_BEEP, 0);
			load_file(NEXT_ALBUM, player,0,&pos);	
			DBGMSG("Ready  call  audio_stop--77777777777777777777777!\n");
			audio_stop();			
			audio_tts(TTS_SEEK_ALBUM, (int)(player->start_place.dirpath));		//tts report switch album
		
			ui_audio_start(player,1);
			break;
	       }	
	 }
	return 0;
}

/********************************************************

Fun:   在mp3模块下，按左键进行不同导航单元 的跳转


*******************************************************/

static int 
key_left_handler(struct audio_player* player)
{

	char *path;
	int pos;
	
	printf("MUSIC: key_left_handler!\n");

	 if(player->play_mode==PLAY_TRACK)
	 {//单曲play 


		switch (player->navi_value)
			{
			case NAVI_SEEK_BOOKMARK:
				printf("handler for pre bookmark\n");
				ui_bmk_handler(PRE_BOOKMARK, player);
				break;
			
			case NAVI_SEEK_SECONDS:
				
				
				if (audio_autostopped()) 
				{//music have stopped 	

						Start_Music_From_Stop();
						
				}
				else
				{

					ui_seek_time(30 * 1000, SEEK_TIME_BACK, player);
				}
				break;
			
				
			case NAVI_SEEK_MINUTES:

			
				if (audio_autostopped()) 
				{//music have stopped 	

					Start_Music_From_Stop();
						
				}
				else
				{
					ui_seek_time(10 * 60 * 1000, SEEK_TIME_BACK, player);
				}
				break;
				

			case NAVI_SEEK_TRACK:
				
					audio_tts(TTS_NORMAL_BEEP, 0);
					 player->loadflag=0;
					if(player->repeat_mode==ALL_ALBUM_REPEAT||player->repeat_mode==STANDARD_PLAY)
					{//所有文件夹循环(含子文件夹)

						
						load_file(PRE_TRACK ,player,1,&pos);		
						
					}
					else
					{//只在第一层的文件中进行切换(loop)
					
						load_file(PRE_TRACK ,player,0,&pos);				

						DBGMSG("Ready  call  audio_stop--aaaaaaaaaaaaaaaaaaaaaaaaaaa!\n");
						
					}
				audio_stop();
				ui_audio_start(player,1);			
				break;


		}
  }

       else
       {//album play 
       
			switch (player->navi_value)
			{
			case NAVI_SEEK_BOOKMARK:
				printf("handler for pre bookmark\n");
				ui_bmk_handler(PRE_BOOKMARK, player);
				break;
			
			case NAVI_SEEK_SECONDS:
				
				if (audio_autostopped()) 
				{//music have stopped 	
					Start_Music_From_Stop();
				}
				else
				{
					ui_seek_time(30 * 1000, SEEK_TIME_BACK, player);
				}
				break;
				
			case NAVI_SEEK_MINUTES:
				if (audio_autostopped()) 
				{//music have stopped 	
					Start_Music_From_Stop();
				}
				else
				{
					ui_seek_time(10 * 60 * 1000, SEEK_TIME_BACK, player);
				}

				break;

			case NAVI_SEEK_TRACK:

					audio_tts(TTS_NORMAL_BEEP, 0);
				//上一首
					{
						player->loadflag=0;
						if(player->repeat_mode==ALL_ALBUM_REPEAT||player->repeat_mode==STANDARD_PLAY)
						{//所有文件夹循环(含子文件夹)

							load_file(PRE_TRACK ,player,1,&pos);	
							audio_stop();
							ui_audio_start(player,1);
							
						}
						else if(player->repeat_mode==SHUFFLE_PLAY)
						{

							//load_file(SHUFFLE_TRACK, player,1,&pos);	
							load_file(PRE_TRACK ,player,1,&pos);	
							audio_stop();
							ui_audio_start(player,1);
								
					
						}
						else 
						{//album repeat 
						

								if(Have_Muisc_InTopLevel())
								{//第一层有mp3文件

									DBGMSG("have  mp3 file in top level! \n");	
									load_file(PRE_TRACK, player,0,&pos);
									audio_stop();
									ui_audio_start(player,1);
									

								}
								else
								{//第一层没有mp3文件

								  	
									DBGMSG("have  no mp3 file in top level! \n");	
									
								
									if(Load_CurLevel_File(PRE_TRACK, player))
									{
										audio_stop();
										ui_audio_start(player,1);		
									}
								}
						}
						
								
						break;
					}

			case NAVI_SEEK_ALBUM:
				//下一个文件夹
				{
					
					
					player->loadflag=0;
					audio_tts(TTS_NORMAL_BEEP, 0);				
					load_file(PRE_ALBUM, player,0,&pos);	

					DBGMSG("Ready  call  audio_stop--bbbbbbbbbbbbbbbbbbbbb!\n");
					audio_stop();
					audio_tts(TTS_SEEK_ALBUM, (int)(player->start_place.dirpath));		//tts report switch album
					ui_audio_start(player,1);			
					break;
				}
		}




       }



	
	return 0;


	
}


/**********************************
fun:  将音乐从stop 状态开启
**********************************/
void Start_Music_From_Stop()
{

		DBGMSG("Enter Start_Music_From_Stop fun!\n");
		
		player.start_place.start_time=0;
		audio_tts(TTS_NORMAL_BEEP, 0);
		ui_audio_start(&player,1);
	
		
		music_setConditionIcon(SBAR_CONDITION_ICON_PLAY);

}

static int 
key_play_stop_handler (void)
{
	DBGMSG("key play stop handler!\n");
	struct audio_player* player = sig_set.player;


	if(audio_stat.stop_flag)
	{// music play 己结束
	
		DBGMSG("music have stop!\n");
		usleep(500);
		player->start_place.start_time=0;
		audio_tts(TTS_NORMAL_BEEP, 0);
		ui_audio_start(player,1);
	
		
		music_setConditionIcon(SBAR_CONDITION_ICON_PLAY);
		

	}
	else
	{

		if (!audio_stat.pause_flag) 
		{
			DBGMSG("Read  call  audio_pause-1111111111111!\n");
			audio_pause(1, 1,0);
			/* This means manual pause the playing */
			player->manual_pause = 1;
			music_setConditionIcon(SBAR_CONDITION_ICON_PAUSE);


		} 
		else 
		{

			DBGMSG("music have play!\n");
			
			
			//audio_recover(0);			do it after vpend
			vprompt_end_recover_audio = 1;
			
			audio_tts(TTS_NORMAL_BEEP, 0);
			player->manual_pause = 0;
			music_setConditionIcon(SBAR_CONDITION_ICON_PLAY);
		

		}	
	
	}

	





	
	

	
	return 0;
}



static void JudgeFileType(int *pretint,char *path)
{
	char *pstr;

	pstr=strlen(path)-1;

	if(strcasestr(path,".mp3") || strcasestr(path,".wav") ||strcasestr(path,"/media/cdda/Track" ))
	{
		*pretint=1;	
	}
	else
	{

		*pretint=0;		
	}

	
	


}


/***************************************

Fun:  检测path的路径是否达到8层

return : 1  达到8层,0没有达到8 层

****************************************/
int   Check_Folder_Level(char *path)
{


	char  *p=path;
	int num=0;


	DBGMSG("Enter  Check_Folder_Level Fun!\n");

	while(*p)
	{

		if(*p=='/')
		{
			num++;
		}
		p++;
		
	}
	
	DBGMSG("Check_Folder_Level Fun_11111111111111111!\n");

	if(num>MAX_FOLDER_LEVEL+2)
	{
		return  1;
	}


	return 0;

}


int IsMusiscFile(char *path)
{
	if(PlextalkIsMusicFile(path) || strstr(path,"/media/cdda/Track" ) || strstr(path, "/media/cdda"))
	{
		DBGMSG("IsMusiscFile  retun 1!\n");
		return 1;
	}
	else
	{
		DBGMSG("IsMusiscFile  retun 0!\n");
		return 0;
	}
}


int Get_Current_Music()
{
	if (StringStartWith(player.start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;

	if(audio_stat.trackflag)
	{

		return 	player.start_place.trackindex;
		
	}
	else  if(player.play_mode==PLAY_TRACK)
	{
		
		
			return file_cursor_curt_count(player.filecur,0);
	}
	else
	{
			return file_cursor_curt_count(player.filecur,1);
	}

	

	
}

int  Get_Total_Music()
{
	if (StringStartWith(player.start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;


	if(audio_stat.trackflag)
	{

		 return player.track_num;
	}
	else  if(player.play_mode==PLAY_TRACK)
	{
		
		return file_cursor_max_count(player.filecur,0);
	}
	else 
	{
		return file_cursor_max_count(player.filecur,1);

	}
	
	
}

int  Get_Total_Album()
{
	if (StringStartWith(player.start_place.path, "/media/cdda"))
		audio_stat.trackflag = 1;
	else
		audio_stat.trackflag = 0;

	if(audio_stat.trackflag)
	{

		 return  0;
	}
	else  if(player.play_mode==PLAY_TRACK)
	{
		
		return   0;
	}
	else 
	{
		return Get_Album_TopLevel(player.filecur);

	}
	
	
}

static int 
key_music_handler (struct nano_data* nano, struct audio_player* player,char *path)
{
	int  selectedcount=0;
	char **selectedfiles;
	int ret=0;
	int flag_ok=0;
	//int  pause_flag=0;
	int i;
	int ret_type , mine;
	char sel_filepath[512];

	
	DBGMSG("Enter  key_music_handler  fun  path=%s---%s!\n",path,player->start_place.path);

	NeuxTimerSetEnabled(nano->tid,FALSE);
#if 1
	
	voice_prompt_abort();		//在进入file list 之前进行发音的中断 
	Break_Music(player, 0,1);

#else

	
	if (audio_stat.stop_flag ==0 &&  audio_stat.pause_flag==0)
	{
		audio_pause(0, 1,0);
		pause_flag=1;
		DBGMSG("key_music_handler   pause !\n");	
	}
#endif

	player->in_file_select = 1;

//	sysfs_write(PLEXTALK_SCALING_SETSPEED, fre_min );

	ShowAllControl(FALSE);

	player->countforsound=0;
	memset(sel_filepath,0x00,sizeof(sel_filepath));

	plextalk_suspend_unlock();
	
	Del_History_Item();
	
	do {


			NhelperFilelistMode(1);

			
		
			ret=NeuxFileDialogOpenEx(path, FL_TYPE_MUSIC, GR_FALSE, &selectedfiles, &selectedcount,Music_onAppMsg,&(player->is_running),folder_contain_music);
			NhelperFilelistMode(0);

    			printf("explorer  ret=%d,selectedcount=%d  patherror=%d\n",ret,selectedcount,player->patherror);
    			
	    		if(ret==MSGBX_BTN_CANCEL)
	    		{
				printf("path is=%s    player->patherror=%d \n", sel_filepath,player->patherror);	
			//	if(player->patherror)
				{
//					if(IsMusiscFile(player->start_place.path)==0)
//			    	{//不是mp3, 或.wav 文件
//			    			printf("not a mp3 file!\n");
//							continue;
//			    	}
					
					 if(!MusicExist(player->start_place.path))
					{//文件不存在
							printf("file not exist!\n");
						     continue;
					}
				}

				printf("%s, %d, step!\n", __func__, __LINE__);
				
				player->patherror=0;
				flag_ok=0;
				break;
	    		}

			printf("%s, %d, step!\n", __func__, __LINE__);	
			if (selectedcount)
			{
				printf("%s, %d, step!\n", __func__, __LINE__);	
#if 0
//				 if(!IsMusiscFile(selectedfiles[0]))
//				{//防止选择的文件不存在


					strcpy(path,PLEXTALK_MOUNT_ROOT_STR);
					selectedcount=0;
					 continue;
				}

#endif
				printf("%s, %d, step!\n", __func__, __LINE__);	

			
				flag_ok=1;
				strcpy(sel_filepath,selectedfiles[0]);
				NeuxFileDialogFreeSelections(&selectedfiles, selectedcount);
			printf("%s, %d, step!\n", __func__, __LINE__);	

				JudgeFileType(&ret_type,sel_filepath);			//1选择是.mp3文件，0表示选择的文件夹

				DBGMSG("explorer return path = %s  file type=%d \n", sel_filepath,ret_type);
				break;
			}
			else
			{
						printf("%s, %d, step!\n", __func__, __LINE__);	

				continue;
			}
			
			
		
	} while (1);

	DBGMSG("Enter  key_music_handler  fun----3333333333333333333333!\n");

	player->in_file_select = 0;
	NeuxWMAppSetCallback(CB_MSG,     Music_onAppMsg);


	
			
	DBGMSG("Enter  key_music_handler  fun-%s ---44444444444444444444444!\n",player->start_place.path);
	
	plextalk_suspend_lock();		
	if(flag_ok==1)
	{//选了文件
		audio_stat.total_play_time=0;
		audio_stat.current_play_time=0;
		nano->process_x=6;
		 nano->process_y=MUSIC_CELL_ICON_Y;
		ShowAllControl(TRUE);
		 
		 player->patherror=0;
		


	
		DBGMSG("Enter  key_music_handler  fun----55555555555555555555555555\n");
		music_grab_key(nano);
		
	
	
		
		recover_music_icon(nano);
		
		SetMedaiIcon(sel_filepath);

//special for bug: bad!!!
		if (StringStartWith(sel_filepath, "/media/cdda"))
			audio_stat.trackflag = 1;
		else
			audio_stat.trackflag = 0;


		DBGMSG("audio_stat.track_flag = %d\n", audio_stat.trackflag);
	
		if(audio_stat.trackflag)
		{
		
			player->track_num=CoolCDDAGetTrackCount();
		}
	
		player->exp = 1;


		DBGMSG("key music return file = %s\n", sel_filepath);
		
		if(set_player_info(sel_filepath, ret_type, player))
		{
				player->start_place.start_time=0;
		}
		



	if(player->play_mode==PLAY_TRACK) 
	{
		player->bmk_no.total_bmk_no=get_track_bmk_maxnum (player->start_place.path);					
		
	}
	else
	{
		player->bmk_no.total_bmk_no=get_album_bmk_maxnum (player->start_place.dirpath);	
	}

	player->bmk_no.current_bmk_no=-1;
		

		DBGMSG("Enter  key_music_handler  fun----777777777777777777777777!\n");
		audio_stop();
		DBGMSG("Enter  key_music_handler  fun----5555555555555555555555!\n");
		
		ui_audio_start(player,1);


		 if(player->play_mode!=PLAY_TRACK)
 		{//
			strcpy(player->start_place.backupfloder,sel_filepath);

			sys_set_music_backupfloder_path(player->start_place.backupfloder);
				
 		}

		
		audio_back_up (player );			//add it by wgp at 2014-1-22
		
		DBGMSG("Enter  key_music_handler  fun----666666666666666666666666666!\n");
	
	}
	else
	{

	
		ShowAllControl(TRUE);

		DBGMSG("Enter  key_music_handler  fun  %s ----aaaaaaaaaaaaaaaaaaaaaaa!\n",player->start_place.path);
		


		if(player->play_info.breakflg && player->play_info.resumeback)
		{
			DBGMSG("Exit Filelist  and  resume music!\n");
			player->play_info.resumeback(0);
		}


		recover_music_icon(nano);
		audio_back_up (player  );

	
	}

		NeuxTimerSetEnabled(nano->tid,TRUE);
		//audio_draw_repeat_mode(nano, player->repeat_mode);
		audio_draw_equalizer(nano, player->effect_value);
	
	return 0;

	


}




static void
fill_elapsed_info (char* buf)
{
	char e_info[128];
	char t_info[128];
	char r_info[128];
	int hour, mins, sec;
	
	int e_time = audio_stat.current_play_time / 1000;
	int r_time = (audio_stat.total_play_time - audio_stat.current_play_time) / 1000;
	int t_time = audio_stat.total_play_time / 1000;
	
	sec = e_time % 60;
	e_time /= 60;
	mins = e_time % 60;
	e_time /= 60;
	hour = e_time % 100;
#if 1
	sprintf(e_info, "%s:\n%02d:%02d:%02d\n", _("elapsed"), hour,mins, sec);
#else
	snprintf(e_info, 64, "%s<%d:%d:%d>", _("elapsed"), hour, mins, sec);
#endif

	sec = r_time % 60;
	r_time /= 60;
	mins = r_time % 60;
	r_time /= 60;
	hour = r_time % 100;
#if 1	
	sprintf(r_info, "%s:\n%02d:%02d:%02d \n", _("remaining"),  hour,mins, sec);
#else
	snprintf(r_info, 64, "%s<%d:%d:%d>", _("remaining"), hour, mins, sec);
#endif

	sec = t_time % 60;
	t_time /= 60;
	mins = t_time % 60;
	t_time /= 60;
	hour = t_time % 100;

#if 1
	sprintf(t_info, "%s:\n%02d:%02d:%02d\n", _("total"),  hour,mins, sec);
#else
	snprintf(t_info, 128, "%s<%d:%d:%d>", _("total"), hour, mins, sec);
#endif

	sprintf(buf, "%s%s%s", e_info, r_info, t_info);

}


static void
fill_track_info (char* buf, struct audio_player* player)
{
	char* title_p = strrchr(player->start_place.path, '/');
	
	if (!title_p)
	{
		DBGMSG("[ERROR]:error of set track info!\n");
		//memset(buf, 0, size);
		return ;
	}

	char* track_name = title_p + 1;
	int track_no = Get_Current_Music();

#if 1
	sprintf(buf, "%s:%d\n%s", _("current track"), track_no, track_name);
#else
	snprintf(buf, size, "%s:%d%s", _("current track"), track_no, track_name);
#endif
}


static void
fill_folder_info (char* buf, struct audio_player* player)
{
	char* album_path = player->start_place.dirpath;

	//这里潜在的问题就是,256可能不够的问题,如果path很长很长

	char album_name[512];
	char src_dir[512];
	int  total_album=0;
	
	/* Get album name */
	
	memset(src_dir, 0, sizeof(src_dir));
	
	//strncpy(src_dir, album_path, 512);
	
	strcpy(src_dir, album_path);
	
	memset(album_name, 0, sizeof(album_name));
	
	int src_size = strlen(src_dir);
	if (*(src_dir + src_size - 1) == '/') {
		*(src_dir + src_size - 1) = 0;
	}
	char* p = strrchr(src_dir, '/');
	int dir_size = p - src_dir + 1;
	int name_size = src_size - dir_size;
	strncpy(album_name, p + 1, name_size);


	int total_index = Get_Total_Music();
	total_album=Get_Total_Album();
#if 1
	sprintf(buf, "%s:%s\n%s:%d\n", _("album name"), album_name, 
		_("total number of tracks"), total_index);
#else
		sprintf(buf, "%s:%s\n%s:%d\n%s:%d\n", _("album name"), album_name, 
		_("total number of tracks"), total_index,_("Total number of albums"),total_album);
#endif
		
}


static void
fill_tmk_info(char* buf, struct audio_player* player)
{
	int bmk_num = player->bmk_no.total_bmk_no;


	
     	if(player->play_mode==PLAY_TRACK) 
     	{
		bmk_num=get_track_bmk_maxnum (player->start_place.path);
		DBGMSG("path=%s\n",player->start_place.path);
     	}
     	else
     	{
		bmk_num=get_album_bmk_maxnum (player->start_place.dirpath);	
		DBGMSG("path=%s\n",player->start_place.dirpath);
     	}

	sprintf(buf, "%s:%d", _("total number of bookmarks"), bmk_num);
}


static void
fill_stamp_info(char* buf, struct audio_player* player)
{
	struct stat64 info;
	
	stat64(player->start_place.path, &info);

	time_t create_time = info.st_ctime;

	struct tm* new_tm = localtime(&create_time);

#if 0
	snprintf(buf, size, "%s:\n%d %s %02d %s %02d %s %02d %s %2d %s %02d %s", _("create date of this file"),
		new_tm->tm_year + 1900, _("year"), 
		new_tm->tm_mon, _("month"), 
		new_tm->tm_mday, _("day"),
		new_tm->tm_hour, _("hours"),
		new_tm->tm_min, _("minutes"),
		new_tm->tm_sec, _("seconds") );
#else
		sprintf(buf, "%s:\n%d%s%02d%s%02d%s \n%02d%s%2d%s%02d%s", _("create date of this file"),
		new_tm->tm_year + 1900, "-", 
		new_tm->tm_mon+1, "-", 
		new_tm->tm_mday, " ",
		new_tm->tm_hour, ":",
		new_tm->tm_min, ":",
		new_tm->tm_sec, "");

#endif	
}


#define MAX_INFO_ITEM	10

static void
key_info_handler (struct nano_data* nano, struct audio_player* player)
{
	int i;
	
	char elapsed_info[512];
	char folder_info[512];
	char track_info[512];
	char bmk_info[512];
	char stamp_info[512];


	int info_item_num = 0;

	/* Fill info item */
	char* music_info[10];
	
	
	DBGMSG("Enter key info handler fun!\n");
	
	g_info_flag=1;
	memset(elapsed_info,0x00,512);
	memset(folder_info,0x00,512);
	memset(track_info,0x00,512);
	memset(bmk_info,0x00,512);
	memset(stamp_info,0x00,512);
	
	
	


//	normal_beep();
	

	NhelperTitleBarSetContent(_("Information")); 	     //还原模块标题字符串
         
	DBGMSG("Enter key info handler fun_1111111111111111 nano->tid=%d!\n",nano->tid);
	
	
	
	
#if 1

		Break_Music(player, 0,0);

		usleep(300000);


#else

	
	 g_music_pause_flag=audio_stat.pause_flag;
	g_music_stop_flag=audio_stat.stop_flag;

	DBGMSG("Enter key info handler fun_aaaaaaaaaaaaaaaaaaa=%d, %d!\n",g_music_stop_flag,g_music_pause_flag);
	if(g_music_stop_flag==0 && g_music_pause_flag==0)
	{

		
		audio_pause(0, 1,0);
	}
#endif

	
	
	
	
	DBGMSG("Enter key info handler fun_bbbbbbbbbbbbbbbbbbbbbb!\n");
	/* Create Info id */
	nano->info_id = info_create();

	DBGMSG("Enter key info handler fun_22222222222222222222!\n");



	/* Elapse info item */
	
	fill_elapsed_info(elapsed_info);
	music_info[info_item_num] = elapsed_info;
	info_item_num ++;

	DBGMSG("Enter key info handler fun_333333333333333333333!\n");

	/* Track info item */
	
	fill_track_info(track_info, player);			// File elapse Information Group
	music_info[info_item_num] = track_info;
	info_item_num ++;

	
	
	DBGMSG("Enter key info handler fun_44444444444444444444444444!\n");

	if (player->play_mode == PLAY_ALBUM) 
	{
		/* Folder info item */
		
		
		
		fill_folder_info(folder_info, player);			//  Folder Information Group
		music_info[info_item_num] = folder_info;
		info_item_num ++;

		DBGMSG("Infor   play_mode PLAY_ALBUM =%s !\n",folder_info);
	}

	/* Bookmark info item */
#if 1	
	fill_tmk_info(bmk_info, player);
	music_info[info_item_num] = bmk_info;
	info_item_num ++;
#endif

#if 1
	DBGMSG("Enter key info handler fun_55555555555555555555555555!\n");

	/* Time stamp info item */
	
	fill_stamp_info(stamp_info, player);
	music_info[info_item_num] = stamp_info;
	info_item_num ++;


	DBGMSG("Enter key info handler fun_666666666666666666666666666666666!\n");
	
//	for (i = 0; i < info_item_num; i++)
//	{
	
//		
	//	DBGMSG("music info =%s \n",music_info[i]);
//	}

#endif

	/* Start info */
	info_start (nano->info_id, music_info, info_item_num, &(player->is_running));

	info_destroy (nano->info_id);

	/* Recover timer and grab key */
	music_grab_key(nano);


	DBGMSG("Enter key info handler fun_77777777777777777777777777!\n");
		
	g_info_flag=0;

	 if(!MusicExist(player->start_place.path))
	 {//退	出info后，playing 的文件不存在
	 
	 		key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);

	 }
	
	NhelperTitleBarSetContent(GetAlbum_Name(player->start_place.dirpath));


	
#if 1

		
	if(player->play_info.breakflg && player->play_info.resumeback)
	{
		DBGMSG("Exit infomation  and  resume music!\n");
		player->play_info.resumeback(0);
	}


#else




		audio_stat.pause_flag= g_music_pause_flag;
		audio_stat.stop_flag=g_music_stop_flag;
	
	DBGMSG("Enter key info handler fun_999999999999999999999999=%d,%d!\n",g_music_stop_flag,g_music_pause_flag);

	if(g_music_stop_flag==0 && g_music_pause_flag==0)
	{

		
		audio_recover(0);
	}
#endif

	recover_music_icon (nano);

	DBGMSG("Enter key info handler fun_888888888888888888888!\n");

}



/*************************************

***********************************/
static void
hotplug_sda_remove_handler (struct nano_data* nano, struct audio_player* player)
{



	if (StringStartWith(player->start_place.path, "/media/sd")) 

	{
		DBGMSG("need to stop present playback.\n");
		audio_stop();
		

		
		key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);
	}

	if (StringStartWith(player->start_place.path, "/media/sr")) 

	{
		DBGMSG("need to stop present playback.\n");
		audio_stop();
		

		
		key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);
	}

}


/*************************************

***********************************/
static void
hotplug_mmcblk_remove_handler (struct nano_data* nano, struct audio_player* player)
{



	if (StringStartWith(player->start_place.path, "/media/mmcblk1") ) 

   {
		DBGMSG("need to stop present playback.\n");
		audio_stop();
		
		
	/*	while(!is_sdcard_mounted()) 
		{
			usleep(50000);
		}*/
		
		key_music_handler(nano, player,PLEXTALK_MOUNT_ROOT_STR);
	}
}

/***************************************
Fun:   用于判断是否有长按键的定时器
*****************************************/
static void Music_OnTimerLongPressKey(WID__ wid)
{

       DBGMSG("have   left/right key long pressed! .\n");
	timer_seek_handler(&nano, &player);	

}	



/**********************************************
fun:  music 界面刷新的定时器

**********************************************/
 static void  Music_OnTimer_PAINT(WID__ wid)
{

	//DBGMSG("Eenter timer_handler Fun!\n");
	
	
	    
	static   int entry_flg;
	static  unsigned long  old_total_time;
	



	nano.focusid=GrGetFocus();
		

	if(g_info_flag)
	{//表示己进入information中 
		return ;
	}

	if(audio_stat.suspendflag!=old_suspendflag)
	{
		old_suspendflag=audio_stat.suspendflag;
		
			if(audio_stat.suspendflag)
			{
				DBGMSG("enable  suspend!\n");
				plextalk_suspend_unlock();
				//NhelperReschduleOffTimer(1);


			}
			else
			{
				DBGMSG("disable  suspend!\n");
				plextalk_suspend_lock();
				//NhelperReschduleOffTimer(0);
			}
	}
 

	if(enter_filelist==1 && player.is_running==0)
	{//当play的music有错，并发音结束，才会进入file list 
	
			char selpath[512];
			DBGMSG("paint music is error=%s!\n",player.start_place.dirpath);
			strcpy(selpath,player.start_place.dirpath);   //等TTS发音结束后再进入filelist 
			  enter_filelist=0;
			  audio_stat.error_flag =0;
			 key_music_handler(&nano, &player,selpath);	
			
	
	}


	if (audio_stat.error_flag && enter_filelist==0) 
	{
		
	
		if(strstr(player.start_place.path,"/media/mmcblk0p2"))
	      	{//打开内卡的内容
	      	
	      		audio_tts(TTS_READ_MEDIA_ERROR1,0);
	      	}
		else
		{
			audio_tts(TTS_READ_MEDIA_ERROR,0);
		}
		 
		 DBGMSG("player.countforsound=%d   %d    %d\n",player.countforsound,player.manual_pause,player.in_file_select);
		  enter_filelist=1;
	 			 
		 	 
	}
	else
	{
			audio_auto_check(&player);	
	}

#if 1


	if(player.countforsound>0  && player.play_info.breakflg && nano.menuflag==0 &&  player.in_file_select ==0)
	{//处理中断发音，没有结束,就自动恢复(并不在menu中,且不在Filelist 中)
	
			player.countforsound--;
			
			if(player.countforsound==0 && player.play_info.resumeback)
			{
				DBGMSG("Auto restore  Playing!\n");
				
		      		player.play_info.resumeback(0);
		      		player.countforsound=-1;
		      		player.is_running=0;
		      		
			}
	}
		




#else

	//DBGMSG("player.countforsound=%d   %d    %d\n",player.countforsound,player.manual_pause,player.in_file_select);
	
	if(player.countforsound>0 && player.manual_pause==0 && player.in_file_select==0  && nano.menuflag ==0)
	{//表示之前有TTS发音
		
		player.countforsound--;
		
		DBGMSG("paint _11111111111111111111 player.countforsound=%d\n",player.countforsound);
		
		if(player.countforsound==0)
		{//表示TTS发音没有收到结束信号在些，在此进行恢复

			DBGMSG("paint _2222222222222222222222\n");
		
			player.is_running=0;
			
			if(nano.playingflag )
			{
				DBGMSG(" force  recover  music playing in Music_OnTimer_PAINT fun!\n");
				audio_recover(0);		
				nano.playingflag=0;
				player.countforsound=-1;
			}

		}

	}

	

	if(nano.recover_flag  &&  player.is_running==0 &&player.manual_pause==0)
	{//表示从menu中退出并TTS没有发音，要回复music 的状态

		DBGMSG("PAINT: %d ,%d,%d,%d,%d!\n",nano.recover_flag,player.is_running,player.manual_pause,g_music_stop_flag,g_music_pause_flag);
		if(g_music_stop_flag==0 && g_music_pause_flag==0)
		{
			DBGMSG("PAINT audio_recover!\n");
			
			audio_recover(0);				
		}
					
		nano.recover_flag=0;
	}
#endif

	
	if(entry_flg==0)
	{

		entry_flg=1;
		
		ShowAllControl(TRUE);
		NeuxTimerSetControl(nano.tid, TIMER_PERIOD,-1);	
		
	}

	
	audio_draw_current_time(&nano, audio_stat.current_play_time / 1000);			//当前时间
	
	if(audio_stat.current_play_time >500)
	{
		player.loadflag=1;
	}

//	if(old_total_time!=(int)audio_stat.total_play_time)
//	{
		//DBGMSG("old total time:%d, total play time:%d\n",old_total_time,audio_stat.total_play_time);
		
		audio_draw_total_time(&nano, audio_stat.total_play_time / 1000);				//总时间 

	
		
//		old_total_time=(int)audio_stat.total_play_time;
		//DBGMSG("old total time1111:%d \n",old_total_time);
//	}
	
	
	audio_draw_progress_bar(&nano, audio_stat.current_play_time, audio_stat.total_play_time, 50);
		



	return 0;
}


/*******************************************
Fun: 进入music模块时，显示一下"Music" 
********************************************/
#define LOGO_MUSIC_Y_POS	24;
static void Music_show_logo (struct nano_data* nano)
{
	GR_GC_ID gc;
	int font_id;
	int w, h, b;
	int x, y;
	const char *logo_str = _("MUSIC");
	
	gc = GrNewGC();
	font_id = GrCreateFont((GR_CHAR*)sys_font_name, /*sys_font_size*/24, NULL);
	GrSetGCUseBackground(gc ,GR_FALSE);
	GrSetGCForeground(gc, theme_cache.foreground);
	GrSetGCBackground(gc, theme_cache.background);
	GrSetFontAttr(font_id, GR_TFKERNING, 0);
	GrSetGCFont(gc, font_id);
	
	GrGetGCTextSize(gc, logo_str, strlen(logo_str), MWTF_UTF8, &w, &h, &b);
	x = (MAINWIN_WIDTH - w) / 2;
	y = (MAINWIN_HEIGHT - h) / 2;
	
	GrText(NeuxWidgetWID(nano->form), gc, x, y, logo_str, strlen(logo_str), MWTF_UTF8 | MWTF_TOP);

	GrFlush();	
}	


/*******************************************
Fun: 进入music模块时，显示一下"Music" 
********************************************/
static void Music_show_wait (struct nano_data* nano)
{



	widget_prop_t wprop;
	label_prop_t lbprop;
	GR_GC_ID gc;
	int font_id;
	int x=40;
	int y=24;




	DBGMSG("Music  Enter Music_show_logo\n");





	gc=GrNewGC();



	font_id = GrCreateFont((GR_CHAR*)sys_font_name, /*sys_font_size*/16, NULL);
	GrSetGCUseBackground(gc ,GR_FALSE);
	GrSetGCForeground(gc,theme_cache.foreground);
	GrSetGCBackground(gc,theme_cache.background);
	GrSetFontAttr(font_id , GR_TFKERNING, 0);
	GrSetGCFont(gc, font_id);




	if (!strncmp(g_config->setting.lang.lang, "zh_CN", 5))
	{ //中文简体
		
		x=20;				
	} 
	else if (!strncmp(g_config->setting.lang.lang, "hi_IN", 5)) 
	{
		x=20;	
	} 
	else if (!strncmp(g_config->setting.lang.lang, "en_US", 5)) 
	{
			
	} else if (!strncmp(g_config->setting.lang.lang, "zh_TW", 5)) 
	{ //中文繁体，big5
		x=20;						
	}
	else
	{
		x=20;
	}
						

	
	
	
	GrText(NeuxWidgetWID(nano->form), gc, x,y,  _("Please wait..."), strlen(_("Please wait...")), MWTF_UTF8 | MWTF_TOP);

	GrFlush();


	audio_tts(TTS_PLEASE_WAIT, 0);		//set bmk success

}	


/*Init gettext*/
static void 
music_gettext_init (void)
{

	/* 更新语言设置(共),注意加锁 */
	CoolShmReadLock(g_config_lock);
	plextalk_update_lang(g_config->setting.lang.lang, "music");
	CoolShmReadUnlock(g_config_lock);

}





void UpdateFormControl(struct nano_data* nano)
{	

	widget_prop_t wprop;
	label_prop_t lbprop;
	WIDGET *line1;




	NeuxWidgetGetProp(nano->form, &wprop);		//得到窗口的属性 
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->form, &wprop);             //设置窗口的属性 
	NeuxFormShow(nano->form, TRUE);
	//NxSetWidgetFocus(nano->form);



	NeuxWidgetGetProp(nano->psymbol, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->psymbol, &wprop);
	
	NeuxThemeIconSetPic(nano->psymbol, THEME_ICON_MUSIC_SYMBOL);
	NeuxWidgetShow(nano->psymbol, TRUE);
	
	

	NeuxWidgetGetProp(nano->lsong_name, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lsong_name, &wprop);
	audio_draw_title(nano, player.start_place.path);
	NeuxWidgetShow(nano->lsong_name, TRUE);




	NeuxWidgetGetProp(nano->lcur_time, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lcur_time, &wprop);
	NeuxWidgetShow(nano->lcur_time, TRUE);



	NeuxWidgetGetProp(nano->ltotal_time, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->ltotal_time, &wprop);
	NeuxWidgetShow(nano->ltotal_time, TRUE);

#if 1	


	NeuxWidgetGetProp(nano->pprocess, &wprop);
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->pprocess, &wprop);
	
	NeuxThemeIconSetPic(nano->pprocess, THEME_ICON_MUSIC_POLE);
	NeuxWidgetShow(nano->pprocess, TRUE);

#else
	NeuxWidgetGetProp(nano->line, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->line, &wprop);

	NeuxWidgetShow(nano->line, TRUE);

#endif

	


	NeuxWidgetGetProp(nano->pcell, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->pcell, &wprop);	
	NeuxThemeIconSetPic(nano->pcell, THEME_ICON_MUSIC_CELL);
	NeuxWidgetMove(nano->pcell, nano->process_x, nano->process_y);
	
	NeuxWidgetShow(nano->pcell, TRUE);	
	
	


			//audio_draw_current_time(&nano, audio_stat.current_play_time / 1000);			//当前时间
			//audio_draw_total_time(&nano, audio_stat.total_play_time / 1000);				//总时间 
			//audio_draw_progress_bar(&nano, audio_stat.current_play_time, audio_stat.total_play_time, 50);

	


	NeuxWidgetGetProp(nano->prepeat_mode, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->prepeat_mode, &wprop);
	audio_draw_repeat_mode(nano, player.repeat_mode);
	



	NeuxWidgetGetProp(nano->lplay_mode, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lplay_mode, &wprop);
	//NeuxWidgetShow(nano->lplay_mode, TRUE);
	audio_draw_equalizer(nano, player.effect_value);
	
}


int 
music_ui_init (struct nano_data* nano, struct audio_player* player)
{	
	

	widget_prop_t wprop;
	label_prop_t lbprop;
	WIDGET *line1;
	DIR* t_dir;
	
	
	
	DBGMSG("Music  Enter music_ui_init fun.\n");

	memset(fre_min,0x00,sizeof(fre_min));
	memset(fre_max,0x00,sizeof(fre_max));
		
	
//	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "userspace");
//	sysfs_read(PLEXTALK_SCALING_MIN_FREQ, fre_min,7);
//	sysfs_read(PLEXTALK_SCALING_MAX_FREQ, fre_max,7);
	
	DBGMSG("fre_min=%s.\n",fre_min);
	DBGMSG("fre_min=%s.\n",fre_max);
	DBGMSG("Music  Enter music_ui_init fun--1111111111111111.\n");
	
	
	
	
	NhelperTitleBarSetContent( _("Music"));						 //设置title 
	NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MUSIC); 		  //设置icon(左边第一个)

  
	music_setConditionIcon(SBAR_CONDITION_ICON_PLAY); 		  //设置icon(左边第二个)
	
	NhelperStatusBarSetIcon(SRQST_SET_MEDIA_ICON, SBAR_MEDIA_ICON_INTERNAL_MEM); 		 	 //设置icon(左边第三个)


	

	DBGMSG("Music  Enter music_ui_init fun--222222222222222222222\n");
	/* Init global sturct for signal handler */
	sig_set.player = (void*)player;
	sig_set.nano   = (void*)nano;




	DBGMSG("Music  Enter music_ui_init fun--333333333333333333333\n");



//-------------------------------------------

		nano->psymbol=NeuxPictureNew(nano->form,6,3+DISTANCE_Y,18,22, 0, NULL);
									
		NeuxWidgetGetProp(nano->psymbol, &wprop);
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano->psymbol, &wprop);

		NeuxWidgetShow(nano->psymbol, FALSE);
		NeuxThemeIconSetPic(nano->psymbol, THEME_ICON_MUSIC_SYMBOL);

		DBGMSG("Music  Enter music_ui_init fun--88888888888888888888888888\n");
//---------------------------------------------



	nano->lsong_name= NeuxLabelNew(nano->form, 25, 3+DISTANCE_Y, 130, 20 , "test.mp3");

	NeuxWidgetGetProp(nano->lsong_name, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lsong_name, &wprop);

	NeuxLabelGetProp(nano->lsong_name, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_LEFT;
	lbprop.transparent = FALSE;
	NeuxLabelSetProp(nano->lsong_name, &lbprop);

	NeuxWidgetSetFont(nano->lsong_name, sys_font_name, 12);
	NeuxLabelSetScroll(nano->lsong_name, TITLEBAR_SCROLL_TIME);

	NeuxWidgetShow(nano->lsong_name, FALSE);

	DBGMSG("Music  Enter music_ui_init fun--44444444444444444444\n");
	
//----------------------------

	nano->lcur_time= NeuxLabelNew(nano->form, 10, 27+DISTANCE_Y, 60, 16 , "00:00:00");

	NeuxWidgetGetProp(nano->lcur_time, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lcur_time, &wprop);

	NeuxLabelGetProp(nano->lcur_time, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_LEFT;
	lbprop.transparent = FALSE;
	NeuxLabelSetProp(nano->lcur_time, &lbprop);
	NeuxWidgetSetFont(nano->lcur_time, sys_font_name, 12);			//设定为12号字体
	//NeuxLabelSetScroll(nano->lcur_time, TITLEBAR_SCROLL_TIME);
	NeuxWidgetShow(nano->lcur_time, FALSE);

	DBGMSG("Music  Enter music_ui_init fun--55555555555555555555555555\n");
//------------------------------------

	nano->ltotal_time= NeuxLabelNew(nano->form, 100, 27+DISTANCE_Y, 60, 16 , "00:00:00");

	NeuxWidgetGetProp(nano->ltotal_time, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->ltotal_time, &wprop);

	NeuxLabelGetProp(nano->ltotal_time, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_LEFT;
	lbprop.transparent = FALSE;
	NeuxLabelSetProp(nano->ltotal_time, &lbprop);
	NeuxWidgetSetFont(nano->ltotal_time, sys_font_name, 12);			//设定为12号字体
	//NeuxLabelSetScroll(nano->ltotal_time, TITLEBAR_SCROLL_TIME);
	NeuxWidgetShow(nano->ltotal_time, FALSE);

	DBGMSG("Music  Enter music_ui_init fun--66666666666666666666666\n");
//------------------------------------

#if  1
		nano->pprocess=NeuxPictureNew(nano->form,10 ,55+DISTANCE_Y,140,2, 0, NULL);
									
		NeuxWidgetGetProp(nano->pprocess, &wprop);
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano->pprocess, &wprop);

		NeuxWidgetShow(nano->pprocess, FALSE);
		NeuxThemeIconSetPic(nano->pprocess, THEME_ICON_MUSIC_POLE);

		DBGMSG("Music  Enter music_ui_init fun--99999999999999999999\n");
#else

	nano->line = NeuxLineNew(nano->form, 10, 55, 140, 2);

	NeuxWidgetGetProp(nano->line, &wprop);
	wprop.bgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->line, &wprop);

	NeuxWidgetShow(nano->line, FALSE);

#endif


//---------------------------------------------

		nano->pcell=NeuxPictureNew(nano->form,6 ,MUSIC_CELL_ICON_Y+DISTANCE_Y,13,13, 0, NULL);
									
		NeuxWidgetGetProp(nano->pcell, &wprop);
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano->pcell, &wprop);

		
		NeuxThemeIconSetPic(nano->pcell, THEME_ICON_MUSIC_CELL);
		NeuxWidgetShow(nano->pcell, FALSE);
		DBGMSG("Music  Enter music_ui_init fun--aaaaaaaaaaaaaaaaaaaaaa\n");

		
		
//------------------------------------
		nano->prepeat_mode=NeuxPictureNew(nano->form,20 ,66+DISTANCE_Y,19,19, 0, NULL);
		NeuxWidgetGetProp(nano->prepeat_mode, &wprop);
		wprop.bgcolor = theme_cache.background;
		NeuxWidgetSetProp(nano->prepeat_mode, &wprop);
		
		NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT0);
		NeuxWidgetShow(nano->prepeat_mode, FALSE);
		DBGMSG("Music  Enter music_ui_init fun--bbbbbbbbbbbbbbbbbbbbbbbbbb\n");

	

		
//---------------------------------------------
	nano->lplay_mode= NeuxLabelNew(nano->form, 80, 66+DISTANCE_Y, 80, 30 , "normal");

	NeuxWidgetGetProp(nano->lplay_mode, &wprop);
	wprop.fgcolor = theme_cache.foreground;
	wprop.bgcolor = theme_cache.background;
	NeuxWidgetSetProp(nano->lplay_mode, &wprop);

	NeuxLabelGetProp(nano->lplay_mode, &lbprop);
	lbprop.autosize = FALSE;
	lbprop.align = LA_CENTER;
	lbprop.transparent = FALSE;
	NeuxLabelSetProp(nano->lplay_mode, &lbprop);
	NeuxWidgetSetFont(nano->lplay_mode, sys_font_name, 12);
//	NeuxLabelSetScroll(nano->lplay_mode, TITLEBAR_SCROLL_TIME);
	NeuxWidgetShow(nano->lplay_mode, FALSE);

	DBGMSG("Music  Enter music_ui_init fun--77777777777777777777777777\n");


	
		t_dir = opendir(PLEXTALK_SETTING_DIR);
		if (!t_dir) 
		{
				DBGMSG("No such directory , build it!\n");
			
			
			if (mkdir(PLEXTALK_SETTING_DIR, 0755) == -1) 
			{
				DBGMSG("build error!\n");
				
			} 
			else 
			{


				Music_Default_Set(player);	

				
				DBGMSG("build new directory success!\n");
			}
		}



	nano->exp_pro = 0;
	DBGMSG("Music  Enter music_ui_init fun--cccccccccccccccccccccccc\n");
	nano->tid = NeuxTimerNew(nano->form, 500 /*TIMER_PERIOD*/,-1);			//创建一个进行界面刷新的定时器
	NeuxTimerSetCallback(nano->tid, Music_OnTimer_PAINT);


//	nano->tsetmusic= NeuxTimerNew(nano->form, 2000,-1);						//创建一个进行界面刷新的定时器
//	NeuxTimerSetCallback(nano->tsetmusic, OnTimer_SetMusic_Para);
	

	
	nano->l_key.lid = NeuxTimerNew(nano->form, 1000,-1);						//800 创建一个定时器进行按键长短的计时
	NeuxTimerSetCallback(nano->l_key.lid, Music_OnTimerLongPressKey);
	NeuxTimerSetEnabled(nano->l_key.lid,FALSE);
	

	audio_player_init(nano, player);
	
	

//	printf("[DEBUG]:%s,%d\n", __func__, __LINE__);
	
	/* This is for key long/short pressed jugement*/
	nano->l_key.l_flag = 0;	
	nano->l_key.s_flag = 0;

/* register signal handler */
	//menu_notofy_receive_cmd(receive_cmd);
	signal(SIGTERM, music_term_sig);
	DBGMSG("Music  Enter music_ui_init fun--dddddddddddddddddddddd\n");
	return 0;
}






static void signal_handler(int signum)
{
	WPRINT("------- signal caught:[%d] --------\n", signum);
	if ((signum == SIGINT) || (signum == SIGTERM) || (signum == SIGQUIT))
	{
		CoolShmPut(PLEXTALK_GLOBAL_CONFIG_SHM_ID);
		WPRINT("------- global config destroyed --------\n");
		NeuxWMAppStop();
		WPRINT("------- NEUX stopped            --------\n");
		exit(0);
	}
}


//===================================================

static
void signalInit(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sa, NULL)){}
    if (sigaction(SIGTERM, &sa, NULL)){}
    if (sigaction(SIGQUIT, &sa, NULL)){}
}


//==========================================

static void Music_initApp(struct nano_data* nano)
{
	
	//signal_init();初始化的信号处理

	plextalk_global_config_open();		/* 打开共享内存配置文件(共) */
	
	plextalk_update_sys_font(getenv("LANG"));			/* 获取系统字体信息 (共)*/	
	app_volume_ctrl("music");


	plextalk_set_menu_disable(1);

	/* 所有对g_config的引用，一定要加读锁，如下所示 */
	CoolShmReadLock(g_config_lock);	
	/* 初始化主题主题信息(共) */

	NeuxThemeInit(g_config->setting.lcd.theme);
	/* 调用完成后，直接在全局变量里面得到了主题信息 */
	/*
	 extern struct plextalk_theme theme_cache;
    */

	CoolShmReadUnlock(g_config_lock);


	music_gettext_init();
	g_info_flag=0;
	
	/* Init TTS */
	player.is_running = 0;
	signal(SIGVPEND, music_tip_voice_sig);
	audio_tts_init(&(player.is_running));
	plextalk_update_tts(getenv("LANG"));
	
	vprompt_cfg.volume = g_config->volume.guide;
	vprompt_cfg.speed	  = g_config->setting.guide.speed;
	
	RegttsSingal();
			

}






 




static void   Music_OnWindowKeyup(WID__ wid,KEY__ key)
{


	  DBGLOG("Music   OnWindowKeyup=%d,%d,%d,%d,%d\n",g_info_flag,nano.menuflag,player.in_file_select,player.loadflag,audio_stat.pause_flag);

#if 1
if (NeuxAppGetKeyLock(0))
{
	DBGLOG("KEY UP LOCK UP!\n");
	
	  if (!key.hotkey) 
	  {
		   voice_prompt_abort();
		   voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		   voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		      tts_lang, tts_role, _("Keylock"));
	  }
  	return;
 }
 
if(g_info_flag||nano.menuflag||player.in_file_select)
{
	DBGLOG("Music_OnWindowKeyup_11111111111111!\n");
	return;
}
     
    if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
    	{//在help 界面,不进行按键的处理

    		DBGLOG("Music_OnWindowKeyup_222222222222222!\n");
		return;
    	}


    if(player.loadflag==0 &&  audio_stat.pause_flag==0 )
    	{
    		DBGLOG("Music_OnWindowKeyup_3333333333333333=%d!\n",nano.l_key.l_flag);


		if(key.ch==MWKEY_RIGHT||key.ch==MWKEY_LEFT)
		{

			if (nano.l_key.l_flag) 
			{
				nano.l_key.l_flag = 0;	
				
				NxSetTimerEnabled(nano.l_key.lid,FALSE);		
						
			}
		}


	
    		

    		
		return;	
    	}

#endif

      switch (key.ch)
    {


      case  MWKEY_RIGHT:
      	{
		DBGLOG("Music_OnWindowKeyup_44444444=%d!\n",nano.l_key.l_flag);
		if (nano.l_key.l_flag) 
		{
			nano.l_key.l_flag = 0;	
			DBGMSG("I Disable  the timer!!!\n");
			NxSetTimerEnabled(nano.l_key.lid,FALSE);		
			
			
			if (nano.l_key.s_flag) 
			{//己达到长按
				
				DBGMSG("not handler as short press!\n");
			} 
			else 
			{
				DBGMSG("handler as short press!\n");
				key_right_handler(&player);
			}
		}
		break;
       }

 
      
       case  MWKEY_LEFT:
       {
	     if (nano.l_key.l_flag) 			//说明timer还存在
		{		
			nano.l_key.l_flag = 0;
			
			NxSetTimerEnabled(nano.l_key.lid,FALSE);
			
			
			if (nano.l_key.s_flag) 
			{//己达到长按
			
				DBGMSG("not handler as short press!\n");
			} 
			else 
			{
				DBGMSG("handler as short press!\n");
				key_left_handler(&player);
			}
			
		}
		break;
	
       }


	
      	}


}



static void
Music_OnWindowKeydown(WID__ wid,KEY__ key)
{

	char    selpath[512];
	int len=0;
	int hp_online;
	static int volume_maxflag[2]={0};
	int *pvolflag=NULL;
	int volume_val=0;
	
    DBGLOG("Music   OnWindowKeydown=%d %d  %d\n",nano.menuflag,g_info_flag,player.play_info.breakflg);





if (NeuxAppGetKeyLock(0))
{
	  if (!key.hotkey) 
	  {
		   voice_prompt_abort();
		   voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		   voice_prompt_string2_ex2(0, PLEXTALK_OUTPUT_SELECT_DAC, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
		      tts_lang, tts_role, _("Keylock"));
	  }
  	return;
 }
 

if(g_info_flag || nano.menuflag || player.in_file_select)
{
	return;
}

   if(NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
    	{//在help 界面,不进行按键的处理
    		
		return;
    	}


      if(player.loadflag==0 &&  audio_stat.pause_flag==0 )
    	{
    		audio_tts(TTS_ERROR_BEEP,0);
		return;	
    	}


      if(player.play_info.breakflg)
      	{

		voice_prompt_abort();
		if(player.play_info.resumeback)
      		{
      			DBGMSG("fd_read  resume  music !\n");
      			player.play_info.resumeback(0);
      		}
		
      	}


      
     
    switch (key.ch)
    {

 
    case VKEY_MUSIC:
    	{//进行文件的选择
    		
			
		 DBGLOG("Music-----have music key  pressed =%s\n",player.start_place.dirpath);
		 memset(selpath,0,sizeof(selpath));
		 strcpy(selpath,player.start_place.dirpath);
	//	 len=strlen(selpath);
		 
	//	  selpath[len-1]=0;						//去掉最后一个"/" 
		 key_music_handler(&nano, &player,selpath);
		 DBGLOG("Music-----Eixt    music key  pressed\n");
		 break;
    	}
   case  MWKEY_UP:
     {
		key_up_handler(&player);
		break;
   	}
      case  MWKEY_DOWN:
     {
		key_down_handler(&player);
		break;
   	}
      case  MWKEY_RIGHT:
      	{
      		
		if(nano.l_key.lid )
		{	
			NeuxTimerSetEnabled(nano.l_key.lid,TRUE);
			
		}
		
		
		nano.l_key.l_flag = 1;
		nano.l_key.s_flag = 0;
		nano.l_key.orient = SEEK_TIME_FORWARD;
		DBGMSG("music have MWKEY_RIGHT  pressed  !\n");
		break;
      		
      	}
       case  MWKEY_LEFT:
       {

       	if(nano.l_key.lid )
		{	
			NeuxTimerSetEnabled(nano.l_key.lid,TRUE);
			
		}	
		
		nano.l_key.l_flag = 1;									
		nano.l_key.s_flag = 0;
		nano.l_key.orient = SEEK_TIME_BACK;
		DBGMSG("music have MWKEY_LEFT  pressed  !\n");
		break;		

       }
       case MWKEY_ENTER:
       {
		DBGLOG("Music-----have MWKEY_ENTER pressed \n");	
		key_play_stop_handler();	
		break;
       		
       }
       
       case MWKEY_MENU:
       {
       	
       	
	 	DBGLOG("Music-----have menu pressed \n");		
		 break;
       }
        case MWKEY_INFO:
        {

			
	 	 key_info_handler(&nano, &player);
	 
	 	DBGLOG("Music----have exit info fun callled! \n");		

   		 break;
   
        }
#if 0       
    case MWKEY_VOLUME_UP:
    		
    case  MWKEY_VOLUME_DOWN:
    		{

    			
	
			sysfs_read_integer(PLEXTALK_HEADPHONE_JACK, &hp_online);

			DBGLOG("Music-----MWKEY_VOLUME_UP DOWN   pressed =%d, %d\n",g_config->volume.music[0],g_config->volume.music[1]);

			if(hp_online)
			{
					volume_val=g_config->volume.music[1];
					pvolflag=&volume_maxflag[1];
					
			}
			else
			{
					volume_val=g_config->volume.music[0];		
					pvolflag=&volume_maxflag[0];
			}
			
			if(volume_val>=PLEXTALK_SOUND_VOLUME_MAX || volume_val<=0)
			{
				if(*pvolflag==0)
				{
					*pvolflag=1;
					DBGLOG("Vol is max or min!\n");

					if(Music_IsPlaying())
					{
						normal_beep();
					}
					else
					{
						
						audio_tts(TTS_VOLUME,volume_val);
					}
				}
				else
				{
					DBGLOG("Vol is error!\n");

					if(Music_IsPlaying())
					{
						volume_error_beep ();
					}
					else
					{
						audio_tts(TTS_VOLUME_ERROR,0);	
					}
				}
			}
			else
			{
				*pvolflag=0;
				DBGLOG("Vol is normal!\n");

					if(Music_IsPlaying())
					{
						normal_beep();
					}
					else
					{
						audio_tts(TTS_VOLUME,volume_val);	
					}
				
			}

			
		
	
		

		break;
    		}
#endif
  
}


       
}

static void
Music_OnWindowRedraw(WID__ wid)
{


 //   DBGLOG("Music    OnWindowRedraw\n");
 
}

static void
Music_OnWindowGetFocus(WID__ wid)
{


  DBGLOG("Music    OnWindowGetFocus\n");

}

void  Music_Mode_Exit()
{

	struct audio_player* player = sig_set.player;

   
    gmusic_formMain = NULL;


	
	DBGLOG("Enter Music_OnWindowDestroy_11111111111\n");		
	
	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "ondemand");
	close(nano.fd);
	DBGLOG("Enter Music_OnWindowDestroy_22222222222222222\n");	
	
	plextalk_global_config_close();
	audio_tts_destroy();
	audio_stop();
	
 	DBGLOG("Enter Music_OnWindowDestroy_333333333333333333\n");	
	sys_set_music_speaker_volume(player->volume);						//set volume
	sys_set_music_speed(player->speed);									//set speed
	sys_set_music_repeat(player->repeat_mode);							//set repeat mode

	DBGLOG("Music Destroy start_place.path=%s\n",player->start_place.path);
	
	sys_set_music_resume_path((char*)player->start_place.path);			//set start place
	sys_set_music_resume_time(audio_stat.current_play_time);				//set start time

	 DBGLOG("Music Destroy  player mode=%d\n",player->play_mode);
	sys_set_music_play_mode(player->play_mode);							//save  play mode

	if(player->filecur!=NULL)
	{

		 file_cursor_destroy (player->filecur);
		 player->filecur=NULL;
	}
}


static void
Music_OnWindowDestroy(WID__ wid)
{

 	DBGLOG("-----------Music     window destroy %d.\n",wid);
 	plextalk_set_menu_disable(0);
	audio_stat.suspendflag=1;
	if(audio_stat.suspendflag!=old_suspendflag)
	{
		old_suspendflag=audio_stat.suspendflag;
		DBGMSG("Music_OnWindowDestroy enable  suspend!\n");
		plextalk_suspend_unlock();

		
	}
		 Del_History_Item();
		Music_Mode_Exit();
	 
    
}



 
/*****************************************
Fun: 创建music 的主窗口

*****************************************/
static void
Create_Music_FormMain (struct nano_data* nano)
{
     widget_prop_t wprop;
    int  form_width,form_height;

    form_width=NeuxScreenWidth();
    form_height=NeuxScreenHeight();

    
    

    DBGMSG("Enter Create_Music_FormMain form_width=%d   form_height=%d \n" ,form_width,form_height);

    form_width=MAINWIN_WIDTH;
    form_height=MAINWIN_HEIGHT;

    nano->form= NeuxFormNew(0,STATUSBAR_HEIGHT+TITLEBAR_HEIGHT, form_width, form_height);



    	NeuxFormSetCallback(nano->form, CB_KEY_DOWN, Music_OnWindowKeydown);
	NeuxFormSetCallback(nano->form, CB_KEY_UP, Music_OnWindowKeyup);
       NeuxFormSetCallback(nano->form,CB_HOTPLUG, Music_onHotplug);
	 NeuxFormSetCallback(nano->form, CB_FOCUS_IN, Music_OnWindowGetFocus);
	NeuxFormSetCallback(nano->form, CB_DESTROY,  Music_OnWindowDestroy);
	NeuxFormSetCallback(nano->form, CB_EXPOSURE, Music_OnWindowRedraw);
	

	NeuxWidgetGetProp(nano->form, &wprop);		//得到窗口的属性 
	wprop.bgcolor = theme_cache.background;
	wprop.fgcolor = theme_cache.foreground;
	NeuxWidgetSetProp(nano->form, &wprop);             //设置窗口的属性 
	NeuxFormShow(nano->form, TRUE);
	


}



void  Del_History_Item()
{
	


	CoolDeleteSectionKey(MUSIC_SAVEFILE, "backup", "backup_path");		
	
	CoolDeleteSectionKey(MUSIC_SAVEFILE, "bookmark", "count");
	
	CoolDeleteSectionKey(MUSIC_SAVEFILE, "bookmark", "current");
	
}



void  Get_Music_FormMenu(struct audio_player* player)
{


	xmlDocPtr doc;
	char * buf = NULL;

	int speed;
	int repeat;
	int equ;


		DBGMSG("Enter Get_Music_Setting!\n");
		
		doc = xml_open(PLEXTALK_SETTING_DIR "setting.xml");
 
	if(doc)
	{
		   xml_get_integer(doc, "/settings/music/speed", &speed);
		   xml_get_integer(doc, "/settings/music/repeat", &repeat);
		   xml_get_integer(doc, "/settings/music/equalizer", &equ);
		
		  xml_close(doc);

		   DBGMSG("Get_Music_Setting speed =%d   repeat=%d  ,equ=%d\n",speed,repeat,equ);

			player->speed=speed;
			
			//audio_speed_set(speed_table[player->speed+2]);
			//sys_set_music_speed(player->speed);	




			player->effect_value=equ;
			//audio_draw_equalizer(&nano, player->effect_value);
			//sys_set_music_equalizer(player->effect_value);
			//sig_set_equ_handler(player,&nano,0);



			player->repeat_mode=repeat;
			//audio_draw_repeat_mode(&nano, player->repeat_mode);
			//sys_set_music_repeat(player->repeat_mode);	



			//NeuxThemeInit(theme_index);
			//UpdateFormControl(&nano);
		
			 CoolShmReadLock(g_config_lock);
			plextalk_update_lang(g_config->setting.lang.lang, "music");
			CoolShmReadUnlock(g_config_lock);
		
	
		  
	 }


}




void  Get_Music_Setting(struct audio_player* player)
{


	xmlDocPtr doc;
	char * buf = NULL;

	int speed;
	int repeat;
	int equ;


		DBGMSG("Enter Get_Music_Setting!\n");
		
		doc = xml_open(PLEXTALK_SETTING_DIR "setting.xml");
 
	if(doc)
	{
		   xml_get_integer(doc, "/settings/music/speed", &speed);
		   xml_get_integer(doc, "/settings/music/repeat", &repeat);
		   xml_get_integer(doc, "/settings/music/equalizer", &equ);
		
		  xml_close(doc);

		   DBGMSG("Get_Music_Setting speed =%d   repeat=%d  ,equ=%d\n",speed,repeat,equ);

			player->speed=speed;
//暂时先这样给
			audio_speed_set(1.0, speed_table[player->speed+2]);
			sys_set_music_speed(player->speed);	




			player->effect_value=equ;
			sys_set_music_equalizer(player->effect_value);
			//sig_set_equ_handler(player,&nano,0);



			player->repeat_mode=repeat;
			
			sys_set_music_repeat(player->repeat_mode);	

		
			//audio_draw_repeat_mode(&nano, player->repeat_mode);
			//audio_draw_equalizer(&nano, player->effect_value);

			NeuxThemeInit(theme_index);
			UpdateFormControl(&nano);
			 CoolShmReadLock(g_config_lock);
			plextalk_update_lang(g_config->setting.lang.lang, "music");
			CoolShmReadUnlock(g_config_lock);
		
	
		  
	 }


}


/* 进程消息事件回调函数，MENU的设置消息会从这里过来 */
static void Music_onAppMsg(const char * src, neux_msg_t * msg)
{

	DBGMSG("music app msg %d .\n", msg->msg.msg.msgId);
#if 1
	switch (msg->msg.msg.msgId)

	{		
	case PLEXTALK_APP_MSG_ID:
		{
		
		app_rqst_t* app = (app_rqst_t*)msg->msg.msg.msgTxt;


		DBGMSG("music app msg type=%d\n",app->type);
		
		switch (app->type)
		{
		/*bookmark相关的设置*/
		case APPRQST_BOOKMARK:
			{
			
			app_bookmark_rqst_t* bmk = (app_bookmark_rqst_t*)msg->msg.msg.msgTxt;
			switch(bmk->op) 
			{
				case APP_BOOKMARK_SET:
				{
						//设置书签
				
					
					DBGMSG("AppMSG music set bookmark!\n");
					music_grab_key(&nano);		
				
					player.set_bmk = 1;
					sig_set_bmk_handler(&player, &nano);
					break;		
				}
					
				case APP_BOOKMARK_DELETE:
				{
					//删除书签
			
				
					DBGMSG("AppMSG delete current music bookmark!\n");
					music_grab_key(&nano);		
				
					player.del_cur_bmk = 1;
					sig_del_cur_bmk_handler(&player, &nano);
					break;
				}

				case APP_BOOKMARK_DELETE_ALL:
				{
					//删除所有书签
				
					
					DBGMSG("AppMSG delete all book mark!\n");
					music_grab_key(&nano);		
					
					player.del_all_bmk = 1;		
					sig_del_all_bmk_handler(&player, &nano);
					break;
				}

				break;
			}
			
			break;
			}
		 case APPRQST_LOW_POWER://
		 	{
				DBGMSG("music have  received the lowpower message\n");
	  			 NeuxAppStop();
				break;
		 	}
  		  

		case  APPRQST_DELETE_CONTENT:
				{



			 DBGMSG("AppMSG   APPRQST_DELETE_CONTENT !\n");
			 app_delete_rqst_t *del=( app_delete_rqst_t *)msg->msg.msg.msgTxt;

			 
			 DBGMSG("AppMSG  del   op=%d  !\n",del->op);
			 
			 switch(del->op) 
			{
				case   APP_DELETE_TRACK:
					{
					
						
				
						DBGMSG("AppMSG APP_DELETE_TRACK!\n");
						music_grab_key(&nano);		
						
						player.del_track = 1;
						ShowAllControl(FALSE);
						sig_del_track_handler(&player, &nano);	
						

					  break;
					}

				case APP_DELETE_ALBUM:
					{
					
						DBGMSG("AppMSG APP_DELETE_ALBUM!\n");
						music_grab_key(&nano);		
						
						player.del_album = 1;
						ShowAllControl(FALSE);
						sig_del_album_handler(&player, &nano);
							

					  break;
					}

				
			 }

			 break;
			}
			
		/*设置相应的state，可能是暂停或者恢复*/
		case APPRQST_SET_STATE:
			{
				app_state_rqst_t *rqst = (app_state_rqst_t*)msg->msg.msg.msgTxt;

				DBGMSG("menu pause =%d, %d ,%d ,%d,!\n",g_info_flag,nano.menuflag,player.in_file_select,player.deltitleok);

				/* Special handler for info */
				if (rqst->spec_info) {
					printf("specical msg for info !!\n");
					
					if (g_info_flag) {
						if (rqst->pause)
							info_pause();
						else
							info_resume();

					}

					return ;
				}
				
				printf("Handler Msg for normal set state!\n");
				if(rqst->pause)
				{

					// disable if auto seek is running!
					if (nano.l_key.l_flag) 
					{
						nano.l_key.l_flag = 0;	
						NxSetTimerEnabled(nano.l_key.lid,FALSE);			
					}
					
					if(g_info_flag)
					{
						info_pause();
						DBGMSG("menu  mesage ,but  in info  \n");	

					}
					else if(nano.menuflag||player.in_file_select ||player.deltitleok==2)
					{
					
						DBGMSG("menu  mesage ,but can not enter \n");	
					}
					else
					{



					
						
							Break_Music(&player,1,0);
							CheckBookMarkAtMenuPop (&player,  audio_stat.current_play_time);
							nano.menuflag=1;	

	
						
				
						
					}
						

						
					
					
					break;
				}
				else
				{//要恢复music 的状态



					DBGMSG("Exit  menu =%s  %d,%d,%d,%d!\n",player.start_place.path,g_info_flag,nano.menuflag,player.in_file_select,player.deltitleok);	

				
					
					if(g_info_flag )
					{
						info_resume();
						DBGMSG("exit menu in info \n");	

					}
					else if(nano.menuflag==0||player.in_file_select ||player.deltitleok==2)
					{
						DBGMSG("exit menu not in  info \n");	
					}
					else if(player.deltitleok==1 || (!MusicExist(player.start_place.path) &&  player.in_file_select ==0))
					{//在menu中del 了文件 

		
						DBGMSG("Exit  menu in deltitleok=%s!\n",player.start_place.path);	
						player.deltitleok=2;
					
						key_music_handler(&nano, &player,PLEXTALK_MOUNT_ROOT_STR);
						player.deltitleok=0;

						nano.menuflag=0;
						
				
						
					}
			
					else 
					{		

					
						DBGMSG("APPMSG menu exit signal recieved  %d!\n",player.play_info.breakflg );					
						
			
						
						if(player.play_info.breakflg && player.play_info.resumeback)
						{
							DBGMSG("Exit menu and  resume music!\n");
								
							player.play_info.resumeback(0);
						}

						nano.menuflag=0;
						
				
						ShowAllControl(TRUE);
						recover_music_icon (&nano);	
						SetMedaiIcon(player.start_place.path);
						NhelperTitleBarSetContent(GetAlbum_Name(player.start_place.dirpath));
						
					}
				}			
				break;
			}
		
		
		case APPRQST_GUIDE_VOL_CHANGE:
				{

		
			

				app_volume_rqst_t* rqst = (app_volume_rqst_t*)app;
				vprompt_cfg.volume = rqst->volume;

				DBGMSG("APPRQST_GUIDE_VOL_CHANGE %d\n",vprompt_cfg.volume);
				break;
			}
		
		case APPRQST_GUIDE_SPEED_CHANGE:
				{
			
		
			

				app_speed_rqst_t* rqst = (app_speed_rqst_t*)app;
				vprompt_cfg.speed = rqst->speed;

				DBGMSG("APPRQST_GUIDE_SPEED_CHANGE  %d\n",vprompt_cfg.speed);
				break;
			}

		case APPRQST_GUIDE_EQU_CHANGE:
				break;
		case APPRQST_SET_SPEED:
			{//调节语速

				//audio_stat.pause_flag=g_music_pause_flag;
			//	audio_stat.stop_flag=g_music_stop_flag;	
						
				DBGMSG("AppMsg   APPRQST_SET_SPEED!=%d   %d !\n",player.speed,app->speed.speed); 
				int old_speed = player.speed;
				if(player.speed!=app->speed.speed)
				{
					player.speed=app->speed.speed;
					if(player.speed>8)  player.speed=8;

					if(player.speed<-2)   player.speed=-2;
						
					DBGMSG("sig set  speed handler=%d\n",app->speed.speed);
					audio_speed_set(speed_table[old_speed+2], speed_table[player.speed+2]);
					sys_set_music_speed(player.speed);	
				}
			      break;
			}
		case APPRQST_SET_REPEAT:
			{//调节模式

				//audio_stat.pause_flag=g_music_pause_flag;
				//audio_stat.stop_flag=g_music_stop_flag;

				DBGMSG("AppMsg   APPRQST_SET_REPEAT=%d    %d!\n",player.repeat_mode,app->repeat.repeat); 
				if(player.repeat_mode!=app->repeat.repeat)
				{
					player.repeat_mode=app->repeat.repeat;
					audio_draw_repeat_mode(&nano, player.repeat_mode);

					DBGMSG("Read call  sys_set_music_repeat!\n");
					sys_set_music_repeat(player.repeat_mode);	
					DBGMSG("end call  sys_set_music_repeat!\n");
						
				}

				break;
			}
		case  APPRQST_SET_EQU:
			{//设置均衡器

				//audio_stat.pause_flag=g_music_pause_flag;
				//audio_stat.stop_flag=g_music_stop_flag;
				
				DBGMSG("AppMsg   APPRQST_SET_EQU=%d    %d!\n",player.effect_value,app->equ.equ ); 
				if(player.effect_value!=app->equ.equ)
				{
					player.effect_value=app->equ.equ;
					audio_draw_equalizer(&nano, player.effect_value);
					
					sys_set_music_equalizer(player.effect_value);


					sig_set_equ_handler(&player,&nano,0);
					
						
				}

				break;	
			}

		case APPRQST_SET_THEME:
			{

				DBGMSG("AppMsg   APPRQST_SET_THEME theme_index =%d   app->theme.index=%d !\n",theme_index ,app->theme.index); 
				
				if (theme_index !=app->theme.index) 
				{ //确定当前需要换主题

					theme_index=app->theme.index;
					NeuxThemeInit(theme_index);
						//拿下面的前背景色去重新绘图，自己的icon(如果有)也要变
					UpdateFormControl(&nano);
				}

			    break;	

			}
		case APPRQST_RESYNC_SETTINGS:
				{//初始化

				  DBGMSG("AppMsg   APPRQST_RESYNC_SETTINGS!\n");
			//	  theme_index=app->theme.index;

				



					CoolShmReadLock(g_config_lock);
					
					NeuxThemeInit(g_config->setting.lcd.theme);
					plextalk_update_lang(g_config->setting.lang.lang, "music");
					plextalk_update_sys_font(g_config->setting.lang.lang);
					plextalk_update_tts(g_config->setting.lang.lang);
					
					CoolShmReadUnlock(g_config_lock);



					

				
				UpdateFormControl(&nano);
				Music_Default_Set(&player);	
				audio_draw_repeat_mode(&nano, player.repeat_mode);
				audio_draw_equalizer(&nano, player.effect_value);				    
			
				

				break;
				}
		case APPRQST_SET_LANGUAGE:
			{//语言变化

			     DBGMSG("AppMsg   APPRQST_SET_LANGUAGE!\n");
			     CoolShmReadLock(g_config_lock);
			     plextalk_update_lang(g_config->setting.lang.lang, "music");
   			     CoolShmReadUnlock(g_config_lock);
   	 
   		 	    plextalk_update_tts(getenv("LANG"));
   			    plextalk_update_sys_font(getenv("LANG"));
   			    audio_draw_equalizer(&nano, player.effect_value);
//   			    NhelperTitleBarSetContent(GetAlbum_Name(player.start_place.dirpath));

			break;
			}
             case  APPRQST_SET_FONTSIZE:
             		{//字体大小有变化

             			DBGMSG("AppMsg   APPRQST_SET_FONTSIZE!\n");
             			plextalk_update_sys_font(getenv("LANG"));
				

				break;
             		}
             	case APPRQST_SET_TTS_VOICE_SPECIES:
		case APPRQST_SET_TTS_CHINESE_STANDARD:

             		{//TTS 有变化 
				plextalk_update_tts(getenv("LANG"));
				break;
             		}
	
		}
		break;

	default:
		break;
		}
	}
#endif

}

void RegttsSingal (void)
{
	char* pdata;   //private data

	/* 得到fd */
	nano.fd = voice_prompt_handle_fd();

	/* 设置回调 */
	NeuxAppFDSourceRegister(nano.fd, pdata, fd_read, NULL);

	/* 设置为读 */
	NeuxAppFDSourceActivate(nano.fd, 1, 0);
}





static void
Music_onHotplug(WID__ wid, int device, int index, int action)
{


	DBGLOG("music onHotplug device: %d ,action=%d\n", device,action);


	if(player.in_file_select ||player.deltitleok||g_info_flag||nano.menuflag)
	{//表示己在File list 当中
			
		return ;
		
	}

	if ((device == MWHOTPLUG_DEV_MMCBLK) 
	&& (action == MWHOTPLUG_ACTION_REMOVE))
	{

			DBGLOG("Music  TF  remove!\n");

			
			hotplug_mmcblk_remove_handler(&nano,&player);
	}
	if ((device ==MWHOTPLUG_DEV_SCSI_DISK) &&  (action == MWHOTPLUG_ACTION_REMOVE))
	{
			DBGLOG("Music  USB  remove!\n");	
			
			hotplug_sda_remove_handler(&nano,&player);
	}


}



#if   0
void Music_Get_Device_Setting_Xml()
{

#define  DEVICE_PATH  "/media/mmcblk0p2/.plextalk/DeviceConfigurationFile.xml"


	xmlDocPtr doc=NULL;
	xmlNodePtr    cur=NULL;
	char* name=NULL;
	char* value=NULL;

	xmlKeepBlanksDefault (0); 


	
	 DBGLOG("enter Get_Device_Setting_Xml .\n");
	
	//create Dom tree
	doc=xmlParseFile(DEVICE_PATH);
	if(doc==NULL)
	{
	   DBGLOG("ERROR: Loading xml file failed.\n");
	   return;
	}

	// get root node
	cur=xmlDocGetRootElement(doc);
	if(cur==NULL)
	{
	   DBGLOG("ERROR: empty file\n");
	   xmlFreeDoc(doc); 
	    return;
	}

	//walk the tree 
	cur=cur->xmlChildrenNode; //get sub node
	cur=cur->xmlChildrenNode; //get sub node
	while(cur !=NULL)
	{
	   name=(char*)(cur->name); 
	      
	   xmlChar* szAttr = xmlGetProp(cur,BAD_CAST "select");
	   DBGLOG("DEBUG: name is:%s, value is: %s\n",name,(char *)szAttr);
	   if(!strcmp(name,"AutoPowerOFFTime"))
	   {	
			g_config->auto_poweroff_time=atoi(szAttr);
			
			 DBGLOG("auto_poweroff_time:%d\n",g_config->auto_poweroff_time);
	   }
	   else if(!strcmp(name,"DayControl"))
	   {
			g_config->time_format=atoi(szAttr);
		
		 DBGLOG("time_format:%d\n",g_config->time_format);
	   }
	   
	    else if(!strcmp(name,"HourFormat"))
	   {
			g_config->hour_system=atoi(szAttr);
		
		 DBGLOG("hour_system:%d\n",g_config->hour_system);
	   }

	   else if(!strcmp(name,"MaximumFrequencyRangeControl"))
	   {
			g_config->radio_hi=atof(szAttr);
		
		 DBGLOG("radio_hi:%f\n",g_config->radio_hi);
	   }

	    else if(!strcmp(name,"MinimumFrequencyRangeControl"))
	   {
			g_config->radio_low=atof(szAttr);
		
		 DBGLOG("radio_low:%f\n",g_config->radio_low);
	   }

	   else if(!strcmp(name,"LanguageSetControl"))
	   {
			g_config->langsetting=atoi(szAttr);
		
		 DBGLOG("langsetting:%d\n",g_config->langsetting);
	   }
	 else if(!strcmp(name,"TTSSettingMenuControl"))
	   {
			g_config->tts_setting=atoi(szAttr);
		
		 DBGLOG("tts_setting:%d\n",g_config->tts_setting);
	   }

	   
	   xmlFree(szAttr); 
	   cur=cur->next;
	}

	// release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return ; 

}
#endif



static void
Music_onSWOn(int sw)
{

DBGMSG("Music_onSWOn !\n");

if(sw == SW_KBD_LOCK)
{
	
	nano.keylocked	=1;				//表示lock  key enable
}

		
	
}

static void
Music_onSWOff(int sw)
{

DBGMSG("Music_onSWOff !\n");
	
	if(sw == SW_KBD_LOCK)
	{
		
		nano.keylocked	=0;				//表示lock  key  disable 

	}

}




//回调函数里面，设置自己模块的icon和titlebar
static void
OnWindowFocusIn (WID__ wid)
{


	DBGMSG("Enter OnWindowFocusIn !\n");
	
	 NhelperTitleBarSetContent(GetAlbum_Name(player.start_place.dirpath));
	 
 	 NhelperStatusBarSetIcon(SRQST_SET_CATEGORY_ICON, SBAR_CATEGORY_ICON_MUSIC);



	 SetMedaiIcon(player.start_place.path);
	


	

	recover_music_icon(&nano);

	 
	
}


/*******************************
Function: music  entry  fun

********************************/
int main(void )
{	


	if (NeuxAppCreate("music"))
	{
		FATAL("unable to create application.!\n");
	}

	DBGMSG("Enter music main fun!\n");
	
	signalInit();
	
	DBGMSG("Music application initialized\n");
	

	sysfs_write(PLEXTALK_SCALING_GOVERNOR, "performance");

	
	Music_initApp(&nano);
	Create_Music_FormMain(&nano);
	

	/* A logo for music module */
	Music_show_logo(&nano);
	
		
	DBGMSG("Music FormMain showed.\n");

      
	audio_tts(TTS_LOGO, 0);	

	NeuxWidgetFocus(nano.form);
	//NeuxWidgetShow(nano.load_label, TRUE);
	// NeuxFormShow(nano.form, TRUE);
	
	 
	NeuxWMAppSetCallback(CB_MSG,     Music_onAppMsg);
	NeuxWMAppSetCallback(CB_SW_ON,   Music_onSWOn);
	NeuxWMAppSetCallback(CB_SW_OFF,  Music_onSWOff);
	


	DBGMSG("Music  Read  enter  music_ui_init.\n");
	
	
		
	if (music_ui_init(&nano, &player) == -1) 
	{
		DBGMSG("music  initialization filure!\n");
		exit(-1);
	}

	DBGMSG("Music  finish music_ui_init \n");
	
	NeuxFormSetCallback(nano.form, CB_FOCUS_IN, OnWindowFocusIn);

	
	
	// start application, events starts to feed in.
	
	NeuxAppStart();


	return 0;
	


}

