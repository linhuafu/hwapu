#ifndef __BOOK_H_
#define __BOOK_H_
#include "neux.h"
#include "ttsplus.h"


#define MEDIA_INNER		PLEXTALK_MOUNT_ROOT_STR"/mmcblk0p2"
#define MEDIA_SDCRD		PLEXTALK_MOUNT_ROOT_STR"/mmcblk1"
#define MEDIA_USB		PLEXTALK_MOUNT_ROOT_STR"/sd"
#define MEDIA_CDROM		PLEXTALK_MOUNT_ROOT_STR"/sr"

extern int tip_is_running;
//extern int last_vol;

extern struct TtsObj* booktts;

typedef void (*MsgFunc) (const char*, neux_msg_t*);

//void book_initKeyLock(void);

//int book_isKeyLock(void);
void book_setAudioMode(int audio);

int book_getCurVolumn(void);

int isMediaReadOnly(const char *path);

int book_set_last_path(char *path);

int book_get_last_path(char *path);

int book_set_backup_path(char *path);

int book_set_bookmark(int num, int count);

int book_del_backup(void);

int book_set_pageinfo(unsigned long ulCurPage, unsigned long ulMaxPage, unsigned long ulFrPage, unsigned long ulSpPage);

int book_del_pageinfo();

#endif
