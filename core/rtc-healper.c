#include <sys/time.h>
#include "rtc-helper.h"
#include "sysfs-helper.h"

void sys_get_time(sys_time_t *stm)
{
	time_t timep;
	struct tm *p;

	time(&timep);
	p = localtime(&timep);

	stm->sec  = p->tm_sec;
	stm->min  = p->tm_min;
	stm->hour = p->tm_hour;
	stm->day  = p->tm_mday;
	stm->mon  = p->tm_mon + 1;
	stm->year = p->tm_year + 1900;
	stm->wday = p->tm_wday;
	stm->raw  = timep;
	//added by km
	if(stm->year == 2038
		&& stm->min == 0
		&& stm->hour == 0
		&& stm->day == 1
		&& stm->mon == 1){
		stm->year = 2000;
		stm->wday = 6;
		sys_set_time(stm);
		system("hwclock -w");
	}//added end
}

int sys_set_time(const sys_time_t *stm)
{
	struct tm tm;
	time_t timep;
	struct timezone tz;
	struct timeval tv;

	tm.tm_sec  = stm->sec;
	tm.tm_min  = stm->min;
	tm.tm_hour = stm->hour;
	tm.tm_mday = stm->day;
	tm.tm_mon  = stm->mon - 1;
	tm.tm_year = stm->year - 1900;

	timep = mktime(&tm);
	gettimeofday(&tv, &tz);
	tv.tv_sec = timep;
	return settimeofday(&tv, &tz);
}

int sys_set_wakealarm(const sys_time_t *stm)
{
	struct tm tm;
	time_t timep;

	tm.tm_sec  = stm->sec;
	tm.tm_min  = stm->min;
	tm.tm_hour = stm->hour;
	tm.tm_mday = stm->day;
	tm.tm_mon  = stm->mon - 1;
	tm.tm_year = stm->year - 1900;

	timep = mktime(&tm);

	return sysfs_write_integer(PLEXTAL_RTC_WAKEALARM, timep);
}

/* Subtract the `struct timeval' values X and Y,
storing the result in RESULT.
Return 1 if the difference is negative, otherwise 0. */
int
timeval_subtract (struct timeval *result,
	struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating @var{y}. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}
