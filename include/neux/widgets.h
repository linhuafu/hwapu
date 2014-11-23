#ifndef NXWIDGETS__H
#define NXWIDGETS__H
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
 * Widgets module header.
 *
 * REVISION:
 *
 * 3) Code refactored: widget inheritance support. -------- 2007-08-21 MG
 * 2) Modify struct-----------------------------------------2006-05-26 EY
 * 1) Initial creation. ----------------------------------- 2006-05-04 MG
 *
 */

#include <unistd.h>
#include <signal.h>
#include <linux/limits.h>
#include "nano-X.h"
#include "nxlist.h"
#include "file-helper.h"
#include "theme-defines.h"

#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif

#define WIDGET           void

#define FORM             WIDGET
#define BUTTON           WIDGET
#define LISTBOX          WIDGET
#define TIMER            WIDGET
#define LABEL            WIDGET
#define PICTURE          WIDGET
#define PANEL            WIDGET
#define LINE             WIDGET
#define PROGRESSBAR      WIDGET
#define SCROLLBAR        WIDGET
#define FILELISTBOX	     WIDGET

#define BITMAP           void
#define IMAGEID			 unsigned int

#define SBAR_WIDTH       6

// Neux layout management type.
typedef enum
{
	NLT_NULL_LAYOUT,
	NLT_BORDER,
	NLT_BOX,
	NLT_CARD,
	NLT_FLOW,
	NLT_GRIDBAG,
	NLT_GRID,
	NLT_SPRING
} NX_LAYOUT_TYPE;

typedef struct
{
    GR_COLOR fgcolor;
    GR_COLOR bgcolor;
} widget_prop_t;

typedef struct
{
    GR_BOOL        transparent;
	NX_LAYOUT_TYPE layout;
} form_prop_t;

typedef struct
{
	NX_LAYOUT_TYPE layout;
} panel_prop_t;

typedef struct
{
	GR_SIZE	 width;
	GR_COLOR fgcolor;
	GR_COLOR bgcolor;
	GR_COLOR bordercolor;
} sbar_prop_t;

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} margin_t;

typedef struct
{
    int w;
    int h;
} region_t;

typedef enum
{
	LT_ICON,		// draw icon, iconbmp is assigned the BITMAP point in the callback  'GetIconCbfptr'.
	LT_IMAGE		// draw image, iconbmp is assigned the iamge id in the callback  'GetIconCbfptr'.
} LISTBOX_GRAPHICTYPE;

typedef struct
{
    GR_COLOR selectedcolor;   // this is the highlight color
    GR_COLOR selectedfgcolor;   // this is the highlight color
	GR_BOOL ismultiselec;
	GR_COLOR selectmarkcolor;

    GR_BOOL  useicon;          // either left / right icon or none.	
	LISTBOX_GRAPHICTYPE graphictype;

    margin_t itemmargin;
	margin_t textmargin;
	region_t iconregion;

	sbar_prop_t sbar;

	GR_BOOL bScroll;
	GR_BOOL bScrollBidi;
	int scroll_interval;
	int scroll_endingscrollcnt;

	GR_BOOL bWrapAround;

    // readonly property
    int      indexitem;
    int      itemheight;
} listbox_prop_t;

typedef enum
{
    LA_LEFT,
    LA_CENTER,
    LA_RIGHT
} LABEL_ALIGN;

typedef struct
{
    GR_BOOL transparent;
    GR_BOOL autosize;
    LABEL_ALIGN align;
} label_prop_t;

typedef enum
{
    BT_NORMAL,		// isn't transparent, rectangle, draw rectangular if it get focus
    BT_GENERAL,		// isn't transparent, rectangle, change the background color if it get focus
    BT_FLAT,		// transparent, change the font color if it get focus
    BT_CIRCLE,		// isn't transparent, circular button, only show the first char of caption
    BT_IMAGE		// need two image for focus /non focus statu.
} BUTTON_TYPE;

typedef enum
{
    BT_LEFT,
    BT_CENTER,
    BT_RIGHT
} BUTTON_ALIGN;

typedef struct
{
    BUTTON_TYPE type;
	BUTTON_ALIGN align;
	GR_COLOR selectedcolor;
	GR_COLOR shadowdcolor;
} button_prop_t;

typedef struct
{
	GR_COLOR blockcolor;
} progressbar_prop_t;

typedef enum
{
   PT_FILE,
   PT_BUFFER
} PICTURE_TYPE;

typedef struct {  /* structure for reading images from buffer   */
	size_t size;	// Size in bytes of the image
	void* start;	// JPG image data.
} picture_buffer_t;

typedef struct
{
	GR_BOOL transparent;
	GR_BOOL autosize;
	GR_BOOL stretch;
	GR_BOOL zoomable;
} picture_prop_t;

typedef enum
{
   LT_NORMAL,
   LT_STRIKETHROUGH
} LISTBOX_ITEMTYPE;

typedef enum
{
	MK_DISABLE,
	MK_CLIP,
	MK_BOOK
} NX_MARK_TYPE;

typedef enum
{
	MK_NONE,
	MK_START,
	MK_BOTH
} NX_MARK_PROP;

typedef struct
{
	NX_MARK_PROP state;
	int startpos;
	int endpos;
} p_vbar_mark_t;

typedef int FONT_ID;

typedef struct
{
	int endOfModal;
	int msgReturn;
	const char *text;
	size_t text_len;
	int line_spacing;
	slist_t lines_head;
	int nr_lines;
	int multi_screen;		//added by ztz
	int multi_count;		//added by ztz
	int multi_screen_perlines;	//added by ztz

	LABEL	*item_lable;;//add appk
} msgbox_context_t;


// Message box bitmap flags.
#define MSGBX_MODAL           	0x80000000
#define MSGBX_ICON_MASK        	0x00FF0000
#define MSGBX_ICON_WARNING    	0x00010000
#define MSGBX_ICON_INFO       	0x00020000
#define MSGBX_ICON_ERROR      	0x00040000
#define MSGBX_ICON_QUERY      	0x00080000
#define MSGBX_BTN_MASK        	0x00FF
#define MSGBX_BTN_OK          	0x0001
#define MSGBX_BTN_CANCEL      	0x0002
#define MSGBX_BTN_YES         	0x0004
#define MSGBX_BTN_NO          	0x0008
#define MSGBX_DEF_BTN_MASK    	0xFF00
#define MSGBX_DEF_BTN_OK     	(MSGBX_BTN_OK      <<8)
#define MSGBX_DEF_BTN_CANCEL 	(MSGBX_BTN_CANCEL  <<8)
#define MSGBX_DEF_BTN_YES    	(MSGBX_BTN_YES     <<8)
#define MSGBX_DEF_BTN_NO     	(MSGBX_BTN_NO      <<8)

#ifndef MAX
# define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
# define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif
#define RANDRANGE(LO, HI)	((rand() % ((int)(HI) - (int)(LO) + 1)) + (int)(LO))
#define FreeAndNull(p)		do { free(p); p = NULL; } while(0)

typedef enum
{
   NX_GRAB_HOTKEY_EXCLUSIVE,
   NX_GRAB_HOTKEY,
   NX_GRAB_EXCLUSIVE,
   NX_GRAB_EXCLUSIVE_MOUSE
} KEY_GRAB_TYPE;

// Callback ID and handle types.
typedef enum
{
   CB_KEY_DOWN,
   CB_KEY_UP,
   CB_FOCUS_IN,
   CB_FOCUS_OUT,
   CB_EXPOSURE,
   CB_TIMER,
   CB_DESTROY,
   CB_GETIMAGE,
   CB_MSG,
   CB_IDLE,
   CB_SSAVER,
   CB_CHANGE,
   CB_HOTPLUG,
   CB_SW_ON,
   CB_SW_OFF,
} CALLBACK_ID;

typedef struct
{
   int ch;			//The name of the key.
   int hotkey;		//Whether is a hotkey.
   //int elapse; 	//The time in millisecond elapse since the key press.
} KEY__;

#define WID__ int

typedef void (*KeydownCbfptr)  (WID__, KEY__);
typedef void (*KeyupCbfptr)  (WID__, KEY__);
typedef void (*FocusinCbfptr)  (WID__);
typedef void (*FocusoutCbfptr) (WID__);
typedef void (*ExposureCbfptr) (WID__);
typedef void (*TimerCbfptr)    (WID__);
typedef void (*DestroyCbfptr)  (WID__);
typedef void (*ChangeCbfptr)  (WID__);
typedef void (*SSaverCbfptr)  (GR_BOOL activate);
typedef void (*HotplugCbfptr)  (WID__, int device, int index, int action);
typedef void (*SWOnCbfptr)  (int sw);
typedef void (*SWOffCbfptr)  (int sw);

typedef void (*GetIconCbfptr) (WID__ sender, int index, BITMAP **iconbmp);
typedef int (*GetItemCbfptr) (WID__ sender, int index, char **text);
typedef int (*GetPicCbfptr) (WID__ sender, int index, void **pbuf);
typedef int (*GetMarkCbfptr) (WID__ sender, int index, p_vbar_mark_t **mark);
typedef int (*GetMTEntrysStateCbfptr) (WID__ sender, int *cindex);


typedef GR_COLOR (*GetItemFgcolorCbfptr) (WID__ sender, int index);
typedef int (*GetItemTypeCbfptr) (WID__ sender, int index);

typedef int (*GetActionCbfptr) (WID__ sender, int index, void **action);
typedef int (*DirFilterCbfptr) (const struct dirent *);

// public APIs.
void		  NeuxFileListboxUpdateIcons(void);		//km
IMAGEID       NeuxLoadImageFromFile(const char *filename);
void          NeuxFreeImage(IMAGEID ID);
BITMAP *      NeuxFontToBitmap(const char *fontname, const char *fonticon, int fontsize, int *bpwidth, int *bpheight);
void          NeuxWidgetShadow(WIDGET *widget);

WIDGET *      NeuxGetWidgetFromWID(int wid);
int           NeuxWidgetWID(WIDGET * widget);
int           NeuxWidgetGC(WIDGET * widget);
void*         NeuxWidgetGetData(WIDGET * widget);
int           NeuxWidgetIsVisible(WIDGET * widget);
int           NeuxWidgetIsEnable(WIDGET * widget);

void          NeuxWidgetShow(WIDGET * widget, int show);
void          NeuxWidgetEnable(WIDGET * widget, int enable);
void          NeuxWidgetFocus(WIDGET * widget);
void          NeuxWidgetSetFocusIdx(WIDGET * widget, int idx);
WIDGET *      NeuxGetActiveWidget(WIDGET * parent);
void          NeuxWidgetMove(WIDGET * widget, int x, int y);
void          NeuxWidgetResize(WIDGET * widget, int w, int h);
void          NeuxWidgetGetSize(WIDGET * widget, int *w, int *h);
void          NeuxWidgetGetProp(WIDGET *widget, widget_prop_t *prop);
void          NeuxWidgetSetProp(WIDGET *widget, const widget_prop_t *prop);
void          NeuxWidgetRedraw(WIDGET *widget);
int           NeuxWidgetGetFontSize(WIDGET *widget);
void          NeuxWidgetSetFont(WIDGET *widget, const char *fontname, int fontsize);
int           NeuxWidgetDestroy(WIDGET ** widget);
void          NeuxWidgetRaise(WIDGET * widget);
GR_BOOL		NeuxWidgetSetBgPixmap(WIDGET *widget, const char *filename);
GR_BOOL		NeuxWidgetUseParentBgPixmap(WIDGET *widget);

GR_BOOL       NeuxWidgetGrabKey(WIDGET *widget, GR_KEY key, KEY_GRAB_TYPE grabtype);
GR_BOOL       NeuxWidgetUngrabKey(WIDGET *widget, GR_KEY key);

BUTTON *      NeuxButtonNew(WIDGET * owner, int x, int y, int w, int h,
							const char * text);
BUTTON *      NeuxButtonNewExt(WIDGET * owner, int x, int y, int w, int h,
							   const char * text,
							   BUTTON_ALIGN align,
							   const char * font,
							   int fontsize,
							   KeydownCbfptr kdn);
void          NeuxButtonSetCallback(BUTTON * button, int cbId, void * fptr);
void          NeuxButtonSetText(BUTTON *button, const char *text);
char *        NeuxButtonGetText(BUTTON * button);
#define       NeuxButtonShow(b, s)      NeuxWidgetShow(b, s)
#define       ButtonEnable(b, e)    NeuxWidgetEnable(b, e)
void          NeuxButtonGetProp(BUTTON * Button, button_prop_t *prop);
void          NeuxButtonSetProp(BUTTON * Button, const button_prop_t *prop);

int           NeuxDoModal(int * endOfModal, int *msgReturn);
int 		  NeuxIsModalActive();
void 		  NeuxClearModalActive();

void          NeuxGetTextSize (WIDGET *widget, char *const text, int len, int *w, int *h, int *b);

FORM *        NeuxFormNew(int x, int y, int w, int h);
void          NeuxFormDestroy(FORM **form);
void          NeuxFormDestroyOnly(FORM **form);
#define       NeuxFormShow(form, show)  NeuxWidgetShow(form, show)
void          NeuxFormSetCallback(FORM * form, int cbId, void * fptr);
#define       FormMove(f, x, y)     NeuxWidgetMove(f, x, y)
void          NeuxFormGetProp(FORM * form, form_prop_t *prop);
void          NeuxFormSetProp(FORM * form, const form_prop_t *prop);

LABEL *       NeuxLabelNew(WIDGET * owner, int x, int y, int w, int h,
                     const char * text);
int 		  NeuxLabelSetScroll(LABEL * label, int interval);
void          NeuxLabelSetText(LABEL * label, const char * text);
char *        NeuxLabelGetText(LABEL * label);
#define       LabelShow(l, s)       NeuxWidgetShow(l, s)
#define       LabelEnable(l, e)     NeuxWidgetEnable(l, e)
void          NeuxLabelGetProp(LABEL * label, label_prop_t *prop);
void          NeuxLabelSetProp(LABEL * label, const label_prop_t *prop);

LISTBOX * 	  NeuxListboxNew(WIDGET * owner, int x, int y, int w, int h,
				GetItemCbfptr getItem,
				GetIconCbfptr getIcon,
				GetItemFgcolorCbfptr getitemfgcolor);

int		NeuxListboxGetPageItems(LISTBOX * listbox);
int 		NeuxListboxTopItem(LISTBOX * listbox);
int           NeuxListboxSelectedItem(LISTBOX * listbox);
void          NeuxListboxRecalcPitems (LISTBOX * listbox);
void 		  NeuxListboxSetIndex(LISTBOX * listbox, int index);
void 	NeuxListboxSetItem(LISTBOX * listbox, int index, int top);
void          NeuxListboxSetCallback(LISTBOX * listbox, int cbId, void * fptr);
int				NeuxListboxGetMultiSelectedCount(LISTBOX * listbox);
int				NeuxListboxGetMultiSelectedData(LISTBOX * listbox, int* selectedIndexarray, int arraysize);
void				NeuxListboxClearMultiSelected(LISTBOX * listbox);

#define       ListboxShow(l, s)     NeuxWidgetShow(l, s)
void          NeuxListboxGetProp(LISTBOX * listbox, listbox_prop_t *prop);
void          NeuxListboxSetProp(LISTBOX * listbox, const listbox_prop_t *prop);

int			NeuxMessageboxNew(KeydownCbfptr keydown_cb,
							HotplugCbfptr hotplug_cb,
							int line_spacing, const char *format, ...);
// km start
int			NeuxFileDialogOpenEx(const char *path, int filtertype, GR_BOOL multisel,
							char ***selectedfiles, int *selectedcount,
							void (*onMsg)(const char * src, /*neux_msg_t*/void * msg),
							int *tts_running,
							int (*validate)(const char *path));

static inline int			NeuxFileDialogOpen(const char *path, int filtertype, GR_BOOL multisel,
							char ***selectedfiles, int *selectedcount,
							void (*onMsg)(const char * src, /*neux_msg_t*/void * msg),
							int *tts_running)
{
	return NeuxFileDialogOpenEx(path,  filtertype,  multisel,selectedfiles, selectedcount,
				onMsg,tts_running,NULL);						
}
//km end
void			NeuxFileDialogFreeSelections(char ***selectedfiles, int selectedcount);

PANEL *       NeuxPanelNew(WIDGET * owner, int x, int y, int w, int h);
PANEL * 		NeuxPanelNewExt(WIDGET * owner, int x, int y, int w, int h, const char * bgfile);

#define       NeuxPanelShow(p, s)        NeuxWidgetShow(p, s)
void          NeuxPanelGetProp(PANEL * panel, panel_prop_t * prop);
void          NeuxPanelSetProp(PANEL * panel, const panel_prop_t * prop);

PICTURE *     NeuxPictureNew(WIDGET * owner, int x, int y, int w, int h,
                       PICTURE_TYPE type, void * buf);
int           NeuxPictureSetPic(PICTURE * picture, PICTURE_TYPE type, const void * buf);
void          NeuxPictureZoom(PICTURE * picture, int zoom);
int           NeuxPictureIsZoomed(PICTURE * picture);
void 		  NeuxPictureRevert(PICTURE * picture);

void          NeuxPicturePanview(PICTURE * picture, int vertical, int horizon);
void          NeuxPictureGetProp(PICTURE * picture, picture_prop_t *prop);
void          NeuxPictureSetProp(PICTURE * picture, const picture_prop_t *prop);

#define       PictureShow(p, s)      NeuxWidgetShow(p, s)
#define       PictureEnable(p, e)    NeuxWidgetEnable(p, e)

PROGRESSBAR * NeuxProgressbarNew(WIDGET * owner, int x, int y, int w, int h,
                              int current, int min, int max, int style);
void          NeuxProgressbarSetControls(PROGRESSBAR * progressbar,
                                      int current, int min, int max);
#define       ProgressbarShow(p, s)  NeuxWidgetShow(p, s)
void          NeuxProgressbarGetProp(PROGRESSBAR * progressbar, progressbar_prop_t *prop);
void          NeuxProgressbarSetProp(PROGRESSBAR * progressbar, const progressbar_prop_t *prop);

int           NeuxScreenHeight(void);
int           NeuxScreenWidth(void);
void          NeuxSnapScreen(const char *path, int x, int y, int w, int h);

SCROLLBAR *   NeuxScrollbarNew(WIDGET * owner, int x, int y, int w, int h,
                            int actvItem, int pageSize, int totalNum, int style);

void          NeuxScrollbarSetControls(SCROLLBAR * scrollbar, int actvItem,
                                    int pageSize, int totalNum);
#define       ScrollbarShow(sc, s)   NeuxWidgetShow(sc, s)

#define 		DELAY_SECOND		1000
TIMER *       NeuxTimerNew(WIDGET * owner, int interval, int xshot);
void          NeuxTimerSetCallback(TIMER * tmr, void * fptr);
void          NeuxTimerSetControl(TIMER *tmr, int interval, int xshot);
void          NeuxTimerSetEnabled(TIMER *tmr, int enable);

#define       NeuxTimerNewWithCallback(owner, interval, xshot, callback) ({\
    TIMER * tmr = NeuxTimerNew(owner, interval, xshot); \
    NeuxTimerSetCallback(tmr, callback); tmr;})
#define       TimerDisable(tmr) NeuxTimerSetEnabled(tmr, FALSE)
#define       TimerEnable(tmr) NeuxTimerSetEnabled(tmr, TRUE)
#define       TimerDelete(tmr) NeuxWidgetDestroy(&tmr)

LINE *        NeuxLineNew(WIDGET * owner, int x, int y, int w, int h);

#define FL_TYPE_BOOK			1
#define FL_TYPE_MUSIC			2
#define FL_TYPE_FILEMANAGEMENT	3

FILELISTBOX * NeuxFileListboxNew(WIDGET * owner, int x, int y, int w, int h, const char * path, int type, GR_BOOL multisel);
void          NeuxFileListboxSetCallback(FILELISTBOX * flbox, int cbId, void * fptr);
char *		  NeuxFileListboxGetPath(FILELISTBOX * flbox);
char *		  NeuxFileListboxGetCurrName(FILELISTBOX * flbox);
int 		  NeuxFileListboxGetCurrType(FILELISTBOX * flbox);		//km
int           NeuxFileListBoxIsFolder(FILELISTBOX * flbox);//appk
char *		  NeuxFileListboxGetFullName(FILELISTBOX * flbox);
char *		  NeuxFileListboxGetFullNameSafe(FILELISTBOX *flbox);		//ztz
int			  NeuxFileListboxGetEntCount(FILELISTBOX * flbox);
int 		  NeuxFileListboxSetPath(FILELISTBOX * flbox, const char * path, int type);
void 		  NeuxFileListboxFileFilter(FILELISTBOX * flbox, DirFilterCbfptr filter);
int			  NeuxFileListboxFindIndex(FILELISTBOX * flbox, char *name, GR_BOOL sequence_search);
char *		  NeuxFileListboxGetFullNameFromIndex(FILELISTBOX * flbox, int index);

typedef IMAGEID 	(*GetImageCbfptr)(WID__ sender, int index);
typedef void 		(*GetBitmapCbfptr)(WID__ sender, int index, BITMAP **bmp);

void          NeuxThemeInit(int theme_index);
void          NeuxThemeGetFile(const char *themeProperty,
                               char *pValue, int iLen);

/*
 * Create a picture from one of the icons
 * defined in the current theme.
 */
PICTURE *     NeuxThemeIconNew(WIDGET * owner,
                          int x, int y, int w, int h,
                          const char *themeElement);
GR_BOOL       NeuxThemeSetBgPixmap(WIDGET *owner,
			              const char *themeElement);
int           NeuxThemeIconSetPic(PICTURE * picture,
                                  const char *themeElement);
IMAGEID       NeuxThemeLoadImage(const char *themeElement);

#endif //NXWIDGET__H
