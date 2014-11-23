#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "cal_box.h"


#define DEBUG_PRINT 1
#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif

#define _(S) gettext(S)

/*this check is check for if there is point in last numbers*/
int
check_have_point  (const char* express)
{
	const char* end = express + strlen(express);
	
	char* p = strrchr(express, '.');

	if (!p) {
		printf("no . in strrchr.\n");
		return 0;
	}
	
	while (p < end) {
		p ++;

		if (OPERATOR(*p)) {
			printf("no point in\n");
			return 0;
		}
	}
	printf("have point in.\n");

	return 1;
}


static int 
bits_count(const char* s)
{
	int size = strlen(s);
	
	char* p = strrchr(s, '.');

	if (!p)
		return size;
	else
		return size - 1;
}


int
get_express_bits (const char* express)
{
	int len = strlen(express);

	const char* p = express + len;

	const char* start_place = NULL;

	while (1) {		
		if (OPERATOR(*p)) {
			start_place = p + 1;
			break;
		}
		
		p --;

		if (p <= express) {
			start_place = express;
			break;
		}
	}

	return bits_count(start_place);
}


int check_first_num (const char* express)
{
	const char* p = express;
	const char* end = express + strlen(express) - 1;	//pay a attention for the char* end
	while (p < end) {

		if(OPERATOR(*p))
			return 0;

		p ++;
	}
	printf("This before is the firset num\n");

	return 1;
}


//how to handler for gettext in different, strlen() may be some problem
static void voice_fix (char* buf)
{
	int len = strlen(buf);
	char g_buf[32];				//this buf maybe used			
	
	memset (g_buf, 0, 32);

	switch (*(buf + len -1)) {

		case '*':
			strcpy(buf + len - 1, _("multiply"));
			break;

		case '/':
			strcpy(buf + len - 1, _("divide")); 
			break;
		case '@':
			strcpy(buf + len - 1, _(" minus")); 
			break;	
		case 'C':
			strcpy(buf + len -1 , _("clear"));
			break;

		default:
			break;
	}
	
}


static  int voice_prefix (char *destbuf,char* buf)
{
	
	int len;
	if(!buf || !destbuf)
		return 0;
	DBGMSG("voice_prefix buf:%s\n",buf);
	len = strlen(buf);
	if(len <= 0)
		return 0;

	if(*buf == '@') {
		snprintf(destbuf,100,"%s%s",_(" minus"),buf+1);
		return 1;
	}
	return 0;
	
}


char* get_all_calculations(const char* express)
{
	char *pBuf;
	char *pStart;
	char *pEnd;
	int cnt = 0;
	if (express==NULL || !(*express))
		return NULL;
	DBGMSG("get_all_calculations1 :%s\n",express);

	pBuf = (char*)malloc(1024);
	memset(pBuf, 0, 1024);
	pStart = express;
	pEnd = express + strlen(express) - 1;


	while(cnt < 1024 && pStart<=pEnd)
	{
		if (SPECIAL_VOICE(*pStart)){
			
			switch(*pStart) {
				case '*':
					strcpy(pBuf + cnt, _("multiply"));
					cnt += strlen(_("multiply"));
					break;
				case '/':
					strcpy(pBuf + cnt, _("divide")); 
					cnt += strlen(_("divide"));
					break;
				case '@':
					strcpy(pBuf + cnt, _(" minus")); 
					cnt += strlen(_(" minus"));
					break;	
				case 'C':
					strcpy(pBuf + cnt , _("clear"));
					cnt += strlen(_("clear"));
					break;
				case '=':
					strcpy(pBuf + cnt , _("equals"));
					cnt += strlen(_("equals"));
					break;
				case '+':
					strcpy(pBuf + cnt , _("plus"));
					cnt += strlen(_("plus"));
					break;	
				case '.':
					strcpy(pBuf + cnt , _("point"));
					cnt += strlen(_("point"));
					break;		
				case '-':
					if(pStart == express)
					{
						strcpy(pBuf + cnt, _(" minus")); 
						cnt += strlen(_(" minus"));
					}
					else
					{
						strcpy(pBuf + cnt , _("minus."));//substract
						cnt += strlen(_("minus."));//substract
					}

					break;			
			}
			
		} else {
			*(pBuf + cnt++) = *pStart;
		}
		pStart++;
		
	}
	DBGMSG("get_all_calculations :%s,cnt=%d\n",pBuf,cnt);
	return pBuf;
}

void free_calculations(char* pBuf)
{
	if(pBuf)
		free(pBuf);
}

	
char* get_prev_op_num (const char* express)
{
	if (!(*express))
		return NULL;		//说明是第一个

	DBGMSG("get_prev_op_num :%s\n",express);
	
	static char buf[100];
	static char retbuf[100];

	memset(buf, 0, 100);
	memset(retbuf, 0, 100);

	const char* p = express;
	const char* end = express + strlen(express) - 1 - 1;
	if( end-1>=p && *(end-1) == '@')
	{
		end--;
	}
	const char* i = end;

	while (i > p) {

		if (PREV_NUM_CHAR(*i)) {
			strcpy(buf, i + 1);
			voice_fix(buf);	
			if(voice_prefix(retbuf,buf)) {
				return retbuf;
			}
			return buf;
		}
		i --;
	}

	strcpy (buf, express);
	
	voice_fix(buf);
	if(voice_prefix(retbuf,buf)) {
		return retbuf;
	}

	return buf;
}


#define KEYLOCK  "/sys/devices/platform/gpio-keys/on_switches"

int 
keylock_locked (void)
{
	char res;
	
	int fd = open(KEYLOCK, O_RDONLY);		//这里是否要设置为非阻塞式读

	if (fd == -1) {
		printf("open key lock error!\n");
		return 0;
	}
	
	if ((read(fd, &res, 1)) != 1) {
		printf("read key_lock error!\n");
		close(fd);
		return 0;
	}

	close(fd);

	if (res == '0') {
		printf("[keypad]:locked\n");
		return 1;
	} else {
		printf("[keypad]:unlocked\n");
		return 0;
	}
}


