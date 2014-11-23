/*
 *  Copyright(C) 2006-2007 Neuros Technology International LLC.
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
 * Widget routines.
 *
 * REVISION:
 *
 * 4) Widget ID hash table added in to save tree traversal time when
 *    widget was destroyed. ------------------------------- 2007-05-09 MG
 * 3) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 2) Remove global reference 'WidgetsChain'.-------------- 2006-05-03 MG
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nx-widgets.h"
#include "wm.h"
#include "shared-mem.h"
#include "neux.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"

// top of the widgets registration tree.
//static NX_WIDGET * curWidget = NULL;
extern NX_WIDGET * mainApp;

// wid<-->NX_WIDGET* hash table.
#define WIDGET_HASH_SIZE 63 /* 2^n-1 */
// TBD: hash algorithm needed, currently a simple cache
#define whash(wid) ((wid)&WIDGET_HASH_SIZE)

static NX_WIDGET * widgetHash[WIDGET_HASH_SIZE + 1];
static int         widgetIdHash[WIDGET_HASH_SIZE + 1];

static void focusin_preprocess(GR_EVENT *event, NX_WIDGET *widget)
{
	NX_WIDGET *parent;
	NX_WIDGET *ww;
	NX_WINDOW *window;
	int counter = 0;

    if (widget->base.type == WIDGET_TYPE_WINDOW)
	{
		window = &widget->spec.container.spec.window;
		// window manage focus for any of its children.
		if (window->focusTabLen)
		{
			while (counter < window->focusTabLen)
			{
				ww = window->focus[(window->focusIdx + counter)%window->focusTabLen];
				// only set focus when it is visible and enabled.
				if ((ww->base.visible == GR_TRUE)&&(ww->base.enabled == GR_TRUE))
				{
					GrRaiseWindow(ww->base.wid);
					GrSetFocus (ww->base.wid);
					window->focusIdx = (window->focusIdx + counter)%window->focusTabLen;
					window->activewidget = ww;
					break;
				}
				counter++;
			}
			// don't find a sub widget which is visible and enabled.
			if (counter == window->focusTabLen)
				window->activewidget = NULL;
		}
	}
	else
	{
		//Search top FORM
		parent = widget;
		while ((parent != NULL) && WIDGET_TYPE_WINDOW != parent->base.type)
		{
			parent = NxGetWidgetParent(parent);
		}
		window = &parent->spec.container.spec.window;
		// window manage focus for any of its children.
		if (window->focusTabLen)
		{
			if (widget != window->focus[window->focusIdx])
			{
				while (counter < window->focusTabLen)
				{
					// reset the focus index of window
					if (widget == window->focus[counter])
					{
						window->focusIdx = counter;
						window->activewidget = widget;
						break;
					}
					counter ++;
				}
			}
		}
	}
	DBGLOG("WIDGET focusin preprocess returning.\n");
}


// only release client resource, don't call 'GrDestroyWindow' to destroy child
static int destroywidget(NX_WIDGET **w)
{
	NX_WIDGET *widget;
	NX_WIDGET *next;
	NX_WIDGET *nnext;

	widget = *w;

	DBGLOG("DESTROYING: w: %p, *w: %p\n", w, *w);
	// invalidate widget
	if (widget->check.runningId != widget->check.checkId) return 1;
	widget->check.runningId--;
	DBGLOG("DESTROYING starts\n");

	// destroy recursively if child widget is present.
	if (widget->child)
	{
		DBGLOG("DESTROYING child: %p\n", widget->child);
		next = widget->child->next;
		destroywidget(&widget->child);
		while (next)
		{
			nnext = next->next;
			destroywidget(&next);
			next = nnext;
		}
	}

	DBGLOG("DESTROYING: w: %p, wid: %d\n", widget, widget->base.wid);
	GrDestroyGC(widget->base.gc);
	if (widget->base.bgpixmap > 0)
		GrDestroyWindow(widget->base.bgpixmap);
	free(widget->base.name);
	if (widget->base.fontid > 0)
		NxPutFontID(widget->base.fontid);
	if (widget->base.fontname)
		free(widget->base.fontname);

	switch (widget->base.type)
	{
	case WIDGET_TYPE_APPLICATION:
		DBGLOG("destroying APPLICATION\n");
		NxAppDestroy(widget);
		break;
	case WIDGET_TYPE_WINDOW:
		DBGLOG("destroying WINDOW\n");
		NxWindowDestroy(widget);
		break;
	case WIDGET_TYPE_BUTTON:
		DBGLOG("destroying BUTTON\n");
		NxButtonDestroy(widget);
		break;
	case WIDGET_TYPE_LISTBOX:
		DBGLOG("destroying LISTBOX\n");
		NxListboxDestroy(widget);
		break;
	case WIDGET_TYPE_TIMER:
		DBGLOG("destroying TIMER\n");
		NxTimerDestroy(widget);
		break;
	case WIDGET_TYPE_LABEL:
		DBGLOG("destroying LABEL\n");
		NxLabelDestroy(widget);
		break;
	case WIDGET_TYPE_PICTURE:
		DBGLOG("destroying PICTURE\n");
		NxPictureDestroy(widget);
		break;
	case WIDGET_TYPE_PANEL:
		DBGLOG("destroying PANEL\n");
		NxPanelDestroy(widget);
		break;
	case WIDGET_TYPE_LINE:
		DBGLOG("destroying LINE\n");
		NxLineDestroy(widget);
		break;
	case WIDGET_TYPE_PROGRESSBAR:
		DBGLOG("destroying PROGRESSBAR\n");
		NxProgressBarDestroy(widget);
		break;
	case WIDGET_TYPE_SCROLLBAR:
		DBGLOG("destroying SCROLLBAR\n");
		NxScrollBarDestroy(widget);
		break;
	}

	//FIXME: release widget object here?
	*w = NULL;

	return 1;
}

// public APIs.
/**
 * Main event handle routine.
 *
 * @param event
 *       Nano-X event.
 */
void
NxEventResolveRoutine (GR_EVENT * event)
{
	GR_WINDOW_ID wid;
	NX_WIDGET *widget;

	WmStartApp();

	switch (event->type)
	{
	case GR_EVENT_TYPE_ERROR:
		GrDefaultErrorHandler (event);
		return;
	case GR_EVENT_TYPE_NONE:
		NxAppIdleHandler(event, mainApp);
		return;
	case GR_EVENT_TYPE_SCREENSAVER:
		NxAppScreenSaverHandler(event, mainApp);
		return;
	case GR_EVENT_TYPE_FD_ACTIVITY:
		nxAppSourceEventHandler(event->fd.fd, event->fd.can_read, event->fd.can_write);
		return;
	case GR_EVENT_TYPE_MSG:
		NxAppMsgHandler(event, mainApp);
		return;
	case GR_EVENT_TYPE_SW_ON:
		NxAppSWOnHandler(event, mainApp);
		return;
	case GR_EVENT_TYPE_SW_OFF:
		NxAppSWOffHandler(event, mainApp);
		return;
	}

	wid = event->general.wid;
	widget = NxGetFromRegistry (wid);
	if (widget == NULL)
	{
		WARNLOG("NULL widget detected.\n");
		if ((event->type != GR_EVENT_TYPE_SCREENSAVER) &&
			(event->type != GR_EVENT_TYPE_FD_ACTIVITY))
		{
			WARNLOG("bogus event: wid = %d  type = %d\n", wid, event->type);
			return;
		}
	}
	else
		widget->base.event = *event;

	//DBGLOG("event type = %d wid = %d \n", event->type, wid);

	switch (event->type)
	{
	case GR_EVENT_TYPE_EXPOSURE:
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			NxButtonExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_LABEL:
			NxLabelExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_PICTURE:
			NxPictureExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_PANEL:
			NxPanelExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_LINE:
			NxLineExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_PROGRESSBAR:
			NxProgressBarExposureHandler(event, widget);
			break;
		case WIDGET_TYPE_SCROLLBAR:
			NxScrollBarExposureHandler(event, widget);
			break;
		}
		break;
	case GR_EVENT_TYPE_BUTTON_DOWN:
		break;
	case GR_EVENT_TYPE_BUTTON_UP:
		break;
	case GR_EVENT_TYPE_MOUSE_ENTER:
		break;
	case GR_EVENT_TYPE_MOUSE_EXIT:
		break;
	case GR_EVENT_TYPE_MOUSE_MOTION:
		break;
	case GR_EVENT_TYPE_MOUSE_POSITION:
		break;
	case GR_EVENT_TYPE_KEY_DOWN:

		// *****************
		// if not hotkey and not get focus on current widget, get rid of the current event.
		// But in editbox, this widget will call IME or softkeyboard and not display them,
		// in this case, keep the current event -- chain,
//		if (!event->keystroke.hotkey)
//		{
//			if (widget->base.type != WIDGET_TYPE_EDITBOX &&
//				widget->base.wid != GrGetFocus())
//				return;
//		}

		if (WmIsAppRunning("help") && !event->keystroke.hotkey)
			return;
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowKeyDownHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			NxButtonKeyDownHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxKeyDownHandler(event, widget);
			break;
		case WIDGET_TYPE_PANEL:
			NxPanelKeyDownHandler(event, widget);
			break;
		}
		break;
	case GR_EVENT_TYPE_KEY_UP:
		if (WmIsAppRunning("help") && !event->keystroke.hotkey)
			return;
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowKeyUpHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			NxButtonKeyUpHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxKeyUpHandler(event, widget);
			break;
		}
		break;
	case GR_EVENT_TYPE_FOCUS_IN:
		DBGLOG("focusin event = %d\n"
			   "wid           = %d\n"
			   "event         = %p\n", event->type, wid, event);

		if (widget->base.wid != GrGetFocus())
			break;
		focusin_preprocess(event, widget);
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowFocusInHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			NxButtonFocusInHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxFocusInHandler(event, widget);
			break;
		case WIDGET_TYPE_LABEL:
			break;
		}
		break;
	case GR_EVENT_TYPE_FOCUS_OUT:
		/*
		  DBGLOG("focusin event = %d\n"
				 "wid           = %d\n"
				 "event         = %d\n", event->type, wid, event);
		*/
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowFocusOutHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			NxButtonFocusOutHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxFocusOutHandler(event, widget);
			break;
		case WIDGET_TYPE_LABEL:
			break;
		}
		break;
	case GR_EVENT_TYPE_UPDATE:
		break;

	case GR_EVENT_TYPE_CHLD_UPDATE:
		/* well let's make things really simple, WM does not even
		   consider to manage application that is not built through
		   Neux widgets.
		if (widget->base.type == WIDGET_TYPE_WMAPPLICATION)
		{
			WmOnChildUpdate(&event->update);
		}
		*/
		break;

	case GR_EVENT_TYPE_CLOSE_REQ:
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowCloseReqHandler(event, widget);
			break;
		case WIDGET_TYPE_BUTTON:
			break;
		}
		break;
	case GR_EVENT_TYPE_TIMER:
		if (widget == NULL)
			break;
		switch (widget->base.type)
		{
		case WIDGET_TYPE_TIMER:
			NxTimerTimerHandler(event, widget);
			break;
		case WIDGET_TYPE_LABEL:
			NxLabelTimerHandler(event, widget);
			break;
		case WIDGET_TYPE_LISTBOX:
			NxListboxTimerHandler(event, widget);
			break;
		}
		break;
	case GR_EVENT_TYPE_HOTPLUG:
		switch (widget->base.type)
		{
		case WIDGET_TYPE_WINDOW:
			NxWindowHotplugHandler(event, widget);
			break;
		}
		break;
	default:
		break;
	}
}

/**
 * Function creates a widget data structure and initializes with
 * default values.
 *
 * @param type
 *       widget type.
 * @param parent
 *       parent widget.
 * @return
 *       newly created widget data pointer.
 */
NX_WIDGET *
NxNewWidget (int type, NX_WIDGET * parent)
{
	NX_CONTAINER * ctn = NULL;
	NX_WIDGET* tmp;

	tmp=(NX_WIDGET *) malloc (sizeof (NX_WIDGET));
	memset(tmp, 0, sizeof(NX_WIDGET));

	tmp->parent = parent;

	tmp->base.type = type;
	//tmp->base.pid = parent->base.wid;
	//tmp->base.ffid = 0;

	tmp->base.name = NULL;
	tmp->base.posx = 0;
	tmp->base.posy = 0;
	tmp->base.width = 0;
	tmp->base.height = 0;

	tmp->base.fgcolor = theme_cache.foreground;
	tmp->base.bgcolor = theme_cache.background;
	tmp->base.fontid = 0;
	tmp->base.fontname = NULL;
	tmp->base.fontsize = sys_font_size;

	tmp->base.visible = GR_FALSE;
	tmp->base.enabled = GR_TRUE;
	//tmp->base.focused = GR_FALSE;
	tmp->base.gc = GrNewGC();

	tmp->base.usebgpixmap = GR_TRUE;
	tmp->base.bgpixmap = 0;

	tmp->base.tag = NULL;
	tmp->base.data = NULL;

	switch (type)
	{
	case WIDGET_TYPE_WINDOW:
		DBGLOG("NEW FORM: %p\n", tmp);
		tmp->spec.container.spec.window.caption = NULL;
		tmp->spec.container.spec.window.imageid = 0;
		tmp->spec.container.spec.window.transparent = GR_FALSE;
		tmp->spec.container.spec.window.activewidget = NULL;
		tmp->spec.container.spec.window.focusTabLen = 0;
		break;
	case WIDGET_TYPE_BUTTON:
		DBGLOG("NEW BUTTON: %p\n", tmp);
		tmp->base.fontname = strdup(sys_font_name);
		tmp->spec.button.imageid = 0;
		tmp->spec.button.align = btCenter;
		tmp->spec.button.caption = NULL;
		tmp->spec.button.type = btFlat;
		tmp->spec.button.bredrawflag = GR_TRUE;
		tmp->spec.button.selectedcolor = theme_cache.selected;
		tmp->spec.button.shadowdcolor = GRAY;
		tmp->spec.button.cb_getimage = NULL;
		break;
	case WIDGET_TYPE_LISTBOX:
		DBGLOG("NEW LISTBOX: %p\n", tmp);
		tmp->base.fontname = strdup(sys_font_name);
		tmp->spec.listbox.useicon = GR_TRUE;
		tmp->spec.listbox.sbar.width = SBAR_WIDTH;
		tmp->spec.listbox.sbar.fgcolor = theme_cache.foreground;
		tmp->spec.listbox.sbar.bordercolor = theme_cache.foreground;
		tmp->spec.listbox.sbar.bgcolor = theme_cache.background;
		tmp->spec.listbox.ismultiselec = GR_FALSE;
		tmp->spec.listbox.multiseleclist.next = NULL;
		tmp->spec.listbox.selectedcount = 0;
		tmp->spec.listbox.selectmarkcolor = theme_cache.selected;
		tmp->spec.listbox.selectedcolor = theme_cache.selected;
		tmp->spec.listbox.selectedfgcolor = theme_cache.foreground;
		tmp->spec.listbox.count = 0;
		tmp->spec.listbox.imageid = 0;
		tmp->spec.listbox.itemmargin.left = 3;
		tmp->spec.listbox.itemmargin.right = 3;
		tmp->spec.listbox.itemmargin.top = 3;
		tmp->spec.listbox.itemmargin.bottom = 3;
		tmp->spec.listbox.cb_geticon = NULL;
		tmp->spec.listbox.cb_getitem = NULL;
		tmp->spec.listbox.cb_getitemfgcolor = NULL;
		tmp->spec.listbox.graphictype = ltIcon;
		break;
	case WIDGET_TYPE_TIMER:
		DBGLOG("NEW TIMER: %p\n", tmp);
		tmp->base.visible = GR_FALSE;
		tmp->spec.timer.timerid = 0;
		tmp->spec.timer.xshot = -1;
		break;
	case WIDGET_TYPE_LABEL:
		DBGLOG("NEW LABEL: %p\n", tmp);
		tmp->base.fontname = strdup(sys_font_name);
		tmp->spec.label.autosize = GR_TRUE;
		tmp->spec.label.align = LA_LEFT;
		tmp->spec.label.imageid = 0;
		tmp->spec.label.caption = NULL;
		tmp->spec.label.transparent = GR_FALSE;
		tmp->spec.label.transb = GR_FALSE;
		tmp->spec.label.automshow = GR_FALSE;
		tmp->spec.label.interval = 500;	 //0.5 s
		tmp->spec.label.mshowx = 0;
		tmp->spec.label.mshowt = 0;
		break;
	case WIDGET_TYPE_PICTURE:
		DBGLOG("NEW PICTURE: %p\n", tmp);
		tmp->spec.picture.buf = NULL;
		tmp->spec.picture.imageid = 0;
		tmp->spec.picture.zoomable = GR_FALSE;
		tmp->spec.picture.bredrawflag = GR_TRUE;
		tmp->spec.picture.zoom = 0;
		break;
	case WIDGET_TYPE_PANEL:
		DBGLOG("NEW PANEL: %p\n", tmp);
		tmp->spec.container.spec.panel.imageid = 0;
		tmp->spec.container.spec.panel.canfocus = GR_FALSE;
		break;
	case WIDGET_TYPE_PROGRESSBAR:
		DBGLOG("NEW PROGRESSBAR: %p\n", tmp);
		tmp->spec.progressbar.orientation = omHorizontal;
		tmp->spec.progressbar.type = ptNormal;
		tmp->spec.progressbar.min = 0;
		tmp->spec.progressbar.max = 100;
		tmp->spec.progressbar.value = 0;
		tmp->spec.progressbar.borderwidth = theme_cache.foreground;
		tmp->spec.progressbar.blockcolor = theme_cache.foreground;
		break;
	}

	// validate widget
	{
		static short widgetRunningId;
		tmp->check.runningId = widgetRunningId++;
		tmp->check.checkId = tmp->check.runningId;
	}

	// update container context.
	if ((parent->base.type == WIDGET_TYPE_WINDOW) ||
		(parent->base.type == WIDGET_TYPE_PANEL))
	{
		ctn = &parent->spec.container;

		//DBGLOG("childNum = %d, ww = %p\n", ctn->childNum, tmp);
		ctn->base.childTab[ctn->base.childNum] = tmp;
		ctn->base.childNum++;
	}

	return tmp;
}


/**
 * Function creates the widget object.
 *
 * @param widget
 *       widget data pointer.
 * @return
 *       1 if successfully created.
 */
int
NxCreateWidget (NX_WIDGET *widget)
{
	//NxSetGCBackground(widget->base.gc, widget->base.bgcolor);
	switch (widget->base.type)
	{
	case WIDGET_TYPE_WINDOW:
		NxWindowCreate(widget);
		break;
	case WIDGET_TYPE_BUTTON:
		NxButtonCreate(widget);
		break;
	case WIDGET_TYPE_LISTBOX:
		NxListboxCreate(widget);
		break;
	case WIDGET_TYPE_TIMER:
		DBGLOG("creating timer widget: %p\n", widget);
		NxTimerCreate(widget);
		break;
	case WIDGET_TYPE_LABEL:
		NxLabelCreate(widget);
		break;
	case WIDGET_TYPE_PICTURE:
		NxPictureCreate(widget);
		break;
	case WIDGET_TYPE_PANEL:
		NxPanelCreate(widget);
		break;
	case WIDGET_TYPE_LINE:
		NxLineCreate(widget);
		break;
	case WIDGET_TYPE_PROGRESSBAR:
		NxProgressBarCreate(widget);
		break;
	case WIDGET_TYPE_SCROLLBAR:
		NxScrollBarCreate(widget);
		break;
	}

	// FORM is the only one that gets realized when first time it is showed.
	// everything else is realized immediately after it is created.
	if (widget->base.type != WIDGET_TYPE_WINDOW)
		widget->base.realized = GR_TRUE;

	if ((widget->base.visible == GR_TRUE) && (widget->base.realized == GR_TRUE))
	{
		GrMapWindow (widget->base.wid);
	}

	NxAddToRegistry(widget);
	return 1;
}

void
NxRedrawWidget (NX_WIDGET *widget)
{
	switch (widget->base.type)
	{
	case WIDGET_TYPE_WINDOW:
		NxWindowDraw(widget);
		break;
	case WIDGET_TYPE_BUTTON:
		NxButtonDraw(widget);
		break;
	case WIDGET_TYPE_LISTBOX:
		NxListboxDraw(widget);
		break;
	case WIDGET_TYPE_LABEL:
		NxLabelDraw(widget);
		break;
	case WIDGET_TYPE_PICTURE:
		NxPictureDraw(widget);
		break;
	case WIDGET_TYPE_PANEL:
		NxPanelDraw(widget);
		break;
	case WIDGET_TYPE_LINE:
		NxLineDraw(widget);
		break;
	case WIDGET_TYPE_PROGRESSBAR:
		NxProgressBarDraw(widget);
		break;
	case WIDGET_TYPE_SCROLLBAR:
		NxScrollBarDraw(widget);
		break;
	}
}

/**
 * Function destroyes the widget object and releases all data it holds.
 *
 * @param widget
 *       target widget.
 * @return
 *       1 if successful.
 */
int
NxDestroyWidget(NX_WIDGET **w)
{
	if (*w == NULL)
		return 1;
	int wid = (*w)->base.wid;

	/*
	 only call 'GrDestroyWindow' to destroy parent . It is not
	 necessary to call 'GrDestroyWindow' to destroy child. if it is
	 so, parent will get GR_EVENT_TYPE_EXPOSURE, but parent
	 has been destroy.
	*/
	if (wid != GR_ROOT_WINDOW_ID)
		GrDestroyWindow(wid);
	else
	{
		// destroy application form
		NX_WIDGET* pwidget = (*w)->child;
		while (pwidget)
		{
			GrDestroyWindow(pwidget->base.wid);
			pwidget = pwidget->next;
		}
	}

	destroywidget(w);

	return 1;
}

void
NxSetWidgetVisible(NX_WIDGET *widget, GR_BOOL visible)
{
	widget->base.visible = visible;
	if (GR_TRUE == widget->base.realized)
	{
		if (visible == GR_FALSE)
		{
			//DBGLOG("unmapping window: %d\n", widget->base.wid);
			GrUnmapWindow(widget->base.wid);
		}
		else
		{
			//DBGLOG("mapping window: %d\n", widget->base.wid);
			GrMapWindow(widget->base.wid);
		}
	}
}

void
NxSetNeuxWidgetEnabled(NX_WIDGET *widget, GR_BOOL enabled)
{
	GR_WINDOW_INFO winfo;
	GR_WINDOW_ID siblingwid;
	NX_WIDGET *sibling;

	widget->base.enabled = enabled;
	GrClearWindow(widget->base.wid, GR_TRUE);
	//if(widget->type == TN_RADIOBUTTONGROUP)
	{
		GrGetWindowInfo(widget->base.wid, &winfo);
		siblingwid = winfo.child;
		while (siblingwid != 0)
		{
			sibling = NxGetFromRegistry(siblingwid);
			if (sibling)
			{
				sibling->base.enabled = enabled;
				GrClearWindow(sibling->base.wid, GR_TRUE);
			}
			GrGetWindowInfo(siblingwid, &winfo);
			siblingwid = winfo.sibling;
		}
	}
}

// set widget focus and adjust its root form/window z-order.
// do not raise form/window here though, upper level call shall handle that.
void
NxSetWidgetFocus(NX_WIDGET *widget)
{
	if ((widget->base.visible == GR_FALSE) || (widget->base.realized == GR_FALSE)) return;

//	if (WmTopAppIsSelf() == 1 && !WmAppSerialIsLocked()) {
		GrRaiseWindow(widget->base.wid);
		GrSetFocus (widget->base.wid);
//	}

	//now centrally manage the z-order and container focus stack.
	{
		NX_WIDGET *parent;

		//Search top FORM and reset Z-order.
		parent = widget;
		while ((parent != NULL) && WIDGET_TYPE_WINDOW != parent->base.type)
		{
			parent = NxGetWidgetParent(parent);
		}

		if (parent != NULL && WIDGET_TYPE_WINDOW == parent->base.type)
		{
			NX_WINDOW * w = &parent->spec.container.spec.window;

			if (WIDGET_TYPE_WINDOW != widget->base.type)
				w->activewidget = (NX_WIDGET *)widget;

			//DBGLOG("WmChangeFormZorder...\n");
			WmChangeFormZorder(parent->base.wid);

			// if widget itself is not FORM, search to change focusIdx.
			if (WIDGET_TYPE_WINDOW != widget->base.type)
			{
				int ii = w->focusTabLen;
				while (ii--)
				{
					if (w->focus[ii] == widget)
					{
						w->focusIdx = ii;
						break;
					}
				}
			}
		}
	}
}

void
NxSetWidgetFocusIdx(WIDGET * widget, int idx)
{
	NX_WIDGET *parent = widget;

	// API is invalid to FORM itself.
	if (WIDGET_TYPE_WINDOW == ((NX_WIDGET *)widget)->base.type)	return;
	if (idx >= FORM_MAX_FOCUS_IDX)
	{
		WARNLOG("Index is out of range.\n");
		return;
	}

	//Search for top FORM
	while ((parent != NULL) && WIDGET_TYPE_WINDOW != parent->base.type)
	{
		parent = NxGetWidgetParent(parent);
	}

	if (parent != NULL && WIDGET_TYPE_WINDOW == parent->base.type)
	{
		if (!parent->spec.container.spec.window.focus[idx])
			parent->spec.container.spec.window.focusTabLen++;

		parent->spec.container.spec.window.focus[idx] = (NX_WIDGET *)widget;
	}
}

void
NxRefreshWidgetGC(NX_WIDGET *widget)
{
	NxSetGCForeground(widget->base.gc, widget->base.fgcolor);
	NxSetGCBackground(widget->base.gc, widget->base.bgcolor);

	if (widget->base.fontname != NULL)
	{
		//DBGLOG("fontid %d\n", widget->base.fontid);
		widget->base.fontid = NxGetFontID(widget->base.fontname, widget->base.fontsize);
	}
	GrSetGCFont (widget->base.gc, widget->base.fontid);
	//DBGLOG("dfontid %d\n", widget->base.fontid);
}

void
NxRefreshWidget(NX_WIDGET * widget)
{
	GR_BOOL b = widget->base.visible;
	NxSetWidgetVisible(widget, GR_FALSE);
	if (b == GR_TRUE)
		NxSetWidgetVisible(widget, GR_TRUE);
}

void
NxGetWidgetAbsXY(NX_WIDGET *widget, int *x, int *y)
{
	*x = 0;
	*y = 0;
	do
	{
		*x+= widget->base.posx;
		*y+= widget->base.posy;
		widget = NxGetWidgetParent(widget);
	}while ((widget != NULL)&&(widget->base.wid != GR_ROOT_WINDOW_ID));

}

GR_BOOL
NxGetWidgetAbsVisible(NX_WIDGET *widget)
{
	do
	{
		if (!widget->base.visible) return GR_FALSE;
		widget = NxGetWidgetParent(widget);
	}while ((widget != NULL)&&(widget->base.wid != GR_ROOT_WINDOW_ID));
	return GR_TRUE;
}

void
NxAddToRegistry(NX_WIDGET * w)
{
	NX_WIDGET * parent = w->parent;
	int hash = whash(w->base.wid);

	if (parent)
	{
		w->next = parent->child;
		if (parent->child)
		{
			parent->child->prev = w;
		}
		parent->child = w;
	}
	else
	{
		DBGLOG("main application registered.\n");
	}

	// add to hash table
	widgetHash[hash] = w;
	widgetIdHash[hash] = w->base.wid;


/* 	DBGLOG("AFTER REGISTRATION: type: %d wid: %d hashcode: %d w = %p\n" */
/* 		   "child = %p, prev = %p next = %p, parent = %p\n", */
/* 		   w->base.type, w->base.wid, whash(w->base.wid), w, */
/* 		   w->child, w->prev, w->next, w->parent); */
}

static NX_WIDGET *
search_registration(NX_WIDGET * w, GR_WINDOW_ID wid)
{
	if (w)
	{
		NX_WIDGET * ww = NULL;

		if (wid != w->base.wid)
		{
			//DBGLOG("w = %p, child = %p, prev = %p next = %p, parent = %p\n",
			//	   w, w->child, w->prev, w->next, w->parent);

			ww = search_registration(w->child, wid);
			if (ww)	return ww;

			ww = search_registration(w->next, wid);
			if (ww)	return ww;

			return NULL;
		}
		else
			DBGLOG("wid = %d found with %p", wid, w);
	}
	return w;
}

NX_WIDGET *
NxGetFromRegistry(GR_WINDOW_ID wid)
{
	NX_WIDGET * w;
	int id;
	int hash = whash(wid);

	id = widgetIdHash[hash];
	w = widgetHash[hash];

	// search hash table first.
	if (w && w->base.wid == wid)
		return w;

//	DBGLOG("SEARCHING FOR: wid: %d, hashcode: %d, hash: %p hashentry: %d\n",
//		   wid, whash(wid), w, w->base.wid);

	// search the registration tree.
	w = search_registration(mainApp, wid);

	DBGLOG("*************** SEARCHING END ***************\n");

	if (w)
	{
		// if found, add into hash tree.
		widgetHash[hash] = w;
		widgetIdHash[hash] = wid;

		DBGLOG("SEARCHING ENDED UP WITH: wid: %d, hashcode: %d, hash: %p hashentry: %d\n",
			   wid, whash(wid), w, w->base.wid);
	}

	return w;
}

void
NxDeleteFromRegistry(NX_WIDGET * w)
{
	NX_WIDGET * prev = w->prev;
	NX_WIDGET * next = w->next;
	NX_WIDGET * parent = w->parent;
	int         hash = whash(w->base.wid);

	DBGLOG("DELETING: w %p wid: %d\n", w, w->base.wid);

	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	if (parent && parent->child == w)
		parent->child = next;

	// clear hash entry.
	if (widgetIdHash[hash] == w->base.wid)
		widgetHash[hash] = NULL;

	free(w);
}


void
NxSetWidgetData(NX_WIDGET *widget, DATA_POINTER p)
{
	widget->base.data = p;
}

DATA_POINTER
NxGetWidgetData(NX_WIDGET *widget)
{
	return(widget->base.data);
}

int
NxSetWidgetBackGroundPixmap(NX_WIDGET *widget, GR_WINDOW_ID bgpixmap,
							GR_COORD srcx, GR_COORD srcy, unsigned long op, int type)
{
	struct widgetbase * base = &widget->base;

	GR_WINDOW_ID    tmp;

	if (base->bgpixmap > 0)
		GrDestroyWindow(base->bgpixmap);

	tmp = GrNewPixmap(base->width, base->height, NULL);
	GrCopyArea(tmp, base->gc,
			   0, 0, base->width, base->height,
			   bgpixmap, srcx, srcy, op);

	switch (type)
	{
	case 1:
		DBGLOG("dd %d %d", widget->base.wid, widget->base.type);
#ifndef FASTGUI
		NxPixmapMaskColor(tmp, base->gc,
						  0, 0, base->width, base->height, base->bgcolor);
#endif
		break;
	default:
		break;
	}

	base->bgpixmap = tmp;
	GrSetBackgroundPixmap(base->wid, base->bgpixmap, GR_BACKGROUND_CENTER);

	// set background pixmap and refresh the window background.
	GrClearWindow(base->wid, GR_FALSE);

	return 1;
}

void
NxAlphaBmp(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_COORD dstx, GR_COORD dsty,
		   GR_SIZE width, GR_SIZE height, GR_WINDOW_ID srcid,
		   GR_COORD srcx, GR_COORD srcy, unsigned char alpha, long transcolor)
{
	GR_PIXELVAL intable[width];
	GR_PIXELVAL outtable[width];

	GR_COORD x;
	GR_COORD y;

	GR_PIXELVAL dc, sc;

	for (y = 0; y < height; y++)
	{
		GrReadArea(dstid, dstx, dsty + y, width, 1, outtable);
		GrReadArea(srcid, srcx, srcy + y, width, 1, intable);

		for (x = 0; x < width; x++)
		{
			sc = intable[x];

			if ((-1 != transcolor) && ((sc & 0xffff) == (transcolor & 0xffff)))
			{
				continue;
			}

			if (0 == alpha) //no alpha
			{
				outtable[x] = sc;
			}
			else
			{
				dc = outtable[x];
				outtable[x] = ((((sc & 0xf81f) + (dc & 0xf81f)) >> 1) & 0xf81f) |
				              ((((sc & 0x07e0) + (dc & 0x07e0)) >> 1) & 0x07e0);
			}
		}

		GrArea(dstid, gc, dstx, dsty + y, width, 1, outtable, MWPF_PIXELVAL);
	}
}

/**
 * Decorates the pop up widget.
 *
 * @param pw
 *       parent widget
 * @param w
 *       decorating widget.
 *
 */
void
NxDecoratePopup(NX_WIDGET * pw, NX_WIDGET * w)
{
#define basescale 		4
#define featherwidth 	((1 << basescale) - 1)

	GR_COORD x, y, i, j;

	GR_WINDOW_ID pid = pw->base.wid;
	GR_GC_ID gc = pw->base.gc;
	GR_PIXELVAL * pixStart, * pix, rgb;
	GR_COORD dx, dy, dw, dh;

	// set decoration area.
	dx = w->base.posx - 5;
	dy = w->base.posy -5;
	dw = w->base.width + 12;
	dh = w->base.height + 12;

	pixStart =
	pix = (GR_PIXELVAL*)malloc(dw * dh * sizeof(GR_PIXELVAL));

	GrReadArea(pid, dx, dy, dw, dh, pix);

	for (y = 0; y < dh; y++)
	{
		for (x = 0; x < dw; x++)
		{
			if ((x < featherwidth) || (x > dw - featherwidth - 1) ||
				 (y < featherwidth) || (y > dh - featherwidth - 1))
			{
				i = (x < featherwidth) ? x : dw - 1 - x;
				j = (y < featherwidth) ? y : dh - 1 - y;

				if ((i < featherwidth) && (j < featherwidth))
				{
					if (i > j)
					{
						i = (featherwidth - j + 1) + (featherwidth - (i - j)) / (featherwidth - 2);
					}
					else
					{
						i = (featherwidth - i + 1) + (featherwidth - (j - i)) / (featherwidth - 2);
					}
					if (i > featherwidth)	i = featherwidth;
				}
				else
				{
					if (i > j) i = j;
					i = featherwidth - i;
				}

				rgb = *pix;
				*pix = ((((rgb & 0xf81f) * i) >> basescale) & 0xf81f) |
				       ((((rgb & 0x07e0) * i) >> basescale) & 0x07e0);
			}
			pix++;
		}
	}
	GrArea(pid, gc, dx, dy, dw, dh, pixStart, MWPF_PIXELVAL);
}

void
NxMakeShadow(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_WINDOW_ID srcid, GR_BOOL usealpha)
{
#define basescale 		4
#define featherwidth	((1 << basescale) - 1)
#define basecolor		255
#define featherstep		basecolor / featherwidth

	GR_RECT rect;
	GR_WINDOW_INFO info;
	GR_COORD x, y, i, j;

	GR_PIXELVAL dc, sc;

	GrGetWindowInfo(srcid, &info);
	if (info.realized)
	{
		rect.x = info.x - 5;
		rect.y = info.y - 5;
		rect.width = info.width + 12;
		rect.height = info.height + 12;
	}

	GR_WINDOW_ID pixm = GrNewPixmap(rect.width, rect.height, NULL);
	NxSetGCForeground(gc, BLACK);
	GrFillRect(pixm, gc, 0, 0, rect.width, rect.height);

	GR_PIXELVAL intable[rect.width];
	GR_PIXELVAL outtable[rect.width];

	for (y = 0; y < rect.height; y++)
	{
		GrReadArea(pixm, 0, 0 + y, rect.width, 1, intable);
		GrReadArea(dstid, rect.x, rect.y + y, rect.width, 1, outtable);
		for (x = 0; x < rect.width; x++)
		{
			dc = outtable[x];
			sc = intable[x];

			if ((x < featherwidth) || (x > rect.width - featherwidth - 1)||
				 (y < featherwidth) || (y > rect.height - featherwidth - 1))
			{
				if (x < featherwidth)
					i = x;
				else
					i = rect.width - 1 - x;
				if (y < featherwidth)
					j = y;
				else
					j = rect.height - 1 - y;

				if ((i < featherwidth) &&(j < featherwidth))
				{
					if (i > j)
					{
						i = (featherwidth - j + 1) + (featherwidth - (i - j)) / (featherwidth - 2);
					}
					else
					{
						i = (featherwidth - i + 1) + (featherwidth - (j - i)) / (featherwidth - 2);
					}
					if (i > featherwidth)
						i = featherwidth;
				}
				else
				{
					if (i > j)
						i = j;
					i = featherwidth - i;
				}

				outtable[x] = ((((dc & 0xf81f) * i) >> basescale) & 0xf81f) |
				              ((((dc & 0x07e0) * i) >> basescale) & 0x07e0);
			}
			else
				outtable[x] = sc;
		}
		GrArea(dstid, gc, rect.x, rect.y + y, rect.width, 1, outtable, MWPF_PIXELVAL);
	}

	GrDestroyWindow(pixm);
}

void
NxPixmapMaskColor(GR_WINDOW_ID dstid, GR_GC_ID gc, GR_COORD dstx, GR_COORD dsty,
				  GR_SIZE width, GR_SIZE height, GR_COLOR color)
{
	GR_PIXELVAL outtable[width];

	GR_COORD x;
	GR_COORD y;

	GR_PIXELVAL dc, sc;

	sc = ((((color) & 0xf8) << 8) | (((color) & 0xfc00) >> 5) | (((color) & 0xf80000) >> 19));

	for (y = 0; y < height; y++)
	{
		GrReadArea(dstid, dstx, dsty + y, width, 1, outtable);
		for (x = 0; x < width; x++)
		{
			dc = outtable[x];
			outtable[x] = ((((sc & 0xf81f) + (dc & 0xf81f)) >> 1) & 0xf81f) |
			              ((((sc & 0x07e0) + (dc & 0x07e0)) >> 1) & 0x07e0);
		}
		GrArea(dstid, gc, dstx, dsty + y, width, 1, outtable, MWPF_PIXELVAL);
	}
}


/**
 * Function checks to see if widget is valid.
 *
 * @param w
 *       checking widget.
 * @return
 *       0 if widget is valid, nonzero otherwise.
 */
int NxCheckWidget(NX_WIDGET * w)
{
	if (!w)	return -1;
	return (w->check.runningId == w->check.checkId) ? 0 : -1;
}

/**
 * Set graphic context foreground color.
 *
 */
void
NxSetGCForeground(GR_GC_ID gc, GR_COLOR color)
{
	static GR_GC_ID cache_gc;
	static GR_COLOR cache_color;

	if ((gc == cache_gc) && (color == cache_color))	return;

	GrSetGCForeground(gc, color);
	cache_gc = gc;
	cache_color = color;
}


/**
 * Set graphic context background color.
 *
 */
void
NxSetGCBackground(GR_GC_ID gc, GR_COLOR color)
{
	static GR_GC_ID cache_gc;
	static GR_COLOR cache_color;

	if ((gc == cache_gc) && (color == cache_color))	return;

	GrSetGCBackground(gc, color);
	cache_gc = gc;
	cache_color = color;
}

void
NxSbarDraw(NX_WIDGET * widget, struct scrollbar *sbar,
                  int v, int rows, int range,
                  int x, int y, int h)
{
	struct widgetbase * base = &widget->base;
	int w = sbar->width;
	int s, b;

	if (rows >= range)
		return;

	b = (h - 2 - 2) * rows / range;
	b = (b < w) ? w : b;
	s = v * (h - 2 - 2 - b) / (range - 1);

	NxSetGCForeground(base->gc, sbar->bordercolor);
	GrRect(base->wid, base->gc, x, y, w, h);

	x += 1;
	y += 1;
	w -= 2;
	h -= 2;
	s += 1;

	NxSetGCForeground(base->gc, sbar->bgcolor);
	GrFillRect(base->wid, base->gc, x, y, w, s);
	GrFillRect(base->wid, base->gc, x, y + (s + b), w, h - (s + b));

	NxSetGCForeground (base->gc, sbar->fgcolor);
	GrFillRect(base->wid, base->gc, x, y + s, w, b);

	NxSetGCForeground (base->gc, base->fgcolor);
}
