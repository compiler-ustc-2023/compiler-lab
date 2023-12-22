#include <stdio.h>

#define NRW 16       // 关键字的总数量
#define TXMAX 500    // length of identifier table
#define MAXNUMLEN 14 // maximum number of digits in numbers
#define NSYM 13      // maximum number of symbols in array ssym and csym,需要更改最大符号数数组括号
#define MAXIDLEN 10  // 变量名最大长度

#define MAXADDRESS 32767 // maximum address
#define MAXLEVEL 32      // maximum depth of nesting block
#define CXMAX 500        // size of code array

// add by Lin
#define ARRAY_DIM 3 // 数组最大维度

#define MAXSYM 30 // maximum number of symbols

#define STACKSIZE 1000 // maximum storage

// 没法使用给定汇编言实现，只能用虚拟的代码地址来表征内置函数的地址 by wu
#define PRINT_ADDR -1
#define RANDOM_ADDR -2
#define CALLSTACK_ADDR -3
enum symtype // 当前读到字符(串的类型)
{
    SYM_NULL,       // 空
    SYM_IDENTIFIER, // 变量或常量之类的
    SYM_NUMBER,     // 数字
    SYM_PLUS,       //+
    SYM_MINUS,      //-
    SYM_TIMES,      //*
    SYM_SLASH,      // /
    SYM_ODD,        // 关键字odd
    SYM_EQU,        // =
    SYM_NEQ,        // 不等于<>
    SYM_LES,        //<
    SYM_LEQ,        //<=
    SYM_GTR,        //>
    SYM_GEQ,        //>=
    SYM_LPAREN,     //(
    SYM_RPAREN,     //)
    SYM_COMMA,      //,
    SYM_SEMICOLON,  //;
    SYM_PERIOD,     //.
    SYM_BECOMES,    // 赋值号:=
    SYM_SCOPE,      // 作用域算符'::',by wu
    SYM_BEGIN,      // 关键字begin
    SYM_END,        // 关键字end
    SYM_IF,         // 关键字if
    SYM_THEN,       // 关键字then
    SYM_WHILE,      // 关键字while
    SYM_DO,         // 关键字do
    SYM_CALL,       // 关键字call
    SYM_CONST,      // 关键字const
    SYM_VAR,        // 关键字var
    SYM_PROCEDURE,  // 关键字procedure
    // 补充数组的左右括号以及取地址符,by Lin
    SYM_LEFTBRACKET,
    SYM_RIGHTBRACKET,
    SYM_ADDRESS,
    // 新增关键字print,random,CALLSTACK,by wu
    SYM_PRINT,
    SYM_RANDOM,
    SYM_CALLSTACK,
    // 新增关键字break,continue, by tian
    SYM_BREAK,
    SYM_CONTINUE,
    // 补充行注释符 by wdy
    SYM_LINECOMMENT,       // //
    SYM_LEFTBLOCKCOMMENT,  // /*
    SYM_RIGHTBLOCKCOMMENT, // */
    // 补充左右移运算符
    SYM_SHL, // <<
    SYM_SHR, // >>
    // 补充逻辑运算符, by wy
    SYM_AND, // &&
    SYM_OR,  // ||
    SYM_NOT  // !
};

enum idtype
{
    ID_CONSTANT,
    ID_VARIABLE,
    ID_PROCEDURE,
    // 加入指针和数组类型，by Lin
    ID_POINTER,
    ID_ARRAY
};

// 新增指令，modified by Lin
enum opcode
{
    LIT,
    OPR,
    LOD,
    STO,
    CAL,
    INT,
    JMP,
    JPC,
    STOA,
    LODA,
    LEA,
    // 新增指令，by wy
    JZ,
    JNZ
};

enum oprcode
{
    OPR_RET,
    OPR_NEG,
    OPR_ADD,
    OPR_MIN,
    OPR_MUL,
    OPR_DIV,
    OPR_ODD,
    OPR_EQU,
    OPR_NEQ,
    OPR_LES,
    OPR_LEQ,
    OPR_GTR,
    OPR_GEQ,
    // 补充左右移运算符 by wdy
    OPR_SHL,
    OPR_SHR,
    // 补充逻辑运算符, by wy
    OPR_AND,
    OPR_OR,
    OPR_NOT
};

typedef struct
{
    int f; // function code
    int l; // level
    int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char *err_msg[] =
    {
        /*  0 */ "",
        /*  1 */ "Found ':=' when expecting '='.",
        /*  2 */ "There must be a number to follow '='.",
        /*  3 */ "There must be an '=' to follow the identifier.",
        /*  4 */ "There must be an identifier to follow 'const', 'var', or 'procedure'.",
        /*  5 */ "Missing ',' or ';'.",
        /*  6 */ "Incorrect procedure name.",
        /*  7 */ "Statement expected.",
        /*  8 */ "Follow the statement is an incorrect symbol.",
        /*  9 */ "'.' expected.",
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
        /* 26 */ "Missing '('",
        /* 27 */ "Missing ','",
        /* 28 */ "Incorrect pc",
        /* 29 */ "Incorrect paramlist",
        /* 30 */ "variable or procedure expected",
        /* 31 */ "Nested comments", // 嵌套注释 add by wdy
        /* 32 */ "There are too many levels.",
        /* 33 */ "break statement not within a loop",
        /* 34 */ "continue statement not within a loop"};

//////////////////////////////////////////////////////////////////////
char ch;               // 最后一次读到的字符
int sym;               //	记录最近一次读取到的子串是什么类型，如变量名SYM_IDENTIFIER或关键字sym = wsym[i]或常数SYM_NUMBER等
char id[MAXIDLEN + 1]; // 最近一次读取的变量名(也可能是关键字)
int num;               // 最近一次读到的数
int cc;                // 记录读取到当前行的哪个字符，便于getch字符获取
int ll;                // 当前解析指令所在行的长度
int kk;
int err;
int cx;                     // 指令数组的当前下标，标记最新要生成指令的位置
int level = 0;              // 当前层次
int start_level = MAXLEVEL; // 搜索符号表时只返回层次小于等于start_level的变量 by wu
int tx = 0;                 // 当前变量表的下标

char line[80]; // 当前解析指令行，长度为ll，以空格作为结束标记

int sign_logic_and = 0; // 逻辑与的标记，用于逻辑与的短路 by wy
int sign_logic_or = 0;  // 逻辑或的标记，用于逻辑或的短路 by wy
int sign_condition = 0; // add by wy
int cx_logic_and[100], cx_logic_or[100];

int loop_level = 0;
int break_mark[MAXLEVEL][CXMAX];
int continue_mark[MAXLEVEL][CXMAX];
// 当前所处的循环层次 以及break/continue的位置标记，用于break/continue对应的pl0代码的回填 by tian

instruction code[CXMAX];

char *word[NRW + 1] = // word中记录了各种关键字，预留了word[0]来存储当前变量名(便于语法分析匹配串)
    {
        "", /* place holder */
        "begin", "call", "const", "do", "end", "if",
        "odd", "procedure", "then", "var", "while",
        // 新增关键字print,random,CALLSTACK,by wu
        "print", "random", "CALLSTACK",
        // 新增关键字break,continue by Tian
        "break", "continue"};

int wsym[NRW + 1] =
    {
        SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
        SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
        // 新增关键字print,random,CALLSTACK,by wu
        SYM_PRINT, SYM_RANDOM, SYM_CALLSTACK,
        // 新增关键字break,continue by Tian
        SYM_BREAK, SYM_CONTINUE};

int ssym[NSYM + 1] =
    {
        SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
        SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,
        // 补充数组的左右括号以及取地址,by Lin
        SYM_LEFTBRACKET,
        SYM_RIGHTBRACKET,
        SYM_ADDRESS};

char csym[NSYM + 1] =
    {
        ' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';',
        // 补充数组的左右括号,by Lin
        // 加入取地址符
        '[', ']', '&'};

// 新增指令STOA,将栈顶的值存在次栈顶指向的地址中
// 新增指令LODA,将栈顶指向的值取出来替换掉当前栈顶
// 新增指令LEA,取变量地址于栈顶
// modified by Lin
#define MAXINS 13
char *mnemonic[MAXINS] =
    {
        "LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC", "STOA", "LODA", "LEA", "JZ", "JNZ"}; // 新增JZ,JNZ指令,by wy

typedef struct
{
    char name[MAXIDLEN + 1];
    int kind;
    int value;
    // 数组每一维的维度，最高为3维，add by Lin
    int dimension[3];
    int depth; // 指针深度
} comtab;      // 常量

comtab table[TXMAX]; // 变量表,常量包括name，kind和value，变量和函数包括name,kind,level和address

typedef struct
{
    char name[MAXIDLEN + 1];
    int kind;
    short level;
    short address;
    // 数组每一维的维度，最高为3维，add by Lin
    int dimension[3];
    int depth; // 指针深度
} mask;        // 变量或函数，和常量共用存储空间，将value的位置用来存储层次level和地址address

FILE *infile;

// EOF PL0.h
