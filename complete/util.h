#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NRW 9			 // 关键字的最大数量
#define MAXNUMLEN 14	 // 数字的长度上限
#define NSYM 9			 // 运算符与界符的最大数量
#define MAXIDLEN 10		 // 标识符的长度上限
#define TXMAX 500		 // 标识符的最大数量
#define MAXLEVEL 3		 // 最大层次数
#define MAXINS 8		 // 指令（中间代码）的最大数量
#define CXMAX 500		 // 汇编代码的数量上限
#define MAXADDRESS 32767 // maximum address
#define STACKSIZE 200	 // 栈大小

char ch = ' '; // 最后一个读取到的字符
FILE *fp;	   // 输入文件指针
FILE *fw;      // 输出文件指针

// 以下三个全局变量用来记录当前读取到的单词，构造符号表
// 在getsym()中被赋值
int sym;			   // 最后一个读取到的symbol
char id[MAXIDLEN + 1]; // 最后一个读取到的标识符
int num;			   // 最后一个读取到的数字（准确地说是正在读取的数字，数字从高位到低位逐位读取）

// 以下变量用于实现报错功能
// 在getch()中使用
int cc = 0;	   // 当前已读的字符个数character count
int ll = 0;	   // 当前行的总长度line length
int lc = 1;	   // 当前行数line count
int err = 0;   // 已找到的错误数
char line[80]; // 输入缓冲，存放一行

// 定义类别码（symbol）
enum symtype
{
	SYM_NULL,SYM_IDENTIFIER,SYM_NUMBER,
	SYM_PLUS,SYM_MINUS,SYM_TIMES,SYM_SLASH,//四则运算
	SYM_EQU,SYM_NEQ,SYM_LES,SYM_LEQ,SYM_GTR,SYM_GEQ,//大小比较
	SYM_LPAREN,SYM_RPAREN,SYM_COMMA,SYM_SEMICOLON,SYM_BECOMES,
	SYM_BEGIN,SYM_END,SYM_IF,SYM_THEN,SYM_WHILE,SYM_DO,
	SYM_CONST,SYM_VAR,SYM_PROGRAM
};

// 定义关键字
char *word[NRW + 1] = {
	"", //第0位存放空值 
	"BEGIN", "CONST", "DO", "END", "IF",
	 "THEN", "VAR", "WHILE","PROGRAM"};
// 关键字对应的类别码
int wsym[NRW + 1] = {
	SYM_NULL, SYM_BEGIN, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_THEN, SYM_VAR, SYM_WHILE,SYM_PROGRAM};

// 定义运算符和界符
char csym[NSYM + 1] = {
	' ', '+', '-', '*', '/', '(', ')', '=', ',',  ';'};
// 运算符和界符对应的类别码
int ssym[NSYM + 1] = { //
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_SEMICOLON};

// 定义标识符的类别码，这个是在符号表的kind中使用的码
enum idtype
{
	ID_CONSTANT,
	ID_VARIABLE
};

// PL/0语言的目标代码的指令
char *mnemonic[MAXINS] = {"LIT", "OPR", "LOD", "STO", "INT", "JMP", "JPC"};
// 指令的功能码
enum opcode
{
	LIT,OPR,LOD,STO,INT,JMP,JPC
};//常数放到栈顶、运算、加载变量、存储结果、初始化、无条件跳转、有条件跳转
// 指令的操作码
enum oprcode
{
	OPR_RET,OPR_NEG,
	OPR_ADD,OPR_MIN,OPR_MUL,OPR_DIV,//基本运算
	OPR_EQU,OPR_NEQ,OPR_LES,OPR_LEQ,OPR_GTR,OPR_GEQ//大小比较
};

// 对常量声明语句的信息进行存储
typedef struct//常量是全局的
{
	char name[MAXIDLEN + 1];
	int kind;
	int value;
} comtab;

// 对变量声明语句的信息进行存储
typedef struct//变量有内外层之分
{
	char name[MAXIDLEN + 1]; // 变量名
	int kind;	   // 变量
	short level;   // 嵌套级别/层次差
	short address; // 存储位置的相对地址
} mask;
comtab table[TXMAX]; // 符号表table[500]，存储常量、变量

// 目标代码code将会对执行语句的信息进行转换和存储
typedef struct
{
	int f; // 指令的功能码
	int l; // 层次差
	int a; // 存储位置的相对地址
	char name[MAXIDLEN + 1];
} instruction;
instruction code[CXMAX];                 

int level = 0; // 嵌套级别。函数可以嵌套，主程序是0层，在主程序中定义的过程是1层，最多三层
int cx = 0;	   // 下一条指令的地址
int tx = 0;	   // 符号表table的索引
int dx = 0;	   // data allocation index

typedef struct{
	int kind;//常数或中间变量
	int value;
}arg;//存放两个操作数arg1、arg2
arg args[3];

int temp[100];//把中间变量的下标映射到线性序列

//报错信息
char *err_msg[] =
	{
		/*  0 */ "Fatal Error:Unknown character.\n",
		/*  1 */ "Found ':=' when expecting '='.",
		/*  2 */ "There must be a number to follow '='.",
		/*  3 */ "There must be an '=' to follow the identifier.",
		/*  4 */ "There must be an identifier to follow 'const', 'var', or 'procedure'.",
		/*  5 */ "Missing ',' or ';'.",
		/*  6 */ "",
		/*  7 */ "",
		/*  8 */ "",
		/*  9 */ "Illegal character after 'END'",
		/* 10 */ "';' expected.",
		/* 11 */ "Undeclared identifier.",
		/* 12 */ "Illegal assignment.",
		/* 13 */ "':=' expected.",
		/* 14 */ "There must be an identifier to follow the 'call'.",
		/* 15 */ "A constant or variable can not be called.",
		/* 16 */ "'then' expected.",
		/* 17 */ "';' or 'end' expected.",
		/* 18 */ "'do' expected.",
		/* 19 */ "Incorrect symbol.",
		/* 20 */ "Relative operators expected.",
		/* 21 */ "Procedure identifier can not be in an expression.",
		/* 22 */ "Missing ')'.",
		/* 23 */ "The symbol can not be followed by a factor.",
		/* 24 */ "The symbol can not be as the beginning of an expression.",
		/* 25 */ "The number is too great.",
		/* 26 */ "The identifier is too long",
		/* 27 */ "The program must have a name",
		/* 28 */ "",
		/* 29 */ "",
		/* 30 */ "",
		/* 31 */ "",
		/* 32 */ "There are too many levels."};
