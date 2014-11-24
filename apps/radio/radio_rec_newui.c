#include "radio_rec_newui.h"

#define _(S) gettext(S)
#if DEBUG_PRINT
#define info_debug(fmt,args...) printf("[%s] %s %d: "fmt, __FILE__,__FUNCTION__,__LINE__,##args)
#else
#define info_debug(fmt,args...) do{}while(0)
#endif



void Radio_Rec_CreateGc(FORM *widget,struct radio_nano* nano)
{
	nano->wid = NeuxWidgetWID(widget);

	nano->pix_wid = NeuxWidgetWID(widget);//GrNewPixmap(MAINWIN_WIDTH, MAINWIN_HEIGHT, NULL);
	nano->pix_gc = NeuxWidgetGC(widget);//GrNewGC();
}


void Radio_Rec_InitStringBuf(struct radio_nano* nano,char *filename)
{
	DBGMSG("Rec_InitStringBuf\n");
	int RecTimeValueOffset,RemaintTimeValueOffset;
	int offset = 0;
	
	nano->textShow.totalItemSize = FILE_NAME_SIZE + REC_TIME_LAEBEL_SIZE + REC_TIME_VALUE_SIZE
					+ REMAIN_TIME_LABEL_SIZE + REMAIN_TIME_VALUE_SIZE;
	
	nano->textShow.pText = (char*)malloc(nano->textShow.totalItemSize); 
	if(nano->textShow.pText == NULL) 
	{
		DBGMSG("malloc fail\n");
		return;
	}
	memset(nano->textShow.pText,0x00,nano->textShow.totalItemSize);

	if(StringStartWith(getenv("LANG"), "hi_IN"))
	{
		offset = 1;
		snprintf(nano->textShow.pText,nano->textShow.totalItemSize,"%s %s\n%s\n%s\n%s",
			filename,_("Recording Time:"),"00:00:00",_("Remaining Time:"),radio_rec_time_to_string(nano->remain_time));
	} 
	else
	{
		offset = 0;
		snprintf(nano->textShow.pText,nano->textShow.totalItemSize,"%s%s\n%s\n%s\n%s",
			filename,_("Recording Time:"),"00:00:00",_("Remaining Time:"),radio_rec_time_to_string(nano->remain_time));
	}


	
	//UTF-8
	
	RecTimeValueOffset = 0 + strlen(filename) + offset + strlen(_("Recording Time:")) + 1;
	RemaintTimeValueOffset = RecTimeValueOffset + strlen("00:00:00") + 1 + strlen(_("Remaining Time:")) + 1;

	nano->textShow.pFilename = nano->textShow.pText;
	nano->textShow.FileNameLen = strlen(filename);	
	nano->textShow.pRecTimeValue = nano->textShow.pText + RecTimeValueOffset;
	nano->textShow.pRemainTimeValue = nano->textShow.pText + RemaintTimeValueOffset;
	nano->textShow.RecTimeValueLen = strlen("00:00:00");
	nano->textShow.RemainTimeValueLen = nano->textShow.RecTimeValueLen;

	DBGMSG("FileNameLen:%d,RecTimeValueOffset:%d,RemaintTimeValueOffset:%d\n",nano->textShow.FileNameLen,RecTimeValueOffset,RemaintTimeValueOffset);

	int i;
	for(i=0;i<strlen(nano->textShow.pText);i++) {
		DBGMSG("buf[%d]=%x\n",i,nano->textShow.pText[i]);
	}
	
}

void Radio_Rec_FreeStringBuf(struct radio_nano* nano)
{
	if(nano->textShow.pText) {
		free(nano->textShow.pText);
		nano->textShow.pText = NULL;	
	}

	nano->textShow.pFilename = NULL;	
	//nano->pRecTimeLabel = NULL;	
	nano->textShow.pRecTimeValue = NULL;	
	//nano->pRemainTimeLabel = NULL;	
	nano->textShow.pRemainTimeValue = NULL;	
}


void Radio_RecShow_showScreen(struct radio_nano* nano)
{
	struct radio_rec_text* thiz;
	int i,j,x,y;
	GR_SIZE gwidth, gheight, gbase;
	GR_TEXTFLAGS flags;
	
	thiz = &(nano->textShow);

	if(!thiz || thiz->total_line <= 0){
		return;
	}

	flags = MWTF_UTF8 | MWTF_TOP;
	
	i = 0;
	y = 0;
	x = 0;
	for(i = thiz->start_line; i < (thiz->start_line +thiz->show_line); i++){
		NxSetGCForeground(nano->pix_gc, theme_cache.foreground);
		NxSetGCBackground(nano->pix_gc, theme_cache.background);
		GrText(nano->wid, nano->pix_gc, x, y, thiz->pText+thiz->lineStart[i], thiz->lineLen[i], flags);
		y += thiz->line_height ;
	}
}



int Radio_RecShow_nextScreen(struct radio_nano* nano)
{
	info_debug("text next screen\n");
	struct radio_rec_text* thiz;
	int i;
	thiz = &(nano->textShow);
	if(!thiz || thiz->total_line <= 0){
		return -1;
	}

	if(thiz->start_line + thiz->screen_line < thiz->total_line){
		thiz->start_line += thiz->screen_line;
		if(thiz->start_line + thiz->screen_line < thiz->total_line){
			thiz->show_line = thiz->screen_line;
		}
		else{
			thiz->show_line = thiz->total_line - thiz->start_line;
		}
	}
	else{
		/* next phase */
		info_debug("no normal screen\n");
		return -1;
	}
	return 0;
}

int Radio_RecShow_prevScreen(struct radio_nano* nano)
{
	struct radio_rec_text* thiz;
	int i;
	thiz = &(nano->textShow);
	
	info_debug("text prev screen\n");

	if(!thiz || thiz->total_line <= 0){
		return -1;
	}

	if(thiz->start_line - thiz->screen_line >= 0){
		thiz->start_line -= thiz->screen_line;
		/* fill screen */
		if(thiz->start_line + thiz->screen_line < thiz->total_line){
			thiz->show_line = thiz->screen_line;
		}
		else{
			thiz->show_line = thiz->total_line - thiz->start_line;
		}
	}
	else{
		/* prev phase */
		//info_debug("no prev screen\n");
		return -1;
	}

	return 0;
}


static int filterinvalidChar(char *pstr, int len)
{
	char *pEnd = pstr + len-1;
	while(pEnd >= pstr && (0x0d == *pEnd || 0x0a == *pEnd)){
			pEnd --;
	}
	len = pEnd - pstr +1;
	return len;
}



int Radio_RecShow_parseText(struct radio_nano* nano, char *text, int size)
{
	struct radio_rec_text* thiz;
	int i;
	thiz = &(nano->textShow);
	
	thiz->pText = text;
	thiz->textSize = size;
	
	thiz->textSize = strlen(text);
	DBGMSG("thiz->textSize=%d\n",thiz->textSize);
	thiz->total_line= utf8GetTextExWordBreakLines(nano->pix_gc, thiz->w, "UTF8", text, thiz->textSize,
		REC_MAX_LINES, thiz->lineStart, thiz->lineLen, NULL);	

	thiz->start_line = 0;
	if(thiz->screen_line < thiz->total_line){
		thiz->show_line = thiz->screen_line;
	}
	else{
		thiz->show_line = thiz->total_line;
	}

	/*去掉行尾不可显示的字符*/
	for(i = 0; i < thiz->total_line; i++){
		if (0 !=  thiz->lineLen[i]) {
			thiz->lineLen[i] = filterinvalidChar(thiz->pText+thiz->lineStart[i],thiz->lineLen[i]);
		}
	}	
	return 1;
}



int Radio_RecShow_Init(struct radio_nano* nano,int fontsize)
{
	struct radio_rec_text* thiz;
	int i;
	thiz = &(nano->textShow);

	thiz->x = MAINWIN_LEFT;
	thiz->y = MAINWIN_TOP;
	thiz->w = MAINWIN_WIDTH;
	thiz->h = MAINWIN_HEIGHT;
	
	thiz->font_size = 16;//fontsize;
	thiz->line_height = fontsize + 1;
	thiz->screen_line = thiz->h/thiz->line_height;
		
	info_debug("font_size=%d\n", thiz->font_size);
	info_debug("screen_line=%d\n", thiz->screen_line);

	Radio_RecShow_parseText(nano, thiz->pText,strlen(thiz->pText));
		
	return 0;
}


