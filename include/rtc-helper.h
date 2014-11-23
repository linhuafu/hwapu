#ifndef __RTC_HELPER_H
#define __RTC_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
	int sec;
	int min;
	int hour;
	int day;
	int mon;
	int year;
	int wday;
	time_t raw;
} sys_time_t;

void sys_get_time(sys_time_t *);
int sys_set_time(const sys_time_t *);
int sys_set_wakealarm(const sys_time_t *);

/* Subtract the `struct timeval' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */
int
timeval_subtract (struct timeval *result,
	struct timeval *x, struct timeval *y);

#endif
