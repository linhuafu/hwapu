#ifndef __RADIO_REC_NEWUI_H__
#define __RADIO_REC_NEWUI_H__

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
#include "radio_type.h"
#include "radio_rec_extern.h"
#include "breaklines.h"
#include "plextalk-theme.h"
#include "plextalk-i18n.h"
#include "nxutils.h"


#define FILE_NAME_SIZE				50
#define REC_TIME_LAEBEL_SIZE		100
#define REC_TIME_VALUE_SIZE			20
#define REMAIN_TIME_LABEL_SIZE		100
#define REMAIN_TIME_VALUE_SIZE		20


int Radio_RecShow_Init(struct radio_nano* nano, int fontsize);
int Radio_RecShow_parseText(struct radio_nano* nano, char *text, int size);
int Radio_RecShow_prevScreen(struct radio_nano* nano);
int Radio_RecShow_nextScreen(struct radio_nano* nano);

void Radio_RecShow_showScreen(struct radio_nano* nano);
void Radio_Rec_FreeStringBuf(struct radio_nano* nano);

void Radio_Rec_InitStringBuf(struct radio_nano* nano,char *filename);

void Radio_Rec_CreateGc(FORM *widget,struct radio_nano* nano);


#endif
