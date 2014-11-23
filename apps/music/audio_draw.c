#include "audio_draw.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "audio_extern.h"
#include "efont.h"
#include "audio_play.h"

//use audio_box 
//#include "audio_box.h"


//#define OSD_DBG_MSG
#include "nc-err.h"

#define TITLE_DYNAMIC_SIZE 126			//for utf8 size

static int title_c = 0;
static int title_a = 0;

#define LOAD_IMG(S) GrLoadImageFromFile(S, 0)








void 
audio_draw_title_num_zero (void)
{
	title_a = 0;
	title_c = 0;
}


void 
audio_draw_title (struct nano_data* nano, const char* path)
{
	char *ptitle=path;
	char *pstr;
	char text[20];
	int len;
	int num;
	
	if (!path) 
	{
		printf("ERROR:path has been to null!\n");
		return;
	}


    ptitle=path+strlen(path)-1;
    
    while(*ptitle)
    	{
		if(*ptitle=='/')
		{
			break;
		}
		else
		{
			ptitle--;	
		
		}
		
    	}
    
    ptitle++;
    
  if(/*audio_stat.trackflag*/0)
  {

	pstr=strstr(ptitle,"Track");
	if(pstr==NULL)   
	{
		NxSetLabelCaption(nano->lsong_name, ptitle);
		return;
	}

	pstr=pstr+6;		//指向后面的数字Track 1, Track 2....

	len=strlen(pstr);
	if(len==0) 
	{
		NxSetLabelCaption(nano->lsong_name, ptitle);
		return;

	}

	num=atoi(pstr);
	
	num=num+1;
	memset(text,0x00,sizeof(text));
	strcpy(text,"Track ");
	
	snprintf(text+6, 20, "%d", num);
	
	NxSetLabelCaption(nano->lsong_name, text);
	
  }
  else
  {
 	NxSetLabelCaption(nano->lsong_name, ptitle);
  }
 
}




#define   DISTANCE_Y        0

void 
audio_draw_progress_bar (struct nano_data* nano,
								  unsigned long time_cur, unsigned long time_tal, 
								  int base)
{


	double  pos;

	if (time_tal <= 0)   return;

		
	

	if (time_cur >time_tal) 
	{
		pos = 132;
		//DBGMSG("audio_draw_progress_bar11111111111111:%d\n",pos);
	}
	else
	{
		pos = 132*( (double)time_cur/(double) time_tal );
		//DBGMSG("audio_draw_progress_bar2222222222222222:%d,%d,%f\n",time_cur,time_tal,pos);
	}



	 NeuxWidgetMove(nano->pcell, (int)pos+6, MUSIC_CELL_ICON_Y);
	NeuxWidgetShow(nano->pcell, GR_TRUE);
	nano->process_x= (int)pos+6;
	nano->process_y=MUSIC_CELL_ICON_Y;
	
	//DBGMSG("audio_draw_progress_bar:%f\n",pos);

	

		
}


void 
audio_draw_music_background (struct nano_data* nano)
{

	/*
	GrSetGCForeground(nano->gc, nano->bg_color);
	GrFillRect(nano->root_pix_id, nano->gc,
		0, 0, 160, 92);
	GrSetGCForeground(nano->gc, nano->fg_color);
	*/
	
}


void 
audio_draw_total_time (struct nano_data* nano, unsigned long time)
{
	char buf[32];

	time_to_string(time, buf, 32);

	DBGMSG("Total  time is:%s\n",buf);
	NxSetLabelCaption(nano->ltotal_time, buf);
	NeuxWidgetShow(nano->ltotal_time, TRUE);
}


void 
audio_draw_current_time (struct nano_data* nano, unsigned long  time)
{
	char buf[32];

	if(audio_stat.current_play_time>audio_stat.total_play_time)
	{

		DBGMSG("play time more than  total time!\n");	
		return 0;
	}
		

	
	time_to_string(time, buf, 32);
	


	//DBGMSG("Current time is:%s\n",buf);
	
	NxSetLabelCaption(nano->lcur_time, buf);


	
	
	
	
}

void 
audio_draw_repeat_mode (struct nano_data* nano, int mode)
{

	

	 DBGMSG("Enter audio_draw_repeat_mode!=%d!\n",mode); 
	
	  if(mode==0)
	  {
	  	NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT0);
	  }
	  else if (mode==1)
	  {
	  	NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT1);	
	  }
	  else if(mode==2)
	  {
	  	NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT2);		
	  }
	  else if(mode==3)
	  {
		NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT3);		
	  		
	  }
	  else 
	  {
	  	NeuxThemeIconSetPic(nano->prepeat_mode, THEME_ICON_MUSIC_REPEAT4);	
	  }


	NeuxWidgetShow(nano->prepeat_mode, TRUE);
	
}


void 
audio_draw_equalizer(struct nano_data* nano, enum sound_effect effect)
{


	
	NeuxLabelSetText(nano->lplay_mode, (char*)effect_string(effect));
	NeuxWidgetShow(nano->lplay_mode, TRUE);

	

}

