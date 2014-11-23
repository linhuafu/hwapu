#ifndef __DBMALLOC_H__
#define __DBMALLOC_H__

#define DEBUG_MALLOC_MEM

void initDebugMalloc(void);

void *dbMalloc (char *file, int line,size_t __size);

void dbFree (char *file, int line,void *__ptr);

void checkMallocEnd(void);

void checkMallocStart(void);

#ifdef DEBUG_MALLOC_MEM

#define app_free(ptr) dbFree(__FILE__, __LINE__, (ptr))
#define app_malloc(size) dbMalloc(__FILE__, __LINE__, (size))

#else

#define app_free(ptr) free((ptr))
#define app_malloc(size) malloc((size))

#endif

#endif
