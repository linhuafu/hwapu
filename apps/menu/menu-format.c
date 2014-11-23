#include "nw-menu.h"
#include "plextalk-keys.h"
#include "menu-defines.h"
#include "storage.h"
#include "plextalk-constant.h"
#include "plextalk-theme.h"
#include "plextalk-ui.h"
#include "nxutils.h"
#include "key-value-pair.h"
#include "mulitlist.h"
#include "xml-helper.h"
#include "file-helper.h"
#include "device.h"
#include "rtc-helper.h"
#include "menu-backup.h"
#define OSD_DBG_MSG
#include "nc-err.h" 
//#include "libinfo.h"


int menu_format_list_init(struct menu *formatmenu,int partions)
{
	int mallosize = 80;
	int n = 1;
	char *device;
	int ret;
	char buf[50];
	int len;
	int i;
	char *pUsb = _("USB Memory");
	int pusblen = strlen(pUsb);
	char *pDev = "/dev/sd";
	int pdevlen = strlen(pDev);
	char *pSD = _("SD Card");
	int psdlen = strlen(pSD);
	char *pSDdevp1 = "/dev/mmcblk1p1";
	int pSDdevp1len = strlen(pSDdevp1);
	char *pSDdev = "/dev/mmcblk1";
	int pSDdevlen = strlen(pSDdev);
	

	memset(buf,0x00,50);
	strcpy(buf,"/sys/class/block/mmcblk1");
	
	if(PlextalkIsFileExist(buf))//SD Card Exist
	{
		strcat(buf,"p1");
		if(PlextalkIsFileExist(buf))//SD Card Partiion1 Exist
		{				
			device = (char*)malloc(mallosize);//format_menu_destroy free
			memset(device,0x00,mallosize);
			snprintf(device,mallosize,"%s",pSD);
			formatmenu->items[n] = device;

			device = (char*)malloc(mallosize);//format_menu_destroy free
			memset(device,0x00,mallosize);
			snprintf(device,mallosize,"%s",pSDdevp1);
			formatmenu->actions[n].data = device;//"/dev/mmcblk1p1";
		}
		else
		{
			device = (char*)malloc(mallosize);//format_menu_destroy free
			memset(device,0x00,mallosize);
			snprintf(device,mallosize,"%s",pSD);
			formatmenu->items[n] = device;//_("SD Card");
			
			device = (char*)malloc(mallosize);//format_menu_destroy free
			memset(device,0x00,mallosize);
			snprintf(device,mallosize,"%s",pSDdev);
			formatmenu->actions[n].data = device;//"/dev/mmcblk1";
		}

		n++;
	}

	//USB
	memset(buf,0x00,50);
	strcpy(buf,"/sys/class/block/sd");
	len = strlen(buf);
	
	for(ret = 0; ret < 26; ret++) //a-z
	{
		buf[len] = 'a'+ret;
		if(PlextalkIsFileExist(buf))
			break;
	}
	
	if(ret < 26)
	{
		for(i = 1; i < partions; i++)
		{
			buf[len+1] = '0'+i;
			if(PlextalkIsFileExist(buf))
			{
				device = (char*)malloc(mallosize);//format_menu_destroy free
				memset(device,0x00,mallosize);

				if(i==1)
				{
					snprintf(device,mallosize,"%s",pUsb);
				}
				else
				{
					snprintf(device,mallosize,"%s %d",pUsb,i-1);
				}
				
				
				formatmenu->items[n] = device;

				device = (char*)malloc(mallosize);//format_menu_destroy free
				memset(device,0x00,mallosize);
				strcpy(device,pDev);
				*(device+pdevlen) =  'a'+ret;
				*(device+pdevlen+1) =  '0'+i;
				
				formatmenu->actions[n].data = device;
				n++;
			}
			else
			{
				break;
			}
			
		}

	}
	
	formatmenu->items_nr = n;
	return 0;
}



