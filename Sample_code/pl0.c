// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "PL0.h"
#include "set.c"

//////////////////////////////////////////////////////////////////////
//输出报错信息
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//////////////////////////////////////////////////////////////////////
/*
获取输入文件的下一个字符记录在ch中，并将字符记录入当前行line中，如果文件结束会自动退出程序
*/
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);		//打印当前行指令对应汇编代码的行号
		while ( (!feof(infile)) // added & modified by alex 01-02-09
			    && ((ch = getc(infile)) != '\n'))
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];
} // getch

//////////////////////////////////////////////////////////////////////
//获取当前下一个SYMBOL，将SYMBOL的各种信息保存到全局变量中
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];		//a记录当前变量名

	while (ch == ' '||ch == '\t')
		getch();

	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;			//将当前串读到a中
			getch();
		}
		while (isalpha(ch) || isdigit(ch));
		a[k] = 0;
		strcpy(id, a);
		word[0] = id;		//将当前变量名保存到关键字数组中
		i = NRW;
		while (strcmp(id, word[i--]));		//让word[i]定位到当前的关键字，如果i为0则为变量
		if (++i)
			sym = wsym[i]; //解析到当前串为关键字
		else
			sym = SYM_IDENTIFIER;   //解析到当前串为变量名
	}
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else if(ch == ':')		// ::, by wu
		{
			sym = SYM_SCOPE;
			getch();
		}
		else
		{
			sym = SYM_NULL;       // illegal?
		}
	}
	else if (ch == '>')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
        else if (ch == '>') {
			sym = SYM_SHR; // >> add by wdy
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
        else if (ch == '<') {
			sym = SYM_SHL; // << add by wdy
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
    // 添加  //  */  /*
	// 并在此处直接处理注释
    // add by wdy
	else if (ch == '/') {
		getch();
		if (ch == '/') {
			sym = SYM_LINECOMMENT; // 行注释
			// 行尾在本程序中是空格，定义在getch() line[++ll] = ' ';
			// 但是不能用 ch == ' ' 来判定行结尾，因为中间可能有插入空格
			// 一旦读完该行，直接返回，不需要做之后的判断
			// 如何表示读完行？可以参考 SEMICOLON 的处理，获取下一个symbol即可
			while (cc < ll) { getch(); }
			getsym();
			return;
		} else if (ch == '*') {
			sym = SYM_LEFTBLOCKCOMMENT;			   // 左块注释
			while (sym != SYM_RIGHTBLOCKCOMMENT) { // 直到找到右块注释为止
				while (ch != '*') {
					if (ch == '/') {
						getch();
						if (ch == '*') {
							error(31); // 不能嵌套注释
						}
					} else
						getch();
				}
				getsym();
			}
			// 获取下一个 symbol，表示结束注释
			getsym();
			return;
		} else {
			sym = SYM_SLASH; // 否则就是除号
		}
	} else if (ch == '*') {
		getch();
		if (ch == '/') {
			sym = SYM_RIGHTBLOCKCOMMENT; // 右块注释
			getch();
		} else {
			sym = SYM_TIMES; // 否则就是乘号
		}
	}
    
    // 这里是处理可能会出现复合运算符的，所以'[' ']' '&'
	// 不需要在这里处理，会统一在 else 中处理 
    // by wdy
	// 添加左右方括号，add by Lin
	else if (ch == '['){
		sym = SYM_LEFTBRACKET;
		getch();
	}
	else if(ch == ']'){
		sym = SYM_RIGHTBRACKET;
		getch();
	}
	//添加取地址符号，add by Lin
	else if (ch == '&'){
		sym = SYM_ADDRESS;
		getch();
	}

	else
	{ // other tokens
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym

//////////////////////////////////////////////////////////////////////
//产生一条指令，x为操作码，y为层次，z为参数，具体见实验文档指令格式
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		printf("%d\n",sym);
		error(n);
		s = uniteset(s1, s2);
		while(! inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

//将一个变量加入到变量表中,modify by Lin
//加了声明数组和指针功能，其中将3个dimension写入变量表中
void enter(int kind, int* dimension, int depth)
{
	mask* mk;

	tx++;
	strcpy(table[tx].name, id);		//将最近一次读到的变量名加入到当前变量表中
	table[tx].kind = kind;			//写入变量类型
	switch (kind)
	{
	case ID_CONSTANT:				//常量，以comtab table的形式存储
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE:				//变量，以mask的形式存储
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		mk->dimension[0] = 0;		//代表不是数组
		mk->depth = 0;				//不是指针
		break;
	case ID_PROCEDURE:				//函数，以mask的形式存储
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->dimension[0] = 0;		//代表不是数组
		mk->depth = 0;				//不是指针
		break;

	//加入指针和数组case，add by Lin
	case ID_POINTER:				//指针，存储和普通变量相同，kind不同
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		mk->dimension[0] = 0;		//代表不是数组
		mk->depth = depth;			
		break;
	case ID_ARRAY:					//数组，存储和普通变量不同，kind不同
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx;
		mk->dimension[0] = dimension[0];
		mk->dimension[1] = dimension[1];
		mk->dimension[2] = dimension[2];
		mk->depth = depth;
		int space = 1;		//为数组分配的空间
		for(int i = 0;i<3;i++){
			if(dimension[i] == 0)
				break;
			else
			{
				space *= dimension[i];
			}
		}
		dx += space;
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
//在变量表中查找名为id的变量的下标，返回下标i，通过table[i]可以访问变量id的内容
int position(char* id, int start_level)				
{
	int i;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0 || ((mask*) &table[i])->level > start_level);	//如果在不同嵌套深度有同名变量优先搜索到当前嵌套深度的变量
	return i;
} // position

//////////////////////////////////////////////////////////////////////
void constdeclaration()		//往变量表中加入一个常量.
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
				getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT, NULL, 0);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	} else	error(4);

	 // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
void vardeclaration(void)			//往变量表中加入一个变量/数组/指针,modified by Lin
{
	int dimension[3] = {0,0,0};		//数组维度
	int i = 0, depth = 0;
	if(sym == SYM_TIMES)			//指针
	{
		while(sym == SYM_TIMES)
		{
			depth += 1;
			getsym();
		}
		if(sym == SYM_IDENTIFIER)
		{
			getsym();
			if(sym == SYM_LEFTBRACKET)		//数组声明
			{
				while(sym == SYM_LEFTBRACKET)
				{
					if(i == ARRAY_DIM)
					{
						error(50);			//数组维度过大
					}
					getsym();
					if(sym == SYM_NUMBER)
					{
						dimension[i++] = num;	//
						getsym();
						if(sym != SYM_RIGHTBRACKET)
							error(52);			//缺少']'
						getsym();
					}
					else error(51);			//数组方括号内为空
				}
				enter(ID_ARRAY,dimension,depth);		//数组元素
			}
			else		//普通指针
			{
				enter(ID_POINTER,NULL,depth);
			}
		}
		else error(4);
	}
	else if (sym == SYM_IDENTIFIER)	//变量
	{
		getsym();
		if(sym == SYM_LEFTBRACKET)		//数组声明
		{
			while(sym == SYM_LEFTBRACKET)
			{
				if(i == ARRAY_DIM)
				{
					error(50);			//数组维度过大
				}
				getsym();
				if(sym == SYM_NUMBER)
				{
					dimension[i++] = num;	//
					getsym();
					if(sym != SYM_RIGHTBRACKET)
						error(52);			//缺少']'
					getsym();
				}
				else error(51);			//数组方括号内为空
			}
			enter(ID_ARRAY,dimension,0);		//数组元素
		}
		else		//普通变量
		{
			enter(ID_VARIABLE, NULL, 0);
		}
	}
	else error(4);
} // vardeclaration

//////////////////////////////////////////////////////////////////////
//打印从from到to的所有指令
void listcode(int from, int to)
{
	int i;
	
	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////
void factor(symset fsys)			//生成因子
//新增语法,by Lin
//1:& ident，取变量地址
//2:ident[express][express]...		//取数组元素
//3:若干个* 接变量，取若干次地址
//4:若干个*加(表达式),					//计算表达式后取地址
{
	void shift(symset fsys);
	int i;	//最近读的关键字在变量表中的下标
	symset set;
	
	//test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an shift.
	while(sym == SYM_NULL)  			//除去空格
	{
		getsym();
	}

	//if (inset(sym, facbegsys))			//这个if没必要
	//{
	if (sym == SYM_IDENTIFIER)
	{
		if ((i = position(id,start_level)) == 0)
		{
			error(11); // Undeclared identifier.
		}
		else
		{
			switch (table[i].kind)
			{
				mask* mk;
			case ID_CONSTANT:
				getsym();
				gen(LIT, 0, table[i].value);
				break;
			case ID_VARIABLE:
				getsym();
				mk = (mask*) &table[i];
				gen(LOD, level - mk->level, mk->address);
				break;
			case ID_POINTER:
				getsym();
				mk = (mask*) &table[i];
				gen(LOD, level - mk->level, mk->address);
				break;
			case ID_PROCEDURE:
				getsym();
				if(sym == SYM_SCOPE){
					getsym();
					if(sym != SYM_IDENTIFIER)
						error(30);
					mk = (mask*) &table[i];		//子函数的符号表表项
					start_level = mk->level + 1;	//子函数本身的层次加一才能索引到子函数中的变量
					factor(fsys);
					start_level = MAXLEVEL;
				}
				// error(21); // Procedure identifier can not be in an shift.
				break;
			case ID_ARRAY:		//新增读取数组元素,by Lin
				getsym();
				mk = (mask*) &table[i];
				if(sym == SYM_LEFTBRACKET)			//调用了数组元素
				{
					while(sym == SYM_NULL)  			//除去空格
					{
						getsym();
					}
					gen(LEA, level - mk->level, mk->address);			//将数组基地址置于栈顶
					int dim = 0;						//当前维度
					while(sym == SYM_LEFTBRACKET)
					{
						if(table[i].dimension[dim++] == 0)
						{
							error(52);						//数组不具备该维度
						}
						else
						{	
							getsym();
							shift(fsys);
						}
						int space = 1;						//当前维度的数组空间
						for(int j = dim; j<ARRAY_DIM; j++)	//计算当前维的空间
						{
							if(table[i].dimension[j]!=0)
							{
								space *= table[i].dimension[j];
							}
							else break;
						}
						gen(LIT, 0, space);
						gen(OPR, 0, OPR_MUL);
						gen(OPR, 0, OPR_ADD);			//更新地址
						while(sym == SYM_NULL)  			//除去空格
						{
							getsym();
						}				
						if(sym != SYM_RIGHTBRACKET)
						{
							error(53);						//没有对应的']'
						}
						getsym();
					}
					gen(LODA, 0, 0);			//将数组对应元素置于栈顶
				}

				break;
			} // switch
		}
	}
	else if(sym == SYM_SCOPE){
		getsym();
		if(sym != SYM_IDENTIFIER)
			error(30);
		start_level = 0;	//索引到main函数层次中定义的变量
		factor(fsys);
		start_level = MAXLEVEL;
	}
	else if (sym == SYM_NUMBER)
	{
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		gen(LIT, 0, num);
		getsym();
	}
	else if (sym == SYM_LPAREN)
	{
		getsym();
		set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
		shift(set);
		destroyset(set);
		if (sym == SYM_RPAREN)
		{
			getsym();
		}
		else
		{
			error(22); // Missing ')'.
		}
	}
	else if(sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
	{  
			getsym();
			factor(fsys);
			gen(OPR, 0, OPR_NEG);
	}
	//新增语法,by Lin
	//1:& ident，取变量地址
	//2:ident[express][express]...		//取数组元素
	//3:若干个* 接变量/数组元素，取若干次地址
	//4:若干个*加(表达式),					//计算表达式后取地址
	//新增语法1
	else if(sym == SYM_ADDRESS)
	{
		getsym();
		while(sym == SYM_NULL)  			//除去空格
		{
			getsym();
		}
		if (sym == SYM_IDENTIFIER)
		{
			if ((i = position(id,start_level)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				mask* mk = (mask*) &table[i];
				gen(LEA, level - mk->level, mk->address);
			}
			getsym();
		}
		else
		{
			error(55);			//取地址符后没接变量
		}
	}

	//新增语法3和4
	else if(sym == SYM_TIMES)
	{
		int depth = 0;
		while(sym == SYM_TIMES)
		{
			depth ++;
			getsym();
		}
		while(sym == SYM_NULL)  			//除去空格
		{
			getsym();
		}
		if(sym == SYM_IDENTIFIER)			//多个*接变量
		{
			if ((i = position(id,start_level)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				switch (table[i].kind)
				{
					mask* mk;
				case ID_CONSTANT:
					error(57);				//不允许取常量指向的地址的内容
					break;
				case ID_VARIABLE:
					error(58);				//访问非指针变量指向的地址
				case ID_POINTER:
					mk = (mask*) &table[i];
					gen(LOD, level - mk->level, mk->address);
					getsym();
					break;
				case ID_PROCEDURE:
					error(21); // Procedure identifier can not be in an shift.
					break;
				case ID_ARRAY:		//新增读取数组元素,by Lin
					getsym();
					mk = (mask*) &table[i];
					if(sym == SYM_LEFTBRACKET)			//调用了数组元素
					{
						while(sym == SYM_NULL)  			//除去空格
						{
							getsym();
						}
						gen(LEA, level - mk->level, mk->address);			//将数组基地址置于栈顶
						int dim = 0;						//当前维度
						while(sym == SYM_LEFTBRACKET)
						{
							if(table[i].dimension[dim++] == 0)
							{
								error(52);						//数组不具备该维度
							}
							else
							{	
								getsym();
								shift(fsys);
							}
							int space = 1;						//当前维度的数组空间
							for(int j = dim; j<ARRAY_DIM; j++)	//计算当前维的空间
							{
								if(table[i].dimension[j]!=0)
								{
									space *= table[i].dimension[j];
								}
								else break;
							}
							gen(LIT, 0, space);
							gen(OPR, 0, OPR_MUL);
							gen(OPR, 0, OPR_ADD);			//更新地址
							while(sym == SYM_NULL)  			//除去空格
							{
								getsym();
							}				
							if(sym != SYM_RIGHTBRACKET)
							{
								error(53);						//没有对应的']'
							}
							getsym();
						}
						gen(LODA, 0, 0);			//将数组对应元素置于栈顶
					}

					break;
				} // switch
			}
		}
		else if (sym == SYM_LPAREN)					//多个*后面接表达式
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			shift(set);
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else 
			error(57);			//非法取地址操作
		while(depth > 0)			//进行若干次取地址运算
		{
			depth --;
			gen(LODA, 0, 0);
		}
	}


	//错误检测,新增元素赋值号及右方括号by Lin
	//将逗号和右括号补充进follow集 by wu
	symset set1 = createset(SYM_BECOMES,SYM_RIGHTBRACKET,SYM_COMMA,SYM_RPAREN);
	fsys = uniteset(fsys, set1);
	test(fsys, createset(SYM_LPAREN, SYM_NULL, SYM_LEFTBRACKET,SYM_BECOMES,SYM_COMMA), 23);
	//} // if
} // factor

//////////////////////////////////////////////////////////////////////
void term(symset fsys)		//生成项
{
	int mulop;
	symset set;
	
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH)
	{
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
void expression(symset fsys)			//生成表达式
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));
	
	term(set);
	while (sym == SYM_PLUS || sym == SYM_MINUS)
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_PLUS)
		{
			gen(OPR, 0, OPR_ADD);
		}
		else
		{
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
} // expression

void shift(symset fsys) {  // 生成移位表达式 by wdy
	int	   shiftop;
	symset set = uniteset(fsys, createset(SYM_SHL, SYM_SHR, SYM_NULL));
	expression(set);
	while (sym == SYM_SHL || sym == SYM_SHR) {
		shiftop = sym;
		getsym();
		expression(set);
		if (shiftop == SYM_SHL) {
			gen(OPR, 0, OPR_SHL);
		} else {
			gen(OPR, 0, OPR_SHR);
		}
	}
	destroyset(set);
}

//////////////////////////////////////////////////////////////////////
void condition(symset fsys)			//生成条件表达式
{
	int relop;
	symset set;

	if (sym == SYM_ODD)
	{
		getsym();
		shift(fsys);
		gen(OPR, 0, 6);
	}
	else
	{
		set = uniteset(relset, fsys);
		shift(set);
		destroyset(set);
		if (! inset(sym, relset))
		{
			error(20);
		}
		else
		{
			relop = sym;
			getsym();
			shift(fsys);
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition

//////////////////////////////////////////////////////////////////////
void statement(symset fsys)			//语句,加入了指针和数组的赋值，modified by Lin
{
	int i, cx1, cx2;
	symset set1, set;
	if (sym == SYM_IDENTIFIER)		//修改，加入数组的赋值，modified by Lin
	{ // variable assignment
		while(sym == SYM_NULL) getsym();
		mask* mk;
		if (! (i = position(id,start_level)))
		{
			error(11); // Undeclared identifier.
		}
		else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY && table[i].kind != ID_POINTER)
		{
			error(12); // Illegal assignment.
			i = 0;
		}
		getsym();
		mk = (mask*) &table[i];
		if (sym == SYM_BECOMES)
		{
			getsym();
			shift(fsys);
			if (i)
			{
				gen(STO, level - mk->level, mk->address);
			}
		}
		else if(sym == SYM_LEFTBRACKET)			//调用了数组元素
		{
			gen(LEA, level - mk->level, mk->address);			//将数组基地址置于栈顶
			int dim = 0;						//当前维度
			while(sym == SYM_LEFTBRACKET)
			{
				if(table[i].dimension[dim++] == 0)
				{
					error(52);						//数组不具备该维度
				}
				else
				{	
					getsym();
					shift(fsys);
				}
				int space = 1;						//当前维度的数组空间
				for(int j = dim; j<ARRAY_DIM; j++)	//计算当前维的空间
				{
					if(table[i].dimension[j]!=0)
					{
						space *= table[i].dimension[j];
					}
					else break;
				}
				gen(LIT, 0, space);
				gen(OPR, 0, OPR_MUL);
				gen(OPR, 0, OPR_ADD);			//更新地址				
				if(sym != SYM_RIGHTBRACKET)
				{
					error(53);						//没有对应的']'
				}
				getsym();
			}
			if (sym == SYM_BECOMES)
			{
				getsym();
			}
			else
			{
				error(13); // ':=' expected.
			}
			shift(fsys);
			gen(STOA, level - mk->level, mk->address);
		}
		else
		{
			error(13); // ':=' expected.
		}
	}
	else if (sym == SYM_TIMES)			//给指针指向内容赋值，add by Lin
	{
		getsym();
		shift(fsys);				//此时栈顶为一个地址
		if (sym == SYM_BECOMES)
		{
			getsym();
		}
		else
		{
			error(13); // ':=' expected.
		}
		shift(fsys);				//栈顶为一个表达式的值
		gen(STOA, 0, 0);
	}



	else if (sym == SYM_CALL)
	{ // procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (! (i = position(id,start_level)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind == ID_PROCEDURE)
			{
				mask* mk;
				mk = (mask*) &table[i];
				gen(CAL, level - mk->level, mk->address);
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	} 
	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}
		cx1 = cx;
		gen(JPC, 0, 0);
		statement(fsys);
		code[cx1].a = cx;	
	}
	else if (sym == SYM_BEGIN)
	{ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(10);
			}
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)
		{
			getsym();
		}
		else
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}
		loop_level ++; // 循环层次+1
		break_mark[loop_level][0] = 0;
		continue_mark[loop_level][0] = 0;
		statement(fsys);
		loop_level --; // 循环层次-1
		gen(JMP, 0, cx1);
		code[cx2].a = cx;
		for(int i = 1; i <= continue_mark[loop_level][0]; i++)
		    code[continue_mark[loop_level][i]].a = cx1;// continue的回填，jmp至循环开始的条件判断处
		for(int i = 1; i <= break_mark[loop_level][0]; i++)
		    code[break_mark[loop_level][i]].a = cx;// break的回填，jmp至循环结束后的下一行代码
	}
	else if(sym == SYM_BREAK) // break by tian
	{
	    if(loop_level == 0)
	    {
	        error(33);
	    }
	    else
	    {
	        break_mark[loop_level][++break_mark[loop_level][0]] = cx;
            gen(JMP, 0, 0);
	    }
	}
	else if(sym == SYM_CONTINUE) // continue by tian
	{
	    if(loop_level == 0)
        {
            error(34);
        }
        else
        {
            continue_mark[loop_level][++continue_mark[loop_level][0]] = cx;
            gen(JMP, 0, 0);
        }
	}
	else if(sym == SYM_PRINT)	//内置函数 print()的实现 by wu
	{
		getsym();
		if(sym != SYM_LPAREN){
			error(26);	//没找到左括号
		}
		getsym();			//跳过左括号
		if(sym == SYM_RPAREN){
			gen(LOD, 0, 0);			//无参数，打印换行符
			getsym();			//跳过右括号
		}
		else{
			int param_count = 0;
			shift(fsys);
			param_count += 1;
			while(sym != SYM_RPAREN){
				if(sym != SYM_COMMA){
					error(27);
					break;
				}
				else{
					getsym();
					shift(fsys);
					param_count += 1;
				}
			}
			getsym();
			gen(LIT, 0, param_count);
		}
		gen(CAL, 0, PRINT_ADDR);
	}
	else if(sym == SYM_CALLSTACK)		//内置函数 CALLSTACK的实现 by wu
	{
		//TODO
	}
	else if(sym == SYM_RANDOM)			////内置函数 random() 的实现 by wu
	{
		getsym();
		if(sym != SYM_LPAREN){
			error(26);	//没找到左括号
		}
		getsym();
		if(sym == SYM_RPAREN){
			gen(LOD, 0, 0);			//无参数，随机生成 0~100 的自然数
			getsym();			//跳过右括号
		}
		else{
			int param_count = 0;
			shift(fsys);
			param_count += 1;
			while(sym != SYM_RPAREN){
				if(sym != SYM_COMMA){
					error(27);
					break;
				}
				else{
					getsym();
					shift(fsys);
					param_count += 1;
					while(sym == SYM_NULL) getsym();
				}
			}
			if(param_count > 2)
				error(29);
			getsym();
			gen(LIT, 0, param_count);
		}
		gen(CAL, 0, RANDOM_ADDR);
	}
	//错误检测
	test(fsys, phi, 19);
} // statement
			
//////////////////////////////////////////////////////////////////////
void block(symset fsys)			//生成一个程序体
{
	int cx0; // initial code index
	mask* mk;
	int block_dx;
	int savedTx;
	symset set1, set;

	dx = 3;
	block_dx = dx;
	mk = (mask*) &table[tx];
	mk->address = cx;
	gen(JMP, 0, 0);			//产生第一条指令JMP 0, 0
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	do						//循环读完整个PL0代码产生汇编程序
	{
		if (sym == SYM_CONST)		//常量声明，无需产生指令
		{ // constant declarations
			getsym();
			do
			{
				constdeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);
		} // if

		if (sym == SYM_VAR)			//变量声明，无需产生指令
		{ // variable declarations
			getsym();
			do
			{
				vardeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);
		} // if
		block_dx = dx; //save dx before handling procedure call!
		while (sym == SYM_PROCEDURE)
		{ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE, NULL, 0);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}

			level++;
			savedTx = tx;
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);				//生成子程序体
			destroyset(set1);		
			destroyset(set);
			tx = savedTx;			//子程序分析完后将子程序的变量从符号表中“删除”
			level--;

			if (sym == SYM_SEMICOLON)
			{
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		dx = block_dx; //restore dx after handling procedure call!

		//这里的检测不知道为什么有bug，先注释了,by Lin
		set1 = createset(SYM_IDENTIFIER, SYM_NULL, SYM_VAR, SYM_SEMICOLON);
		set = uniteset(statbegsys, set1);
		//test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));

	code[mk->address].a = cx;
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);		//生成语句
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);
} // block

//////////////////////////////////////////////////////////////////////
 // base 沿着控制链回溯 levelDiff 次，返回目的深度父函数的 bp
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	
	while (levelDiff--)
		b = stack[b];
	return b;
}

//////////////////////////////////////////////////////////////////////
//执行汇编语言程序
void interpret()
{
	int pc;        // program counter
	int stack[STACKSIZE];
	int top;       // top of stack,栈顶寄存器
	int b;         // program, base, and top-stack register, 函数栈bp
	instruction i; // instruction register,指令寄存器

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	int param_num;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;
				pc = stack[top + 3];
				b = stack[top + 2];
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
            // 增加左移右移运算符实现
            case OPR_SHL:
				top--;
				stack[top] <<= stack[top + 1];
				break;
			case OPR_SHR:
				top--;
				stack[top] >>= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			} // switch
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;

		//添加了LODA和SOT和LEA指令，modified by Lin
		case LODA:
			stack[top] = stack[stack[top]];
			break;
		case STOA:
			stack[stack[top-1]] = stack[top];
			printf("%d\n", stack[top]);
			break;
		case LEA:
			stack[++top] = base(stack, b, i.l) + i.a;
			break;

		case STO:
			stack[base(stack, b, i.l) + i.a] = stack[top];
			printf("%d\n", stack[top]);
			top--;
			break;
		case CAL:
			switch (i.a)		//先判断是不是内置函数 by wu
			{
			case PRINT_ADDR:
				param_num = stack[top];
				printf("system call: print\n");
				if(param_num == 0){
					printf("\n");
				}
				else{
					while(param_num){
						printf("%d ",stack[top-param_num]);		//逆序打印
						param_num -= 1;
					}
					printf("\n");
				}
				break;
			case RANDOM_ADDR:
				printf("system call: random\n");
				param_num = stack[top];
				int random_num;
				if(param_num == 0){
					random_num = (int)rand();
					printf("%d ", random_num);
				}
				else if(param_num == 1){
					random_num = (int)rand() % stack[top-1];
					printf("%d ", random_num);
				}
				else if(param_num == 2){
					random_num = (int)rand() % (stack[top-1] - stack[top-2]) + stack[top-2];
					printf("%d ", random_num);
				}
				stack[++top] = random_num;
				printf("\n");
				break;
			case CALLSTACK_ADDR:
				//TODO by wu
				break;
			default:
				if(i.a >= 0){
					stack[top + 1] = base(stack, b, i.l);		//访问链
					// generate new block mark
					stack[top + 2] = b;							//控制链
					stack[top + 3] = pc;						//返回地址
					b = top + 1;
					pc = i.a;
				}
				else{
					error(28);		//程序地址错误
				}
				break;
			}
			break;
		case INT:
			top += i.a;
			break;
		case JMP:
			pc = i.a;
			break;
		case JPC:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
int main ()
{
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);				//空链表
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL); 
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_NULL);
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);
	block(set);			//对应实验文档中程序体生成
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
            // 为什么框架这里要用 fwrite ? 就为了展现这是汇编代码吗？
			// fwrite(&code[i], sizeof(instruction), 1, hbin); 
            fprintf(hbin, "%d %d %d\n", code[i].f, code[i].l, code[i].a);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	listcode(0, cx);

	return 0;
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
//print(random(100),random(100),random(20,50));