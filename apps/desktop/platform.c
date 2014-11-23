/*
 *  Copyright(C) 2007 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Power management routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-04-24 EY
 *
 */

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "platform.h"
#include "sysfs-helper.h"

void
NhelperPowerOff(void)
{
	NhelperDisplay(0);

	if (system("poweroff") == 0)
		exit(0);
	else
		DBGMSG("Can't Power Off!\n");
}

void
NhelperRestart(void)
{
	if (system("reboot") == 0)
		exit(0);
	else
		DBGMSG("Can't Reboot!\n");
}

void NhelperSuspend(void)
{
	if (sysfs_write(PLEXTALK_POWER_STATE, "mem"))
		DBGMSG("Can't Suspend!\n");
}


//ztz	--start--

int DisplayOff = 0;

void NhelperDisplay(int on_off)
{
	if (on_off) {
		sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 0);	
		sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 0);
	} else {
		sysfs_write_integer("/sys/class/backlight/pwm-backlight/bl_power", 4);
		sysfs_write_integer("/sys/class/lcd/jz-lcm/lcd_power", 4);	
	}

	DisplayOff = !on_off ? 1 : 0;
}
//ztz	--end--
