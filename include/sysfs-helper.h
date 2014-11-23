#ifndef __SYSFS_HELPER_H
#define __SYSFS_HELPER_H

#define PLEXTALK_POWER_STATE		"/sys/power/state"

/* power */
#define PLEXTALK_MAIN_BATT_PRESENT				"/sys/class/power_supply/main-battery/present"
#define PLEXTALK_MAIN_BATT_STATUS				"/sys/class/power_supply/main-battery/status"
#define PLEXTALK_MAIN_BATT_VOLTAGE_NOW			"/sys/class/power_supply/main-battery/voltage_now"
#define PLEXTALK_MAIN_BATT_VOLTAGE_MIN_DESIGN	"/sys/class/power_supply/main-battery/voltage_min_design"
#define PLEXTALK_MAIN_BATT_VOLTAGE_MAX_DESIGN	"/sys/class/power_supply/main-battery/voltage_max_design"
#define PLEXTALK_MAIN_BATT_HEALTH				"/sys/class/power_supply/main-battery/health"		//add lhf

#define PLEXTALK_BACKUP_BATT_PRESENT			"/sys/class/power_supply/backup-battery/present"
#define PLEXTALK_BACKUP_BATT_STATUS				"/sys/class/power_supply/backup-battery/status"
#define PLEXTALK_BACKUP_BATT_VOLTAGE_NOW		"/sys/class/power_supply/backup-battery/voltage_now"
#define PLEXTALK_BACKUP_BATT_VOLTAGE_MIN_DESIGN	"/sys/class/power_supply/backup-battery/voltage_min_design"
#define PLEXTALK_BACKUP_BATT_VOLTAGE_MAX_DESIGN	"/sys/class/power_supply/backup-battery/voltage_max_design"

#define PLEXTALK_VBUS_PRESENT		"/sys/class/power_supply/usb/present"
#define PLEXTALK_AC_PRESENT			"/sys/class/power_supply/ac/present"

/* UDC */
#define PLEXTALK_UDC_ONLINE			"/sys/class/udc/musb-hdrc.0.auto/online"

/* LCD */
#define PLEXTALK_LCD_BRIGHTNESS		"/sys/class/backlight/pwm-backlight/brightness"

#define PLEXTALK_LCD_CONTRAST		"/sys/class/lcd/jz-lcm/contrast"
#define PLEXTALK_LCD_MAX_CONTRAST	"/sys/class/lcd/jz-lcm/max_contrast"

#define PLEXTALK_LCD_BLANK			"/sys/class/graphics/fb0/blank"

/* cpufreq */
#define PLEXTALK_SCALING_GOVERNOR	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define PLEXTALK_SCALING_SETSPEED	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"
#define PLEXTALK_SCALING_MAX_FREQ	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define PLEXTALK_SCALING_MIN_FREQ	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"
#define PLEXTALK_SCALING_CUR_FREQ	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

/* headphone jack */
#define PLEXTALK_HEADPHONE_JACK		"/sys/devices/platform/jz-codec/headphone_jack"

/* LED */
#define PLEXTALK_LED_RED_BRIGHTNESS		"/sys/class/leds/led:red/brightness"
#define PLEXTALK_LED_RED_TRIGGER		"/sys/class/leds/led:red/trigger"
#define PLEXTALK_LED_RED_DELAYON		"/sys/class/leds/led:red/delay_on"
#define PLEXTALK_LED_GREEN_BRIGHTNESS	"/sys/class/leds/led:green/brightness"
#define PLEXTALK_LED_GREEN_TRIGGER		"/sys/class/leds/led:green/trigger"
#define PLEXTALK_LED_GREEN_DELAYON		"/sys/class/leds/led:green/delay_on"

/* RTC */
#define PLEXTAL_RTC_WAKEALARM		"/sys/class/rtc/rtc0/wakealarm"

/* Auto Off */
#define PLEXTALK_AUTO_OFF_SYS_FILE		"/sys/power-helper/auto_poweroff_expires"
#define PLEXTALK_AUTO_OFF_ARRIVED		"/sys/power-helper/auto_poweroff_expired"

int sysfs_read(const char *path, char *buffer, size_t len);
int sysfs_write(const char *path, const char *val);

int sysfs_read_integer(const char *path, int *val);
int sysfs_write_integer(const char *path, int val);

#endif
