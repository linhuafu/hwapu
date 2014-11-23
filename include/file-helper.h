#ifndef FILE_HELPER__H
#define FILE_HELPER__H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>

/** is it our book file. */
int PlextalkIsBookFile(const char *name);

/** is it our music file. */
int PlextalkIsMusicFile(const char *name);

int PlextalkIsDsiayBook(const char *name);

/** is it a directory. */
int PlextalkIsDirectory(const char *name);

/** is file present. */
int PlextalkIsFileExist(const char * name);

/*
 * make sure the path end of slash
 */
void PlextalkAddSlashToPath(char *path, int bufsize);

/**
 * concatenate filename to given path, insert '/' if needed.
 */
char* PlextalkCatFilename2Path(char *destbuf, const int bufsize,
						const char *path, const char *filename);

/**
 * get the file extension if available.
 */
char* PlextalkGetFileExtension(const char * name);

/**
 * get filename off the full path.
 */
char* PlextalkGetFilenameFromPath(const char *fullname);

/**
 * get path off the fullname.
 */
void PlextalkGetPathFromFullname(const char *fullname, char *pathbuf, const int bufsize);

/**
 * get realname off the filename.
 */
void PlextalkGetRealnameOfFilename(const char *filename, char *realnamebuf, const int bufsize);

/** Trim string space at head and at end
 * @param src
 *        src string will be transfer.
 * @return
 *        result, trim string.
 *	NOTE: caller must free result, because it is malloc in function inside.
 */
char *PlextalkStringTrim(const char *src);

int PlextalkIsFileHidden(const char *filename);

void PlextalkCheckAndCreateDirectory(const char *path);

/**
 * Get file size
 *
 * @param fileName
 *		file name
 * @return
 *		file size, unit byte
 */
int PlextalkGetFileSize(const char *fileName);

/**
 * Get directory size (unfollow symlinks)
 *
 * @param fileName
 *		directory name
 * @return
 *		directory size, unit byte
 */
int64_t PlextalkGetDirectorySize(const char *dirname);

typedef struct
{
 	struct dirent ** namelist;	/* directory/file name list. */
	unsigned int     num;       /* total number of directory/file entries. */
	unsigned int     dnum;	    /* total number of valid directory entries. */
} dir_node_t;

/*
 * scan dir, use alphasort
 */
int PlextalkScanDir(const char *dirname, dir_node_t *node,
		int (*filter) (const struct dirent *));

void PlextalkDestroyDir(dir_node_t *);

/*
 * select all except '.' and '..'
 */
int PlextalkFilterAll(const struct dirent *);

/*
 * select all except start with '.' (used for file-manamegent)
 */
int PlextalkFilterAllUnhidden(const struct dirent *dir);

/*
 * select directories (used for saveto)
 */
int PlextalkFilterDir(const struct dirent *);

/*
 * select directories and music files
 */
int PlextalkFilterMusic(const struct dirent *);

/*
 * select directories and book files
 */
int PlextalkFilterBook(const struct dirent *);

/*
 * calculaute given path's depth
 * depth of media's root is 1
 */
int PlextalkMediaPathDepth(const char *);

/*
 * get given directory's depth
 */
int PlextalkDirDepth(const char *);

#endif /* FILE_HELPER__H */
