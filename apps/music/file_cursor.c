#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

#include "file-helper.h"
#include "file_cursor.h"
//#include "dmalloc.h"

#define RESERVED			8
#define MAX_FOLDER_DEPTH 	8
#define MAX_FILE_LENGTH		512

#define __USE_FMSG
#ifdef __USE_FMSG
#define FMSG(fmt, arg...) do {\
	printf("[FMSG]%s,%d-->"fmt, __FUNCTION__ , __LINE__, ##arg); \
} while(0)
#else
#define FMSG(fmt, arg...) do {;}while(0)
#endif

static int  g_endofmusic=0;

typedef struct {
	int index;
	dir_node_t node;
	const char* folder_path;
} DirInfo;

struct file_cursor {
	int pos_flag;
	int mode;
	char* root_folder;
};

static char root_folder[MAX_FILE_LENGTH];
static char ret_file[MAX_FILE_LENGTH];
static char ret_file_buf[MAX_FILE_LENGTH];
static int level;
static DirInfo stack[MAX_FOLDER_DEPTH];

static dir_node_t one_stack;
static int one_index;
static const char* one_folder;

//static int onelevelindex= -1;

const char* file_cursor_one_prev_track (FileCursor* fc, int *pos);
const char* file_cursor_one_next_track (FileCursor* fc, int *pos);


static int MusicCompar (const struct dirent **a, const struct dirent**b)
{
	/* put music files before dir*/
	if (((*a)->d_type == DT_DIR) ^ ((*b)->d_type == DT_DIR))
		return (*a)->d_type == DT_DIR ? 1 : -1;

	return strcasecmp ((*a)->d_name, (*b)->d_name);
}

int FilterMusic(const struct dirent *dir)
{
	/* don't select  '.', '..' and hidden files */
	if (dir->d_name[0] == '.' /*&&
		(dir->name[1] == '\0' ||
		(dir->name[1] == '.' && dir->name[2] == '\0'))*/)
		return 0;

	if (dir->d_type != DT_DIR)
		return PlextalkIsMusicFile(dir->d_name);

	return !PlextalkIsDsiayBook(dir->d_name);
}

static inline int
PlextalkIsDir3(const struct dirent *dir, const char *folder)
{	
	if (dir->d_type != DT_UNKNOWN)
		return (dir->d_type == DT_DIR);

	char buffer[512];

	snprintf(buffer, 512, "%s/%s", folder, dir->d_name);
	
	return PlextalkIsDirectory(buffer);
}

int MusicScanDir(const char *dirname, dir_node_t *node,int (*filter) (const struct dirent *))
{
	char *cwd;
	int ndirs;
	int i;
	int filenum=0;
	int ret;

	FMSG("folder name = %s\n", dirname);

	memset(node, 0, sizeof(*node));
	
	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
		return -1;

	if (chdir(dirname) == -1) {
		free(cwd);
		return -1;
	}

	ndirs = scandir(dirname, &node->namelist, filter, filter == PlextalkFilterDir ? alphasort : MusicCompar);     //经此函数后，扫描的是文件在前面，文件夹在后面

	FMSG("scandir ndirs =%d \n",ndirs);
	
	for (i = 0; i< ndirs; i++)
		FMSG("name=%s \n", node->namelist[i]->d_name);

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
		filenum = 0;
		i = 0;

		while (i < ndirs) {
			if (PlextalkIsDir3(node->namelist[i], dirname))
				break;
			else
				filenum ++;
			i ++;
		}
		
		node->dnum=node->num -filenum;
	}
	
out:
	chdir(cwd);
	free(cwd);
	
	return ret;
}

void MusicDestroyDir(dir_node_t *node)
{
	FMSG("enter MusicDestroyDir fun!\n");

	struct dirent ** name;
	unsigned int num;

	num = node->num;
	node->num = 0;
	node->dnum = 0;	
	name = node->namelist;
	node->namelist = NULL;

	if (name) {
		while (num--) {
			if (name[num]) 
				free(name[num]);
		}
		
		free(name);
	}
}

static int
one_cursor_init (const char* folder)
{
	int ret;

	memset(&one_stack, 0, sizeof(one_stack));
	one_index = 0;
	ret = MusicScanDir(folder, &one_stack, PlextalkFilterMusic);
	if (ret < 0 || (one_stack.num <= 0) || (one_stack.num == one_stack.dnum))
		return -1;

	return 0;
}

static void
one_cursor_uninit (FileCursor* fc)
{
	MusicDestroyDir(&one_stack);
	free(fc->root_folder);
}


/***********************************
得到第一层的文件
*************************************/
static int 
one_cursor_next (void)
{
	int ret;
	char* f_path;
	int f_len;

	stack[0].index ++;

	if (stack[0].index>((signed int)(stack[0].node.num) - 1))
		stack[0].index=0;

	while (stack[0].index < ((signed int)(stack[0].node.num) - 1)) {

		if (PlextalkIsDir3(stack[0].node.namelist[stack[0].index], stack[0].folder_path))
			stack[0].index ++;
		else {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[0].folder_path, stack[0].node.namelist[stack[0].index]->d_name); 
			return 1;
		}
	}
	
	return -1;
}

static int 
one_cursor_prev (void)
{
	int ret;
	char* f_path;
	int f_len;

	stack[0].index --;
	if (stack[0].index < 0)
		stack[0].index = (signed int)(stack[0].node.num-1); 

	while (stack[0].index > 0) {
		if (PlextalkIsDir3(stack[0].node.namelist[stack[0].index], stack[0].folder_path))
			stack[0].index --;
		else {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[0].folder_path, stack[0].node.namelist[stack[0].index]->d_name);	
			return 1;
		}		
	}	

	return -1;
}

static int 
one_cursor_first (void)
{
	one_index = 0;
	snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", one_folder ,one_stack.namelist[one_index]->d_name);
	one_index ++;
	
	return 1;
}


static int
one_cursor_last (void)
{
	one_index = one_stack.num - 1;
	snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", one_folder ,one_stack.namelist[one_index]->d_name);
	one_index ++;

	return 1;
}

static int MaxFolderDepth;

static int 
cursor_push_stack (const char* folder)
{
	int ret;

	level++;
/*
	if (level >= MAX_FOLDER_DEPTH) {
		printf("Max folder Depth!\n");
		level --;
		return -2;
	}
*/
	if (level >= MaxFolderDepth) {
		FMSG("MaxFolderDepth return -2!!\n");
		FMSG("level = %d. MaxFolderDepth = %d\n", level, MaxFolderDepth);
		level --;
		return -2;
	}
	
	memset(&stack[level], 0, sizeof(DirInfo));
	ret = MusicScanDir(folder, &(stack[level].node), FilterMusic);
	if (ret < 0 || (stack[level].node.num <= 0)) 	
		return -1;
			
	stack[level].folder_path = folder;
	stack[level].index = -1;

	return 0;
}

static int
cursor_pop_stack ()
{
	if (level == 0)	//到达最顶层，不出栈
		return -1;
	
	free((void*)stack[level].folder_path);
	MusicDestroyDir(&(stack[level].node));
	level --;
	
	return 0;
}

enum {
	STACK_INIT = 0,				//init status
	STACK_PROC,					//stack in progress
	STACK_OVER,					//stack pop over
};

static int 
cursor_status (void)
{
	FMSG("cursor_status level  =%d   index=%d\n", level,stack[level].index );
	if (((level == 0) && (stack[level].index == -1)) || 
		((level == 0) && (stack[level].index == 0)))
		{
			return STACK_INIT;
		}
	else if ((level == 0) && (stack[level].index == (stack[level].node.num - 1)))
		return STACK_OVER;
	else
		return STACK_PROC;
}

static void
cursor_seek_first (void)
{
	stack[level].index = -1;
}


static void
cursor_seek_last (void)
{	
	stack[level].index = stack[level].node.num ;		//-- will be called
}

static int
cursor_init (const char* folder)
{
	//int f_len;
	//int ret;

	memset(stack, 0, sizeof(DirInfo) * MAX_FOLDER_DEPTH);
	level = -1;
	
	if (cursor_push_stack(folder) == -1) {
		FMSG("cursor init failure!\n");
		return -1;
	}
	
	return 0;
}

/***********************************
fun:  得到当前level (层)的下一首mp3
*************************************/
char* CurLevel_Cursor_next()
{
	if (stack[level].node.num<=stack[level].node.dnum)	//此层没有mp3文件
		return NULL;
	
	while (1) {
		stack[level].index ++;
		if(stack[level].index >(signed int)(stack[level].node.num) - 1)
			stack[level].index =0;

		if (!PlextalkIsDir3(stack[level].node.namelist[stack[level].index], stack[level].folder_path)) {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[level].folder_path, stack[level].node.namelist[stack[level].index]->d_name);    //得到歌曲名
			return ret_file;
		}
	}

	return NULL;
}

/***********************************
fun:  得到当前level (层)的上一首mp3
*************************************/
char* CurLevel_Cursor_prev()
{
	FMSG("enter CurLevel_Cursor_prev!\n");

	if(stack[level].node.num<=stack[level].node.dnum)	//此层没有mp3文件
		return NULL;

	while (1) {
		stack[level].index--;
		if(stack[level].index < 0)
			stack[level].index =(signed int)(stack[level].node.num) - 1;

		if (!PlextalkIsDir3(stack[level].node.namelist[stack[level].index], stack[level].folder_path)) {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[level].folder_path, stack[level].node.namelist[stack[level].index]->d_name);    //得到歌曲名
			return ret_file;
		}
	}

	return NULL;
}

static int
cursor_next ()
{
	int ret;
	char* f_path;
	int f_len;

	g_endofmusic=0;
	
	while (stack[level].index < ((signed int)(stack[level].node.num) - 1)) {
		stack[level].index ++;

		if (!PlextalkIsDir3(stack[level].node.namelist[stack[level].index], stack[level].folder_path)) {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[level].folder_path, stack[level].node.namelist[stack[level].index]->d_name);    //得到歌曲名

			return 1;
		} else  {//文件类型是文件夹
			f_len = strlen(stack[level].folder_path) + strlen(stack[level].node.namelist[stack[level].index]->d_name) + RESERVED;
			f_path = malloc(f_len);
			memset(f_path,0x00,f_len);
			snprintf(f_path, f_len, "%s/%s", stack[level].folder_path, 
				stack[level].node.namelist[stack[level].index]->d_name);
			ret = cursor_push_stack(f_path);
			if (ret == -1) {
				cursor_pop_stack();
				continue;
			} else if (ret == -2){
				continue;
			}
		
			return cursor_next();
		}
	}

	if (cursor_pop_stack() == -1) {
		FMSG("cursor has been to the end!\n");
		return -1;
	} else
		return cursor_next();
}



static int 
cursor_prev (void)
{
	int ret;
	char* f_path;
	int f_len;

	while (stack[level].index > 0 ) {
		stack[level].index --;
		
		if (!PlextalkIsDir3(stack[level].node.namelist[stack[level].index], stack[level].folder_path)) {
			snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[level].folder_path, 
				stack[level].node.namelist[stack[level].index]->d_name);

			return 1;
		} else {
			f_len = strlen(stack[level].folder_path) + 
				strlen(stack[level].node.namelist[stack[level].index]->d_name) + RESERVED;
			f_path = malloc( f_len);
			memset(f_path,0x00,f_len);
			snprintf(f_path, f_len, "%s/%s", stack[level].folder_path, 
				stack[level].node.namelist[stack[level].index]->d_name);
			ret = cursor_push_stack(f_path);
			if (ret == -1) {
				cursor_pop_stack();
				continue;
			} else if (ret == -2) {
				continue;
			}
			stack[level].index = stack[level].node.num ;
			return cursor_prev();
		}
	}	

	if (cursor_pop_stack() == -1) {
		FMSG("cursor prev has been to the begining\n");
		return -1;
	} else 
		return cursor_prev();		
}

static int 
diff_album(const char* path1, const char* path2)
{
	assert(path1);
	assert(path2);

	int ret;

	char* p1 = strrchr(path1, '/');
	if (!p1) {
		printf("error in strrchr path1!\n");
		return 1;
	}

	char* p2 = strrchr(path2, '/');
	if (!p2) {
		printf("error in strrchr path2!\n");
		return 1;
	}

	int size1 = p1 - path1;
	int size2 = p2 - path2;

	if (size1 != size2)
		ret = 1;
	else {
		if (!strncmp(path1, path2, size1))
			ret = 0;
		else
			ret = 1;
	}

	return ret;
}

static void init_global_vals (void) {

	static char root_folder[MAX_FILE_LENGTH];

	memset(root_folder, 0, MAX_FILE_LENGTH);
	memset(ret_file, 0, MAX_FILE_LENGTH);
	memset(ret_file_buf, 0, MAX_FILE_LENGTH);
	memset(stack, 0, sizeof(DirInfo) * MAX_FOLDER_DEPTH);
	level = -1;
	one_index = -1;

	memset(&one_stack, 0, sizeof(one_stack)); 
 	one_folder = NULL;
}

#define last_char(S)	S[strlen(S) - 1]

static int 
get_folder_depth (const char *folder)
{
	int f_size = strlen(folder);
	int i;
	int count = 0;
	int depth = 0;

	for (i = 0; i < f_size; i++) {
		if (folder[i] == '/')
			count ++;
	}
	
	if (count <= 2) {
		FMSG("Err: count for '/' err!\n");
		depth = 0;
	} else {
		depth = count - 2;
		if (last_char(folder) == '/')
			depth -= 1;
	}
	FMSG("Get folder depth = %d\n", depth);

	return depth;
}

FileCursor* file_cursor_create (const char* folder, int mode)
{
	int f_len;
	int ret;

	init_global_vals();

	MaxFolderDepth = MAX_FOLDER_DEPTH - get_folder_depth(folder);
	FMSG("MaxFolderDepth = %d\n", MaxFolderDepth);
	
	FileCursor* fc = malloc(sizeof(FileCursor));
	if (!fc) {
		printf("malloc filecursor error!\n");
		return NULL;
	}

	f_len = strlen(folder);
	fc->root_folder = malloc(f_len + RESERVED);
	if (!fc->root_folder) {
		printf("calloc folder error!\n");
		return NULL;
	}
	memset(fc->root_folder,0x00,f_len + RESERVED);	
	memcpy(fc->root_folder, folder, f_len);
	fc->mode = mode;

	if (mode == MODE_ONE_LEVEL) {
		ret = one_cursor_init(fc->root_folder);
		if (ret == - 1) {
			FMSG("one cursor init error!\n");
			free(fc->root_folder);
			
			return NULL;
		}
		one_folder = fc->root_folder;
	} else {
		if (cursor_init(fc->root_folder) == -1) {
			FMSG("[error] FileCursor: cursor init error!\n");
			free(fc->root_folder);
			return NULL;
		}

		memset(root_folder, 0, MAX_FILE_LENGTH);
		snprintf(root_folder, MAX_FILE_LENGTH, "%s", fc->root_folder);
	}

	return fc;
}

#if 0
const char* file_cursor_first_track (FileCursor* fc, int* pos_flag)
{
	//int ret;

	if (fc->mode == MODE_ONE_LEVEL) {
		printf("file cursor: one level first track!\n");
		one_cursor_first();

	} else {		// MODE_MUX_LEVEL
		/* if stack in init status, set cursor to next */
		if (cursor_status() == STACK_INIT) 
			cursor_next();

		if (cursor_status() == STACK_PROC) 
			while(cursor_prev() != -1);		//cursor prev until to the begning!\n
		else {			/* status: STACK_OVER , reinit and cursor to next */
			cursor_seek_first();
			cursor_next();
		}
	}

	*pos_flag = POS_BEGIN;

	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}



const char* file_cursor_last_track (FileCursor* fc, int* pos_flag)
{
	if (fc->mode == MODE_ONE_LEVEL) {
		printf("file cursor: one level last track!\n");
		one_cursor_last();

	} else {	//MODE_MUX_LEVEL
		switch (cursor_status()) {

			case STACK_INIT:
				cursor_seek_last();
				cursor_prev();
				break;

			case STACK_PROC:
				while(cursor_next() != -1);
				break;

			case STACK_OVER:
				cursor_prev();
				break;
		}
	}

	*pos_flag = POS_END;

	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}
#endif

static void 
set_pos (int* pos) 
{
	FMSG("enter set_pos fun!\n");
	
	switch (cursor_status()) {
		case STACK_INIT:
			*pos = POS_BEGIN;
			break;
			
		case STACK_PROC:
			*pos = POS_CUR;
			break;

		case STACK_OVER:
			*pos = POS_CUR;//POS_END;
			break;
	}

	FMSG("enter set_pos fun_1111 = %d!\n",g_endofmusic);	
	
	if (g_endofmusic) {
		*pos = POS_END;
		g_endofmusic = 0;
	}
}

const char* file_cursor_next_track (FileCursor* fc, int* pos_flag, int mode)
{
	FMSG("mode = %d\n", mode);
	
	if (mode) {
		FMSG("step!\n");
		int ret;
		
		if (fc->mode == MODE_ONE_LEVEL) {
			FMSG("file cursor: one level next track!\n");
			one_cursor_next();
		} else {	//MODE_MUX_LEVEL
			FMSG("step!\n");
			switch (cursor_status()) {
				case STACK_INIT:
				case STACK_PROC:
					ret = cursor_next();
					if (ret == -1)
					{
						FMSG("Arrive  the end of album!\n");
						cursor_seek_first();
						cursor_next();
						g_endofmusic=1;
					}
					break;

				case STACK_OVER:
					{
						FMSG("STACK_OVER   Arrive  the end of album!\n");						
						cursor_seek_first();
						cursor_next();
						g_endofmusic=1;
						break;
					}
			}
		}
		set_pos(pos_flag);

		return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
	} else {
		return file_cursor_one_next_track (fc, pos_flag);
	}
}

const char* file_cursor_prev_track (FileCursor* fc, int* pos_flag, int mode)
{
	if (mode) {
		if (fc->mode == MODE_ONE_LEVEL) {
			FMSG("file cursor: one level prev track!\n");
			one_cursor_prev();
		} else {		//MODE_MUX_LEVEL
			switch (cursor_status()) {
				case STACK_INIT: 	
					cursor_seek_last();
					cursor_prev();
					break;

				case STACK_PROC:
				case STACK_OVER:
					cursor_prev();					//prev track
					break;
			}
		}
		set_pos(pos_flag);

		return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
	} else {
		return file_cursor_one_prev_track(fc, pos_flag);
	}
}

const char* file_cursor_next_album (FileCursor* fc)
{
	if (fc->mode == MODE_ONE_LEVEL) {
		FMSG("No album in one level!\n");
		
		return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
	} else { 	//MODE_MUX_LEVEL
		memset(ret_file_buf, 0, MAX_FILE_LENGTH);
		memcpy(ret_file_buf, ret_file, MAX_FILE_LENGTH);

		while (cursor_next() != -1) {
			if (diff_album(ret_file_buf, ret_file))
				return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
		}
		
		FMSG("Has been the the end!\n");
		cursor_seek_first();
		cursor_next();
	}

	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}

void TopLevel_toFirst()
{
	stack[0].index = -1;
}

/******************************
fun:判断第一层是否有mp3文件，如有return 1,否则 return 0
******************************/
int Have_Muisc_InTopLevel (void)
{//为解决客户9537 bug

    return 0;
  //	return (stack[0].node.num > stack[0].node.dnum);
}

/********************************
Fun:  判断第一层的歌曲是否play 到最后一个
**********************************/
int IsTopLevel_End(void)
{
	int index = stack[0].index;

	FMSG("enter IsTopLevel_End fun =%d  %d!\n", stack[0].node.num,stack[0].node.dnum);

	if (stack[0].node.num <= stack[0].node.dnum)
		return 1;

	if (index >= stack[0].node.num-stack[0].node.dnum-1)
		return  1;
	else
		return 0;
}

const char* file_cursor_one_next_track (FileCursor* fc, int *pos)
{	
	int i = 0;

	if (stack[0].node.dnum == stack[0].node.num) 
	{
		FMSG("stack[0].node.dnum == stack[0].node.num\n");
		return NULL;
	} else {
		FMSG("stack[0].index = %d\n",stack[0].index);
		do {
			stack[0].index ++;
			if (stack[0].index > stack[0].node.num - 1)
				stack[0].index  = 0;	
		} while (PlextalkIsDir3(stack[0].node.namelist[stack[0].index], stack[0].folder_path));
			
		FMSG("stack[0].index=%d\n", stack[0].index);
		snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[0].folder_path, 
				stack[0].node.namelist[stack[0].index]->d_name);
		FMSG("ret_file=%s\n",ret_file);
	}

	if (!stack[0].index)
		*pos = POS_BEGIN;
	else if (stack[0].index == stack[0].node.num - stack[0].node.dnum - 1)
		*pos = POS_END;
	else 
		*pos = POS_CUR;
	
	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}


const char* file_cursor_one_prev_track (FileCursor* fc, int *pos)
{	
	if (stack[0].node.dnum == stack[0].node.num)
		return NULL;
	else {
		do {
			FMSG("stack[0].index = %d\n", stack[0].index);
			stack[0].index--;
			if (stack[0].index < 0)
				stack[0].index = stack[0].node.num - 1;		
		} while (PlextalkIsDir3(stack[0].node.namelist[stack[0].index], stack[0].folder_path));

		FMSG("stack[0].index=%d\n", stack[0].index);
		snprintf(ret_file, MAX_FILE_LENGTH, "%s/%s", stack[0].folder_path, 
				stack[0].node.namelist[stack[0].index]->d_name);
		FMSG("ret_file=%s\n", ret_file);
	}

	if (!stack[0].index)
		*pos = POS_BEGIN;
	else if (stack[0].index == stack[0].node.num - stack[0].node.dnum - 1)
		*pos = POS_END;
	else 
		*pos = POS_CUR;
	
	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}

const char* file_cursor_prev_album (FileCursor* fc)
{
	if (fc->mode == MODE_ONE_LEVEL) {
		FMSG("No album in one level!\n");

		return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
 	} else {	//MODE_MUX_LEVEL
		memset(ret_file_buf, 0, MAX_FILE_LENGTH);
		memcpy(ret_file_buf, ret_file, MAX_FILE_LENGTH);

		while (cursor_prev() != -1) {
			if (diff_album(ret_file_buf, ret_file)) {
				memset(ret_file_buf, 0, MAX_FILE_LENGTH);
				memcpy(ret_file_buf, ret_file, MAX_FILE_LENGTH);
				
				while (cursor_prev() != -1) {
					if (diff_album(ret_file_buf, ret_file)) {
						if (cursor_next() != -1)
							return ret_file;
					}
				}
			}
		}
		FMSG("has been to the begining!\n");
		cursor_seek_last();
		cursor_prev();
 	}

	return PlextalkIsMusicFile(ret_file) ? ret_file : NULL;
}

void file_cursor_destroy (FileCursor* fc)
{
	if (fc->mode == MODE_ONE_LEVEL) {
		one_cursor_uninit(fc);
	} else {	//MODE_MUX_LEVEL
		if (level >= 0)
			while (cursor_pop_stack() != -1);
	}

	free(fc);	
	free((void*)stack[0].folder_path);
	PlextalkDestroyDir(&(stack[0].node));
}

int file_cursor_curt_count (FileCursor* fc, int mode)
{
#if 0
	int count;

	if (mode) {
		if (fc->mode == MODE_ONE_LEVEL) {
			
			count = one_index + 1;

		} else {	// MODE_MUX_LEVEL
			
			count = stack[level].index + 1;
		}
	} else 
		count = onelevelindex;
#endif

	return mode ? stack[level].index + 1 : stack[0].index + 1;
}

int Get_Album_TopLevel(FileCursor* fc)
{
	return stack[0].node.num;
}

int file_cursor_max_count(FileCursor* fc, int mode)
{

#if 0
	int count;

	if (mode)
	{
		if (fc->mode == MODE_ONE_LEVEL) 
		{
			
			count = one_stack.num - one_stack.dnum;
			
		}
		else 
		{	//MODE_MUX_LEVEL
				
			count = stack[level].node.num - stack[level].node.dnum;
		}
		
	}
	else
		count = stack[0].node.num - stack[0].node.dnum;
#endif	
	return mode ? stack[level].node.num - stack[level].node.dnum :  stack[0].node.num - stack[0].node.dnum;
}

