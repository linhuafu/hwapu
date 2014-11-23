#ifndef __CAL_BOX_H__
#define __CAL_BOX_H__

#define OPERATOR(S) (((S) == '+') || ((S) == '-') || ((S) == '*') || ((S) == '/'))
#define SPECIAL_VOICE(S) ((OPERATOR(S)) || ((S) == '@') || ((S) == 'C') || ((S) == '.') || ((S) == '='))
#define PREV_NUM_CHAR(S) ((OPERATOR(S)) || ((S) == 'C'))

int check_have_point (const char* express);

int get_express_bits(const char* express);

int check_first_num (const char* express);

char* get_prev_op_num (const char* express);

int keylock_locked (void);

#endif
