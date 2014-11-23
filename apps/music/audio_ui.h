
#ifndef __AUDIO_UI_H__
#define __AUDIO_UI_H__

#if 0
#define KEY_UP        	  63490
#define KEY_DOWN      	  63491
#define KEY_LEFT      	  63488
#define KEY_RIGHT  	  	  63489
#define KEY_FORWORD   	  46
#define KEY_BACK      	  44    
#define KEY_PLAY_STOP 	  32
#define KEY_MUSIC     	  109		//key m,for select music 
#define KEY_VOL_UP		  104		//key h, for set volume up
#define KEY_VOL_DOWN	  103		//key g, for set volume down

#define KEY_SEEK_FORHEAD  112		//key p, for seek 5s forhead
#define KEY_SEEK_BACKWARD 111		//key o, for seek 5s backward 


#define KEY_MENU MWKEY_KP0
#endif


char * GetAlbum_Name(char *path);

int music_setConditionIcon(int state);


#endif

