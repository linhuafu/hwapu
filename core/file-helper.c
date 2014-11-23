#define __USE_BSD
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

#include "nxutils.h"
#include "file-helper.h"
#include "plextalk-config.h"

static const char * const book[] = {
	"txt",
	"doc",
	"docx",
	"htm",
	"html",
	"epub",
	NULL
};

static const char * const music[] = {
  	"mp3",
	"wav",
	NULL
};

/** is it a directory. */
int PlextalkIsDirectory(const char *name)
{
    struct stat64 st;

	if ( !stat64(name, &st) ) {
		if ( S_ISDIR(st.st_mode) ) {
			return 1;
		}	
	}

	return 0;
}

static inline int
PlextalkIsDir(const struct dirent *dir)
{	
	if (dir->d_type != DT_UNKNOWN)
		return (dir->d_type == DT_DIR);

	return PlextalkIsDirectory(dir->d_name);
}

static int isTheFile(const char * name, const char * const * p)
{
    char* ext;

	ext = PlextalkGetFileExtension(name);
	if (NULL == ext)
		return 0;

	while (*p) {
		if (!strcasecmp(ext, *p))
			return 1;
		p++;
	}

	return 0;
}

/** is it our book file. */
int PlextalkIsBookFile(const char *name)
{
    return isTheFile(name, book);
}

/** is it our music file. */
int PlextalkIsMusicFile(const char *name)
{
    return isTheFile(name, music);
}

int PlextalkIsDsiayBook(const char *name)
{
	DIR *dp;
	struct dirent *ep;
	const size_t ncc_htm_len = strlen("ncc.htm");
	const size_t dot_opf_len = strlen(".opf");
	size_t d_name_len;
	int ret = 0;

	dp = opendir(name);
	if (dp == NULL)
		return 0;

	while (ep = readdir (dp)) {
		if (PlextalkIsDir(ep))
			continue;

		/* this catch "ncc.htm" and "ncc.html" */
		if (!strncasecmp(ep->d_name, "ncc.htm", ncc_htm_len) &&
			(ep->d_name[ncc_htm_len] == '\0' ||
			 ((ep->d_name[ncc_htm_len] | 0x20) == 'l' && ep->d_name[ncc_htm_len + 1] == '\0')))
		{
			ret = 1;
			break;
		}

		/* now catch "*.opf" */
		d_name_len = strlen(ep->d_name);
		if (d_name_len <= dot_opf_len)
			continue;
		if (!strcasecmp(ep->d_name + d_name_len - dot_opf_len, ".opf")) {
			ret = 1;
			break;
		}
	}

	closedir (dp);
	return ret;
}

/** is file present. */
int PlextalkIsFileExist(const char * name)
{
	struct stat64 st;

	return !stat64(name, &st);
}

/*
 * make sure the path end of slash
 */
void PlextalkAddSlashToPath(char *path, int bufsize)
{
	int len;

	if (!path || bufsize <= 2)
		return;

	len = strlen(path);
	if (len == 0) {
		*path++ = '/';
		*path = '\0';
		return;
	}
	if (len + 1 >= bufsize)
		return;

	if (path[len - 1] != '/') {
		path[len] = '/';
		path[len + 1] = 0;
	}
}

/**
 * concatenate filename to given path, insert '/' if needed.
 */
char* PlextalkCatFilename2Path(char *destbuf, const int bufsize,
						const char *path, const char *filename)
{
	strlcpy(destbuf, path, bufsize);
	PlextalkAddSlashToPath(destbuf, bufsize);
	strlcat(destbuf, filename, bufsize);
	return destbuf;
}

/**
 * get the file extension if available.
 */
char* PlextalkGetFileExtension(const char * name)
{
	char *ext = strrchr(name, '.');
	if (ext != NULL)
		ext++;
	return ext;
}

/**
 * get filename off the full path.
 */
char* PlextalkGetFilenameFromPath(const char *fullname)
{
    char* name;

	name = strrchr(fullname, '/');
	if (NULL == name)
	  return (char*)fullname;

	return name + 1;
}

/**
 * get path off the fullname.
 */
void PlextalkGetPathFromFullname(const char *fullname, char *pathbuf, const int bufsize)
{
	const char *tmp;

	tmp = strrchr(fullname, '/');
	if (tmp == NULL)
		strlcpy(pathbuf, fullname, bufsize);
	else
		strlcpy(pathbuf, fullname, tmp - fullname + 1);
}

/**
 * get realname off the filename.
 */
void PlextalkGetRealnameOfFilename(const char *filename, char *realnamebuf, const int bufsize)
{
	const char *tmp;

	tmp = strrchr(filename, '.');
	if (tmp == NULL)
		strlcpy(realnamebuf, filename, bufsize);
	else
		strlcpy(realnamebuf, filename, tmp - filename + 1);
}

/** Trim string space at head and at end
 * @param src
 *        src string will be transfer.
 * @return
 *        result, trim string.
 *	NOTE: caller must free result, because it is malloc in function inside.
 */
char* PlextalkStringTrim(const char *src)
{
	int i, len;
	char *dst = NULL;

	if (!src) goto bail;

	len = strlen(src);
	for (i = 0; i <= len - 1; i++) {
		if (!isspace(src[i]))
			break;
	}
	if (i != len) {
		while (isspace(src[--len]));

		len = len - i + 1;
		dst = (char *)malloc(len + 1);
		strlcpy(dst, src + i, len + 1);
		return dst;
	}

bail:
	dst = strdup("");
	return dst;
}

int PlextalkIsFileHidden(const char *filename)
{
	return *filename == '.';
}

void PlextalkCheckAndCreateDirectory(const char *path)
{
	struct stat64 st;
	char tmp[PATH_MAX];
	char *tmpstr;
	size_t sz;

	//initial checks for safety and in case we don't need to do anything
	if (path == NULL) return;

	sz = strlen(path);
	if (sz == 0 || (sz == 1 && path[0] == '/')) return; //no need to check the existence of root

	if (!stat64(path, &st)) return;  //do nothing if path already exists completely

	strlcpy(tmp, path, sizeof(tmp)); //copy string to internal buffer so we can manipulate it

	// Loop over each path piece, skipping the root.
	tmpstr = (tmp[0] == '/') ? tmp + 1 : tmp;
	tmpstr = strchr(tmpstr, '/');
	while (tmpstr) {
		//temporarily terminate the string, so stat can check
		*tmpstr = '\0';
		if (stat64(tmp, &st))
			mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		//restore the string and go on checking
		*tmpstr = '/';

		tmpstr = strchr(tmpstr + 1, '/');
	}
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/**
 * Get file size
 *
 * @param fileName
 *		file name
 * @return
 *		file size, unit byte
 */
int PlextalkGetFileSize(const char *fileName)
{
	struct stat64 buf;

	if (!stat64(fileName, &buf))
		return buf.st_size;

	return 0;
}

/**
 * Get directory size
 *
 * @param fileName
 *		directory name
 * @return
 *		directory size, unit byte
 */
static int64_t do_PlextalkGetDirectorySize(const char *dirname)
{
	int64_t total;
	struct stat64 st;

	if (stat64(dirname, &st) != 0)
		return -1;

	total = st.st_size;

	if (!S_ISLNK(st.st_mode) && S_ISDIR(st.st_mode) &&
		!(dirname[0] == '.' && (dirname[1] == '\0' ||
		                       (dirname[1] == '.' && dirname[2] == '\0')))) {
		DIR *dp;
		struct dirent *ep;

		dp = opendir(dirname);
		if (dp == NULL)
			return -1;

		if (chdir(dirname) != 0) {
			closedir(dp);
			return -1;
		}
		while (ep = readdir (dp)) {
			int64_t sum = do_PlextalkGetDirectorySize(ep->d_name);
			if (sum == -1) {
				closedir(dp);
				return -1;
			}
			total += sum;
		}
		chdir("..");

		closedir(dp);
	}

	return total;
}

int64_t PlextalkGetDirectorySize(const char *dirname)
{
	char *cwd;
	int64_t total;

	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
		return -1;

	total = do_PlextalkGetDirectorySize(dirname);

	chdir(cwd);
	free(cwd);

	return total;
}

/*
 * select all except '.' and '..' (used for file-manamegent)
 */
int PlextalkFilterAll(const struct dirent *dir)
{
	/* don't select  '.' and '..' */
	if (dir->d_name[0] == '.' &&
		(dir->d_name[1] == '\0' ||
		(dir->d_name[1] == '.' && dir->d_name[2] == '\0')))
		return 0;

	return 1;
}

/*
 * select all except '.' and '..' (used for file-manamegent)
 */
int PlextalkFilterAllUnhidden(const struct dirent *dir)
{
	/* don't select  '.', '..' and hidden files */
	if (dir->d_name[0] == '.' /*&&
		(dir->d_name[1] == '\0' ||
		(dir->d_name[1] == '.' && dir->d_name[2] == '\0'))*/)
		return 0;

	return 1;
}

/*
 * select directories (used for saveto)
 */
int PlextalkFilterDir(const struct dirent *dir)
{
	/* don't select  '.', '..' and hidden files */
	if (dir->d_name[0] == '.' /*&&
		(dir->d_name[1] == '\0' ||
		(dir->d_name[1] == '.' && dir->d_name[2] == '\0'))*/)
		return 0;

	return PlextalkIsDir(dir);
}

/*
 * select directories and music files
 */
int PlextalkFilterMusic(const struct dirent *dir)
{
	/* don't select  '.', '..' and hidden files */
	if (dir->d_name[0] == '.' /*&&
		(dir->d_name[1] == '\0' ||
		(dir->d_name[1] == '.' && dir->d_name[2] == '\0'))*/)
		return 0;

	if (!PlextalkIsDir(dir))
		return PlextalkIsMusicFile(dir->d_name);

	return !PlextalkIsDsiayBook(dir->d_name);
}

/*
 * select directories and book files
 */
int PlextalkFilterBook(const struct dirent *dir)
{
	if (PlextalkFilterDir(dir))
		return 1;

	return PlextalkIsBookFile(dir->d_name);
}

static int PlextalkCompar (const struct dirent **a, const struct dirent**b)
{
	/* we always put directory in front of files */
	if (PlextalkIsDir(*a) ^ PlextalkIsDir(*b))
		return PlextalkIsDir(*a) ? -1 : 1;

//	return strcoll((*a)->d_name, (*b)->d_name);
	return strcasecmp((*a)->d_name, (*b)->d_name);
}


static inline int
PlextalkIsDir2(const struct dirent *dir, const char *dir_name)
{	
	if (dir->d_type != DT_UNKNOWN)
		return (dir->d_type == DT_DIR);

	char dir_full_path[512];

	snprintf(dir_full_path, 512, "%s/%s", dir_name, dir->d_name);
	return PlextalkIsDirectory(dir_full_path);
}


int PlextalkScanDir(const char *dirname, dir_node_t *node,
		int (*filter) (const struct dirent *))
{
	char *cwd;
	int ndirs;
	int ret;

	memset(node, 0, sizeof(*node));

	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
		return -1;

	if (chdir(dirname) == -1) {
		free(cwd);
		return -1;
	}

	ndirs = scandir(dirname, &node->namelist, filter, filter == PlextalkFilterDir ? alphasort : PlextalkCompar);

	ret = 0;
	if (ndirs == -1) {
		ret = -1;
		goto out;
	}
	if (ndirs == 0)
		goto out;
	
	node->num = ndirs;

	if (filter == PlextalkFilterDir)
		node->dnum = ndirs;
	else {
		int first = 0;

		while (ndirs > 0) {
			int half = ndirs / 2;

			if (!PlextalkIsDir(node->namelist[first + half]))
				ndirs = half;
			else {
				first += half + 1;
				ndirs -= half + 1;
			}
		}

		node->dnum = first;
	}

out:
	chdir(cwd);
	free(cwd);
	
	return ret;
}

void PlextalkDestroyDir(dir_node_t *node)
{
	struct dirent ** name;
	unsigned int num;

	num = node->num;
	node->num = 0;
	node->dnum = 0;

	name = node->namelist;
	node->namelist = NULL;

	if (name) {
		while ( num-- ) {
			if (name[num])
				free(name[num]);
		}
		free(name);
	}
}

int PlextalkMediaPathDepth(const char *path)
{
	int depth;
	const char *end;

	if (!StringStartWith(path, PLEXTALK_MOUNT_ROOT_STR))
		return -1;

	path += strlen(PLEXTALK_MOUNT_ROOT_STR);
	if (*path == '\0')
		return 0;
	if (*path != '/')
		return -1;

	depth = 0;
	while (*++path == '/');

	end = path + strlen(path);
	while (end[-1] == '/')
		end--;

	while (path < end) {
		if (path[0] == '.' && (path[1] == '\0' || path[1] == '/')) {
			path++;
		} else if (path[0] == '.' && path[1] == '.' && (path[2] == '\0' || path[2] == '/')) {
			path += 2;
			depth--;
		} else {
			const char *ptr;

			depth++;
			ptr = memchr(path, '/', end - path + 1);
			if (ptr == NULL || ptr == end)
				break;

			path = ptr + 1;
		}

		while (*path == '/')
			path++;
	}

	return depth;
}

static int do_PlextalkDirDepth(const char *dirname)
{
	DIR *dp;
	struct dirent *ep;
	int max_depth = 0;
	int entries = 0;

	dp = opendir(dirname);
	if (dp == NULL)
		return -1;

	if (chdir(dirname) != 0) {
		closedir(dp);
		return -1;
	}
	while (ep = readdir (dp)) {
		if (ep->d_name[0] == '.' && (ep->d_name[1] == '\0' ||
			                         (ep->d_name[1] == '.' && ep->d_name[2] == '\0')))
			continue;
		entries++;
		if (PlextalkIsDirectory(ep->d_name)) {
			int curr_depth = do_PlextalkDirDepth(ep->d_name);
			if (curr_depth < 0)
				return -1;
			if (max_depth < curr_depth)
				max_depth = curr_depth;
		}
	}

	chdir("..");

	closedir(dp);

	return max_depth + (entries != 0);
}

int PlextalkDirDepth(const char *path)
{
	char *cwd;
	int depth;

	if (!PlextalkIsDirectory(path))
		return 0;

	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
		return -1;

	depth = do_PlextalkDirDepth(path);

	chdir(cwd);
	free(cwd);

	return depth;
}
