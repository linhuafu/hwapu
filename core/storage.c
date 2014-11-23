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
 * Memory storage management routines.
 *
 * REVISION:
 *
 * 2) Implement storagestat function ---------------------- 2006-07-25 EY
 * 1) Initial creation. ----------------------------------- 2006-07-20 MG
 *
 */

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <mntent.h>

#include "nc-err.h"
#include "storage.h"

#if 0
#define DBGLOG(fmt, arg...) DPRINT(fmt, ##arg)
#else
#define DBGLOG(fmt, arg...) do {;}while(0)
#endif

const char *
CoolCreateReadableSizeString(unsigned long long size, unsigned long display_unit)
{
	/* The code will adjust for additional (appended) units. */
	static const char zero_and_units[] = { '0', 0, 'K', 'M', 'G', 'T' };
	static const char fmt[] = "%Lu";
	static const char fmt_tenths[] = "%Lu.%d %c";

	static char str[21];		/* Sufficient for 64 bit unsigned integers. */

	int frac;
	const char *u;
	const char *f;

	u = zero_and_units;
	f = fmt;
	frac = 0;

	if (size == 0)
		return u;

	if (display_unit) {
		size += display_unit / 2;	/* Deal with rounding. */
		size /= display_unit;		/* Don't combine with the line above!!! */
	} else {
		++u;
		while (size >= KILOBYTE &&
			   u < zero_and_units + sizeof(zero_and_units) - 1) {
			f = fmt_tenths;
			++u;
			frac = ((((int)(size % KILOBYTE)) * 10) + (KILOBYTE / 2)) / KILOBYTE;
			size /= KILOBYTE;
		}
		if (frac >= 10) {
			++size;
			frac = 0;
		}
	}

	snprintf(str, sizeof(str), f, size, frac, *u);
	return str;
}

/**
 * Enumerate the mount points and calls a user-defined function on all of them
 * @param cb A function that will be called for each mount point with arguments:
 *           - the mount point path
 *           - a pointer to arbitrary user data passed along from this function's arguments.
 *           This function shall return 1 when it wants to stop the iteration.
 * @param user_data It will be passes as-is to the callback function.
 * @return 1 if the callback returned 1, -1 on errors, 0 otherwise.
 */
int CoolStorageEnumerate(storage_enum_func cb, void* user_data)
{
	FILE *mount_table;
	struct mntent *mount_entry;
	int ret = 0;

	if (cb == NULL)
		return -1;

	if (!(mount_table = setmntent("/proc/mounts", "r")))
	{
		WARNLOG("Unable to read /proc/mounts: %s.\n", strerror(errno));
		return -1;
	}

	while ((mount_entry = getmntent(mount_table)) != NULL) {
		if (cb(mount_entry, user_data) == 1) {
			ret = 1;
			break;
		}
	}

	endmntent(mount_table);

	return ret;
}

/** get available storage.
 *
 * @param si
 *       pointer to availabe storage ID array
 * @return
 *         number of storage available.
 *         -1 scan error.
 */
int CoolStorageAvailable(storage_info_t *si, int sicount)
{
	FILE *mount_table;
	struct mntent *mount_entry = NULL;
	struct statfs s;
	int rlt;

	mount_table = setmntent("/proc/mounts", "r");
	if (mount_table == NULL)
	{
	  	WARNLOG("unable to set mount entry.\n");
		return -1;
	}

	for (rlt = 0; rlt < sicount; ) {
		const char *device;
		const char *mount_point;

		mount_entry = getmntent(mount_table);
		if (mount_entry == NULL) {
			endmntent(mount_table);
			//WARNLOG("end of mount table.\n");
			break;
		}

		device = mount_entry->mnt_fsname;
		mount_point = mount_entry->mnt_dir;

		if (statfs(mount_point, &s) != 0) {
			endmntent(mount_table);
		  	WARNLOG("mount points error.\n");
			return -1;
		}

		DBGLOG("device = %s || mp = %s\n", device, mount_point);

		if (s.f_blocks > 0) {
			if (strcmp(device, "rootfs") == 0)
				continue;
			if (strcmp(device, "/dev/root") == 0)
				continue;
			if (strcmp(device, "devtmpfs") == 0)
				continue;
			if (strcmp(mount_entry->mnt_type, "tmpfs") == 0)
				continue;

			strlcpy(si->device, device, sizeof(si->device));
			strlcpy(si->mount_point, mount_point, sizeof(si->mount_point));
			si->total = s.f_blocks * s.f_bsize;
			si->free  = s.f_bavail * s.f_bsize;
			si->ro    = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
			si++;

			rlt++;
		}
	}

  	return rlt;
}

/** helper function to special path in storage struct point.
 *
 * @param si
 *        all storage struct point.
 * @param sicount
 *	     count of all storage
 * @param path
 *        special path will be find.
 * @return
 *         index of all storage
 *         -1 storage not found.
 */
int CoolStorageSearch(const storage_info_t *si, int sicount, const char *path)
{
	int i;

	for (i = 0; i < sicount; i++)
	{
		if (strcmp(si->mount_point, path) == 0)
			return i;
		si++;
	}

	return -1;
}

struct media_readonly {
	const char *device;
	int readonly;
};

static int media_readonly_enum(struct mntent *mount_entry, void *data)
{
	struct media_readonly *media_ro = data;

	if (!strcmp(mount_entry->mnt_fsname, media_ro->device)) {
		media_ro->readonly = hasmntopt (mount_entry, MNTOPT_RO) != NULL;
		return 1;
	}

	return 0;
}

int CoolStorageReadOnly(const char *device)
{
	struct media_readonly media_ro = {
		.device = device,
	};
	int ret;

	ret = CoolStorageEnumerate(media_readonly_enum, &media_ro);
	if (ret == 1)
		return media_ro.readonly;

	return -1;
}

int CoolCdromMediaPresent(void)
{
	FILE *fp;
	int count;
	int n;

	fp = popen("/sbin/cdrom_id /dev/cdrom | /bin/grep ID_CDROM_MEDIA", "r");
	if (fp == NULL)
		return -1;

	n = fscanf(fp, "ID_CDROM_MEDIA=%d", &count);
	pclose(fp);

	return n == 1 ? count : -1;
}

int CoolCDDAGetTrackCount(void)
{
	FILE *fp;
	int count;
	int n;

	fp = popen("/sbin/cdrom_id /dev/cdrom | /bin/grep ID_CDROM_MEDIA_TRACK_COUNT_AUDIO", "r");
	if (fp == NULL)
		return -1;

	n = fscanf(fp, "ID_CDROM_MEDIA_TRACK_COUNT_AUDIO=%d", &count);
	pclose(fp);

	return n == 1 ? count : -1;
}
