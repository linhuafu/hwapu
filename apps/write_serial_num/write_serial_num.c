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

#define Debug 1
#ifdef Debug
#define Debug_printf(fmt,arg...)    printf(fmt,##arg)
#else
#define Debug_printf(fmt,arg...)     do{;}while(0)
#endif


#define SERIAL_NUM_FILE_IN_SD1 "/media/mmcblk1/Plextalk_Serial_num_configure.txt"
#define SERIAL_NUM_FILE_IN_SD1P1 "/media/mmcblk1p1/Plextalk_Serial_num_configure.txt"
#define UPDATE_PATH "/dev/mmcblk0"
#define SERIAL_NUM_LEN 12
#define SERIAL_NUM_HEAD "514188"

#define PATH_SIZE   50 //存放镜像路径的大小
char SERIAL_NUM_FILE[PATH_SIZE];

void String_add_1(char *str);
int write_serial_num();


int main (int argc,char **argv)
{
	GR_WINDOW_ID window;
	GR_GC_ID gc;
	struct stat buf;
	struct statfs diskInfo;
	int iRet;
	
	if(stat(SERIAL_NUM_FILE_IN_SD1, &buf) == 0)
	{
		strcpy(SERIAL_NUM_FILE,SERIAL_NUM_FILE_IN_SD1);
	}else if(stat(SERIAL_NUM_FILE_IN_SD1P1, &buf) == 0){
		strcpy(SERIAL_NUM_FILE,SERIAL_NUM_FILE_IN_SD1P1);
	}else{
		Debug_printf("no Serial_num_configure.xml in sd !\n");
		return 0;	
	}
	
	iRet=write_serial_num();
	if(iRet==1)
	{
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
		
		GrClearWindow(window,0);
		GrRect(window,gc,0,0,160,128);
		GrText(window,gc, 15,35,"Write serial num success !!!", -1, GR_TFASCII|MWTF_TOP);
		GrText(window,gc, 18,75,"Please remove SD card !!!", -1, GR_TFASCII|MWTF_TOP);
		GrFlush();
		
		if(stat(SERIAL_NUM_FILE_IN_SD1, &buf) == 0)
		{
			system("umount -l /media/mmcblk1/");
		}else if(stat(SERIAL_NUM_FILE_IN_SD1P1, &buf) == 0){
			system("umount -l /media/mmcblk1p1/");
		}
		while(1)
		{
			if(stat("/dev/mmcblk1", &buf) == 0){
				Debug_printf("sd card is exist\n");
			}else if(stat("/dev/mmcblk1p1", &buf) == 0){
				Debug_printf("sd card is exist !\n");
			}else{
				break;	
			}
			sleep(1);
		}
		GrDestroyWindow(window);
		GrClose();
		system("sync");
		system("sync");
		system("sync");
		system("reboot -f");
	}				
	return 0;
}

int write_serial_num()
{
	struct stat buf;
	char Serial_NUM[SERIAL_NUM_LEN+1];
	int i;
	int iRet;
	int mmcblk0_fd;	  //升级镜像拷贝地址
	int Serial_file_fd;
	int mmcblk0_offset1=9*1024*1024; //序列号地址
	int mmcblk0_offset2=9*1024*1024+512*1024;//备份的序列号地址
	Debug_printf("start runing\n");
	
	if(stat(SERIAL_NUM_FILE, &buf) == 0){

		//打开序列号文件
		if((Serial_file_fd = open(SERIAL_NUM_FILE ,O_RDWR))!=-1)
		{
			Debug_printf("open %s success\n",SERIAL_NUM_FILE);
		}	
		else
		{
			Debug_printf("open %s file error !\n",SERIAL_NUM_FILE);
			goto OPEN_ERROR;	
		}
		//读出序列号
		iRet = lseek(Serial_file_fd, 0, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = read(Serial_file_fd, Serial_NUM, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		Serial_NUM[SERIAL_NUM_LEN]='\0';
		Debug_printf("serial num: %s\n",Serial_NUM);
		
		
		//open /dev/mmcblk0
		if((mmcblk0_fd = open(UPDATE_PATH ,O_RDWR))!=-1)
		{
			Debug_printf("open %s success\n",UPDATE_PATH);
		}	
		else
		{
			Debug_printf("open %s file error !\n",UPDATE_PATH);
			goto OPEN_ERROR;	
		}

		//write serial num1
		Debug_printf("write serial num\n");
		iRet = lseek(mmcblk0_fd, mmcblk0_offset1, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = write(mmcblk0_fd, Serial_NUM, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		//write serial num2
		iRet = lseek(mmcblk0_fd, mmcblk0_offset2, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = write(mmcblk0_fd, Serial_NUM, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		usleep(500000);
		system("sync");
		usleep(500000);
		system("sync");
		Debug_printf("Serial_NUM_Next:%s\n",Serial_NUM);
		String_add_1(Serial_NUM);
		Debug_printf("Serial_NUM_Next1:%s\n",Serial_NUM);
		
		//写序列号到文件
		iRet = lseek(Serial_file_fd, 0, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = write(Serial_file_fd, Serial_NUM, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		usleep(500000);
		system("sync");
		usleep(500000);
		system("sync");
		
		close(Serial_file_fd);
		close(mmcblk0_fd);
		Debug_printf("write serial num over!\n");
		return 1;		
	}	
OPEN_ERROR:
GENERALERR:
	return -1;
}
void String_add_1(char *str)
{
	int len;
	len=strlen(str);
	Debug_printf("len:%d\n",len);
	while(len--)
	{
		if(str[len]=='9')
		{
				str[len]='0';
				if(len==6)
					break;	
		}
		else
		{
				str[len]+=1;
				break;
		}
	}	
}
