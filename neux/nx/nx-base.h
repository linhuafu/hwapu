/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Widget Base header.
 *
 * REVISION:
 *
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-04-08 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _NXBASE_H_
#define _NXBASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#include "nx-widgets.h"
#include "widgets.h"

#define MENU_SEPARATOR '-'

typedef void* DATA_POINTER;

struct widgetbase
{
	NX_WIDGET_TYPE 	type;
	GR_WINDOW_ID    wid;
	char *          name;

	int             posx;
	int             posy;
	GR_SIZE         width;
	GR_SIZE         height;

	GR_BOOL         enabled;
	GR_BOOL         visible;
	GR_GC_ID        gc;

	GR_COLOR        fgcolor;
	GR_COLOR        bgcolor;

	GR_FONT_ID      fontid;
	char *          fontname;
	GR_SIZE         fontsize;

	GR_BOOL         usebgpixmap;
	GR_WINDOW_ID    bgpixmap;

	GR_EVENT        event;
	char *          tag;
	DATA_POINTER    data;

	GR_BOOL         realized;
};

typedef struct nxwidget NX_WIDGET;

#define CONTAINER_MAX_CHILD 64 /* maximum child per container. */
struct containerbase
{
	NX_WIDGET *    childTab[CONTAINER_MAX_CHILD];
	int            childNum;
	NX_LAYOUT_TYPE layout;
};

typedef NX_WIDGET *(*CreateFuncPtr) (NX_WIDGET *); //owner
typedef void (*RedrawFuncPtr) (void);
typedef void (*DestroyFuncPtr) (void);

typedef void (*CallBackFuncPtr) (NX_WIDGET *sender, DATA_POINTER dptrOut);
typedef void (*EventDispatchFuncPtr) (GR_EVENT *, NX_WIDGET *);
typedef struct
{
	GR_BOOL 		inherit;
	CallBackFuncPtr	fp;		//call back function point
	DATA_POINTER 	dptr;   //input data point
}CallBackStruct;

/* Specialized callback prototypes (getItem, getIcon etc). For the user they work in
   almost the same way as the regular callbacks like keydown, exposure, etc (see include/widgets.h).
   However, internally instead of storing a CallBackStruct we just store the user-defined function pointer
   because there's no need for the other two members of that structure */

//typedef int (*GetIconFuncPtr) (int widget, int index, void **iconbmp, void **selecticon);
//typedef int (*GetItemFuncPtr) (int widget, int index, char **text);

#define GetIconFuncPtr			GetIconCbfptr
#define GetItemFuncPtr			GetItemCbfptr
#define GetPicFuncPtr			GetPicCbfptr
#define GetMarkFuncPtr			GetMarkCbfptr
#define GetItemFgcolorFuncPtr	GetItemFgcolorCbfptr
#define GetItemTypeFuncPtr		GetItemTypeCbfptr
#define GetImageFuncPtr			GetImageCbfptr
#define GetBitmapFuncPtr		GetBitmapCbfptr
#define GetMTEntrysStateFuncPtr	GetMTEntrysStateCbfptr

typedef int (*GetActionFuncPtr) (int widget, int index, void **action);

#endif //_NXBASE_H_
