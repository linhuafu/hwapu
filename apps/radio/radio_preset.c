#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "radio_preset.h"
#include "radio_play.h"
#include "nc-err.h"
#include "plextalk-config.h"

#define RADIO_STATION_PATH		PLEXTALK_SETTING_DIR"station"
#define RADIO_TMP_PATH            "/tmp/.radio"

/* station structure */
struct _station {
	int no;
	double freq;
};

struct _preset_data {
	int preset_fd;
	int preset_no;						//current selected channels
	int preset_total;					//total nums of preset station
	struct _station station[MAX_PRE_NUM];
};

static struct _preset_data preset_data;


/*function : create preset station or read preset data*/
int
preset_init (void)
{	
	int res;

	preset_data.preset_fd = open(RADIO_STATION_PATH, O_RDWR);

	if (preset_data.preset_fd == -1) {
		preset_data.preset_fd    = open(RADIO_STATION_PATH, O_RDWR|O_CREAT|O_TRUNC, 666);
		if (preset_data.preset_fd != -1)
		{
			preset_data.preset_total = 0;

			res = write (preset_data.preset_fd, &preset_data.preset_total, 4);
			if (res != 4) {
				DBGMSG ("write preset_total = 0 into file error!\n");
				goto fail;
			}

			preset_data.preset_no    = 0;
			res = write (preset_data.preset_fd, &preset_data.preset_no, 4);
			if (res != 4) {
				DBGMSG("write preset no error!\n");
				goto fail;
			}
			
			memset (preset_data.station, 0, MAX_PRE_NUM * sizeof (struct _station));
			res = write (preset_data.preset_fd, 
				preset_data.station, 
				MAX_PRE_NUM * sizeof (struct _station));
			
			if (res != MAX_PRE_NUM * sizeof (struct _station)) {
				DBGMSG ("Write station[16] = 0 into file error!\n");
				goto fail;
			}
		}
		
	} else {
	
		res = read (preset_data.preset_fd, &preset_data.preset_total, 4);
		if (res != 4) {
			DBGMSG ("Read station total nums from file error!\n");
			goto fail;
		}

		res = read(preset_data.preset_fd, &preset_data.preset_no, 4);
		if (res != 4) {
			DBGMSG("Read preset_no error!\n");
			goto fail;
		}
		
		res = read(preset_data.preset_fd, 
			preset_data.station, 
			MAX_PRE_NUM * sizeof (struct _station));
		
		if (res != MAX_PRE_NUM * sizeof (struct _station)) {
			DBGMSG ("Read station[16] from file error!\n");
			goto fail;
		}
	}
	close(preset_data.preset_fd);
	preset_data.preset_fd = -1;
	
	return 0;

fail:
	if(preset_data.preset_fd != -1)
	{
		close(preset_data.preset_fd);
		preset_data.preset_fd = -1;
	}
	return -1;
}

void write_preset_tmppath(int bChanel)
{
	int res;
	int fd;

	fd = open(RADIO_TMP_PATH, O_RDWR);
	if (fd == -1) {
		fd  = open(RADIO_TMP_PATH, O_RDWR|O_CREAT|O_TRUNC, 666);
		
	} 
	res = write(fd, &preset_data.preset_total, 4);
	if (res != 4) {
		DBGMSG ("write preset_total = 0 into file error!\n");
		goto fail;
	}

	
	if(preset_data.preset_total > 0 && bChanel)
	{
		res = write (preset_data.preset_fd, &preset_data.station[preset_data.preset_no].no, 4);
		if (res != 4) {
			DBGMSG("write preset no error!\n");
			goto fail;
		}
		res = write (preset_data.preset_fd, &preset_data.station[preset_data.preset_no].freq, sizeof(double));
		if (res != sizeof(double)) {
			DBGMSG("write preset no error!\n");
			goto fail;
		}
	}
	close(fd);
	fd = -1;
fail:
	if(fd != -1)
	{
		close(fd);
		fd = -1;
	}
	
}


/* Set new frequency to new channels */
enum preset_ret 
set_preset_station(double new_freq)
{
	int i;

	if ( preset_data.preset_total >= MAX_PRE_NUM ) {
		DBGMSG ("Preset total have to max!\n");
		return PRESET_MAX_NUMS;
	}

	/* can not compare two float number with "=="
     * use sub to compare
	 */
	double ret;
	for (i = 0; i < preset_data.preset_total; i++) {
		ret = preset_data.station[i].freq - new_freq;

		if ((ret < 0.01) && (ret > -0.01)) {
			DBGMSG ("Find freq in preset station!\n");
			return PRESET_ALREADY;
		}
	}

	preset_data.station[preset_data.preset_total].no = preset_data.preset_total + 1;
	preset_data.station[preset_data.preset_total].freq = new_freq;
	preset_data.preset_total ++ ;
	preset_data.preset_no = preset_data.preset_total;

	preset_close();

	return PRESET_SUCCESS;
}


double 
get_preset_station(int orient)
{
	//station num is one bigger than station num

	//pay atttion for those with no station
	if (preset_data.preset_total == 0) {
		return 0;		
	}
	
	if (orient == NEXT_STATION) {
		preset_data.preset_no ++;		
		if (preset_data.preset_no > preset_data.preset_total) {
			preset_data.preset_no = 1;
		}
					
	} else {
		preset_data.preset_no --;
		if (preset_data.preset_no < 1) {
			preset_data.preset_no = preset_data.preset_total;	
		}
	}

	preset_close();

	return preset_data.station[preset_data.preset_no - 1].freq;
}


/* This must be called  when leave the radio function */
int 
preset_close (void)
{
	preset_data.preset_fd = open(RADIO_STATION_PATH, O_RDWR);
	if(preset_data.preset_fd == -1){
		DBGMSG ("Finish:file is not exsite\n");
		return -1;
	}
	lseek (preset_data.preset_fd, 0, SEEK_SET);
	int res = write (preset_data.preset_fd, &preset_data.preset_total, 4);
	if (res != 4) {
		DBGMSG ("Finish:Write preset_total to file failure!\n");
		goto fail;
	}

	res = write (preset_data.preset_fd, &preset_data.preset_no, 4);
	if (res != 4) {
		DBGMSG("[ERROR]:write preset no error!\n");
		goto fail;
	}
	
	res = write (preset_data.preset_fd, preset_data.station, 
		MAX_PRE_NUM * sizeof(struct _station));
	if (res != MAX_PRE_NUM * sizeof(struct _station)) {
		DBGMSG ("Finish:Write preset station to file failure!\n");
		goto fail;
	}

	close (preset_data.preset_fd);
	preset_data.preset_fd = -1;
	return 0;
fail:
	close (preset_data.preset_fd);
	preset_data.preset_fd = -1;
	return -1;
}


static int 
check_freq_preset (void)
{
	/* this preset_no's freq should be station[preset_no - 1]; */
	return preset_data.station[preset_data.preset_no - 1].freq == radio_get_freq();
}


/* Delete the current preset station */
int 
del_cur_preset_station (void)
{
	struct _station buf[16];

	if (preset_data.preset_total == 0) {
		DBGMSG ("This is not station to delete!\n");
		return -1;
	}

	if (!check_freq_preset ()) {
		DBGMSG ("This is not the preset station, can't delete!\n");
		return -1;
	}

	int copy_size = preset_data.preset_total - preset_data.preset_no;
		
	memset (buf, 0, MAX_PRE_NUM * sizeof(struct _station));
	memcpy (buf, &preset_data.station[preset_data.preset_no].no, copy_size * sizeof(struct _station));		
	//copy one more,for delet a station
	memcpy (&preset_data.station[preset_data.preset_no - 1].no,
		buf, (copy_size + 1) * sizeof(struct _station));
	
	preset_data.preset_total --;

	preset_close();

	return 0 ;
}


int
del_all_preset_station (void)
{
	preset_data.preset_total = 0;
	preset_data.preset_no = 0;
	memset (preset_data.station, 0, MAX_PRE_NUM * sizeof (struct _station));
	preset_close();
	return 0;
}


int 
get_cur_station_no (void)
{	
	return preset_data.preset_no;
}


int 
get_total_station_no (void)
{
	return preset_data.preset_total;
}


void recreate_station_file(void)
{
	char cmd[512];

	snprintf(cmd, 512, "rm -rf %s", RADIO_STATION_PATH);
	system(cmd);
	usleep(1000);

	preset_data.preset_fd    = open(RADIO_STATION_PATH, O_RDWR|O_CREAT|O_TRUNC, 666);

	preset_data.preset_total = 0;
	write (preset_data.preset_fd, &preset_data.preset_total, 4);

	preset_data.preset_no    = 0;
	write (preset_data.preset_fd, &preset_data.preset_no, 4);
			
	memset (preset_data.station, 0, MAX_PRE_NUM * sizeof (struct _station));
	write (preset_data.preset_fd, preset_data.station, 
		MAX_PRE_NUM * sizeof (struct _station));

	close(preset_data.preset_fd);
	preset_data.preset_fd = -1;	
}
