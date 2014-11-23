

/*
功能：打开文件
输入：file文件路径
返回：0--不成功 1--成功
*/
int txt_openfile(char *filename);
 
 
/*
功能：关闭
输入: 无
返回：无
*/
void txt_closefile();
 
/*
功能：从文件的偏移offset处获取长度为len的文本

输入：offset 文件偏移

      buf  存放字符

      len 欲获取的长度

输出：*len 实际获取的长度

返回：-1 到文件结尾

         0  正常
*/
int getText(unsigned long offset, char *buf,  int *len);

 

/*
功能：根据字符偏移获取字节偏移

输入：offset偏移，nChar字符个数

返回：字节偏移
*/
unsigned long getByteOffset(unsigned long offset, int nChar);

 

/*
功能：从offset处查找往前/后查找n个句子

输入：offset 为开始位置，n为个数（负数：往前找，正数：往后找）

返回：新句子的偏移， 为0xffffffff表示未找到
*/
unsigned long skipSentences(unsigned long offset, int n);

 

/*

功能：从offset处往前/后查找n个标题

输入：offset 为开始位置，n为个数（负数：往前找，正数：往后找）

返回：新位置的偏移， 为0xffffffff表示未找到

备注：该函数只要doc,docx提供
*/
unsigned long skipPara(unsigned long offset, int n);

unsigned long txtGetTotalSize(void);


