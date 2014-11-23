#ifndef __REC_LED_H__
#define __REC_LED_H__

#define LED_GREEN   0x01
#define LED_RED 	0x02
#define LED_ORANGE	0x03
#define LED_OFF	    0x04

void led_ctrl (int color, int blink);

#endif
