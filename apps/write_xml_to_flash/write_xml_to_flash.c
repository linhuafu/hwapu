#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include<signal.h>
#include <sys/time.h>

#define Debug 1
#ifdef Debug
#define Debug_printf(fmt,arg...)    printf(fmt,##arg)
#else
#define Debug_printf(fmt,arg...)     do{;}while(0)
#endif
#define DEVICE_CONFIGURATION_FILE "/media/mmcblk1p1/DeviceConfigurationFile.xml" 
#define UPDATE_PATH "/dev/mmcblk0"
#define BUFFER_SIZE 1024
#define FILE_LENGTH_BIT 10
int main(int argc,char **argv)
{
	struct stat buf;
	int mmcblk0_offset1=9*1024*1024+1*1024;//xml文件大小写入地址
	int mmcblk0_offset2=9*1024*1024+2*1024;//xml文件写入地址
	int xml_file_fd;
	int mmcblk0_fd;	  //升级镜像拷贝地址
	int xml_file_length;
	int iRet;
	char file_length[FILE_LENGTH_BIT+1];
	char buffer[BUFFER_SIZE];
	
	memset(file_length,'\0',FILE_LENGTH_BIT+1);
	if((mmcblk0_fd = open(UPDATE_PATH ,O_RDWR))!=-1)
	{
		Debug_printf("open %s success\n",UPDATE_PATH);
	}	
	else
	{
		Debug_printf("open %s file error !\n",UPDATE_PATH);
		goto OPEN_ERROR;	
	}
	if((xml_file_fd = open(DEVICE_CONFIGURATION_FILE ,O_RDWR))!=-1)
	{
		Debug_printf("open %s success\n",DEVICE_CONFIGURATION_FILE);
	}	
	else
	{
		Debug_printf("open %s file error !\n",DEVICE_CONFIGURATION_FILE);
		goto OPEN_ERROR;	
	}
	xml_file_length=lseek(xml_file_fd,0,SEEK_END);
	Debug_printf("DeviceConfigurationFile_length %d \n",xml_file_length);
	
	iRet=lseek(xml_file_fd,0,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	iRet=lseek(mmcblk0_fd,mmcblk0_offset1,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	sprintf(file_length,"%d",xml_file_length);
	Debug_printf("file_length %s\n",file_length);
	Debug_printf("file_length1 %d\n",strlen(file_length));
	//write DeviceConfigurationFile length
	iRet = write(mmcblk0_fd, file_length, FILE_LENGTH_BIT);
	if(FILE_LENGTH_BIT != iRet)
	{
		Debug_printf("write err !\n");
		iRet = -1;
		goto GENERALERR;
	}
	
	iRet=lseek(mmcblk0_fd,mmcblk0_offset2,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	
	//写文件到/dev/mmcblk0
	while(xml_file_length >= BUFFER_SIZE)
	{
		iRet = read(xml_file_fd, buffer, BUFFER_SIZE);
		Debug_printf("read iRet:%d\n",iRet);
		if(BUFFER_SIZE != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		iRet = write(mmcblk0_fd, buffer, BUFFER_SIZE);
		Debug_printf("write 1 iRet:%d\n",iRet);
		if(BUFFER_SIZE != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		xml_file_length -= BUFFER_SIZE;
	} 
	if(xml_file_length)
	{
		iRet = read(xml_file_fd, buffer, xml_file_length);
		if(xml_file_length != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		iRet = write(mmcblk0_fd, buffer, xml_file_length);
		Debug_printf("write 2 iRet:%d\n",iRet);
		if(xml_file_length != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}		
	}
	
	Debug_printf("write success\n");
	close(mmcblk0_fd);
	close(xml_file_fd);
	return 0;
OPEN_ERROR:
GENERALERR:
error1:
	return -1;
}