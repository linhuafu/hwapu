/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
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
 * Generic shared memory routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-02-24 MG
 *
 */

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "shared-mem.h"
#include "nxlist.h"

#define _ID_LEN				32
#define _TOK_LEN			128		/* 128 is more than enough. */
#define SHARED_MEM_MAGIC	0x20130906

typedef struct {
	int        magic;
	int        refCount;
	shm_lock_t lock;
} shm_ctl_t;

// NOTE: shm_ctl_t needs to be 128 bit aligned.
typedef union {
	shm_ctl_t ctrl;
	int       dummy[4 * ((sizeof(shm_ctl_t) + 3) / 4)];
} shm_ctl_ut;

typedef struct {
	slist_t list;
	int    refCount;
	int    fd;
	void*  shm;
	int    bytes;
	char   id[_ID_LEN];
} shm_reg_t;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                  /* value for SETVAL */
	struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;    /* array for GETALL, SETALL */
	/* Linux specific part: */
	struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif


typedef struct {
	void*       buf;	// shmem addr
	shm_lock_t* lock;	// shmem lock
    int         bytes;	// shmem size, can be used to do error
    char*       id;
} shm_internal_handle_t;

/* per process data. */
static pthread_mutex_t shmLock = PTHREAD_MUTEX_INITIALIZER;
static DEFINE_SLIST_HEAD(shmHead);


static inline void CoolShmUnlink(const char *tok)
{
	DBGLOG("unlink: %s\n", tok);

	if (shm_unlink(tok)) {
		switch (errno) {
		case EACCES:       DBGLOG("EACCS\n"); break;
		case EPERM:        DBGLOG("EPERM\n"); break;
		case EFAULT:       DBGLOG("EFAULT\n"); break;
		case EISDIR:       DBGLOG("EISDIR\n"); break;
		case EBUSY:        DBGLOG("EBUSY\n"); break;
		case EINVAL:       DBGLOG("EINVAL\n"); break;
		case ENAMETOOLONG: DBGLOG("ENAMETOOLONG\n"); break;
		case ENOENT:       DBGLOG("ENOENT\n"); break;
		case ENOTDIR:      DBGLOG("ENOTDIR\n"); break;
		case ENOMEM:       DBGLOG("ENOMEM\n"); break;
		case EROFS:        DBGLOG("EROFS\n"); break;
		case ELOOP:        DBGLOG("ELOOP\n"); break;
		case EIO:          DBGLOG("EIO\n"); break;
		}
	}
}

static inline void CoolShmDeleteSem(int semid)
{
	union semun arg;

	// remove shared memory semaphore set.
	if (semctl(semid, 0, IPC_RMID, arg) == -1) {
		switch (errno) {
		case EACCES: DBGLOG("EACCS\n"); break;
		case EFAULT: DBGLOG("EFAULT\n"); break;
		case EIDRM:  DBGLOG("EIDRM\n"); break;
		case EINVAL: DBGLOG("EINVAL\n"); break;
		case EPERM:  DBGLOG("EPERM\n"); break;
		case ERANGE: DBGLOG("ERANGE\n"); break;
		}
		FATALLOG("semctl removal failed.\n");
	}
}

static int CoolShmCreateSem(const char *tok)
{
	key_t key;
	int semid;
	union semun arg;

	DBGLOG("creating key for tok: %s\n", tok);

	if ((key = ftok(tok, 1)) == -1) {
		FATALLOG("token creation failed.\n");
		return -1;
	}

	DBGLOG("creating semaphore.\n");

	/* create a semaphore set with 2 semaphore: */
	if ((semid = semget(key, 2, 0666 | IPC_CREAT)) == -1) {
		FATALLOG("semget failed.\n");
		return -1;
	}

	DBGLOG("semaphore created:= %d\n", semid);

	/* initialize semaphores 1: */
	arg.val = 1;
	if ((semctl(semid, 0, SETVAL, arg) == -1) ||
		(semctl(semid, 1, SETVAL, arg) == -1) ) {
		CoolShmDeleteSem(semid);
		FATALLOG("semctl failed.\n");
		return -1;
	}

	return semid;
}

static int CoolShmComparName(slist_t * item, char * idb)
{
	shm_reg_t * reg = container_of(item, shm_reg_t, list);
	DBGLOG("registered id: %s\n looking for: %s\n", reg->id, idb);
	return strcmp(reg->id, idb) == 0;
}

/**
 * Get shared memory with given size.
 *
 * @param id
 *        shared memory ID string, only first 15 characters are significant.
 * @param bytes
 *        shared memory size in bytes.
 * @param creator
 *        returned status to indicate whether this is a shared mem creator.
 * @param lock
 *        returned shared mem lock object pointer.
 * @return
 *        pointer to shared memory, visible in current process address
 *        space if successful, NULL otherwise.
 *        creator = 1: creator will have this flag set when returning.
 *
 */
void* CoolShmGet(const char *id, int bytes, int *creator, shm_lock_t **lock)
{
	char tok[_TOK_LEN];
	char idb[_ID_LEN];
	slist_t * list;
	shm_reg_t * reg;
	shm_ctl_t * ctl;
	int shm_fd;
	void* shm;

	strlcpy(idb, id, sizeof(idb));

	bytes += sizeof(shm_ctl_ut); // extra bytes for internal tracking
	*creator = 0;

	pthread_mutex_lock(&shmLock);

	// search if memory is already created.
	list = nxSlistIterate(&shmHead, CoolShmComparName, idb);

	if (list == NULL) {
		// not found, lets fetch/create it.
		strlcpy(tok, STATE_FILE, sizeof(tok));
		strlcat(tok, idb, sizeof(tok));

		if ((shm_fd = shm_open(idb, (O_CREAT | O_EXCL | O_RDWR), (S_IRUSR | S_IWUSR))) > 0 ) {
			*creator = 1;
		}
		else if ((shm_fd = shm_open(idb, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR))) < 0) {
			DBGLOG("Could not create shm object.\n");
			goto err1;
		}

		/* Set the size of the SHM to be the size of the struct. */
		ftruncate(shm_fd, bytes);

		/* Connect the conf pointer to set to the shared memory area,
		 * with desired permissions
		 */
		if ((shm = mmap(0, bytes, (PROT_READ | PROT_WRITE),
						MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
			DBGLOG("Could not map share-mem.\n");
			goto err2;
		}

		DBGLOG("opened shared memory [shm:%p] [bytes:%d]\n", shm, bytes);

		// fetch/create successful, record it.
		reg = (shm_reg_t*) calloc(1, sizeof(shm_reg_t));
		if (reg == NULL) {
			DBGLOG("Could not allocate shm_reg_t.\n");
			goto err3;
		}

		strlcpy(reg->id, idb, sizeof(reg->id));
		reg->shm = shm + sizeof(shm_ctl_ut);
		reg->bytes = bytes;
		reg->refCount = 1;
		reg->fd = shm_fd;

		nxSlistInsert(&shmHead, &reg->list);

		DBGLOG("opened shared memory [fd: %d] [shm:%p]\n", reg->fd, reg->shm);

		ctl = (shm_ctl_t*)(reg->shm - sizeof(shm_ctl_ut));

		// create lock object.
		if (*creator) {
			ctl->lock.semid = CoolShmCreateSem(tok);
			if (ctl->lock.semid == -1)
				goto err4;
			ctl->lock.rc = 0;
			ctl->refCount = 1;
			//now, validate the shared memory.
			ctl->magic = SHARED_MEM_MAGIC;
		} else {
			int retry = 10; // wait for 10 seconds before reporting error.

			while (--retry && SHARED_MEM_MAGIC != ctl->magic) {
				//possibly two processes complete to create the shared memory.
				//very unlikely, however possible, let's wait for a bit.
				WARNLOG("shared memory in inconsistent state, waiting to retry...!\n");
				sleep(1);
			}

			if (!retry) {
				WARNLOG("shared memory in inconsistent state.!\n");
				goto err4;
			}

			// update global ref counter.
			// apply the lock itself, indeed we are accessing shared memory here as well.
			CoolShmWriteLock(&ctl->lock);
			ctl->refCount++;
			CoolShmWriteUnlock(&ctl->lock);
		}
	}
	else {
		reg = container_of(list, shm_reg_t, list);
		reg->refCount++;
		ctl = (shm_ctl_t*)(reg->shm - sizeof(shm_ctl_ut));
	}

	pthread_mutex_unlock(&shmLock);

	if (lock)
		*lock = &ctl->lock;

	DBGLOG("shared memory [ID: %s] ref count = %d/%d\n", id, reg->refCount, ctl->refCount);
	return reg->shm;

err4:
	nxSlistRemove(&shmHead, list);
	free(reg);
err3:
	munmap(shm, bytes);
err2:
	close(shm_fd);
	if (*creator) {
		CoolShmUnlink(idb);
		*creator = 0;
	}
err1:
	pthread_mutex_unlock(&shmLock);
	return NULL;
}


/**
 * put back shared memory with given ID.
 *
 * @param id
 *        shared memory ID string.
 *
 */
void CoolShmPut(const char *id)
{
	char idb[_ID_LEN];
	slist_t * list;
	shm_reg_t * reg;
	shm_ctl_t * ctl;
	int refCount;

	strlcpy(idb, id, sizeof(idb));

	pthread_mutex_lock(&shmLock);

	// search if memory is already created.
	list = nxSlistIterate(&shmHead, CoolShmComparName, idb);

	if (list == NULL) {
		pthread_mutex_unlock(&shmLock);
		WARNLOG("Shared memory [ID: %s] not found!\n", idb);
		return;
	}

	reg = container_of(list, shm_reg_t, list);
	ctl = (shm_ctl_t*)(reg->shm - sizeof(shm_ctl_ut));
	reg->refCount--;

	DBGLOG("shared memory [ID: %s] ref count = %d/%x\n", id, reg->refCount, ctl->refCount);

	if (reg->refCount) {
		pthread_mutex_unlock(&shmLock);
		return;
	}

	DBGLOG("removing shm registration\n");

	nxSlistRemove(&shmHead, list);
	pthread_mutex_unlock(&shmLock);

	// update global ref counter.
	// apply the lock itself, indeed we are accessing shared memory here as well.
	CoolShmWriteLock(&ctl->lock);
	refCount = --ctl->refCount;
	CoolShmWriteUnlock(&ctl->lock);

	if (!refCount) { //last reference.
//		char tok[_TOK_LEN];

//		strlcpy(tok, STATE_FILE, sizeof(tok));
//		strlcat(tok, idb, sizeof(tok));

		CoolShmDeleteSem(ctl->lock.semid);
		CoolShmUnlink(idb);
	}

	DBGLOG("closing shared memory [ID: %s]\n", id);

	munmap(reg->shm - sizeof(shm_ctl_ut), reg->bytes);
	close(reg->fd);
	free(reg);
}


/** shared memory reader lock.
 *  Locks only when writer is busy.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
int CoolShmReadLock(shm_lock_t *lock)
{
    struct sembuf rdsb = {0, -1, 0};  /* set to allocate resource */
    struct sembuf wrsb = {1, -1, 0};  /* set to allocate resource */
	int ret = 0;

	if (semop(lock->semid, &rdsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		return -1;
    }

	if (lock->rc++ == 0) {
		if (semop(lock->semid, &wrsb, 1) == -1) {
			FATALLOG("semop failed.\n");
			ret = -1;
		}
	}

	rdsb.sem_op = 1;
	if (semop(lock->semid, &rdsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		ret = -1;
    }

    return ret;
}

/** shared memory reader unlock.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
int CoolShmReadUnlock(shm_lock_t *lock)
{
    struct sembuf rdsb = {0, -1, 0};  /* set to allocate resource */
    struct sembuf wrsb = {1,  1, 0};  /* set to release resource */
	int ret = 0;

	if (semop(lock->semid, &rdsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		return -1;
    }

	if (--lock->rc == 0) {
		if (semop(lock->semid, &wrsb, 1) == -1) {
			FATALLOG("semop failed.\n");
			ret = -1;
		}
	}

	rdsb.sem_op = 1;
	if (semop(lock->semid, &rdsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		ret = -1;
    }

    return ret;
}


/** shared memory write lock.
 *  Locks when reader or another writer is in progress.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
int CoolShmWriteLock(shm_lock_t *lock)
{
    struct sembuf wrsb = {1, -1, 0};  /* set to allocate resource */

	if (semop(lock->semid, &wrsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		return -1;
    }

    return 0;
}

/** shared memory write unlock.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
int CoolShmWriteUnlock(shm_lock_t *lock)
{
    struct sembuf wrsb = {1,  1, 0};  /* set to release resource */

	if (semop(lock->semid, &wrsb, 1) == -1) {
		FATALLOG("semop failed.\n");
		return -1;
    }

    return 0;
}


/**
 * Get shared memory with given size.
 * @param handle
 *        returned shared mem handle.
 * @param id
 *        shared memory ID string, only first 15 characters are significant.
 * @param bytes
 *        shared memory size in bytes.
 * @return
 *       -1      faile
 *        0      shared memory has exist
 *        1      first creat shared memory
 *
 */
int CoolShmHelperOpen(shm_handle_t *handle, const char *id, int bytes)
{
	shm_internal_handle_t* smhandle;
	int first;

	smhandle = (shm_internal_handle_t*) calloc(1, sizeof(shm_internal_handle_t));
	if (!smhandle) {
		WARNLOG("Unable to allocate shm handle!\n");
		return -1;
	}

	smhandle->buf = CoolShmGet(id, bytes, &first, &smhandle->lock);
	if (!smhandle->buf) {
		WARNLOG("Unable to fetch shared memory!\n");
		return -1;
	}

	smhandle->id = strdup(id);
	smhandle->bytes = bytes;

	handle->shm = smhandle;
	return first;
}

void CoolShmHelperClose(shm_handle_t *handle)
{
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	if (!tmp)
		return;

	handle->shm = NULL;

	CoolShmPut(tmp->id);
	free(tmp->id);
	free(tmp);
}

/**
 * Get shared memory with given size.
 * @param handle
 *        shared mem handle.
 * @param dat
 *        return to the readed data.
 * @param bytes
 *        How many bytes to read
 * @param offset
 *        the offset in shared memory start to read.
 * @return
 *     value = -1      handle illegal;
 *     value = -2      offset illegal;
 *	   value >  0      the bytes read;
 *
 */

int CoolShmHelperRead(shm_handle_t *handle, void *dat, int offset, int bytes)
{
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	int nspace;

	if (!tmp )
		return -1;
	if (offset < 0 || offset >= tmp->bytes)
		return -2;

	nspace = tmp->bytes - offset;
	nspace = (nspace > bytes) ? bytes : nspace;

	CoolShmReadLock(tmp->lock);
	memcpy(dat, tmp->buf + offset, nspace);
	CoolShmReadUnlock(tmp->lock);

	return nspace;
}

/**
 * Set shared memory with given size.
 * @param handle
 *        shared mem handle.
 * @param dat
 *        point to the data that will write.
 * @param bytes
 *        write size in bytes .
 * @param offset
 *        the offset in share memory start to write.
 * @return
 *     value = -1      handle illegal;
 *     value = -2      offset illegal;
 *	   value >  0      the bytes  written;
 *
 */

int CoolShmHelperWrite(shm_handle_t *handle, const void *dat, int offset, int bytes)
{
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	int nspace;

	if (!tmp )
		return -1;
	if (offset < 0 || offset >= tmp->bytes)
		return -2;

	nspace = tmp->bytes - offset;
	nspace = (nspace > bytes) ? bytes : nspace;

	CoolShmWriteLock(tmp->lock);
	memcpy(tmp->buf + offset, dat, nspace);
	CoolShmWriteUnlock(tmp->lock);

	return nspace;
}
