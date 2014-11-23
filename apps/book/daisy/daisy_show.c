#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include "daisy_show.h"
#define OSD_DBG_MSG
#include "nc-err.h"
#include "typedef.h"
#include "convert.h"
#include "daisy_parse.h"
#include "Convert.h"
#include "Plextalk-config.h"
#include "plextalk-theme.h"
#include "Plextalk-ui.h"
#include "Shared-mem.h"
#include "Breaklines.h"
#include "dbmalloc.h"


char* dsShow_getCurScrText(DaisyShow *thiz){
	if(!thiz || !thiz->show_line){
		return 0;
	}
	
	int i,start;
	size_t ret_in, ret_out; 	//for tts play
	
	ret_in = 0;
	start = thiz->start_line;
	for(i = start; i<(start + thiz->show_line) && i<thiz->total_line;i++)
	{
		ret_in += thiz->lineLen[i];
	}
	
	DBGMSG("ret_in=%d\n",ret_in);
	ret_out = sizeof(thiz->pScrText)-2;
	memset(thiz->pScrText, 0, ret_out);
	utf32_to_utf8(
		(char*)thiz->pText + thiz->lineStart[start],
		&ret_in,
		thiz->pScrText,
		&ret_out
		);
	
	return thiz->pScrText;
}

int dsShow_nextScreen(DaisyShow *thiz)
{
	DBGMSG("daisy next screen\n");

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
		DBGMSG("no normal screen\n");
		return -1;
	}
	return 0;
}

int dsShow_prevScreen(DaisyShow *thiz)
{
	DBGMSG("daisy prev screen\n");

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
		DBGMSG("no prev screen\n");
		return -1;
	}

	return 0;
}

int dsShow_showScreen(DaisyShow *thiz)
{
	int i,y;
	char *pLine;
	if(0==thiz->total_line || 0==thiz->pText || thiz->iTextSize<=0){
		return -1;
	}
	
	i = 0;
	y = 0;
	
	while(i < thiz->show_line){
		pLine = thiz->pText + thiz->lineStart[i+thiz->start_line];
		GrText(thiz->wid, thiz->show_gc, 0, y, pLine, 
			thiz->lineLen[i+thiz->start_line], 
			MWTF_UC32 | MWTF_TOP);
		i++;
		y += thiz->line_height;
	}
	return 0;
}

#if 0
int dsShow_parse_lines(DaisyShow *thiz, utf32_t *text, int size)
{
	char *brks;
	int skip = MAX_SHOW_LEN / thiz->font_size;
	int  width, byte_len,ch_len;
	int brk_index;
	//int count;
	int cal_width;
//	size_t in_ret, out_ret;
//	gchar *p, *q;
//	char dbg_info[1024];
	int line_idx = 0;
	size_t i, start;
	utf32_t *line_start;
	size_t line_len;
	
    enum _State
    {
        STAT_INIT,
        STAT_IN_LINE,
        STAT_OUT_LINE,
    }state = STAT_INIT;

	DBGMSG("size=%d\n", size);
	thiz->total_line = 0;
	thiz->show_line = 0;
	if(size <=0){
		return -1;
	}
	
	brks = (char *)malloc(size);
	set_linebreaks_utf32((const utf32_t *)text, 
		size, "zh", brks);

	start = 0;
	brk_index = -1;
	
	i = 0;
	
	while(i < size){
		if(brks[i] != LINEBREAK_NOBREAK){
			brk_index = i;
		}
		switch(state){
		case STAT_INIT:
		{
			if(text[i] == 0xA){
				state = STAT_OUT_LINE;
			}
			else{
				state = STAT_IN_LINE;
			}
		}
		break;
		case STAT_IN_LINE:
		{
			if(text[i] == 0xA){
				state = STAT_OUT_LINE;
				thiz->show_text[line_idx] = (char*)text+start;
				thiz->show_txtlen[line_idx] = (i - start)*4;
				line_idx++;

				start = i+1; /*skip 0xA*/
				brk_index = -1;
				break;
			}
			
			//ch_len = count;
			ch_len = (i - start+1);
			if(ch_len <= skip){
				break;
			}
			
			
			byte_len = (i - start+1)*4;

			//info_err("byte_len=%d\n", byte_len);
			width = cal_text_len(thiz,(char*)(text+start), byte_len);
			cal_width = MAX_SHOW_LEN;
			
			//info_err("width=%d, cal with=%d\n", width, cal_width);
			if(width <= cal_width){
				break;
			}

			state = STAT_OUT_LINE;
			line_start = text+start;
			if(brk_index > 0){
				line_len = brk_index - start;
				start = brk_index;
				brk_index = -1;
				//DBGMSG("i =%d, start=%d\n", i, start);
			}
			else{
				DBGMSG("force line break\n");
				line_len = i - start;
				start = i;
			}
			if(brks[i] != LINEBREAK_NOBREAK){
				brk_index = i;
			}

			thiz->show_text[line_idx] = (char*)line_start;
			thiz->show_txtlen[line_idx] = line_len*4;
			line_idx++;
			state = STAT_IN_LINE; /*because i*/
		}
		break;
		case STAT_OUT_LINE:
		{
			if(text[i] == 0xA){
				state = STAT_OUT_LINE;
				/*empty new line*/
				line_start = text+start;
				line_len = 0;
				start = i+1; /*skip 0xA*/
				brk_index = -1;
				
				thiz->show_text[line_idx] = (char*)line_start;
				thiz->show_txtlen[line_idx] = line_len*4;
				line_idx++;
			}
			else{
				state = STAT_IN_LINE;
				
			}
		}
		break;
		default:break;
		}

		i++;
		if(line_idx>=MAX_SHOW_LINE){
			info_err("so much lines\n");
			break;
		}
	}
	free(brks);

	if((start < size) && (line_idx<MAX_SHOW_LINE)){
		line_start = text+start;
		line_len = size - start;
		brk_index = -1;
	
		thiz->show_text[line_idx] = (char*)line_start;
		thiz->show_txtlen[line_idx] = line_len*4;
		line_idx++;
	}

	thiz->total_line = line_idx;
	thiz->start_line = 0;
	if(thiz->screen_line < thiz->total_line){
		thiz->show_line = thiz->screen_line;
	}
	else{
		thiz->show_line = thiz->total_line;
	}
	/*fill show lines*/
	DBGMSG("parse end\n");
	return 0;
}
#endif

int dsShow_parseText(DaisyShow *thiz, char *text, int size)
{
	thiz->pText = text;
	thiz->iTextSize = size;
	thiz->total_line = 0;
	
	if(NULL ==thiz->pText || thiz->iTextSize <=0){
		return -1;
	}
	DBGMSG("size:%d\n", size);
	thiz->total_line= utf32GetTextExWordBreakLines(thiz->show_gc, thiz->w, getenv("LANG"), text, size,
			MAX_SHOW_LINE, thiz->lineStart, thiz->lineLen, NULL);
	if(-1 == thiz->total_line){
		thiz->total_line = 0;
	}
	thiz->start_line = 0;
	DBGMSG("total line:%d\n", thiz->total_line);
	if(thiz->screen_line < thiz->total_line){
		thiz->show_line = thiz->screen_line;
	}
	else{
		thiz->show_line = thiz->total_line;
	}
	return 0;
}

int dsShow_Init(DaisyShow *thiz,int fontsize)
{
	thiz->x = MAINWIN_LEFT;
	thiz->y = MAINWIN_TOP;
	thiz->w = MAINWIN_WIDTH;
	thiz->h = MAINWIN_HEIGHT;
	
	thiz->font_size = fontsize;
	thiz->line_height = fontsize + 1;
	thiz->screen_line = thiz->h/thiz->line_height;
		
	DBGMSG("font_size=%d\n", thiz->font_size);
	DBGMSG("screen_line=%d\n", thiz->screen_line);
	return 0;
}


