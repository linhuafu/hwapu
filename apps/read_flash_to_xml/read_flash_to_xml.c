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
#define DEVICE_CONFIGURATION_FILE "/opt/plextalk/data/config/DeviceConfigurationFile.xml" 
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
	
	if(0==system("mount -o remount,rw /"))
	{
		Debug_printf(" mount -o remount,rw / success!\n");
	}
	else
	{
		Debug_printf("mount -o remount,rw /  failure!\n");	
	}	
	if((mmcblk0_fd = open(UPDATE_PATH ,O_RDWR))!=-1)
	{
		Debug_printf("open %s success\n",UPDATE_PATH);
	}	
	else
	{
		Debug_printf("open %s file error !\n",UPDATE_PATH);
		goto OPEN_ERROR;	
	}
	if((xml_file_fd = open(DEVICE_CONFIGURATION_FILE ,O_RDWR|O_TRUNC|O_CREAT, 0660))!=-1)
	{
		Debug_printf("open %s success\n",DEVICE_CONFIGURATION_FILE);
	}	
	else
	{
		Debug_printf("open %s file error !\n",DEVICE_CONFIGURATION_FILE);
		goto OPEN_ERROR;	
	}

	iRet=lseek(mmcblk0_fd,mmcblk0_offset1,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	//read DeviceConfigurationFile length
	iRet = read(mmcblk0_fd, file_length, FILE_LENGTH_BIT);
	if(FILE_LENGTH_BIT != iRet)
	{
		Debug_printf("read err !\n");
		iRet = -1;
		goto GENERALERR;
	}
	file_length[FILE_LENGTH_BIT]='\0';
	Debug_printf("file_length[FILE_LENGTH_BIT]= %s\n",file_length);
	xml_file_length=atoi(file_length);
	Debug_printf("xml_file_length= %d\n",xml_file_length);
	
	if(xml_file_length==0)
		return 0;
	
	iRet=lseek(mmcblk0_fd,mmcblk0_offset2,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	
	//读文件到/dev/mmcblk0
	while(xml_file_length >= BUFFER_SIZE)
	{
		iRet = read(mmcblk0_fd, buffer, BUFFER_SIZE);
		Debug_printf("read iRet:%d\n",iRet);
		if(BUFFER_SIZE != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		iRet = write(xml_file_fd, buffer, BUFFER_SIZE);
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
		iRet = read(mmcblk0_fd, buffer, xml_file_length);
		if(xml_file_length != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		iRet = write(xml_file_fd, buffer, xml_file_length);
		Debug_printf("write 2 iRet:%d\n",iRet);
		if(xml_file_length != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}		
	}
	Debug_printf("write success\n");
	//clear DeviceConfigurationFile length
	memset(file_length,'\0',FILE_LENGTH_BIT+1);
	iRet=lseek(mmcblk0_fd,mmcblk0_offset1,SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPEN_ERROR;
	}
	iRet = write(mmcblk0_fd, file_length, FILE_LENGTH_BIT);
	if(FILE_LENGTH_BIT != iRet)
	{
		Debug_printf("write err !\n");
		iRet = -1;
		goto GENERALERR;
	}
	close(mmcblk0_fd);
	close(xml_file_fd);
	if(0==system("mount -o remount,ro /"))
	{
		Debug_printf(" mount -o remount,ro / success!\n");
	}
	else
	{
		Debug_printf("mount -o remount,ro /  failure!\n");	
	}	
	return 0;
OPEN_ERROR:
GENERALERR:
error1:
	return -1;
}