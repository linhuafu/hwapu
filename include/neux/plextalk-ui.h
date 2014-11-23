#ifndef PLEXTALK_UI__H
#define PLEXTALK_UI__H

#include "application.h"

#define STATUSBAR_TOP		0
#define STATUSBAR_LEFT		0
#define STATUSBAR_WIDTH		NeuxScreenWidth()
#define STATUSBAR_HEIGHT	16

#define TITLEBAR_TOP		(STATUSBAR_TOP + STATUSBAR_HEIGHT)
#define TITLEBAR_LEFT		0
#define TITLEBAR_WIDTH		NeuxScreenWidth()
#define TITLEBAR_HEIGHT		(16 + 1)	/* extra line under caption */

#define MAINWIN_TOP			(TITLEBAR_TOP + TITLEBAR_HEIGHT)
#define MAINWIN_LEFT		0
#define MAINWIN_WIDTH		NeuxScreenWidth()
#define MAINWIN_HEIGHT		(NeuxScreenHeight() - STATUSBAR_HEIGHT - TITLEBAR_HEIGHT)

#endif
