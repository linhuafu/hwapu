#ifndef __REC_NEWUI_H__
#define __REC_NEWUI_H__


#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <nano-X.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
//#define OSD_DBG_MSG
#define DEBUG_PRINT 0
#include "nc-err.h"
#include "plextalk-ui.h"
#include "application.h"
#include "neux.h"
#include "widgets.h"
#include "desktop-ipc.h"
#include "plextalk-keys.h"
#include "rec_draw.h"
#include "rec_extern.h"
#include "breaklines.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"
#include "nxutils.h"


#define FILE_NAME_SIZE				50
#define REC_TIME_LAEBEL_SIZE		100
#define REC_TIME_VALUE_SIZE			20
#define REMAIN_TIME_LABEL_SIZE		100
#define REMAIN_TIME_VALUE_SIZE		20


int RecShow_Init(struct rec_nano* nano,int fontsize);
int RecShow_parseText(struct rec_nano* nano, char *text, int size);
int RecShow_prevScreen(struct rec_nano* nano);
int RecShow_nextScreen(struct rec_nano* nano);

void RecShow_showScreen(struct rec_nano* nano);
void Rec_FreeStringBuf(struct rec_nano* nano);

void Rec_InitStringBuf(struct rec_nano* nano,char *filename);

void Rec_CreateGc(FORM *widget,struct rec_nano* nano);


#endif
