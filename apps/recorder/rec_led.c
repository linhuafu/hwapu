#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "rec_led.h"

void 
led_ctrl (int color, int blink)
{
	switch (color) {

		case LED_GREEN:{
			if (blink) 
				printf("No need to led green blink.\n");
			else {
				system("echo 0 > /sys/class/leds/led:red/brightness; echo 0 > /sys/class/leds/led:green/brightness");
				system("echo 100 > /sys/class/leds/led:green/brightness");
			}
		}
		break;

		case LED_ORANGE:{
			if (blink) {
				//那这里，前面要加个熄灭的动作，不然有问题
				system("echo 0 > /sys/class/leds/led:red/brightness; echo 0 > /sys/class/leds/led:green/brightness");
				system ("echo timer > /sys/class/leds/led:red/trigger ; echo timer > /sys/class/leds/led:green/trigger");
				system ("echo 300 > /sys/class/leds/led:red/delay_on ; echo 300 > /sys/class/leds/led:green/delay_on");

			} else {
				system("echo 0 > /sys/class/leds/led:red/brightness; echo 0 > /sys/class/leds/led:green/brightness");
				system("echo 100 > /sys/class/leds/led:red/brightness; echo 100 > /sys/class/leds/led:green/brightness");
			}
		}
		break;

		case LED_OFF: {
			system("echo 0 > /sys/class/leds/led:red/brightness; echo 0 > /sys/class/leds/led:green/brightness");
		}	
		break;
	}
}
