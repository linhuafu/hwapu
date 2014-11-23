#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define  MWINCLUDECOLORS
#include <microwin/mwtypes.h>
#include <microwin/nano-X.h>
GR_WINDOW_ID window;
GR_GC_ID gc;

#define PATH_SIZE   50 //存放镜像路径的大小

#define ENCRYPT_PTVFILE_IN_SD1P1 "/media/mmcblk1p1/ptv1update.dat" //SD卡上被加密文件的路径
#define ENCRYPT_PTVFILE_IN_SD1 "/media/mmcblk1/ptv1update.dat" //SD卡上被加密文件的路径
#define ENCRYPT_PTVFILE_IN_MEM "/media/mmcblk0p2/ptv1update.dat" //内部存储空间中的ptv1update.dat.aes

#define SD1P1_PATH   "/media/mmcblk1p1/"
#define SD1_PATH   "/media/mmcblk1/"
#define INTER_MEM_PATH "/media/mmcblk0p2/"

char SD_path[PATH_SIZE];

int main (int argc,char **argv)
{
	struct stat buf;
	struct statfs diskInfo;
	int free_M;
	
	if(argc < 2){
		printf("please input ./upgrader_show  1 or 2 \n");
	}
	
	if(GrOpen() < 0){
		fprintf(stderr, "Could not connect to Nano-X Server\n");
		return -1;
	}

	window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 160, 128,10, BLACK, WHITE);
	gc = GrNewGC();//新建图形设备上下文
	GrSetGCBackground (gc, BLACK); //设置图形上下文背景色
	GrSetGCUseBackground (gc, 0);
	GrSetGCForeground (gc, WHITE); //设置图形上下文前景色
	GrMapWindow(window); //绘制窗口
	
	if(strcmp(argv[1],"1")==0)
	{
		if(stat(ENCRYPT_PTVFILE_IN_MEM, &buf) == 0)
		{
			statfs(INTER_MEM_PATH, &diskInfo);
			unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
	    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
	  	free_M=(int)(freeDisk>>20);
	  	printf("MEM DISK_FREE == %d MB\n",free_M);
	  	if(free_M < 25)
	  	{
	  			GrClearWindow(window,0);
					GrRect(window,gc,0,0,160,128);
					GrText(window,gc, 4,10,"Not enough space on the Udisk", -1, GR_TFASCII|MWTF_TOP);//Not enough space on the target media ,Processing is interrupted !
					GrText(window,gc, 4,30,"for upgrader system !!!", -1, GR_TFASCII|MWTF_TOP);
					GrText(window,gc, 4,50,"Please guaranteed free capacity", -1, GR_TFASCII|MWTF_TOP);
					GrText(window,gc, 4,70,"greater than 25M !!!", -1, GR_TFASCII|MWTF_TOP);
					GrText(window,gc, 4,90,"Please poweroff the system !!!", -1, GR_TFASCII|MWTF_TOP);
					GrFlush();
					while(1);
	  	}
		}else{
			if(stat(ENCRYPT_PTVFILE_IN_SD1P1, &buf) == 0)
			{
				strcpy(SD_path,SD1P1_PATH);
			}else{
				strcpy(SD_path,SD1_PATH);
			}
			statfs(SD_path, &diskInfo);
		 	unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
	 		unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
	 		free_M=(int)(freeDisk>>20);
  		printf("SD DISK_FREE == %d MB\n",free_M);
  		if(free_M < 25)
	 		{
  			GrClearWindow(window,0);
				GrRect(window,gc,0,0,160,128);
				GrText(window,gc, 4,10,"Not enough space on the SD", -1, GR_TFASCII|MWTF_TOP);//Not enough space on the target media ,Processing is interrupted !
				GrText(window,gc, 4,30,"card for upgrader system !!!", -1, GR_TFASCII|MWTF_TOP);
				GrText(window,gc, 4,50,"Please guaranteed free capacity", -1, GR_TFASCII|MWTF_TOP);
				GrText(window,gc, 4,70,"greater than 25M !!!", -1, GR_TFASCII|MWTF_TOP);
				GrText(window,gc, 4,90,"Please poweroff the system !!!", -1, GR_TFASCII|MWTF_TOP);
				GrFlush();
				while(1);
  		}				
		}
	}
	if(strcmp(argv[1],"2")==0)
	{
		GrClearWindow(window,0);
		GrRect(window,gc,0,0,160,128);
		GrText(window,gc, 15,30,"Prepare the environment ", -1, GR_TFASCII|MWTF_TOP);
		GrText(window,gc, 48,50,"for upgrade", -1, GR_TFASCII|MWTF_TOP);
		GrText(window,gc, 43,70,"Please wait !!!", -1, GR_TFASCII|MWTF_TOP);
		GrFlush();
		while(1);	
	}

	GrDestroyWindow(window);
	GrClose();
}

