#include <nano-X.h>

#define OSD_DBG_MSG
#include "nc-err.h"

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
#include "plextalk-keys.h"
#include "Plextalk-i18n.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "libvprompt.h"
#include "Vprompt-defines.h"
#include "nxutils.h"
#include "file_menu.h"
#include "file_main.h"

#undef _
#define _(x) x


typedef struct item{
	char *str;
	OP_MODE op_mode;
}Item;

typedef struct menu {
	unsigned int items_nr;
	Item *items;
}Menu;

const Item file_Items[]={
	{_("Copy"),OP_COPY},
	{_("Cut"), OP_CUT},
	{_("Paste"),OP_PASTE},
	{_("Delete"),OP_DELETE},
};


const Menu file_menu = {
	.items_nr		= ARRAY_SIZE(file_Items),
	.items			= file_Items,
};

extern int voice_prompt_running;

static const Menu *menu = &file_menu;

static FORM *  formMenu;
#define widget formMenu
static WIDGET* listbox1;
static int actItem;
static int topItem;
static OP_MODE op_mode;

static TIMER *key_timer;
static int long_pressed_key = -1;

void doVoicePrompt(const char* lead, const char *str)
{
	if (lead != NULL)
		voice_prompt_music(0, &vprompt_cfg, lead);
	if (NULL != str)
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, str);
	voice_prompt_running = 1;
}


static int
getItem(WID__ sender, int index, char **text)
{
	if (index == -1)
		return menu->items_nr;

	*text = gettext(menu->items[index].str);

	return index - NeuxListboxTopItem(listbox1);
}

static void
OnListboxKeydown(WID__ wid,KEY__ key)
{
	if(checkFileExit()){
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;
	
	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		voice_prompt_abort();
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_BU);
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
			tts_lang, tts_role, _("Keylock"));
		return;
	}

	switch (key.ch) {
	case MWKEY_UP:
	case MWKEY_DOWN:
		voice_prompt_abort();
		if (actItem == NeuxListboxSelectedItem(listbox1)) {
			doVoicePrompt(VPROMPT_AUDIO_KON, NULL);
		} else {
			actItem = NeuxListboxSelectedItem(listbox1);
			doVoicePrompt(VPROMPT_AUDIO_PU, gettext(menu->items[actItem].str));
		}
		break;

	case MWKEY_LEFT:
		voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_PU, NULL);
		//exit
		NeuxClearModalActive();
		break;

	case MWKEY_RIGHT:
	case MWKEY_ENTER:	//select
		{
			actItem = NeuxListboxSelectedItem(listbox1);
			op_mode = menu->items[actItem].op_mode;
			voice_prompt_abort();
			doVoicePrompt(VPROMPT_AUDIO_PU, gettext(menu->items[actItem].str));
			NeuxClearModalActive();
		}
		break;
		
	case MWKEY_INFO:
		voice_prompt_abort();
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext(menu->items[NeuxListboxSelectedItem(listbox1)].str));
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		break;

	case MWKEY_MENU:
		long_pressed_key = key.ch;
		NeuxTimerSetControl(key_timer, 1000, 1);
		break;
		
	case VKEY_BOOK:
	case VKEY_MUSIC:
	case VKEY_RECORD:
	case VKEY_RADIO:
		voice_prompt_abort();
		doVoicePrompt(VPROMPT_AUDIO_BU, NULL);
		break;
	}
}

static void
OnListboxKeyup(WID__ wid, KEY__ key)
{
	if(checkFileExit()){
		return;
	}

	if (NeuxWMAppIsRunning(PLEXTALK_IPC_NAME_HELP))
		return;

	if (NeuxAppGetKeyLock(0)) {
		DBGMSG("key lock\n");
		return;
	}

	if (long_pressed_key != key.ch)
		return;

	long_pressed_key = -1;
	NeuxTimerSetEnabled(key_timer, GR_FALSE);

	switch (key.ch)
	{
	case MWKEY_MENU:
		FileManagementExit();
		break;
#if 0		
	case MWKEY_INFO:
		voice_prompt_abort();
		voice_prompt_string2(0, &vprompt_cfg, ivTTS_CODEPAGE_UTF8,
					tts_lang, tts_role, gettext(menu->items[NeuxListboxSelectedItem(listbox1)].str));
		voice_prompt_music(0, &vprompt_cfg, VPROMPT_AUDIO_PU);
		break;
#endif
	default:
		break;
	}
}

static void
OnKeyTimer(WID__ wid)
{
	long_pressed_key = -1;
}

static void
OnWindowDestroy(WID__ wid)
{
	DBGLOG("---------------window destroy %d.\n",wid);
	widget = NULL;
}

void
CreateFormMenu(void)
{
 	widget_prop_t wprop;
	listbox_prop_t lsprop;

	widget = NeuxFormNew( MAINWIN_LEFT,
	                      MAINWIN_TOP,
	                      MAINWIN_WIDTH,
	                      MAINWIN_HEIGHT);

	NeuxFormSetCallback(widget, CB_KEY_DOWN, OnListboxKeydown);
	NeuxFormSetCallback(widget, CB_KEY_UP, OnListboxKeyup);
	NeuxFormSetCallback(widget, CB_DESTROY,  OnWindowDestroy);
//	NeuxFormSetCallback(widget, CB_FOCUS_IN, OnWindowGetFocus);
	key_timer = NeuxTimerNewWithCallback(widget, 0, 0, OnKeyTimer);

	listbox1 = NeuxListboxNew(widget, 0,  0, MAINWIN_WIDTH, MAINWIN_HEIGHT,
							getItem, NULL, NULL);
	
	NeuxListboxSetCallback(listbox1, CB_KEY_DOWN, OnListboxKeydown);
	NeuxListboxSetCallback(listbox1, CB_KEY_UP, OnListboxKeyup);

    NeuxListboxGetProp(listbox1, &lsprop);
    lsprop.useicon = FALSE;
    lsprop.ismultiselec = FALSE;
    lsprop.bWrapAround = TRUE;
    lsprop.bScroll = TRUE;
    lsprop.bScrollBidi = FALSE;
    lsprop.scroll_interval = 500;
    lsprop.scroll_endingscrollcnt = -1;
	lsprop.sbar.fgcolor = theme_cache.foreground;
	lsprop.sbar.bordercolor = theme_cache.foreground;
	lsprop.sbar.bgcolor = theme_cache.background;
	lsprop.sbar.width = SBAR_WIDTH;
    lsprop.itemmargin.left = 0;
    lsprop.itemmargin.right = 0;
    lsprop.itemmargin.top = 0;
    lsprop.itemmargin.bottom = 0;
    lsprop.textmargin.left = 2;
    lsprop.textmargin.right = 2;
    lsprop.textmargin.top = 0;
    lsprop.textmargin.bottom = 0;
    NeuxListboxSetProp(listbox1, &lsprop);

	NeuxWidgetSetFont(listbox1, sys_font_name, sys_font_size);
	NxListboxRecalcPitems(listbox1);

	menu = &file_menu;

	actItem = 0;
	topItem = 0;
	NeuxListboxSetItem(listbox1, actItem - topItem, topItem);

    NeuxWidgetShow(listbox1, TRUE);

	NeuxFormShow(widget, TRUE);
	NeuxWidgetFocus(widget);

	doVoicePrompt(NULL, gettext(menu->items[actItem].str));
}

OP_MODE op_select(void)
{
	int endOfModal;
	CreateFormMenu();
	op_mode = OP_NONE;
	endOfModal = 0;
	NeuxDoModal(&endOfModal, NULL);
	NeuxFormDestroy(&widget);
	return op_mode;
}


