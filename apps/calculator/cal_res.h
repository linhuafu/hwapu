#ifndef __CAL_RESAULT_H__
#define __CAL_RESAULT_H__


/*if the str is not legal, the error_flag will be set to 1, 
 *otherwise error_flag will be set to 0 
 */
double get_cal_resault (char* str, int* error_flag);


double get_power_cal_resault (const char* str, int* error_flag);

#endif

