#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define DEBUG_PRINT 1
#define OSD_DBG_MSG
#include "nc-err.h"


#if DEBUG_PRINT
#define info_debug DBGMSG
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


#define OPERATOR(S) (((S) == '+') || ((S) == '-') || ((S) == '*') || ((S) == '/'))
#define ARRAY_SIZE(S)	(sizeof(S)/sizeof(S[0]))


static char  op_stack[64];
static int   op_stack_p;

static double num_stack[64];
static int   num_stack_p;

static void 
push_op (char value)
{
	if (value == 'x') {
		op_stack[op_stack_p] = '*';
	} else {
		op_stack[op_stack_p] = value;
	}
	op_stack_p ++ ;
}


static void 
push_num (double value)
{
	num_stack[num_stack_p] = value;
	num_stack_p ++;
}


static double 
my_atof (const char* str, int begin, int end)
{
	char buf[32];

	int copy_size = end - begin + 1;
		
	memset(buf, 0, 32);
	memcpy(buf, str + begin, copy_size);

	return atof(buf);
}


static double 
cal_handler (void)
{
	double res;//   = num_stack[0];
	int   num_i = 1;
	int   op_i  = 0;
	int   i;
	double temp;

//对优先级作处理，具体算法可查看<计算器列式计算没有优先级的问题修改.txt	>
	if((num_stack_p != op_stack_p) || (num_stack_p <= 2)){
		goto normalcal;
	}
	for(i=0;i<op_stack_p;i++){//不需要对数组进行清0之类的操作，
								//因为op_stack_p可以保证，
								//程序不会访问到本次有效数据之外的数据
		if(op_stack[i] == '*'){
			temp = num_stack[i] * num_stack[i+1];
		}else if(op_stack[i] == '/'){
			temp = num_stack[i] / num_stack[i+1];
		}else{
			continue;
		}
		num_stack[i] = 0;
		num_stack[i+1] = temp;
		if(i<1){
			op_stack[i] = '+';
		}else{
			op_stack[i] = op_stack[i-1];
		}
	}
//

normalcal:
	res   = num_stack[0];
	//count is smaller than num_stack_P
	while (num_i < num_stack_p) {
		
		switch (op_stack[op_i]) {
			
			case '+':
				res += num_stack[num_i];
				break;

			case '-':	
				res -= num_stack[num_i];
				break;

			case '*':
				res *= num_stack[num_i];
				break;

			case '/':
				res /= num_stack[num_i];
				break;
		}
		
		num_i ++;
		op_i  ++;
	}

	return res;	
}


/* it will return 1 if there is no div zero happens*/
static int 
check_div_zero(const char* expression)
{
#if 0//有问题
	int i;
	int size = strlen(expression);
	char ret;

	for (i = 0; i < size; i++) {

		if (strncmp(expression + i, "/0", 2) == 0) {
			
			if (i == size - 1){

				return 1; 
			} else if (i == size -2){				

				return 0;
			} else {
				ret = expression[i+2];
				if ((ret == '+' )|| (ret == '-') || (ret == '*') || (ret == '/')) {
					return 0;
				}
			} 
		}
	}

	return 1;
#else
	int i;
	int size = strlen(expression);
	char ret;
	char *p;
	float value;

	p = strstr(expression,"/0");
	while(p)
	{
		if(strlen(p) == 2)
		{
			return 0;//有除以0
		} 
		else if(strlen(p) > 2)
		{
			value = atof(p+2);
			if(value <1.0e-12 && value>-1.0e-12)
			{
				return 0;
			}

			p+=2;
			p = strstr(p,"/0");

		}
	}

	return 1;

#endif
}


/*it will return 1 if the bits is OK*/
static int 
check_ret_bits(double resault)
{
	double max = 1000000000000;
	double min = -1000000000000;

	if ((resault < max) && (resault > min))
		return 1;
	else 
		return 0;
}


//error_flag 返回值 1:代表除以0出错,2:代表上下溢出
double
get_cal_resault(char* express, int* error_flag)
{
	printf("In express: %s\n", express);

	double resault;

	char expression[128];
	int size;
	int i = 0;
	
	memset(expression, 0, 128);

	if ((*express) == '-' || (*express) == '@') 
	{
		memcpy(expression + 1, express, strlen(express));
		*expression = '0';
		*(expression+1) = '-';
	} 
	else
	{
		memcpy(expression, express, strlen(express));
	}
	
	if (!check_div_zero(expression)) {
		printf(" div zero!!\n");
		*error_flag = 1;
		return 0;
	}
	
	size = strlen(expression);

	op_stack_p  = 0;
	num_stack_p = 0;

	for (i = 0; i < size; i++) {
		if ((expression[i] >= 48 && expression[i] <= 57) 
			|| expression[i] == '.'
			|| expression[i] == '@') {

			int j = i;
			while (1) {
				if((expression[j] >= 48 && expression[j] <= 57) 
					|| expression[j]== '.'
					|| expression[j]== '@'){

					if(expression[j] == '@'){
						expression[j] = '-';
					}
 					j++;
				} else {
 			
					double x = my_atof(expression, i, j-1);
					push_num(x);

					i = j - 1;
					break;
				}
			}
		} else {
			push_op(expression[i]);
		}
	}

	for(i = 0;i<num_stack_p;i++)
	{
		printf("stack num:%f\n",num_stack[i]);
	}
	for(i = 0;i<op_stack_p;i++)
	{
		printf("stack opt:%x\n",op_stack[i]);
	}	

	resault = cal_handler();

	if (!check_ret_bits(resault)) {
		*error_flag = 2;
		return 0;
	}

	*error_flag = 0;
	return resault;
}



static double 
get_power_cal_num (const char* str)
{
	const char* beg = str;
	const char* end = str + strlen(str) - 1;

	const char* num_p = NULL;

	while (beg < end) {

		if (OPERATOR(*beg)) {
			num_p = beg - 1;
			break;
		} 

		beg ++;
	}
	return my_atof(str, 0, num_p - str);
}


static int
get_power_cal_index (const char* str)
{
	char* p = strchr(str, '=');
	const char* end = str + strlen(str) - 1;
	int n = end - p +1;
	int index = 0;
	
	if (*(p-1) == '*') {
		index = n + 1;
	} else if (*(p-1) == '/') {
		index = - (n);
	} 
	
	return index;
}


double 
get_power_cal_resault (const char* str, int* error_flag)
{
	double result;
	int i;
	char buf[128];

	if (str == NULL) {
		*error_flag = 1;

		return 0;
	}
		
	int len = strlen(str);
	memset(buf,0x00,128);
	for(i=0;i<len;i++)
	{
		if('@' == *(str+i))
		{
			buf[i] = '-';
		}
		else
		{
			buf[i] = *(str+i);
		}
	}
	
	double base = get_power_cal_num(buf);

	int index = get_power_cal_index(buf);

	printf("get_power_cal_resault str:%s\n",str);
	printf("get_power_cal_resault buf:%s\n",buf);
	printf("get_power_cal_resault base:%f\n",base);
	printf("get_power_cal_resault index:%f\n",index);
	
	if ((base == 0) && (index <= 0)) {
		*error_flag = 1;

		return 0;
	}

	result = pow(base,index);
	
	if (!check_ret_bits(result)) {
		*error_flag = 1;
		return 0;
	}

		
	*error_flag = 0;

	return result;
}

