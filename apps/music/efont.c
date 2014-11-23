#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "efont.h"

#define ERROR_PRINT 1
#define DEBUG_PRINT 1
#define NORMAL_PRINT 1

#if 0
struct EFont  *efont_create(char *name, GR_SIZE height)
{
	int skin = sys_get_global_skin();
	GR_COLOR fg_color = sys_get_cur_theme_fgcolor(skin);
	GR_COLOR bg_color = sys_get_cur_theme_bgcolor(skin);
	

	struct EFont * thiz = (struct EFont *)malloc(sizeof(struct EFont)) ;

	memset(thiz, 0, sizeof(struct EFont));
	
	thiz->gc = GrNewGC();
	
	thiz->color  	= EFONT_COLOR_BLACK;
	thiz->height = height;	
	thiz->id   	= GrCreateFont((GR_CHAR *)name, height, NULL);
	
	GrSetFontAttr(thiz->id , GR_TFKERNING|GR_TFANTIALIAS , 0);
	
	if (strcmp((char *)name, (char *)GBK_HZK_FONT) == 0)
		thiz->flag = MWTF_UTF8;
	else if(strcmp((char *)name, (char *)TTF_STSONG) == 0)
	{
		thiz->flag = MWTF_UTF8; //MWTF_DBCS_GBK;
		GrSetFontAttr(thiz->id , GR_TFKERNING, 0);
	}
	else if(strcmp((char *)name, (char *)TTF_SIMFANG) == 0)
	{
		thiz->flag = MWTF_UC16; //MWTF_DBCS_GBK;
		GrSetFontAttr(thiz->id , GR_TFKERNING, 0);
	}
	else
		thiz->flag = GR_TFASCII;

	/*set gc font*/
	GrSetGCFont(thiz->gc, thiz->id);
	GrSetGCUseBackground(thiz->gc ,GR_FALSE);
	GrSetGCForeground(thiz->gc ,fg_color);
	GrSetGCBackground(thiz->gc, bg_color);


	return thiz;
}

#endif



GR_GC_ID efont_get_gc(struct EFont *thiz)
{
	if(thiz){

		return thiz->gc;
	}

	return 0;
}

GR_TEXTFLAGS efont_get_flags(struct EFont *thiz)
{
	if(thiz){
		return thiz->flag;
	}

	return 0;

}


void efont_destroy(struct EFont *thiz)
{
	if(thiz){
		if(thiz->gc){
			GrDestroyGC(thiz->gc);
		}
		if(thiz->id){
			GrDestroyFont(thiz->id);
		}
		free(thiz);
	}
}
