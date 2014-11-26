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
 * filelistbox widget support routines.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2006-11-07 EY
 *
 */
#include <limits.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "widgets.h"
#include "events.h"
#include "nxlist.h"
#include "file-helper.h"
#include "plextalk-config.h"
#include "storage.h"
#include "plextalk-i18n.h"
#include "plextalk-theme.h"
#include "desktop-ipc.h"

typedef struct
{
	slist_t      list;
	int          topItem;
	int          actItem;
} dir_reg_t;

typedef struct
{
	int             type;
	char			path[PATH_MAX];
	slist_t     	head;
    dir_node_t      node;
	DirFilterCbfptr filter;
} dir_ctrl_t;

#define FILELIST_ICON_DAISY		0
#define FILELIST_ICON_DOC		1
#define FILELIST_ICON_DOCX		2
#define FILELIST_ICON_TEXT		3
#define FILELIST_ICON_EPUB		4
#define FILELIST_ICON_HTML		5
#define FILELIST_ICON_MUSIC		6
#define FILELIST_ICON_FOLDER	7
/* media */
#define FILELIST_ICON_INTERNAL	8
#define FILELIST_ICON_SD		9
#define FILELIST_ICON_USB		10

static const char* icons_name[] = {
	THEME_ICON_FILTER_DAISY,
	THEME_ICON_FILTER_DOC,
	THEME_ICON_FILTER_DOCX,
	THEME_ICON_FILTER_TEXT,
    THEME_ICON_FILTER_EPUB,
	THEME_ICON_FILTER_HTML,

	THEME_ICON_FILTER_MUSIC,
	THEME_ICON_FILTER_FOLDER,
	THEME_ICON_MEDIA_TYPE_INTERNAL,
	THEME_ICON_MEDIA_TYPE_SD,
	THEME_ICON_MEDIA_TYPE_USB,
};

static GR_IMAGE_ID icons[11];
static int icons_inited;

static void
destroyIcons(void)
{
	int i;

	if (--icons_inited > 0)
		return;

	for (i = 0; i < ARRAY_SIZE(icons_name); i++)
		GrFreeImage(icons[i]);
	memset(icons, 0, sizeof(icons));
}

static void
initIcons()
{
	int i;

	if (icons_inited++ > 0)
		return;

	for (i = 0; i < ARRAY_SIZE(icons); i++)
		icons[i] = NeuxThemeLoadImage(icons_name[i]);
}

void
NeuxFileListboxUpdateIcons(void)
{
	int i;

	if (icons_inited <= 0)
		return;

	for (i = 0; i < ARRAY_SIZE(icons_name); i++) {
		GrFreeImage(icons[i]);
		icons[i] = NeuxThemeLoadImage(icons_name[i]);
	}
}

static inline char *
full_path(char * path, char * fname)
{
	static char fpath[PATH_MAX];
	PlextalkCatFilename2Path(fpath, PATH_MAX, path, fname);
	return fpath;
}

static inline int
isDaisyBook(dir_ctrl_t *dc, int index)
{
	return PlextalkIsDsiayBook(full_path(dc->path, dc->node.namelist[index]->d_name));
}

/*
 * CDDA if virtual
 */
static int
scanDir(dir_ctrl_t *dc)
{
	int ret;

	if (dc->type != FL_TYPE_MUSIC)
		return PlextalkScanDir(dc->path, &dc->node, dc->filter);
	
	if (!strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR "/cdda")) {
		int track_count, i;

		dc->node.num = 0;
		dc->node.dnum = 0;

		track_count = CoolCDDAGetTrackCount();
		if (track_count <= 0)
			return -1;

		dc->node.namelist = malloc(track_count * sizeof(struct dirent *));
		if (dc->node.namelist == NULL)
			return -1;

		for (i = 0; i < track_count; i++) {
			dc->node.namelist[i] = malloc(sizeof(struct dirent));
			if (dc->node.namelist[i] == NULL)
				break;
			dc->node.namelist[i]->d_ino = -1;
			dc->node.namelist[i]->d_off = -1;
			dc->node.namelist[i]->d_type = DT_UNKNOWN;
			dc->node.namelist[i]->d_reclen = sprintf(dc->node.namelist[i]->d_name, "Track %d", i + 1);
		}
		dc->node.num = i;
	} else {
		ret = PlextalkScanDir(dc->path, &dc->node, dc->filter);
		if (ret < 0)
			return ret;

		if (!strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR) &&
			CoolCDDAGetTrackCount() > 0) {
			/* add a virtual cdda entry */
			struct dirent ** p = realloc(dc->node.namelist, (dc->node.num + 1) * sizeof(struct dirent *));
			if (p == NULL)
				return 0;
			dc->node.namelist = p;

			dc->node.namelist[dc->node.num] = malloc(sizeof(struct dirent));
			if (dc->node.namelist[dc->node.num] == NULL)
				return 0;
			dc->node.namelist[dc->node.num]->d_ino = -1;
			dc->node.namelist[dc->node.num]->d_off = -1;
			dc->node.namelist[dc->node.num]->d_reclen = strlen("cdda");
			dc->node.namelist[dc->node.num]->d_type = DT_DIR;
			strcpy(dc->node.namelist[dc->node.num]->d_name, "cdda");

			dc->node.num++;
			dc->node.dnum++;
		}
	}

	return 0;
}

static void
getIcon(WID__ sender, int index, BITMAP **iconbmp)
{
	NX_WIDGET *w = (NX_WIDGET *)NxGetFromRegistry(sender);
	dir_ctrl_t *dc = NxGetWidgetData(w);

	*iconbmp = NULL;

	if (!strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR)) {
		if (StringStartWith(dc->node.namelist[index]->d_name, "mmcblk0"))
			*iconbmp = &icons[FILELIST_ICON_INTERNAL];
		else if (StringStartWith(dc->node.namelist[index]->d_name, "mmcblk"))
			*iconbmp = &icons[FILELIST_ICON_SD];
		else if (StringStartWith(dc->node.namelist[index]->d_name, "sd") ||
		         StringStartWith(dc->node.namelist[index]->d_name, "sr") ||
		         !strcmp(dc->node.namelist[index]->d_name, "cdda"))
			*iconbmp = &icons[FILELIST_ICON_USB];
	} else if (!strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR "/cdda")) {
		*iconbmp = &icons[FILELIST_ICON_MUSIC];
	} else {
		if ((dc->node.namelist[index]->d_type != DT_UNKNOWN) ? (dc->node.namelist[index]->d_type == DT_DIR)
		                                                     : PlextalkIsDirectory(dc->node.namelist[index]->d_name)) {
			if (isDaisyBook(dc, index))
				*iconbmp = &icons[FILELIST_ICON_DAISY];
			else
				*iconbmp = &icons[FILELIST_ICON_FOLDER];
		} else {
			if (PlextalkIsMusicFile(dc->node.namelist[index]->d_name))
				*iconbmp = &icons[FILELIST_ICON_MUSIC];
			else {
				char *ext = PlextalkGetFileExtension(dc->node.namelist[index]->d_name);
				if (ext != NULL) {
					if (!strcasecmp(ext, "doc"))
						*iconbmp = &icons[FILELIST_ICON_DOC];
					else if (!strcasecmp(ext, "docx"))
						*iconbmp = &icons[FILELIST_ICON_DOCX];
					else if (!strcasecmp(ext, "txt"))
						*iconbmp = &icons[FILELIST_ICON_TEXT];
					else if (!strcasecmp(ext, "epub"))
						*iconbmp = &icons[FILELIST_ICON_EPUB];
					else if (!strcasecmp(ext, "html") || !strcasecmp(ext, "htm"))
						*iconbmp = &icons[FILELIST_ICON_HTML];
				}
			}
		}
	}
}

static int
getItem(WID__ sender, int index, char **text)
{
	NX_WIDGET *w = (NX_WIDGET *)NxGetFromRegistry(sender);
	dir_ctrl_t *dc = (dir_ctrl_t *)(w->base.data);

	if (index == -1)
		return dc->node.num;

	*text = dc->node.namelist[index]->d_name;

	if (!strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR)) {
		if (StringStartWith(dc->node.namelist[index]->d_name, "mmcblk0"))
			*text = _("Internal Memory");
		else if (StringStartWith(dc->node.namelist[index]->d_name, "mmcblk"))
			*text = _("SD Card");
		else if (StringStartWith(dc->node.namelist[index]->d_name, "sd"))
			*text = _("USB Memory");
		else if (StringStartWith(dc->node.namelist[index]->d_name, "sr"))
			*text = _("CDROM");
		else if (!strcmp(dc->node.namelist[index]->d_name, "cdda"))
			*text = _("CDDA");
	}

	return index - NeuxListboxTopItem(w);
}

static int
keyLeft(dir_ctrl_t *dc, NX_WIDGET *w)
{
	char *ptr;
	dir_reg_t *r;
	int topItem, actItem;

	if (!strcmp(PLEXTALK_MOUNT_ROOT_STR, dc->path))
		return -1;

	ptr = strrchr(dc->path, '/');
	*ptr++ = '\0';

	PlextalkDestroyDir(&dc->node);
	scanDir(dc);

	if (nxSlistEmpty(&dc->head)) {
		actItem = NeuxFileListboxFindIndex(w, ptr, !strcmp(PLEXTALK_MOUNT_ROOT_STR, dc->path));
		if (actItem < 0)
			actItem = 0;
		NeuxListboxSetIndex(w, actItem);
		return 0;
	}

	r = container_of(dc->head.next, dir_reg_t, list);
	dc->head.next = r->list.next;
	actItem = r->actItem;
	topItem = r->topItem;
	free(r);

	/* new media may be added */
	if (!strcmp(PLEXTALK_MOUNT_ROOT_STR, dc->path)) {
		int i = NeuxFileListboxFindIndex(w, ptr, GR_TRUE);
		if (i < 0)
			i = 0;
		if (actItem != i) {
			NeuxListboxSetIndex(w, i);
			return 0;
		}
	}

	NeuxListboxSetItem(w, actItem - topItem, topItem);
	return 0;
}

static int
keyRight(dir_ctrl_t *dc, NX_WIDGET *w)
{
	dir_reg_t *r;
	int topItem, actItem;

	if (dc->node.num <= 0) 
		return -1;

	actItem = NeuxListboxSelectedItem(w);
	
	if (actItem >= dc->node.dnum || isDaisyBook(dc, actItem)) 
		return -1;
	
	r = (dir_reg_t*)malloc(sizeof(dir_reg_t));
	if (r == NULL)
		return -1;

	r->topItem = NeuxListboxTopItem(w);
	r->actItem = actItem;
	nxSlistInsert(&dc->head, &r->list);

	strlcat(dc->path, "/", sizeof(dc->path));
	strlcat(dc->path, dc->node.namelist[actItem]->d_name, sizeof(dc->path));

	PlextalkDestroyDir(&dc->node);
	scanDir(dc);

	NeuxListboxSetIndex(w, 0);

	return 0;
}

static int
setPath(FILELISTBOX * flbox, const char * path, int type)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	char *fname = NULL;
	int actItem;

	dc->type = type;
	switch (type) {
	case FL_TYPE_BOOK:
		dc->filter = PlextalkFilterBook;
		break;
	case FL_TYPE_MUSIC:
		dc->filter = PlextalkFilterMusic;
		break;
	default:
		dc->filter = PlextalkFilterAllUnhidden;//PlextalkFilterAll; //km
		break;
	}

	if (type != FL_TYPE_MUSIC && StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR "cdda"))
		path = PLEXTALK_MOUNT_ROOT_STR;

	strlcpy(dc->path, path, sizeof(dc->path));
	if (strcmp(dc->path, PLEXTALK_MOUNT_ROOT_STR) != 0) {
		fname = strrchr(dc->path, '/');
		*fname++ = '\0';
	}

	scanDir(dc);

	actItem = 0;
	if (fname != NULL && *fname != '\0') {
		actItem = NeuxFileListboxFindIndex(flbox, fname, !strcmp(PLEXTALK_MOUNT_ROOT_STR, dc->path));
		if (actItem < 0)
			actItem = 0;
	}

	NeuxListboxSetIndex(flbox, actItem);
}

static void
FileListboxFreeCtrl(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);

	nxSlistFree(&dc->head, offsetof(dir_reg_t, list));
	PlextalkDestroyDir(&dc->node);
	free(dc);
}

//km start
static int
isFileListboxMaxDepth(dir_ctrl_t *dc)
{
	int cnt = 0;
	char *curr;
	if(dc->node.num > 0){
		curr = full_path(dc->path, dc->node.namelist[0]->d_name);
		//DBGMSG("isFileListboxMaxDepth :%s\n",curr);
		while(*curr){
			if(*curr++ == '/')
				cnt++;
		}
		
	}
	//DBGMSG("isFileListboxMaxDepth cnt:%d\n",cnt);

	return (cnt >= 10);//isFileListMaxDepth
	
}

static void
onKeyDown(NX_WIDGET *sender, DATA_POINTER ptr)
{

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))//app
		return;

	if (NeuxAppGetKeyLock(0)) {

		 DBGMSG("File Listbox the key is locked!\n");
		 return;
	 }//app

	dir_ctrl_t *dc = NxGetWidgetData(sender);

	switch (sender->base.event.keystroke.ch)
	{
	case MWKEY_LEFT:
		NeuxListboxClearMultiSelected(sender);
		keyLeft(dc, sender);
		break;
	case MWKEY_ENTER:
		/* music may select album to play, or file-management may popup menu */
		//if (dc->type != FL_TYPE_BOOK)
			break;
		/* fall through */
	case MWKEY_RIGHT:
		if(isFileListboxMaxDepth(dc))//app
		{
			//DBGMSG("onKeyDown have 8 layers!\n");
			break;
		}
		NeuxListboxClearMultiSelected(sender);
		keyRight(dc, sender);
		break;
	}

	KEY__ tempkey;
	tempkey.ch = sender->base.event.keystroke.ch;
	tempkey.hotkey = sender->base.event.keystroke.hotkey;
	(*(KeydownCbfptr)ptr)(sender->base.wid, tempkey);
}
//km end
static void
onDestroy(NX_WIDGET *sender, DATA_POINTER ptr)
{
	FileListboxFreeCtrl(sender);
	destroyIcons();
}

FILELISTBOX *
NeuxFileListboxNew(WIDGET * owner,
                   int x, int y, int w, int h,
                   const char * path, int type,
                   GR_BOOL multisel)
{
	NX_WIDGET * listbox;
	dir_ctrl_t *dc;

	initIcons();

	listbox = NxNewWidget(WIDGET_TYPE_LISTBOX, (NX_WIDGET *)owner);
	listbox->base.posx = x;
	listbox->base.posy = y;
	listbox->base.width = w;
	listbox->base.height = h;
	listbox->spec.listbox.cb_geticon = getIcon;
	listbox->spec.listbox.cb_getitem = getItem;
	listbox->spec.listbox.sbar.fgcolor = theme_cache.foreground;
	listbox->spec.listbox.sbar.bordercolor = theme_cache.foreground;
	listbox->spec.listbox.sbar.bgcolor = theme_cache.background;
	listbox->spec.listbox.sbar.width = SBAR_WIDTH;
	listbox->spec.listbox.useicon = TRUE;
	listbox->spec.listbox.itemmargin.top = 0;
	listbox->spec.listbox.itemmargin.bottom = 0;
	listbox->spec.listbox.itemmargin.left = 0;
	listbox->spec.listbox.itemmargin.right = 0;
	listbox->spec.listbox.textmargin.top = 0;
	listbox->spec.listbox.textmargin.bottom = 0;
	listbox->spec.listbox.textmargin.left = 1;
	listbox->spec.listbox.textmargin.right = 1;
	listbox->spec.listbox.iconregion.w = 16;
	listbox->spec.listbox.iconregion.h = -1;
	listbox->spec.listbox.ismultiselec = multisel;
	listbox->spec.listbox.bScroll = GR_TRUE;
	listbox->spec.listbox.scroll_endingscrollcnt = -1;
	listbox->spec.listbox.graphictype = ltImage;

	// if any of the specified position is -1, we'll rely on layout manager to position it.
	if ((-1 != x) && (-1 != y))
	{
		NxCreateWidget(listbox);
	}

	NxListboxRecalcPitems(listbox);

	dc = (dir_ctrl_t*)malloc(sizeof(dir_ctrl_t));
	dc->head.next = NULL;

	NxSetWidgetData(listbox, dc);

	setPath(listbox, path, type);

	NxRegisterCallBack(&listbox->spec.listbox.cb_destroy, onDestroy, NULL);
	return listbox;
}

/** Function sets call back of the filelistbox.
 *
 * @param flbox
 *       target filelistbox.
 * @param cbId
 *       call back function ID.
 * @param fptr
 *       call back function pointer.
 *
 */
void
NeuxFileListboxSetCallback(FILELISTBOX * flbox, int cbId, void * fptr)
{
	NX_LISTBOX *me = &((NX_WIDGET *)flbox)->spec.listbox;

	switch (cbId)
	{
	case CB_KEY_DOWN:
		NxRegisterCallBack(&me->cb_keydown, onKeyDown, fptr);
		break;
	default:
		NeuxListboxSetCallback(flbox, cbId, fptr);
		break;
	}
}

int
NeuxFileListboxSetPath(FILELISTBOX * flbox, const char * path, int type)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);

	nxSlistFree(&dc->head, offsetof(dir_reg_t, list));
	PlextalkDestroyDir(&dc->node);

	setPath(flbox, path, type);
	return 0;
}

char *
NeuxFileListboxGetPath(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	return dc->path;
}

char *
NeuxFileListboxGetCurrName(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	return dc->node.namelist[NeuxListboxSelectedItem(flbox)]->d_name;
}

//km start
int
NeuxFileListboxGetCurrType(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	return dc->node.namelist[NeuxListboxSelectedItem(flbox)]->d_type;
}
int NeuxFileListBoxIsFolder(FILELISTBOX * flbox)
{
	int types;
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	
	if(dc->node.num < 1 || !dc->node.namelist)
		return 0;
	types = dc->node.namelist[NeuxListboxSelectedItem(flbox)]->d_type;

	if(PlextalkIsDsiayBook(NeuxFileListboxGetFullName(flbox)))
		return 0;
	else if(types == DT_DIR)
		return 1;
	else 
		return 0;
}
//km end

int
NeuxFileListboxFindIndex(FILELISTBOX * flbox, char *name, GR_BOOL sequence_search)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	int i;

	if (sequence_search) {
		for (i = 0; i < dc->node.dnum; i++) {
//			if (!strcoll(name, dc->node.namelist[i]->d_name))
			if (!strcasecmp(name, dc->node.namelist[i]->d_name))
				return i;
		}
	} else {
		int l, h;

		l = 0;
		h = dc->node.dnum - 1;
		for (i = 0; i < 2; i++)
		{
			while (l <= h)
			{
				int m = (l + h) / 2;
//				int r = strcoll(name, dc->node.namelist[m]->d_name);
				int r = strcasecmp(name, dc->node.namelist[m]->d_name);
				if (r < 0)
					h = m - 1;
				else if (r > 0)
					l = m + 1;
				else
					return m;
			}

			l = dc->node.dnum;
			h = dc->node.num - 1;
		}
	}

	return -1;
}

char *
NeuxFileListboxGetFullName(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	int slected = NeuxListboxSelectedItem(flbox);
	return full_path(dc->path, dc->node.namelist[slected]->d_name);
}

char *
NeuxFileListboxGetFullNameSafe(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	int slected = NeuxListboxSelectedItem(flbox);

	if (dc->node.num == 0)
		return dc->path;
	else
		return full_path(dc->path, dc->node.namelist[slected]->d_name);
}


char *
NeuxFileListboxGetFullNameFromIndex(FILELISTBOX * flbox, int index)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	return full_path(dc->path, dc->node.namelist[index]->d_name);
}

int
NeuxFileListboxGetEntCount(FILELISTBOX * flbox)
{
	dir_ctrl_t *dc = NxGetWidgetData(flbox);
	return dc->node.num;
}
