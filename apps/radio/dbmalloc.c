
#include <stdio.h>
#include <string.h> /*strerror*/
#include <errno.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "dbmalloc.h"

#ifdef DEBUG_MALLOC_MEM

typedef struct memMallocNode
{
    void *pAddr;
    size_t size;
}MALLOC;

#define LIST_LEN 500

static MALLOC list[LIST_LEN];
static int g_MallocMemNum = 0;

const char magicChar[] = {'\x12', '\x34','\x56','\x78','\x87','\x65','\x43','\x21',0};

void initDebugMalloc()
{
	memset(list, 0, sizeof(list));
}

static void insertList(void *pAddr, size_t size)
{
	int i;
	for(i=0; i< LIST_LEN; i++){
		if(0 ==list[i].pAddr){
			list[i].pAddr = pAddr;
			list[i].size = size;
			return;
		}
	}
	DBGMSG("***************insert error*************\n");
	return;
}

static size_t findPSize(void *pAddr)
{
	int i;
	
	for(i=0; i< LIST_LEN; i++){
		if(pAddr ==list[i].pAddr){
			return list[i].size;
		}
	}
	DBGMSG("***************NOT FIND*************\n");
	return 0;
}

static size_t DeletePSize(void *pAddr)
{
	int i;
	size_t size;
	
	for(i=0; i< LIST_LEN; i++){
		if(pAddr ==list[i].pAddr){
			size = list[i].size;
			list[i].size = 0;
			list[i].pAddr = 0;
			return size;
		}
	}
	DBGMSG("***************NOT FIND*************\n");
	return 0;
}

void *dbMalloc (char *file, int line, size_t __size)
{
	size_t dbSize = __size + 16;
	char *p = (char*)malloc(dbSize);
	if(p){
		memcpy(p, magicChar, 8);
		memcpy(p + dbSize -8, magicChar, 8);
		insertList(p, dbSize);
		g_MallocMemNum ++;
//		printf("malloc:%x\n",(int)(p + 8));
		return (void *)(p + 8);
	}
	return NULL;
}

void checkMalloc(char *file, int line,void *__ptr)
{
	size_t size;
	if(!__ptr){
		DBGMSG("%s:%d %s\n", file, line, "***************FREE ERR*************\n");
		return;
	}
	char *p = (char*)__ptr;
	p -= 8;
	size = findPSize(p);
	if(!size){
		return;
	}
	DBGMSG("%s:%d\n", file, line);
	
	char *pp = p;
//	printf("%x %x %x %x %x %x %x %x\n", pp[0],pp[1],pp[2],pp[3],pp[4],pp[5],pp[6],pp[7]);

	pp = p + size -8;
//	printf("%x %x %x %x %x %x %x %x\n", pp[0],pp[1],pp[2],pp[3],pp[4],pp[5],pp[6],pp[7]);


}

void dbFree (char *file, int line,void *__ptr)
{
	size_t size;
	if(!__ptr){
		DBGMSG("%s:%d %s\n", file, line, "***************FREE ERR*************\n");
		return;
	}
	char *p = (char*)__ptr;
	p -= 8;
	size = DeletePSize(p);
	if(!size){
		return;
	}
	if(0 != memcmp(p, magicChar, 8)){
		DBGMSG("%s:%d %s\n", file, line, "***************lower over*************\n");
		char *pp = p;
//		printf("%x %x %x %x %x %x %x %x\n", pp[0],pp[1],pp[2],pp[3],pp[4],pp[5],pp[6],pp[7]);
	}
	if(0 != memcmp(p + size -8, magicChar, 8)){
		DBGMSG("%s:%d %s\n", file, line, "***************upper over*************\n");
		char *pp = p + size -8;
//		printf("%x %x %x %x %x %x %x %x\n", pp[0],pp[1],pp[2],pp[3],pp[4],pp[5],pp[6],pp[7]);
	}
	g_MallocMemNum --;
	free(p);
}


void checkMallocEnd(void)
{
	DBGMSG("<--¼ì²âÄÚ´æÐ¹Â¶**\r\n");
	if(g_MallocMemNum)
	{
		DBGMSG("********************%d¸öÄÚ´æÃ»ÓÐÊÍ·Å****************\n",g_MallocMemNum);
	}
	return;
}

void checkMallocStart(void)
{
	DBGMSG("**¼ì²âÄÚ´æÐ¹Â¶-->\n");
	g_MallocMemNum = 0;
}

#else

void initDebugMalloc(void)
{
}

void checkMallocEnd(void)
{
}

void checkMallocStart(void)
{
}

#endif


