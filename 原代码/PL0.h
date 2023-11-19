#include <stdio.h>

#define NRW        11     // 关键字的总数量
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       10     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // 变量名最大长度

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  1000   // maximum storage

enum symtype			//当前读到字符(串的类型)
{
	SYM_NULL,			//空
	SYM_IDENTIFIER,		//变量
	SYM_NUMBER,			//数字
	SYM_PLUS,			//+
	SYM_MINUS,			//-
	SYM_TIMES,			//*
	SYM_SLASH,			// /
	SYM_ODD,			//关键字odd
	SYM_EQU,			// =
	SYM_NEQ,			//不等于<>
	SYM_LES,			//<
	SYM_LEQ,			//<=
	SYM_GTR,			//>
	SYM_GEQ,			//>=
	SYM_LPAREN,			//(
	SYM_RPAREN,			//)
	SYM_COMMA,			//,
	SYM_SEMICOLON,		//;
	SYM_PERIOD,			//.
	SYM_BECOMES,		//赋值号:=
    SYM_BEGIN,			//关键字begin
	SYM_END,			//关键字end
	SYM_IF,				//关键字if
	SYM_THEN,			//关键字then
	SYM_WHILE,			//关键字while
	SYM_DO,				//关键字do
	SYM_CALL,			//关键字call
	SYM_CONST,			//关键字const
	SYM_VAR,			//关键字var
	SYM_PROCEDURE		//关键字procedure
};

enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE
};

enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels."
};

//////////////////////////////////////////////////////////////////////
char ch;         // 最后一次读到的字符
int  sym;        //	记录最近一次读取到的子串是什么类型，如变量名SYM_IDENTIFIER或关键字sym = wsym[i]或常数SYM_NUMBER等
char id[MAXIDLEN + 1]; //最近一次读取的变量名(也可能是关键字)
int  num;        // 最近一次读到的数
int  cc;         // 记录读取到当前行的哪个字符，便于getch字符获取
int  ll;         // 当前解析指令所在行的长度
int  kk;
int  err;
int  cx;         // 指令数组的当前下标，标记最新要生成指令的位置
int  level = 0;	//当前层次
int  tx = 0;	//当前变量表的下标

char line[80];	//当前解析指令行，长度为ll，以空格作为结束标记

instruction code[CXMAX];

char* word[NRW + 1] =	//word中记录了各种关键字，预留了word[0]来存储当前变量名(便于语法分析匹配串)
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while"
};

int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE
};

int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
};

#define MAXINS   8
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"
};

typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;		//常量

comtab table[TXMAX];	//变量表,常量包括name，kind和value，变量和函数包括name,kind,level和address

typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;			//变量或函数，和常量共用存储空间，将value的位置用来存储层次level和地址address

FILE* infile;

// EOF PL0.h
