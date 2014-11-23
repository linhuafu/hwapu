#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
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
#include "mwtypes.h"
#include <microwin/nano-X.h>//nano-X图形显示
#include "md5.h"
#include <sys/time.h>
//libxml2使用的头文件 
#include <libxml/parser.h>
#include <libxml/tree.h>
#include<signal.h>
#include <sys/time.h>
#include "upgrader_tts.h"

//#include "aes.c" //AES核心算法
//#include "md5.c" //md5密钥生成算法

#define Debug 1
#ifdef Debug
#define Debug_printf(fmt,arg...)    printf(fmt,##arg)
#else
#define Debug_printf(fmt,arg...)     do{;}while(0)
#endif

struct timeval tv_start, tv_over;

//serial num
#define SERIAL_NUM_FILE "/media/mmcblk1p1/Serial_num_configure.xml"
#define SERIAL_NUM_LEN 12
#define SERIAL_NUM_HEAD "514188"
//
#define BUFFER_SIZE 1024
#define FILE_LENGTH_BIT 10

#define MD5_MALLOC_SZ 0x100000				//MD5定义每次校验的数据块为1M
#define AES_MALLOC_SZ 0x100000					//AES每一次以一M为整体进行解密
#define CELL_SZ 16											//AES解密算法以每16个字节进行加密解密
#define CELL_NU 65536                   //CELL_NU=AES_MALLOC_SZ/CELL_SZ

#define MD5_VALUE_SIZE 32           //MD5校验值的大小（32）

#define Test_Debug 1

#define OFFSET_BOOT 0           //boot启动地址
#define OFFSET_FS 20480*512      //fs文件系统
#define OFFSET_UDISK 839680*512 //UDISK文件系统  

#define IMAGE_MESS_NUM 2 ////Image_parameter[0] == offset, Image_parameter[1]== size
#define NUM_OF_IMAGE 3 //镜像的个数
#define PATH_SIZE   50 //存放镜像路径的大小
#define CONFIG_MESS_SIZE    100   //每个信息在config中长度为100,50为描叙，50为值
#define CONFIE_MESS_VALUE_OFFSET 50  //每个信息的值在信息起始偏移的50处
#define XML_MESSAGE_IN_CONFIG 200 //XML文件大小占用config200字节，前面100为size,后面100为MD5 Value
#define IMAGE_MESSAGE_IN_CONFIG 200 //Image文件大小占用config200字节，前面100为offset,后面100为size
#define BUF_DIGIT_LEN 20

#define CONFIG_FILE_SIZE 2048 //定义Config文件的大小
#define XML_OFFSET_IN_CONFIG 2048 
#define PLEXTALK_XML_OFFSET_IN_CONFIG 4096
#define UPDATE_PATH "/dev/mmcblk0"
#define CONFIG_SAVED_PATH_IN_SD "/media/mmcblk1p1/plextalk_config"
#define CONFIG_SAVED_PATH_IN_MEM "/media/mmcblk0p2/plextalk_config"
#define ENCRYPT_PTVFILE_IN_SD "/media/mmcblk1p1/ptv1update.dat" //SD卡上被加密文件的路径
#define ENCRYPT_PTVFILE_IN_MEM "/media/mmcblk0p2/ptv1update.dat" //内部存储空间中的ptv1update.dat.aes
#define XML_FILE_PATH_IN_SD  "/media/mmcblk1p1/ptv1update.xml"
#define XML_FILE_PATH_IN_MEM  "/media/mmcblk0p2/ptv1update.xml"
#define USER_SET_FILE_PATH "/opt/plextalk/usr_setting/"
#define SD_PATH   "/media/mmcblk1p1/"
#define INTER_MEM_PATH "/media/mmcblk0p2/"
#define VERSION_XML_IN_SYS "/opt/plextalk/data/config/version.xml"

char Config_saved_path[PATH_SIZE];
char Xml_file_path[PATH_SIZE];
char SD_DEV[PATH_SIZE];

//ptv1update.dat镜像位置，内部存储空间和SD卡中
enum Position_ON {MEM,SD}Ptv1update_Position; 



//全局变量  LCD显示用途
GR_GC_ID gc;  					//图形上下文号
GR_WINDOW_ID window;   //窗口号
int Lcd_row=0;                 //LCD上显示第几行（从0开始计数）
int Print_buffer_row=0;       //每次写入的message信息存放到LCD buffer第Print_buffer_row行(从0开始计数)
#define LCD_PRINT_ROW 8        //LCD可以显示的行数
#define LCD_PRINT_ROW_LEN 33    //LCD显示行每行的长度（35）
char Lcd_Print_buffer[LCD_PRINT_ROW][LCD_PRINT_ROW_LEN];  //LCD显示缓存数据存放处，总共有八行，每行最多显示35个字符
int Print_count=0;         //LCD显示message的总次数

#define UPDATE_ITEM 10
int Update_option[UPDATE_ITEM]; //从0到8依次代表如下，version存储为1，这表示版本号符合要求可以升级
#define SETTING_FILE_OPTION 0
#define BOOKMARK_FILE_OPTION 1
#define INTER_MEMORY_OPTION  2
#define REBOOT_OPTION 3
#define SAVE_PTV1UPDATE_OPTION 4
#define UPDATE_SOFTWARE_OPTION 5
#define TARGET_LANGUAGE_OPTION 6

//选择值
#define SELECT_TAKEOVER 1
#define SELECT_INITIALISE 0
#define SELECT_REBOOT 1
#define SELECT_POWEROFF 0
#define SELECT_LEAVE 1
#define SELECT_DELETE 0
#define SELECT_CHINESE 0
#define SELECT_HINDI 1
#define SELECT_BOTH_LANGUAGE 2
#define SELECT_UPDATE 0
#define SELECT_VERIFY 1
#define SELECT_BOTH_OPERATE 2

/*
  <User_Setting_file select="1">Take over(1) or Initialise(0)</User_Setting_file>
  <Bookmark_file select="1">Take over(1) or Initialise(0)</Bookmark_file>
  <Contnent_in_internal_memory select="0">Take over(1) or Initialise(0)</Contnent_in_internal_memory>
  <Operation_mode_1 select="1">Reboot(1) or Poweroff(0)</Operation_mode_1>
  <Operation_mode_2 select="1">Leave the [ptv1update.dat] file(1) or Delete the [ptv1update.dat] file (0)</Operation_mode_2>
  <Operation_of_update_software select="2">Only update(0) or Only verify(1) or update and verify(2)</Operation_of_update_software>
  <Target_language select="0">Only Chinese(0) or Only Hindi(1) or Both language(2)</Target_language>
  <Version_Chinese Old_version="1.0" Latest_version="1.5">Old version=xx or later,  Latest version=xx or later</Version_Chinese>
  <Version_Hindi Old_version="1.0" Latest_version="1.5">Old version=xx or later,  Latest version=xx or later</Version_Hindi>
*/

//函数声明
int read_config(int fd,int offset,int *Parameter);
int Decrypt_Ptv1_to_file(int ptv_encrypt_fd,int offset,int size,char *file_name);
int MD5_check_Image(int mmcblk_fd,int offset,int size,char *Saved_MD5_Value);
int Read_MD5_value(int fd,int num,char *MD5_value);
int Update_Image(int ptv_encrypt_fd,int ptv_offset,int size,int mmcblk0_fd,int mmcblk0_offset);
int update_main();//升级主函数
int Print_to_LCD(char *message);//输出字符串到LCD
int generate_ptv1update_xml(int ptv1update_fd,int config_fd);//产生ptv1update.xml并进行验证
int Parse_ptv1update_xml(char *Xml_file,char *version);//读取ptv1update.xml中的信息
void Print_Update_error(char *message);//循环显示错误信息，并提示重启
int Backup_User_file();//升级前进行备份操作
int Operate_after_update();//升级后的操作，文件恢复等
void store_DeviceConfigurationFile(); //保存DeviceConfigurationFile.xml
int write_serial_num(int mmcblk0_fd);//烧写序列号
int deal_with_upgrader_error();//升级失败处理
void String_add_1(char *str);//数字字符串加1操作
int write_xml_to_flash(char *source);//写DeviceConfigurationFile.xml文件到flash
/*int main_bb(int argc,char **argv)
{
	upgrader_tts_init ();
	upgrader_play_error_wav ();
	Wait_tts_play_over();

	//sleep(3);
	printf("destory!\n");
	upgrader_tts_destroy();
	return 0;
}*/
int main(int argc, char **argv)
{
	
	int i;
	int iRet;
	struct stat64 buf;
	struct statfs diskInfo;
	if(GrOpen() < 0){
		fprintf(stderr, "Could not connect to Nano-X Server\n");
		return -1;
	}
	//新建窗口
	window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, 160, 128,10, BLACK, WHITE);
	gc = GrNewGC();//新建图形设备上下文
	GrSetGCBackground (gc, BLACK); //设置图形上下文背景色
	GrSetGCUseBackground (gc, 0);
	GrSetGCForeground (gc, WHITE); //设置图形上下文前景色
	GrMapWindow(window); //绘制窗口
	//tts初始化
	upgrader_tts_init ();
	iRet=update_main();
	if(iRet==-1)
	{
		sleep(1);
		if(SD==Ptv1update_Position)
		{
			while(stat64(SD_DEV, &buf) != 0) {
	        		Print_to_LCD("  SD card remove please poweroff system !");
				upgrader_play_tts("SD card remove please poweroff system");
				Wait_tts_play_over();
	      			sleep(2);
	    		}
		}
		if(SD==Ptv1update_Position)
		{
	 	   statfs("/media/mmcblk1p1", &diskInfo);
		   unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
  		   unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
  		   Debug_printf("free = %llu",freeDisk);
		   while(freeDisk<=10)
		   {
		   	Print_to_LCD("Not enough space on the target media ,Processing is interrupted !");
			upgrader_play_tts(tts_text[Guide_VerUp_CapaFullError]);
			Wait_tts_play_over();
			sleep(1);
		   }
		}
		if(MEM==Ptv1update_Position)
		{
	 	   statfs("/media/mmcblk0p2", &diskInfo);
		   unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
  		   unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
  		   Debug_printf("free = %llu",freeDisk);
		   while(freeDisk<=10)
		   {
		   	Print_to_LCD("Not enough space on the target media ,Processing is interrupted !");
			upgrader_play_tts(tts_text[Guide_VerUp_CapaFullError]);
			Wait_tts_play_over();
			sleep(2);
		   }
		}
		if(access(SD_PATH,W_OK)!=0)
		{
		   while(1)
		   {
		   	Print_to_LCD("Read only media ,Processing is interrupted !");
			upgrader_play_tts(tts_text[Guide_VerUp_WriteProtectError]);
			Wait_tts_play_over();
			sleep(2);
		   }
		}
		if(access(INTER_MEM_PATH,R_OK)!=0)
		{
		   while(1)
		   {
		   	Print_to_LCD("Read error,It may not access the media correctly !");
			upgrader_play_tts(tts_text[Guide_FileManage_MemoryReadError]);
			Wait_tts_play_over();
			sleep(2);
		   }
		}
		if(access(INTER_MEM_PATH,W_OK)!=0)
		{
		   while(1)
		   {
		   	Print_to_LCD("Write error,It may not access the media correctly !");
			upgrader_play_tts(tts_text[Guide_FileManage_MemoryWriteError]);
			Wait_tts_play_over();
			sleep(2);
		   }
		}
		while(1){
			Print_to_LCD("Update System failure!");
			Print_to_LCD("please power off the system !");
			sleep(2);
		}
		//upgrader_play_error_wav ();
		Print_to_LCD("Update System failure!");
		Print_to_LCD("Reboot System 3s later!");
		sleep(3);
		if(system("poweroff")== 0)
			Debug_printf("reboot success!\n");
		else
			Debug_printf("reboot failure!\n");
	}
	//tts销毁
	upgrader_tts_destroy();
	GrDestroyWindow(window);
	GrClose();
	return 0;
}
int update_main()
{
	int ptv1update_fd;//升级包
	int config_fd;		//配置文件
	int Mmcblk_fd;	  //升级镜像拷贝地址
	int config_offset= 0;//config文件中相应的偏移
	int Update_time=0;
	int Mmcblk_offset[NUM_OF_IMAGE]={OFFSET_BOOT,OFFSET_FS,OFFSET_UDISK};
	int Image_parameter[IMAGE_MESS_NUM];//Image_parameter[0] == offset, Image_parameter[1]== size
	char MD5_VALUE[MD5_VALUE_SIZE+1];
	char Version[PATH_SIZE];
	char *version_value=Version;
	FILE *fp;
	char PATH[256];
	int usb_supply;
	int i;
	int iRet=-2;
	if((ptv1update_fd = open(ENCRYPT_PTVFILE_IN_MEM,O_RDONLY))!=-1)
	{
		Debug_printf("%s open success\n",ENCRYPT_PTVFILE_IN_MEM);
		Ptv1update_Position=MEM;
	}	
	else if((ptv1update_fd = open(ENCRYPT_PTVFILE_IN_SD,O_RDONLY))!=-1)
			{
				Debug_printf("%s open success\n",ENCRYPT_PTVFILE_IN_SD);
				Ptv1update_Position=SD;
			}else{
				Print_to_LCD("ptv1update.dat not exit!");
				goto GENER_ERROR;	
		 }
	Print_to_LCD("Available update was found !");
	upgrader_play_tts(tts_text[Guide_VerUp_UpdaterFound]);
	Wait_tts_play_over();
	do{
		if((fp=popen("cat /sys/class/power_supply/usb/present","r"))==NULL)
		{
			Debug_printf("cat /sys/class/power_supply/usb/present  error!\n");
			return -1;
		}
		fscanf(fp,"%d",&usb_supply);
		Debug_printf("usb_supply:%d",usb_supply);
		if(usb_supply==0){
			Print_to_LCD("Connect ac adaptor for updating !");
			upgrader_play_tts(tts_text[Guide_VerUp_USBPowerConnect]);
			Wait_tts_play_over();
			sleep(1);
		}
		pclose(fp);
	}while(usb_supply==0);
	//Starting update
	Print_to_LCD("Starting update !");
	upgrader_play_tts(tts_text[Guide_VerUp_StartUpdate]);
	Wait_tts_play_over();
	
	if(MEM==Ptv1update_Position)
	{
		strcpy(Config_saved_path,CONFIG_SAVED_PATH_IN_MEM);
		strcpy(Xml_file_path,XML_FILE_PATH_IN_MEM);
		Print_to_LCD("Please do not use or turn the player off  until the player has restarted itself automatically !");
		upgrader_play_tts(tts_text[Guide_VerUp_Caution1]);
		Wait_tts_play_over();
	}
	if(SD==Ptv1update_Position)
	{
		strcpy(Config_saved_path,CONFIG_SAVED_PATH_IN_SD);
		strcpy(Xml_file_path,XML_FILE_PATH_IN_SD);
		Print_to_LCD("Please do not use or turn the player off  or remove SD card until the player has restarted itself automatically !");
		upgrader_play_tts(tts_text[Guide_VerUp_Caution2]);
		Wait_tts_play_over();
	}
	//加密函数中的处理
	KeyExpansion();
	//生成config文件
	if(Decrypt_Ptv1_to_file(ptv1update_fd,config_offset,CONFIG_FILE_SIZE,Config_saved_path)==-1)
	{
		return -1;
	}
	if((config_fd = open(Config_saved_path,O_RDONLY))!=-1)
	{
		Debug_printf(" open config success\n");
	}	
	else
	{
		Debug_printf("open file error !\n");
		goto OPEN_ERROR;	
	}
	//获取版本号信息
	get_version_num(version_value);
	Debug_printf("version_value:%s\n", Version);
	//生成ptv1update.xml文件
	if(generate_ptv1update_xml(ptv1update_fd,config_fd)==-1)
		return -1;
	//解析ptv1update.xml文件
	Parse_ptv1update_xml(Xml_file_path,version_value);
	//进行DeviceConfigurationFile.xml文件升级
	if(SD==Ptv1update_Position)//SD卡中
	{
		sprintf(PATH,"%sDeviceConfigurationFile.xml",SD_PATH);
		Debug_printf("write_xml_to_flash source:%s\n",PATH);
		if(write_xml_to_flash(PATH)==-1)
		{
			deal_with_upgrader_error();
			while(1){
				Print_to_LCD("Failed to write DeviceConfigurationFile.xml!");
				sleep(2);
			}
		}
	}
	if(MEM==Ptv1update_Position)
	{
		sprintf(PATH,"%sDeviceConfigurationFile.xml",INTER_MEM_PATH);
		Debug_printf("write_xml_to_flash source:%s\n",PATH);
		if(write_xml_to_flash(PATH)==-1)
		{
			deal_with_upgrader_error();
			while(1){
				Print_to_LCD("Failed to write DeviceConfigurationFile.xml!");
				sleep(2);
			}
		}
	}
	if(Backup_User_file()==-1)//升级前备份文件
		return -1;
	//以读写方式打开
	if((Mmcblk_fd = open(UPDATE_PATH ,O_RDWR))!=-1)
	{
		Debug_printf("open %s success\n",UPDATE_PATH);
	}	
	else
	{
		Debug_printf("open  %s file error !\n",UPDATE_PATH);
		goto OPEN_ERROR;	
	}
	
	//当ptv1uopdate.dat存在于内部存储空间中或者保存内存空间选项选中时不升级Udisk.bin
	if(Ptv1update_Position==MEM||Update_option[INTER_MEMORY_OPTION]==SELECT_TAKEOVER)
		Update_time=NUM_OF_IMAGE-1;
	else
		Update_time=NUM_OF_IMAGE;
	if(SD == Ptv1update_Position )
	{
		system("busybox fuser -m /media/mmcblk0p2 | xargs busybox kill");
		if(system("umount /media/mmcblk0p2/")== 0)
			Debug_printf("umount /media/mmcblk0p2/ success!\n");			
		else
			Debug_printf("umount /media/mmcblk0p2/ failure!\n");
	}
	//升级镜像并校验
	//for(i = 1;i < 2; i++)
	for(i = 0;i < Update_time; i++)
	{
		//if(i==0)
			//break;
		read_config(config_fd,i,Image_parameter);//读取镜像信息
		Read_MD5_value(config_fd,i,MD5_VALUE);	 //获取MD5校验值保存到char MD5_VALUE[MD5_VALUE_SIZE+1]中
		Debug_printf("Image_offset: %d \n",Image_parameter[0]);
		Debug_printf("Image_size: %d \n",Image_parameter[1]);
		if(Update_option[UPDATE_SOFTWARE_OPTION]==SELECT_UPDATE||Update_option[UPDATE_SOFTWARE_OPTION]==SELECT_BOTH_OPERATE)//只升级和都操作时
		{
			if(Update_Image(ptv1update_fd,Image_parameter[0],Image_parameter[1],Mmcblk_fd,Mmcblk_offset[i])==-1)
				return -1;
		}
		if(0==system("sync"))
		{
			Debug_printf("sync  ok\n");
		}
		else
		{	
			Debug_printf("sync  failure\n");
		}
		if(Update_option[UPDATE_SOFTWARE_OPTION]==SELECT_VERIFY&&i==0||Update_option[UPDATE_SOFTWARE_OPTION]==SELECT_BOTH_OPERATE)//只校验和都操作时
		{
			iRet=MD5_check_Image(Mmcblk_fd,Mmcblk_offset[i],Image_parameter[1],MD5_VALUE);
			if(iRet==-1)
				return -1;
			else while(iRet == 0){
					Print_to_LCD("Failed to verify,Processing is interrupted !");
					upgrader_play_tts(tts_text[Guide_VerUp_VerifyError]);
					Wait_tts_play_over();
					sleep(1);
				}
		}
	}	
	if(write_serial_num(Mmcblk_fd)!=0){
		deal_with_upgrader_error();
		while(1){
			Print_to_LCD("Failed to write Serial num !");
			upgrader_play_tts("Failed to write Serial num ");
			Wait_tts_play_over();	
			sleep(1);
		}
	}
	close(config_fd);
	close(Mmcblk_fd);
	close(ptv1update_fd);
	
	Operate_after_update();//升级后的一些处理
	return 0;
OPEN_ERROR:
	close(ptv1update_fd);
GENER_ERROR:
	return -1;
}

//烧写序列号
int write_serial_num(int mmcblk0_fd)
{
	struct stat64 buf;
	xmlDocPtr doc=NULL; //文档指针
	xmlNodePtr cur=NULL;//当前节点指针
	char *value=NULL;
	char *Serial_num;
	char Serial_NUM_1[SERIAL_NUM_LEN+1];
	char Serial_NUM_2[SERIAL_NUM_LEN+1];
	char Serial_NUM_Next[SERIAL_NUM_LEN+1];
	int Select;
	int i;
	int iRet;
	int mmcblk0_offset1=9*1024*1024; //序列号地址9M
	int mmcblk0_offset2=9*1024*1024+512*1024;//备份的序列号地址9.5M
	Debug_printf("start runing write_serial_num\n");
	
	if(stat64(SERIAL_NUM_FILE, &buf) == 0){
		xmlKeepBlanksDefault(0); 
		doc=xmlParseFile(SERIAL_NUM_FILE);	//create Dom tree
		if(doc==NULL)
		{
		   Debug_printf("ERROR: Loading xml file failed.\n");
		   return -1;
		}
		cur=xmlDocGetRootElement(doc);// get root node
		if(cur==NULL)
		{
		   Debug_printf("ERROR: empty file\n");
		   xmlFreeDoc(doc); 
		   return -2; 
		}
		//walk the tree 
		cur=cur->xmlChildrenNode; //get sub node
		if(!xmlStrcmp(cur->name, BAD_CAST"write_serial_num"))
		{
				Debug_printf("write_serial_num!\n");
				cur=cur->xmlChildrenNode;
		}else{
			Debug_printf("please check /media/mmcblk1p1/Serial_num_configure.xml\n");
			xmlFreeDoc(doc);
			xmlCleanupParser();	
			return 0;
		}
			 
		xmlChar* Attribute = xmlGetProp(cur,BAD_CAST "select");
		Select=atoi((char *)Attribute);
		Debug_printf("Select:%d\n",Select);
	  	xmlFree(Attribute); 
		cur=cur->next;
		
		Serial_num=(char*)xmlNodeGetContent(cur);
		Debug_printf("serial num: %s\n",Serial_num);
		strcpy(Serial_NUM_Next,Serial_num);
		
		//read serial num1
		iRet = lseek(mmcblk0_fd, mmcblk0_offset1, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = read(mmcblk0_fd, Serial_NUM_1, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		Serial_NUM_1[SERIAL_NUM_LEN/2]='\0';
		Debug_printf("read Serial num 1:%s\n",Serial_NUM_1);
		//read serial num2
		iRet = lseek(mmcblk0_fd, mmcblk0_offset2, SEEK_SET);
		if(-1 == iRet)
		{
			Debug_printf("lseek begin error\n");
			iRet = -1;
			goto OPEN_ERROR;
		}
		iRet = read(mmcblk0_fd, Serial_NUM_2, SERIAL_NUM_LEN);
		if(SERIAL_NUM_LEN != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		Serial_NUM_2[SERIAL_NUM_LEN/2]='\0';
		Debug_printf("read Serial num 2:%s\n",Serial_NUM_2);
		if((Select==1)||(strcmp(Serial_NUM_1,SERIAL_NUM_HEAD)!=0)||(strcmp(Serial_NUM_2,SERIAL_NUM_HEAD)!=0))
		{
			//write serial num1
			iRet = lseek(mmcblk0_fd, mmcblk0_offset1, SEEK_SET);
			if(-1 == iRet)
			{
				Debug_printf("lseek begin error\n");
				iRet = -1;
				goto OPEN_ERROR;
			}
			iRet = write(mmcblk0_fd, Serial_num, SERIAL_NUM_LEN);
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
			iRet = write(mmcblk0_fd, Serial_num, SERIAL_NUM_LEN);
			if(SERIAL_NUM_LEN != iRet)
			{
				Debug_printf("write err !\n");
				iRet = -1;
				goto GENERALERR;
			}
			//Serial_NUM_Next
			Debug_printf("Serial_NUM_Next:%s\n",Serial_NUM_Next);
			String_add_1(Serial_NUM_Next);
			Debug_printf("Serial_NUM_Next2:%s\n",Serial_NUM_Next);
			xmlNodeSetContent(cur,BAD_CAST Serial_NUM_Next);
			xmlSaveFormatFileEnc(SERIAL_NUM_FILE, doc, "UTF-8", 1);
			Debug_printf("write serial num over!\n");
			Print_to_LCD("write Serial num success!");
			upgrader_play_tts("write Serial num success");
			Wait_tts_play_over();	
		}
		//release resource of xml parser in libxml2
		xmlFreeDoc(doc);
		xmlCleanupParser();	
	}	
	return 0;
OPEN_ERROR:
GENERALERR:
	return -1;
}
//字符串加1操作
void String_add_1(char *str)
{
	int len;
	len=strlen(str);
	while(len--)
	{
		if(str[len]=='9')
		{
			str[len]='0';
			if(len==6)
				break;
		}else{
			str[len]+=1;
			Debug_printf("string add 1 success!\n");
			break;
		}
	}
	
}
//升级前备份文件
int Backup_User_file()
{
	char cmd[256];
	struct stat64 buf;
	if(stat64("/dev/mmcblk1p1", &buf) == 0){
		strcpy(SD_DEV,"/dev/mmcblk1p1");
	}
	else if(stat64("/dev/mmcblk1", &buf) == 0){
		strcpy(SD_DEV,"/dev/mmcblk1");
	}
		
	//进行user setting file文件的备份
	if(SELECT_INITIALISE == Update_option[SETTING_FILE_OPTION])
	{
		sprintf(cmd,"umount -l %s",USER_SET_FILE_PATH);
		if(system(cmd)== 0)
		{
			Debug_printf("umount -l user set file success!\n");
		}
		else
		{
			Debug_printf("umount -l user set file failure!\n");	
			return -1;
		}
		if(stat64(USER_SET_FILE_PATH, &buf) == 0){
			sprintf(cmd,"mkfs.ext4  /dev/mmcblk0p3");
			if(system(cmd)== 0)
			{
				Debug_printf("clear user set file success!\n");
			}
			else
			{
				Debug_printf("clear user set file failure!\n");	
				return -1;
			}
		} 
	}else {
		sprintf(cmd,"umount -l %s",USER_SET_FILE_PATH);
		if(system(cmd)== 0)
		{
			Debug_printf("umount -l user set file success!\n");
		}
		else
		{
			Debug_printf("umount -l user set file failure!\n");	
			return -1;
		}
	}
	//进行bookmark文件备份(只有ptv1update.dat存在于SD卡中才进行bookmark文件备份)
	if(SELECT_TAKEOVER==Update_option[BOOKMARK_FILE_OPTION]&&SD==Ptv1update_Position)
	{
		if(0==system("find /media/mmcblk0p2/ -name *.bmk | sed 's/\\ /\\\\\\ /g' | xargs tar -cvf /media/mmcblk1p1/bookmark.tar"))
		{
			Debug_printf(" do tar file success!\n");
		}
		else
		{
			Debug_printf("do tar file failure!\n");	
			//return -1;
		}
	}
	return 0;
}
int get_version_num(char *version)
{
	xmlDocPtr doc=NULL;
	xmlNodePtr    cur=NULL;
	char* name=NULL;
	char* value=NULL;
	xmlKeepBlanksDefault (0); 
	//create Dom tree
	doc=xmlParseFile(VERSION_XML_IN_SYS);
	if(doc==NULL)
	{
	   Debug_printf("ERROR: Loading xml file failed.\n");
	   exit(1); 
	}
	// get root node
	cur=xmlDocGetRootElement(doc);
	if(cur==NULL)
	{
	   Debug_printf("ERROR: empty file\n");
	   xmlFreeDoc(doc); 
	   exit(2); 
	}
	//walk the tree 
	cur=cur->xmlChildrenNode; //get sub node
	while(cur !=NULL)
	{
		if(!xmlStrcmp(cur->name, BAD_CAST"software"))
		{
			value=(char*)xmlNodeGetContent(cur);
			strcpy(version,value);
			xmlFree(value);
			Debug_printf("version_value:%s\n", version);
		}
	   cur=cur->next;
	}
	// release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();
}
int modify_language_set(char *path)
{
	xmlDocPtr doc=NULL;
	xmlNodePtr    cur=NULL;
	xmlNodePtr  ptr=NULL;
	char* name=NULL;
	char* value=NULL;

	
	if(path==NULL)
	{
	   Debug_printf("ERROR: argc must be 2 or above.\n");
	   return -1;
	}
	Debug_printf("path:%s\n", path);
	xmlKeepBlanksDefault (0); 
	//create Dom tree
	doc=xmlParseFile(path);
	if(doc==NULL)
	{
	   Debug_printf("ERROR: Loading xml file failed.\n");
	   exit(1); 
	}

	// get root node
	cur=xmlDocGetRootElement(doc);
	if(cur==NULL)
	{
	   Debug_printf("ERROR: empty file\n");
	   xmlFreeDoc(doc); 
	   exit(2); 
	}
	cur=cur->xmlChildrenNode; //get sub node
	if(!xmlStrcmp(cur->name, BAD_CAST"languages"))
	{
			Debug_printf("hello!\n");
			cur=cur->xmlChildrenNode;
	}
	if(SELECT_CHINESE == Update_option[TARGET_LANGUAGE_OPTION])
	{
		while(cur!=NULL)
		{
			 xmlChar* szAttr = xmlGetProp(cur,BAD_CAST "locale");
	  		 Debug_printf("DEBUG: name is:select, value is: %s\n",(char *)szAttr);
			 if(!xmlStrcmp((char *)szAttr, BAD_CAST"hi_IN"))
			 {
			 	Debug_printf("chinese only!\n");
				ptr=cur->next;
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
				cur=ptr;
				xmlFree(szAttr); 
			 }else{
				cur=cur->next;
			 }
		}
	}
	if(SELECT_CHINESE == Update_option[TARGET_LANGUAGE_OPTION])
	{
		while(cur!=NULL)
		{
			 xmlChar* szAttr = xmlGetProp(cur,BAD_CAST "locale");
	  		 Debug_printf("DEBUG: name is:select, value is: %s\n",(char *)szAttr);
			 if(!xmlStrcmp((char *)szAttr, BAD_CAST"hi_IN"))
			 {
				ptr=cur->next;
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
				cur=ptr;
				xmlFree(szAttr); 
			 }else{
				cur=cur->next;
			 }
		}
	}else if(SELECT_HINDI== Update_option[TARGET_LANGUAGE_OPTION]){
			while(cur!=NULL)
			{
				 xmlChar* szAttr = xmlGetProp(cur,BAD_CAST "locale");
		  		 Debug_printf("DEBUG: name is:select, value is: %s\n",(char *)szAttr);
				 if(!xmlStrcmp((char *)szAttr, BAD_CAST"zh_CN"))
				 {
					ptr=cur->next;
					xmlUnlinkNode(cur);
					xmlFreeNode(cur);
					cur=ptr;
					xmlFree(szAttr); 
				 }else{
					cur=cur->next;
				 }
				  if(!xmlStrcmp((char *)szAttr, BAD_CAST"zh_TW"))
				 {
					ptr=cur->next;
					xmlUnlinkNode(cur);
					xmlFreeNode(cur);
					cur=ptr;
					xmlFree(szAttr); 
				 }else{
					cur=cur->next;
				 }
			}
	}
	xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);
		
	//release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0; 
}
void store_DeviceConfigurationFile()
{
	xmlDocPtr doc=NULL;
	xmlNodePtr    cur=NULL;
	xmlNodePtr  ptr=NULL;
	char* name=NULL;
	char* value=NULL;
	char path[PATH_SIZE];

	if(SD==Ptv1update_Position)//SD卡中
	{
		strcpy(path,XML_FILE_PATH_IN_SD)	;	
	}
	if(MEM==Ptv1update_Position)
	{
		strcpy(path,XML_FILE_PATH_IN_MEM);	
	}

	printf("path:%s\n", path);
	xmlKeepBlanksDefault (0); 
	//create Dom tree
	doc=xmlParseFile(path);
	if(doc==NULL)
	{
	   printf("ERROR: Loading xml file failed.\n");
	   exit(1); 
	}

	// get root node
	cur=xmlDocGetRootElement(doc);
	if(cur==NULL)
	{
	   printf("ERROR: empty file\n");
	   xmlFreeDoc(doc); 
	   exit(2); 
	}
	cur=cur->xmlChildrenNode; //get sub node
	
	ptr=cur->next;
	xmlUnlinkNode(cur);
	xmlFreeNode(cur);
	cur=ptr;

	if(SD==Ptv1update_Position)//SD卡中
	{
		sprintf(path,"%sDeviceConfigurationFile.xml",SD_PATH);
		Debug_printf("path:%s\n",path);
		xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);
	}
	if(MEM==Ptv1update_Position)
	{
		sprintf(path,"%sDeviceConfigurationFile.xml",INTER_MEM_PATH);
		Debug_printf("path:%s\n",path);
		xmlSaveFormatFileEnc(path, doc, "UTF-8", 1);
	}
	//release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();
	Debug_printf("store store_DeviceConfigurationFile over\n");
	
}

//写DeviceConfigurationFile.xml文件到flash
int write_xml_to_flash(char *source)
{
	struct stat64 buf;
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
	if((xml_file_fd = open(source ,O_RDWR))!=-1)
	{
		Debug_printf("open %s success\n",source);
	}	
	else
	{
		Debug_printf("open %s file error !\n",source);
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
//升级后的操作，文件恢复等
int Operate_after_update()
{
	char cmd[256];
	struct stat64 buf;
	if(SD == Ptv1update_Position ){
		if(0==system("mount -t vfat -o rw,nosuid,nodev,noatime,sync,fmask=0177,dmask=0077,codepage=437,iocharset=utf8,shortname=mixed,quiet,errors=remount-ro,user  /dev/mmcblk0p2 /media/mmcblk0p2/"))
			Debug_printf(" mount -t vfat -o rw,nosuid,nodev,noatime,sync,fmask=0177,dmask=0077,codepage=437,iocharset=utf8,shortname=mixed,quiet,errors=remount-ro,user /dev/mmcblk0p2 /media/mmcblk0p2/ success!\n");
		else
			Debug_printf("mount -t vfat -o rw,nosuid,nodev,noatime,sync,fmask=0177,dmask=0077,codepage=437,iocharset=utf8,shortname=mixed,quiet,errors=remount-ro,user /dev/mmcblk0p2 /media/mmcblk0p2/ failure!\n");
	}
	//进行bookmark备份文件还原操作
	if(SELECT_TAKEOVER==Update_option[BOOKMARK_FILE_OPTION]&&SD==Ptv1update_Position)//(只有ptv1update.dat存在于SD卡中才进行升级)
	{
		if(stat64("/media/mmcblk1p1/bookmark.tar", &buf) == 0) {
			if(0==system("tar -xvf /media/mmcblk1p1/bookmark.tar -C / "))
			{
				system("rm -f  /media/mmcblk1p1/bookmark.tar");
				Debug_printf("recover bookmark file success!\n");
			}
			else
			{	
				Print_to_LCD("recover bookmark file failure!");
			}
		}
	}
	
	//删除生成的config文件和ptv1update.xml文件
	if(SD==Ptv1update_Position)//SD卡中
	{
		if(0==system("rm -f  /media/mmcblk1p1/plextalk_config "))
			Debug_printf(" delete /media/mmcblk1p1/config success!\n");
		else
			Debug_printf("delete /media/mmcblk1p1/config failure!\n");
			
		if(0==system("rm -f  /media/mmcblk1p1/ptv1update.xml "))
			Debug_printf(" delete /media/mmcblk1p1/ptv1update.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk1p1/ptv1update.xml failure!\n");			
	}
	if(MEM==Ptv1update_Position)
	{
		if(0==system("rm -f  /media/mmcblk0p2/plextalk_config "))
			Debug_printf(" delete /media/mmcblk0p2/config success!\n");
		else
			Debug_printf("delete /media/mmcblk0p2/config failure!\n");
			
		if(0==system("rm -f  /media/mmcblk0p2/ptv1update.xml "))
			Debug_printf(" delete /media/mmcblk0p2/ptv1update.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk0p2/ptv1update.xml failure!\n");	
	}

	//不保存ptv1update.dat文件              
	if(SELECT_DELETE==Update_option[SAVE_PTV1UPDATE_OPTION]||MEM==Ptv1update_Position)
	{
		if(SD==Ptv1update_Position)//SD卡中
		{
			if(0==system("rm -f  /media/mmcblk1p1/ptv1update.dat "))
			{
				//Print_to_LCD("delete ptv1update.dat success!");
				Debug_printf(" delete %s success!\n",ENCRYPT_PTVFILE_IN_SD);
			}
			else
			{
				Print_to_LCD("delete ptv1uodate.dat failure!");
				Debug_printf("recover bookmark file failure!\n");	
			}		
		}
		if(MEM==Ptv1update_Position)
		{
			if(0==system("rm -f  /media/mmcblk0p2/ptv1update.dat"))
			{
				//Print_to_LCD("delete ptv1update.dat success!");
				Debug_printf(" delete %s success!\n",ENCRYPT_PTVFILE_IN_MEM);
			}
			else
			{
				Print_to_LCD("delete ptv1uodate.dat failure!");
				Debug_printf("delete %s failure!\n"ENCRYPT_PTVFILE_IN_MEM);	
			}
		}
	}
	Print_to_LCD("Update is completed successfully !");
	upgrader_play_tts(tts_text[Guide_VerUp_Finish]);
	Wait_tts_play_over();
	if(Update_option[REBOOT_OPTION]==SELECT_REBOOT){
		Print_to_LCD("Restarting,Please wait !");
		upgrader_play_tts(tts_text[Guide_VerUp_Restart]);
		Wait_tts_play_over();
	}else if(Update_option[REBOOT_OPTION]==SELECT_POWEROFF){
		Print_to_LCD("system will power off !");
		upgrader_play_tts("system will power off");
		Wait_tts_play_over();
		sleep(1);
	}
	if(SD==Ptv1update_Position){
		//Print_to_LCD("please remove SD card!");
		//upgrader_play_tts("please remove SD card!");
		//Wait_tts_play_over();
		if(stat64("/media/mmcblk1p1/AiSound5", &buf) == 0) {
			if(0==system("rm -rf  /media/mmcblk1p1/AiSound5"))
			{
				Debug_printf(" delete AiSound5 success!\n");
			}
			else
			{
				Print_to_LCD("delete AiSound5  failure!");
				Debug_printf("delete AiSound5  failure!\n");	
			}	
		}else
	 	    Debug_printf("AiSound5 not exist!\n");	
		if(0==system("umount -l  /media/mmcblk1p1/"))
		{
			Debug_printf("umount -l  /media/mmcblk1p1/  ok\n");
		}
		else
		{	
			Debug_printf("umount -l  /media/mmcblk1p1/ failure\n");
		}
		while(stat64(SD_DEV, &buf) == 0) {
			Print_to_LCD("please remove SD card!");
			upgrader_play_tts("please remove SD card!");
			Wait_tts_play_over();
		     	sleep(2); 
	    }
	}
	//删除备份的AiSound5
	if(MEM==Ptv1update_Position)
	{
		if(stat64("/media/mmcblk0p2/AiSound5", &buf) == 0) {
			if(0==system("rm -rf  /media/mmcblk0p2/AiSound5"))
			{
				Debug_printf(" delete AiSound5 success!\n");
			}
			else
			{
				Print_to_LCD("delete AiSound5  failure!");
				Debug_printf("delete AiSound5  failure!\n");	
			}	
		}else
	 	    Debug_printf("AiSound5 not exist!\n");	
	}
	Debug_printf("start reboot!!!\n");
	if(0==system("sync"))
	{
		Debug_printf("sync  ok\n");
	}
	else
	{	
		Debug_printf("sync  failure\n");
	}
	//重启系统
	if(Update_option[REBOOT_OPTION]==SELECT_REBOOT)
	{
		if(system("busybox reboot -nf")== 0)
		{
			//Print_to_LCD("recover bookmark file success!");
			Debug_printf(" reboot success!\n");
		}
		else
			Debug_printf("reboot failure!\n");	
	}else if(Update_option[REBOOT_OPTION]==SELECT_POWEROFF)
		{
			if(system("busybox poweroff -nf")== 0)
			{
				//Print_to_LCD("recover bookmark file success!");
				Debug_printf("poweroff success!\n");
			}
			else
				Debug_printf("poweroff failure!\n");	
	}
		
	return 0;
}
//升级失败处理
int deal_with_upgrader_error()
{
	if(SD==Ptv1update_Position)//SD卡中
	{
		if(0==system("rm -f  /media/mmcblk1p1/plextalk_config "))
			Debug_printf(" delete /media/mmcblk1p1/config success!\n");
		else
			Debug_printf("delete /media/mmcblk1p1/config failure!\n");
			
		if(0==system("rm -f  /media/mmcblk1p1/ptv1update.xml "))
			Debug_printf(" delete /media/mmcblk1p1/ptv1update.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk1p1/ptv1update.xml failure!\n");	
		
		if(0==system("rm -f  /media/mmcblk1p1/DeviceConfigurationFile.xml "))
			Debug_printf(" delete /media/mmcblk1p1/DeviceConfigurationFile.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk1p1/DeviceConfigurationFile.xml failure!\n");	
		
		if(0==system("rm -rf  /media/mmcblk1p1/AiSound5"))
		{
			Debug_printf(" delete AiSound5 success!\n");
		}
		else
		{
			Print_to_LCD("delete AiSound5  failure!");
			Debug_printf("delete AiSound5  failure!\n");	
		}
	}
	if(MEM==Ptv1update_Position)
	{
		if(0==system("rm -f  /media/mmcblk0p2/plextalk_config "))
			Debug_printf(" delete /media/mmcblk0p2/config success!\n");
		else
			Debug_printf("delete /media/mmcblk0p2/config failure!\n");
			
		if(0==system("rm -f  /media/mmcblk0p2/ptv1update.xml "))
			Debug_printf(" delete /media/mmcblk0p2/ptv1update.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk0p2/ptv1update.xml failure!\n");	
		
		if(0==system("rm -f  /media/mmcblk0p2/DeviceConfigurationFile.xml "))
			Debug_printf(" delete /media/mmcblk0p2/DeviceConfigurationFile.xml success!\n");
		else
			Debug_printf("delete /media/mmcblk0p2/DeviceConfigurationFile.xml failure!\n");
		
		if(0==system("rm -rf  /media/mmcblk0p2/AiSound5"))
		{
			Debug_printf(" delete AiSound5 success!\n");
		}
		else
		{
			Print_to_LCD("delete AiSound5  failure!");
			Debug_printf("delete AiSound5  failure!\n");	
		}	
	}
}
//读取ptv1update.xml中的信息
int Parse_ptv1update_xml(char *Xml_file,char *version)
{
	xmlDocPtr doc=NULL; //文档指针
	xmlNodePtr    cur=NULL;//当前节点指针
	char* name=NULL;
	char* value=NULL;
	char *version_old;
	char *version_new;
	int i;

	xmlKeepBlanksDefault(0); 
	doc=xmlParseFile(Xml_file);	//create Dom tree
	if(doc==NULL)
	{
	   Debug_printf("ERROR: Loading xml file failed.\n");
	   Print_Update_error("error happen");
	   exit(1); 
	}
	cur=xmlDocGetRootElement(doc);// get root node
	if(cur==NULL)
	{
	   Debug_printf("ERROR: empty file\n");
	   Print_Update_error("error happen");
	   xmlFreeDoc(doc); 
	   exit(2); 
	}
	//walk the tree 
	cur=cur->xmlChildrenNode; //get sub node
	if(!xmlStrcmp(cur->name, BAD_CAST"plextalk_update_option"))
	{
			Debug_printf("plextalk_update_option!\n");
			cur=cur->xmlChildrenNode;
	}
	for(i = 0; i < 6; i++)
	{
		 //驱动节点属性值，并显示出来
	   name=(char*)(cur->name); 
	   value=(char*)xmlNodeGetContent(cur);
	   Debug_printf("DEBUG: name is: %s, Content is: %s\n", name, value);
	   xmlFree(value); 
	   
	   xmlChar* Attribute = xmlGetProp(cur,BAD_CAST "select");
	   Debug_printf("DEBUG: name is:select, value is: %s\n",(char *)Attribute);
	   Update_option[i]=atoi((char *)Attribute);
	   Debug_printf("Update_option[%d]:%d\n",i,Update_option[i]);
	   xmlFree(Attribute); 
	   cur=cur->next;
	}
	   //  在这里添加版本号的检查来决定是否升级
	while(cur !=NULL)
	{
		if(!xmlStrcmp(cur->name, BAD_CAST"Version"))
		{
			version_old=(char *)xmlGetProp(cur,BAD_CAST "Old_version");
			version_new=(char *)xmlGetProp(cur,BAD_CAST "Latest_version");
			Debug_printf("version_old:%s\n", version_old);
			Debug_printf("version_new:%s\n", version_new);
			Debug_printf("version:%s\n", version);
			if(strlen(version_old)==0&&(strcmp(version_new,version)>=0) \
				||(strcmp(version_old,version)<=0)&&strlen(version_new)==0 \
				||strlen(version_old)==0&&strlen(version_new)==0 \
				||(strcmp(version_old,version)<=0)&&(strcmp(version_new,version)>=0))
			{
				Debug_printf("version ok\n");
			}else{
				for(i = 0; i < 1; i++){
					Print_to_LCD("version check error");
					upgrader_play_tts("version check error");
					Wait_tts_play_over();
				}
				deal_with_upgrader_error();
				Print_Update_error("version check error");
			}
			xmlFree(version_old);
			xmlFree(version_new);
		}	
		cur=cur->next;
	}
	
	//release resource of xml parser in libxml2
	xmlFreeDoc(doc);
	xmlCleanupParser();
	//保存DeviceConfigurationFile.xml
	store_DeviceConfigurationFile();
	return 0; 
}
//产生ptv1update.xml并进行验证
int generate_ptv1update_xml(int ptv1update_fd,int config_fd)
{
	int offset=CONFIE_MESS_VALUE_OFFSET;//ptv1update.xml文件大小存放在config文件中的偏移位置
	int xml_size;
	int iRet;
	int i;

	char buf_digit[BUF_DIGIT_LEN];
	char MD5_value[MD5_VALUE_SIZE+1];
	int xml_file_fd;
	
	memset(buf_digit,BUF_DIGIT_LEN,0);
	lseek(config_fd,offset,0);
	read(config_fd,buf_digit,BUF_DIGIT_LEN);
	xml_size=atoi(buf_digit);
	Debug_printf("xml_size : %d\n",xml_size);
		
	offset+=CONFIG_MESS_SIZE;
	memset(MD5_value,MD5_VALUE_SIZE,0);
	lseek(config_fd,offset,0);
	read(config_fd,MD5_value,MD5_VALUE_SIZE);
	MD5_value[MD5_VALUE_SIZE]='\0';
#if Test_Debug
	Debug_printf("MD5 Value in config:");
	for (i=0;i<MD5_VALUE_SIZE;i++)
	{
		Debug_printf("%c",MD5_value[i]);
	}
	Debug_printf("\n");
#endif
	if(Decrypt_Ptv1_to_file(ptv1update_fd,XML_OFFSET_IN_CONFIG,xml_size,Xml_file_path)==-1)
		return -1;
	
	if((xml_file_fd = open(Xml_file_path,O_RDONLY))!=-1)
	{
		Debug_printf("%s open success\n",Xml_file_path);
	}	
	else
	{
		Debug_printf("file not exit!");
		goto GENER_ERROR;	
	}
	iRet=MD5_check_Image(xml_file_fd,0,xml_size,MD5_value);
	Debug_printf("iRet:%d\n",iRet);
	if(iRet==0)
	{
		Debug_printf("ptv1update.dat is illegal!!\n ");
		Print_Update_error("ptv1update.dat is illegal!");
		exit(0);
	}
	close(xml_file_fd);
	return iRet;
GENER_ERROR:
	return -1;
}

//对升级后的镜像进行校验操作
int MD5_check_Image(int mmcblk_fd,int offset,int size,char *Saved_MD5_Value)
{
	unsigned char decrypt_out[16];
	int i;
	int iRet;
	unsigned char *prd;
	char MD5_value[MD5_VALUE_SIZE+1];
	MD5_CTX md5;
	MD5Init(&md5);                          //初始化用于md5加密的结构

	iRet = lseek(mmcblk_fd, offset, SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	
	prd = (unsigned char*)malloc(MD5_MALLOC_SZ);
	if(NULL == prd)
	{
		Debug_printf("malloc err!");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	//生成MD5校验值
	while(size >= MD5_MALLOC_SZ)
	{
		iRet = read(mmcblk_fd, prd, MD5_MALLOC_SZ);
		if(MD5_MALLOC_SZ != iRet)
		{
			Debug_printf("read err !\n");
			while(1)
		   	{
		   		Print_to_LCD("Read error,It may not access the media correctly !");
				upgrader_play_tts(tts_text[Guide_FileManage_MemoryReadError]);
				Wait_tts_play_over();
				sleep(2);
			}
			iRet = -1;
			goto GENERALERR;
		}
		size -= MD5_MALLOC_SZ;
		MD5Update(&md5, prd, MD5_MALLOC_SZ);   //对欲加密的字符进行加密
	} 
	if(size)
	{
		iRet = read(mmcblk_fd, prd, size);
		if(size != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}

		MD5Update(&md5, prd, size);   //对欲加密的字符进行加密
	}
	MD5Final(decrypt_out,&md5);

	//把结果转换为32个字符字符串存入入MD5_value中
	for(i=0;i<16;i++)
	{
		MD5_value[2*i]=decrypt_out[i]/16;
		if (MD5_value[2*i]<10)
		{
			MD5_value[2*i]+='0';//转换为数字
		}else
		{
			MD5_value[2*i]+='a' - 10;//转换为字母
		}
		MD5_value[2*i+1]=decrypt_out[i]%16;
		if (MD5_value[2*i+1]<10)
		{
			MD5_value[2*i+1]+='0';
		}else
		{
			MD5_value[2*i+1]+='a'-10;
		}
	}
	MD5_value[MD5_VALUE_SIZE]='\0';
	if(strcmp(MD5_value,Saved_MD5_Value)==0)
	{
		Debug_printf("\ncheck MD5 Value success!\n");  
		iRet=1;
	}
	else
		{
			Debug_printf("\ncheck MD5 Value failure!\n");
			iRet=0;
		}
	
#if Test_Debug                                             //获得最终结果
	Debug_printf("print ..........\n");

	for(i=0;i<16;i++)
		Debug_printf("%2x ",decrypt_out[i]);
	Debug_printf("\n");
	
	Debug_printf("MD5 Value :");
	for (i=0;i<MD5_VALUE_SIZE;i++)
	{
		Debug_printf("%c",MD5_value[i]);
	}
	Debug_printf("\n");
#endif 	

GENERALERR:
	free(prd);
OPRRDFILEERR:
	return iRet;
}

//解密ptv1update.dat并进行解密后数据升级操作
int Update_Image(int ptv_encrypt_fd,int ptv_offset,int size,int mmcblk0_fd,int mmcblk0_offset)
{
	int i;

	int iRet=0;
	int last_nu=0;
	int last_sz=0;
	unsigned char *prd;
	
	//读文件指针定位
	iRet = lseek(ptv_encrypt_fd, ptv_offset, SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}

	//写文件指针定位
#if Test_Debug
	Debug_printf("mmcblk0_offset:%d\n",mmcblk0_offset/512);
#endif
	iRet = lseek(mmcblk0_fd, mmcblk0_offset, SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	
	prd = (unsigned char*)malloc(AES_MALLOC_SZ);//分配用于存储1M加密数据
	if(NULL == prd )
	{
		Debug_printf("malloc err!");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	Debug_printf("---------> filesize: %d\n",size);
	while(size >= AES_MALLOC_SZ)//AES_MALLOC_SZ=1M
	{
		gettimeofday(&tv_start, NULL);
		iRet = read(ptv_encrypt_fd, prd, AES_MALLOC_SZ);
		if(AES_MALLOC_SZ != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}

		for(i = 0; i < CELL_NU; i++)//1M数据进行解密处理
		{
			Decrypt_InvCipher(prd + i* CELL_SZ);
		}
		iRet = write(mmcblk0_fd, prd, AES_MALLOC_SZ);
		
		if(AES_MALLOC_SZ != iRet)
		{
			Debug_printf("write err !\n");
		        while(1)
		   	{
		   		Print_to_LCD("Write error,It may not access the media correctly !");
				upgrader_play_tts(tts_text[Guide_FileManage_MemoryWriteError]);
				Wait_tts_play_over();
				sleep(2);
			}
			iRet = -1;
			goto GENERALERR;
		}
		size -= AES_MALLOC_SZ;
		Debug_printf("---------> remain size: %d M\n",size/(1024*1024));
		gettimeofday(&tv_over, NULL);
		Debug_printf(" 1M data cost time:%d s\n",(tv_over.tv_sec-tv_start.tv_sec));
	} 
	if(size)
	{
		last_nu = size/CELL_SZ;	
		last_sz = size%CELL_SZ;	//always 0 因为加密出来的数据永远都是16bytes的倍数
#if Test_Debug
		Debug_printf("last_nu : %d",last_nu);
		Debug_printf("last_sz :%d",last_sz);
#endif
		if(last_sz>0)//当解密生成的镜像文件大小不是16的整数倍数，还是以16的整数倍进行解密，解密成功之后只保存size大
			last_nu++;

		iRet = read(ptv_encrypt_fd, prd, last_nu*CELL_SZ);
		if(last_nu*CELL_SZ != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		for(i = 0; i < last_nu; i++)
		{
			Decrypt_InvCipher(prd + i* CELL_SZ);
		}
		
		iRet = write(mmcblk0_fd, prd, size);
		if(size != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
	}
	Debug_printf(".............update over ..........\n");
	return iRet;
GENERALERR:
	free(prd);
OPRRDFILEERR:

	return -1;
}

//从ptv1update.dat不同地方解密出相应的文件文件
int Decrypt_Ptv1_to_file(int ptv_encrypt_fd,int offset,int size,char *file_name)
{
	int i;

	int wr_decrypt_fd;
	int iRet;
	int last_nu=0;
	int last_sz=0;
	unsigned char *prd;

	//读文件指针定位
	iRet = lseek(ptv_encrypt_fd, offset, SEEK_SET);
	if(-1 == iRet)
	{
		Debug_printf("lseek begin error\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	//system("touch ")
	wr_decrypt_fd = open(file_name, O_RDWR|O_CREAT, 0660);
	if(-1 == wr_decrypt_fd)
	{
		Debug_printf("open file for write err\n");
		iRet = -1;
		goto OPRWRFILEERR;
	}
	
	prd = (unsigned char*)malloc(AES_MALLOC_SZ);//分配用于存储1M加密数据
	if(NULL == prd )
	{
		Debug_printf("malloc err!");
		iRet = -1;
		goto OPRWRFILEERR;
	}
	Debug_printf("---------> filesize: %d\n",size);
	while(size >= AES_MALLOC_SZ)//AES_MALLOC_SZ=1M
	{
		iRet = read(ptv_encrypt_fd, prd, AES_MALLOC_SZ);
		if(AES_MALLOC_SZ != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		for(i = 0; i < CELL_NU; i++)//1M数据进行解密处理
		{
			Decrypt_InvCipher(prd + i* CELL_SZ);
		}
		iRet = write(wr_decrypt_fd, prd, AES_MALLOC_SZ);
		if(AES_MALLOC_SZ != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		size -= AES_MALLOC_SZ;
	} 
	if(size)
	{
		last_nu = size/CELL_SZ;	
		last_sz = size%CELL_SZ;	//always 0 因为加密出来的数据永远都是16bytes的倍数
#if Test_Debug
		Debug_printf("last_nu : %d",last_nu);
		Debug_printf("last_sz :%d",last_sz);
#endif
		if(last_sz>0)//当解密生成的镜像文件大小不是16的整数倍数，还是以16的整数倍进行解密，解密成功之后只保存size大
			last_nu++;

		iRet = read(ptv_encrypt_fd, prd, last_nu*CELL_SZ);
		if(last_nu*CELL_SZ != iRet)
		{
			Debug_printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		for(i = 0; i < last_nu; i++)
		{
			Decrypt_InvCipher(prd + i* CELL_SZ);
		}
		
		iRet = write(wr_decrypt_fd, prd, size);
		if(size != iRet)
		{
			Debug_printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
	}

	Debug_printf("...............update %s over ..........\n",file_name);
GENERALERR:
	free(prd);
OPRWRFILEERR:
	close(wr_decrypt_fd);
OPRRDFILEERR:
	return iRet;
}

//从配置文件config中读出相应镜像的偏移地址和大小
int read_config(int fd,int num,int *Parameter)
{
	int offset;
	char buf_digit[BUF_DIGIT_LEN];

	offset=num*IMAGE_MESSAGE_IN_CONFIG+CONFIE_MESS_VALUE_OFFSET+XML_MESSAGE_IN_CONFIG;//第num个镜像的相关偏移位置，前面200字节的偏移是存放ptv1update.xml的相关信息
	memset(buf_digit,BUF_DIGIT_LEN,0);
	lseek(fd,offset,0);
	read(fd,buf_digit,BUF_DIGIT_LEN);
	Parameter[0]=atoi(buf_digit);
	
	offset += CONFIG_MESS_SIZE;
	memset(buf_digit,BUF_DIGIT_LEN,0);
	lseek(fd,offset,0);
	read(fd,buf_digit,BUF_DIGIT_LEN);
	Parameter[1]=atoi(buf_digit);
	return 0;
}

//从config配置文件中读取相应镜像文件的MD5校验值
int Read_MD5_value(int fd,int num,char *MD5_value)
{
	int offset;
	int i;
	
	offset=NUM_OF_IMAGE*IMAGE_MESSAGE_IN_CONFIG+CONFIG_MESS_SIZE*num+CONFIE_MESS_VALUE_OFFSET+XML_MESSAGE_IN_CONFIG;//MD5的校验值存放在config文件的最后面，前面200字节的偏移是存放ptv1update.xml的相关信息
	//每个MD5_value结果为32个字符
	memset(MD5_value,MD5_VALUE_SIZE+1,0);
	lseek(fd,offset,0);
	read(fd,MD5_value,MD5_VALUE_SIZE);
	MD5_value[MD5_VALUE_SIZE]='\0';
#if Test_Debug
	Debug_printf("MD5 Value in config:");
	for (i=0;i<MD5_VALUE_SIZE;i++)
	{
		Debug_printf("%c",MD5_value[i]);
	}
	Debug_printf("\n");
#endif
}

//输出字符串到LCD 
int Print_to_LCD(char *message)
{
	int i;
	int Buff_row;//从第Buff_row行开始显示
	int Message_len;
	char *ptr;
	
	Message_len=strlen(message);
	ptr=message;
	while( Message_len > LCD_PRINT_ROW_LEN-1)
	{
		memcpy(Lcd_Print_buffer[Print_buffer_row],ptr,LCD_PRINT_ROW_LEN-1);
		Lcd_Print_buffer[Print_buffer_row][LCD_PRINT_ROW_LEN-1]='\0';//最后一个存入结束字符
		Print_count++;//总共显示行数增加一行
		Debug_printf("LCD print:%s\n",Lcd_Print_buffer[Print_buffer_row]);
		
		Print_buffer_row++;
		if(Print_buffer_row==LCD_PRINT_ROW)
			Print_buffer_row=0;
		if(Print_count <= LCD_PRINT_ROW)//总共显示行数小于等于8行，行显示从上到下
			Buff_row = 0;
		else //总共显示行数大于8行，行显示每次更新最后一行（即第八行）
			Buff_row=Print_buffer_row;
			
		GrClearWindow(window,0);
		GrRect(window, gc,0,0,160,128);
		for(i=0;i<LCD_PRINT_ROW;i++)
		{
			GrText(window, gc, 3,i*15+3, Lcd_Print_buffer[Buff_row], -1, GR_TFASCII|MWTF_TOP);
			Buff_row++;
			if(Buff_row==LCD_PRINT_ROW)
				Buff_row=0;
		}
		GrFlush();
		Message_len-=LCD_PRINT_ROW_LEN-1;
		ptr+=LCD_PRINT_ROW_LEN-1;//指针移动
	}
		
	strcpy(Lcd_Print_buffer[Print_buffer_row], ptr);
	Print_count++;//总共显示行数增加一行
	Debug_printf("LCD print:%s\n",Lcd_Print_buffer[Print_buffer_row]);
		
	Print_buffer_row++;
	if(Print_buffer_row==LCD_PRINT_ROW)
		Print_buffer_row=0;
		
	if(Print_count <= LCD_PRINT_ROW)//总共显示行数小于等于8行，行显示从上到下
		Buff_row = 0;
	else //总共显示行数大于8行，行显示每次更新最后一行（即第八行）
		Buff_row=Print_buffer_row;
			
	GrClearWindow(window,0);
	GrRect(window, gc,0,0,160,128);
	for(i=0;i<LCD_PRINT_ROW;i++)
	{
		GrText(window, gc, 3,i*15+3, Lcd_Print_buffer[Buff_row], -1, GR_TFASCII|MWTF_TOP);
		Buff_row++;
		if(Buff_row==LCD_PRINT_ROW)
			Buff_row=0;
	}
	GrFlush();
	return 1;
}

//循环显示错误信息，并提示重启
void Print_Update_error(char *message)
{
	while(1)
	{
			Print_to_LCD(message);
			Print_to_LCD("Please power off the system!!!");
			sleep(1);
	}
}


