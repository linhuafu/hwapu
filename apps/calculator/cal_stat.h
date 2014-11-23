#ifndef __CAL_STAT_H__#define __CAL_STAT_H__#define CAL_STR_MAX_SIZE 128#define UI_SHOW_MAX_SIZE	(13 * 4 - 2)			//ui can show only 19 words 4 line#define CAL_STR_MAX_LINES	4enum roll {

	ROLL_UP = 0,
	ROLL_DOWN,
};struct cal_expression {	char cal_str[CAL_STR_MAX_SIZE];	int cal_str_p;	int choice;	double resault;	int show_res_flag;	int error_flag;	int bits;	char res_buf[32];		//for store the resault	int res_tts_flag;		//if the res need to tts again};extern struct cal_expression* const express;void cal_str_init(void);
void cal_str_update(int roll);void cal_str_back (void);
void cal_str_git(void);
int cal_result_state (void);#endif
