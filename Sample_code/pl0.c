// pl0 compiler source code

#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "PL0.h"
#include "set.c"

//////////////////////////////////////////////////////////////////////
// 输出报错信息
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
        printf("%5d  ", cx);   // 打印当前行指令对应汇编代码的行号
        while ((!feof(infile)) // added & modified by alex 01-02-09
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
// 获取当前下一个SYMBOL，将SYMBOL的各种信息保存到全局变量中
void getsym(void)
{
    int i, k;
    char a[MAXIDLEN + 1]; // a记录当前变量名

    while (ch == ' ' || ch == '\t')
        getch();

    if (isalpha(ch))
    { // symbol is a reserved word or an identifier.
        k = 0;
        do
        {
            if (k < MAXIDLEN)
                a[k++] = ch; // 将当前串读到a中
            getch();
        } while (isalpha(ch) || isdigit(ch));
        a[k] = 0;
        strcpy(id, a);
        word[0] = id; // 将当前变量名保存到关键字数组中
        i = NRW;
        while (strcmp(id, word[i--]))
            ; // 让word[i]定位到当前的关键字，如果i为0则为变量
        if (++i)
            sym = wsym[i]; // 解析到当前串为关键字
        else
            sym = SYM_IDENTIFIER; // 解析到当前串为变量名
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
        } while (isdigit(ch));
        if (k > MAXNUMLEN)
            error(25); // The number is too great.
    }
    else if (ch == ':')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_BECOMES; // :=
            getch();
        }
        else if (ch == ':') // ::, by wu
        {
            sym = SYM_SCOPE;
            getch();
        }
        else
        {
            sym = SYM_NULL; // illegal?
        }
    }
    else if (ch == '>')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_GEQ; // >=
            getch();
        }
        else if (ch == '>')
        {
            sym = SYM_SHR; // >> add by wdy
            getch();
        }
        else
        {
            sym = SYM_GTR; // >
        }
    }
    else if (ch == '<')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_LEQ; // <=
            getch();
        }
        else if (ch == '<')
        {
            sym = SYM_SHL; // << add by wdy
            getch();
        }
        else if (ch == '>')
        {
            sym = SYM_NEQ; // <>
            getch();
        }
        else
        {
            sym = SYM_LES; // <
        }
    }
    // 添加  //  */  /*
    // 并在此处直接处理注释
    // add by wdy
    else if (ch == '/')
    {
        getch();
        if (ch == '/')
        {
            sym = SYM_LINECOMMENT; // 行注释
            // 行尾在本程序中是空格，定义在getch() line[++ll] = ' ';
            // 但是不能用 ch == ' ' 来判定行结尾，因为中间可能有插入空格
            // 一旦读完该行，直接返回，不需要做之后的判断
            // 如何表示读完行？可以参考 SEMICOLON 的处理，获取下一个symbol即可
            while (cc < ll)
            {
                getch();
            }
            getsym();
            return;
        }
        else if (ch == '*')
        {
            sym = SYM_LEFTBLOCKCOMMENT; // 左块注释
            while (sym != SYM_RIGHTBLOCKCOMMENT)
            { // 直到找到右块注释为止
                while (ch != '*')
                {
                    if (ch == '/')
                    {
                        getch();
                        if (ch == '*')
                        {
                            error(31); // 不能嵌套注释
                        }
                    }
                    else
                        getch();
                }
                getsym();
            }
            // 获取下一个 symbol，表示结束注释
            getsym();
            return;
        }
        else
        {
            sym = SYM_SLASH; // 否则就是除号
        }
    }
    else if (ch == '*')
    {
        getch();
        if (ch == '/')
        {
            sym = SYM_RIGHTBLOCKCOMMENT; // 右块注释
            getch();
        }
        else
        {
            sym = SYM_TIMES; // 否则就是乘号
        }
    }

    // 这里是处理可能会出现复合运算符的，所以'[' ']' '&'
    // 不需要在这里处理，会统一在 else 中处理
    // by wdy
    // 添加左右方括号，add by Lin
    else if (ch == '[')
    {
        sym = SYM_LEFTBRACKET;
        getch();
    }
    else if (ch == ']')
    {
        sym = SYM_RIGHTBRACKET;
        getch();
    }
    // 添加取地址符号，add by Lin
    else if (ch == '&')
    {
        getch();
        if (ch == '&')
        {
            sym = SYM_AND; // && add by wy
            getch();
        }
        else
        {
            sym = SYM_ADDRESS;
        }
    }
    // 添加逻辑运算符||
    else if (ch == '|')
    {
        getch();
        if (ch == '|')
        {
            sym = SYM_OR; // || add by wy
            getch();
        }
    }
    else
    { // other tokens
        i = NSYM;
        csym[0] = ch;
        while (csym[i--] != ch)
            ;
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
// 产生一条指令，x为操作码，y为层次，z为参数，具体见实验文档指令格式
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

    if (!inset(sym, s1))
    {
        printf("%d\n", sym);
        error(n);
        s = uniteset(s1, s2);
        while (!inset(sym, s))
            getsym();
        destroyset(s);
    }
} // test

//////////////////////////////////////////////////////////////////////
int dx; // data allocation index

// 将一个变量加入到变量表中,modify by Lin
// 加了声明数组和指针功能，其中将3个dimension写入变量表中
void enter(int kind, int *dimension, int depth)
{
    mask *mk;

    tx++;
    strcpy(table[tx].name, id); // 将最近一次读到的变量名加入到当前变量表中
    table[tx].kind = kind;      // 写入变量类型
    switch (kind)
    {
    case ID_CONSTANT: // 常量，以comtab table的形式存储
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        table[tx].value = num;
        break;
    case ID_VARIABLE: // 变量，以mask的形式存储
        printf("%s %d %d\n",id ,dx , level);
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = dx++;
        mk->dimension[0] = 0; // 代表不是数组
        mk->depth = 0;        // 不是指针
        break;
    case ID_PROCEDURE: // 函数，以mask的形式存储
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->dimension[0] = 0; // 代表不是数组
        mk->depth = 0;        // 不是指针
        break;

    // 加入指针和数组case，add by Lin
    case ID_POINTER: // 指针，存储和普通变量相同，kind不同
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = dx++;
        mk->dimension[0] = 0; // 代表不是数组
        mk->depth = depth;
        break;
    case ID_ARRAY: // 数组，存储和普通变量不同，kind不同
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = dx;
        mk->dimension[0] = dimension[0];
        mk->dimension[1] = dimension[1];
        mk->dimension[2] = dimension[2];
        mk->depth = depth;
        int space = 1; // 为数组分配的空间
        for (int i = 0; i < 3; i++)
        {
            if (dimension[i] == 0)
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
// 在变量表中查找名为id的变量的下标，返回下标i，通过table[i]可以访问变量id的内容
int position(char *id, int start_level)
{
    int i;
    strcpy(table[0].name, id);
    i = tx + 1;
    while (strcmp(table[--i].name, id) != 0 || ((mask *)&table[i])->level > start_level)
        ; // 如果在不同嵌套深度有同名变量优先搜索到当前嵌套深度的变量
    return i;
} // position

//////////////////////////////////////////////////////////////////////
void constdeclaration() // 往变量表中加入一个常量.
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
    }
    else
        error(4);

    // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
void vardeclaration(void) // 往变量表中加入一个变量/数组/指针,modified by Lin
{
    int dimension[3] = {0, 0, 0}; // 数组维度
    int i = 0, depth = 0;
    if (sym == SYM_TIMES) // 指针
    {
        while (sym == SYM_TIMES)
        {
            depth += 1;
            getsym();
        }
        if (sym == SYM_IDENTIFIER)
        {
            getsym();
            if (sym == SYM_LEFTBRACKET) // 数组声明
            {
                while (sym == SYM_LEFTBRACKET)
                {
                    if (i == ARRAY_DIM)
                    {
                        error(50); // 数组维度过大
                    }
                    getsym();
                    if (sym == SYM_NUMBER)
                    {
                        dimension[i++] = num; //
                        getsym();
                        if (sym != SYM_RIGHTBRACKET)
                            error(42); // 缺少']'
                        getsym();
                    }
                    else
                        error(51); // 数组方括号内为空
                }
                enter(ID_ARRAY, dimension, depth); // 数组元素
            }
            else // 普通指针
            {
                enter(ID_POINTER, NULL, depth);
            }
        }
        else
            error(4);
    }
    else if (sym == SYM_IDENTIFIER) // 变量
    {
        getsym();
        if (sym == SYM_LEFTBRACKET) // 数组声明
        {
            while (sym == SYM_LEFTBRACKET)
            {
                if (i == ARRAY_DIM)
                {
                    error(50); // 数组维度过大
                }
                getsym();
                if (sym == SYM_NUMBER)
                {
                    dimension[i++] = num; //
                    getsym();
                    if (sym != SYM_RIGHTBRACKET)
                        error(42); // 缺少']'
                    getsym();
                }
                else
                    error(51); // 数组方括号内为空
            }
            enter(ID_ARRAY, dimension, 0); // 数组元素
        }
        else // 普通变量
        {
            enter(ID_VARIABLE, NULL, 0);
        }
    }
    else
        error(4);
} // vardeclaration

//////////////////////////////////////////////////////////////////////
// 打印从from到to的所有指令
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

// 生成基础的类型 by wdy
p_type_array gen_basic_type() {
	p_type_array tmp_array = (p_type_array)malloc(sizeof(type_array));
	tmp_array->array_depth = 0;
	tmp_array->array_name  = NULL;
	return tmp_array;
}

// 生成当前数组指针所管辖的区域 by wdy
int get_array_manage_space(p_type_array array) {
	// array_depth 为当前处理的数组的维度
	// 假设有数组定义 var brr[4][4][4]
	// 比如 3 表示当前处理的是三维数组的数组名 brr，管理数组后 2 个维度相乘
	// 2 表示当前处理的是 brr[1] 或者 *(brr + 1), 管理数组后 1 个维度
	// 1 表示处理的是 brr[1][2] 或者 (*(brr + 1) + 2)
	// 其实 1 这里就跟处理简单的 int一样了
	// 0 表示处理简单的 int
	int space = 1;
	if (array->array_name) { // 如果当前处理的是数组
		int array_dim = 0;
		for (int k = 0; k < ARRAY_DIM; k++) {
			if (array->array_name->dimension[k] != 0) {
				array_dim++;
			} else
				break;
		} // 得到当前数组的维度
		  // end_index = (array_dim - 1)
		  // (array_dim - 1) - start_index + 1 = array->array_depth - 1
		  // So start_index = array_dim - array->array_depth + 1
		for (int i = array_dim - 1; i > array_dim - array->array_depth; i--) {
			space *= array->array_name->dimension[i];
		}
	}
	return space;
}

//////////////////////////////////////////////////////////////////////
p_type_array factor(symset fsys, p_type_array array) // 生成因子
// 新增语法,by Lin
// 1:& ident，取变量地址
// 2:ident[express][express]...		//取数组元素
// 3:若干个* 接变量，取若干次地址
// 4:若干个*加(表达式),					//计算表达式后取地址
{
    p_type_array expression(symset fsys, p_type_array array);
    int i; // 最近读的关键字在变量表中的下标
    symset set;
    p_type_array res = array;
    // test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an shift.
    while (sym == SYM_NULL) // 除去空格
    {
        getsym();
    }

    if (sym == SYM_IDENTIFIER)
    {
        if ((i = position(id, start_level)) == 0)
        {
            error(11); // Undeclared identifier.
        }
        else
        {
            switch (table[i].kind)
            {
                mask *mk;
                int	  space;
            case ID_CONSTANT:
				getsym();
				space = get_array_manage_space(array);
				if (space == 1) {
					gen(LIT, 0, table[i].value);
				} else {
					gen(LIT, 0, space);
					gen(LIT, 0, table[i].value);
					gen(OPR, 0, OPR_MUL);
				}
				break;
			case ID_VARIABLE:
				getsym();
				mk	  = (mask *)&table[i];
				space = get_array_manage_space(array);
				if (space == 1) {
					gen(LOD, level - mk->level, mk->address);
				} else {
					gen(LIT, 0, space);
					gen(LOD, level - mk->level, mk->address);
					gen(OPR, 0, OPR_MUL);
				}
				break;
			case ID_POINTER:
				getsym();
				mk	  = (mask *)&table[i];
				space = get_array_manage_space(array);
				if (space == 1) {
					gen(LOD, level - mk->level, mk->address);
				} else {
					gen(LIT, 0, space);
					gen(LOD, level - mk->level, mk->address);
					gen(OPR, 0, OPR_MUL);
				}
				break;
            case ID_PROCEDURE:
                getsym();
                if (sym == SYM_SCOPE)
                {
                    getsym();
                    if (sym != SYM_IDENTIFIER)
                        error(30);
                    mk = (mask *)&table[i];      // 子函数的符号表表项
                    start_level = mk->level + 1; // 子函数本身的层次加一才能索引到子函数中的变量
                    res			= factor(fsys, array);
                    start_level = MAXLEVEL;
                }
                // error(21); // Procedure identifier can not be in an shift.
                break;
            case ID_ARRAY: // 新增读取数组元素,by Lin, by wdy
                getsym();
				mk						   = (mask *)&table[i];
				int array_depth_first_meet = 0;
				for (int k = 0; k < ARRAY_DIM; k++) {
					if (mk->dimension[k] != 0) {
						array_depth_first_meet++;
					} else
						break;
				}							// 得到当前数组的维度
				if (sym == SYM_LEFTBRACKET) // 调用了数组元素
				{
					res->array_depth = array_depth_first_meet;
					res->array_name	 = mk;
					gen(LEA, level - mk->level,
						mk->address); // 将数组基地址置于栈顶

					int dim = 0; // 当前维度
					while (sym == SYM_LEFTBRACKET) {
						if (table[i].dimension[dim++] == 0) {
							error(42); // 数组不具备该维度
						} else {
							getsym();
							expression(fsys, gen_basic_type());
						}
						int space = 1; // 当前维度的数组空间
						for (int j = dim; j < ARRAY_DIM;
							 j++) // 计算当前维的空间
						{
							if (table[i].dimension[j] != 0) {
								space *= table[i].dimension[j];
							} else
								break;
						}
						if (space > 1) {
							gen(LIT, 0, space);
							gen(OPR, 0, OPR_MUL);
						}
                        gen(OPR, 0, OPR_ADD); // 更新地址
						while (sym == SYM_NULL) // 除去空格
						{
							getsym();
						}
						if (sym != SYM_RIGHTBRACKET) {
							error(43); // 没有对应的']'
						}
						getsym();
						res->array_depth--;
					}
					if (res->array_depth == 0) { // 索引结束才能取值
						gen(LODA, 0, 0); // 将数组对应元素置于栈顶
					}
					// 否则只是计算地址
				} else { // 只有一个数组名，没有方括号
					gen(LEA, level - mk->level,
						mk->address); // 将数组基地址置于栈顶
					// 返回这个数组的信息
					res->array_depth = array_depth_first_meet;
					res->array_name	 = mk;
				}
				break;
            } // switch
        }
    }
    else if (sym == SYM_SCOPE)
    {
        getsym();
        if (sym != SYM_IDENTIFIER)
            error(30);
        start_level = 0; // 索引到main函数层次中定义的变量
        res			= factor(fsys, array);
        start_level = MAXLEVEL;
    }
    else if (sym == SYM_NUMBER)
    {
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        int space = get_array_manage_space(array);
		gen(LIT, 0, num * space);
        getsym();
    }
    else if (sym == SYM_LPAREN)
    {
        getsym();
        set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
        res = expression(set, array);
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
    else if (sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
    {
        getsym();
        res = factor(fsys, array);
        gen(OPR, 0, OPR_NEG);
    }
    // 新增语法,by Lin
    // 1:& ident，取变量地址
    // 2:ident[express][express]...		//取数组元素
    // 3:若干个* 接变量/数组元素，取若干次地址
    // 4:若干个*加(表达式),					//计算表达式后取地址
    // 新增语法1
    else if (sym == SYM_ADDRESS)
    {
        getsym();
        while (sym == SYM_NULL) // 除去空格
        {
            getsym();
        }
        if (sym == SYM_IDENTIFIER)
        {
            if ((i = position(id, start_level)) == 0)
            {
                error(11); // Undeclared identifier.
            }
            else
            {
                mask *mk = (mask *)&table[i];
                gen(LEA, level - mk->level, mk->address);
            }
            getsym();
        }
        else
        {
            error(55); // 取地址符后没接变量
        }
    }

    // 新增语法3和4
    else if (sym == SYM_TIMES)
    {
        int depth = 0;
        while (sym == SYM_TIMES)
        {
            depth++;
            getsym();
        }
        while (sym == SYM_NULL) // 除去空格
        {
            getsym();
        }
        if (sym == SYM_IDENTIFIER) // 多个*接变量
        {
            if ((i = position(id, start_level)) == 0)
            {
                error(11); // Undeclared identifier.
            }
            else
            {
                switch (table[i].kind)
                {
                    mask *mk;
                case ID_CONSTANT:
                    error(44); // 不允许取常量指向的地址的内容
                    break;
                case ID_VARIABLE:
                    error(58); // 访问非指针变量指向的地址
                case ID_POINTER:
                    mk = (mask *)&table[i];
                    gen(LOD, level - mk->level, mk->address);
                    getsym();
                    break;
                case ID_PROCEDURE:
                    error(21); // Procedure identifier can not be in an shift.
                    break;
                case ID_ARRAY: // 新增读取数组元素,by Lin, by wdy
                    getsym();
					mk						   = (mask *)&table[i];
					int array_depth_first_meet = 0;
					for (int k = 0; k < ARRAY_DIM; k++) {
						if (mk->dimension[k] != 0) {
							array_depth_first_meet++;
						} else
							break;
					} // 得到当前数组的维度
					res->array_depth = array_depth_first_meet;
					res->array_name	 = mk;
					if (depth >
						array_depth_first_meet + res->array_name->depth) {
						error(40); // 解引用符数量不能超过数组维度 +
								   // 数组元素指针深度
					}
					if (sym == SYM_LEFTBRACKET) // 调用了数组元素
					{
						gen(LEA, level - mk->level,
							mk->address); // 将数组基地址置于栈顶
						int dim = 0;	  // 当前维度
						while (sym == SYM_LEFTBRACKET) {
							if (table[i].dimension[dim++] == 0) {
								error(42); // 数组不具备该维度
							} else {
								getsym();
								expression(fsys, gen_basic_type());
							}
							int space = 1; // 当前维度的数组空间
							for (int j = dim; j < ARRAY_DIM;
								 j++) // 计算当前维的空间
							{
								if (table[i].dimension[j] != 0) {
									space *= table[i].dimension[j];
								} else
									break;
							}
							if (space > 1) {
								gen(LIT, 0, space);
								gen(OPR, 0, OPR_MUL);
							}
                            gen(OPR, 0, OPR_ADD); // 更新地址
							while (sym == SYM_NULL) // 除去空格
							{
								getsym();
							}
							if (sym != SYM_RIGHTBRACKET) {
								error(43); // 没有对应的']'
							}
							getsym();
							res->array_depth--;
							if (depth >
								res->array_depth + res->array_name->depth) {
								error(41); // 解引用符数量不能超过数组维度 +
										   // 数组元素指针深度
							}
						}
						if (res->array_depth == 0) { // 索引结束才能取值
							gen(LODA, 0, 0); // 将数组对应元素置于栈顶
						}
						// 否则只是计算地址
					} else { // 只有一个数组名，没有方括号
						gen(LEA, level - mk->level,
							mk->address); // 将数组基地址置于栈顶
					}
					break;
                } // switch
            }
        }
        else if (sym == SYM_LPAREN) // 多个*后面接表达式
        {
            getsym();
            set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
            res = expression(set, array);
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
            error(44);    // 非法取地址操作
        // 最终取地址的两种情况，分别是对简单变量取地址，对数组元素取地址
        if (res->array_depth >
			0) { //! 如果当前解引用的是数组，那么只要减少数组维度，也就是计算地址即可，不能取值
			res->array_depth -= depth; // 解引用 depth 次，相当于减少 depth 维
			if (res->array_depth == 0) {
				gen(LODA, 0,
					0); // !如果解引用到了最后一维，那么就必须取值，否则只是计算地址
			}
		} else { // 否则就是多重指针，那么就要减少指针的深度
			while (depth > 0) // 进行若干次取地址运算
			{
				depth--;
				gen(LODA, 0, 0);
			}
		}
    }

    // 错误检测,新增元素赋值号及右方括号by Lin
    // 将逗号和右括号补充进follow集 by wu
    symset set1 = createset(SYM_BECOMES, SYM_RIGHTBRACKET, SYM_COMMA, SYM_RPAREN);
    fsys = uniteset(fsys, set1);
    test(fsys, createset(SYM_LPAREN, SYM_NULL, SYM_LEFTBRACKET, SYM_BECOMES, SYM_COMMA), 23);

    return res;
} // factor

//////////////////////////////////////////////////////////////////////
p_type_array mul_and_div_expr(symset fsys, p_type_array array) // 生成项
{
	int	   mulop;
	symset set;

	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
	p_type_array res = factor(set, array);
	while (sym == SYM_TIMES || sym == SYM_SLASH) {
		mulop = sym;
		getsym();
		factor(set, gen_basic_type()); // 数组与 int 的乘除法是正常的
		if (mulop == SYM_TIMES) {
			gen(OPR, 0, OPR_MUL);
		} else {
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
	return res;
} // mul_and_div_expr

//////////////////////////////////////////////////////////////////////
p_type_array add_and_sub_expr(symset fsys, p_type_array array) {
	int	   addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));

	p_type_array res = mul_and_div_expr(set, array);
	while (sym == SYM_PLUS || sym == SYM_MINUS) {
		addop = sym;
		getsym();
		mul_and_div_expr(set, array); // 只有数组和 int 的加减法会出问题，所以要特殊处理
		if (addop == SYM_PLUS) {
			gen(OPR, 0, OPR_ADD);
		} else {
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
	return res;
} // add_and_sub_expr

//////////////////////////////////////////////////////////////////////
p_type_array shift_expr(symset fsys, p_type_array array) { // 生成移位表达式
	int			 shiftop;
	symset		 set = uniteset(fsys, createset(SYM_SHL, SYM_SHR, SYM_NULL));
	p_type_array res = add_and_sub_expr(set, array);
	while (sym == SYM_SHL || sym == SYM_SHR) {
		shiftop = sym;
		getsym();
		add_and_sub_expr(set, gen_basic_type());
		if (shiftop == SYM_SHL) {
			gen(OPR, 0, OPR_SHL);
		} else {
			gen(OPR, 0, OPR_SHR);
		}
	}
	destroyset(set);
    return res;
}
//////////////////////////////////////////////////////////////////////
p_type_array condition_expr(symset fsys, p_type_array array) // 生成条件表达式
{
	int	   relop;
	symset set;
    p_type_array res;
	if (sym == SYM_ODD) {
		getsym();
		res = shift_expr(fsys, array);
		gen(OPR, 0, 6);
	} else {
		set = uniteset(relset, fsys);
		res = shift_expr(set, array);
		destroyset(set);
		// if (!inset(sym, relset)) {
		// 	error(20); // 可以没有关系运算 by wy
		// } else
        if(inset(sym, relset)) {
			relop = sym;
			getsym();
			shift_expr(fsys, gen_basic_type());
			switch (relop) {
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
		}	  // else
	}		  // else
    return res;
} // condition_expr

// void end_condition(int JPcx)
// {
//     // 回填JMP_and跳转地址
//     while (sign_logic_and > 0)
//     {
//         code[cx_logic_and[sign_logic_and--]].a = JPcx;
//     }
// }

//////////////////////////////////////////////////////////////////////
// 生成逻辑表达式，add by wy
p_type_array logic_and_expression(symset fsys, p_type_array array)
{
    int sign = sign_logic_and;
    symset set = uniteset(fsys, createset(SYM_AND, SYM_NULL));
    p_type_array res = condition_expr(set, array);
    // 第一个表达式后面跟了逻辑运算符，那么将其转换为 bool 值
    if(sym == SYM_AND){ 
        gen(OPR, 0, OPR_NOT);
        gen(OPR, 0, OPR_NOT); // 两次取反，将表达式的值转变成bool值
    }   
    while (sym == SYM_AND)
    {
        sign_logic_and++; // 记录逻辑与的计算次数
        cx_logic_and[sign_logic_and] = cx;
        gen(JZ, 0, 0);
        getsym();
        condition_expr(set, gen_basic_type());
        gen(OPR, 0, OPR_AND);
    } // while
    destroyset(set);
    while (sign_logic_and > sign)
    {
        code[cx_logic_and[sign_logic_and--]].a = cx;
    }
    return res;
}

//////////////////////////////////////////////////////////////////////
p_type_array logic_or_expression(symset fsys, p_type_array array)
{
    int sign = sign_logic_or;
    symset set = uniteset(fsys, createset(SYM_OR, SYM_NULL));
    p_type_array res = logic_and_expression(set, array);
    // 第一个表达式后面跟了逻辑运算符，那么将其转换为 bool 值
    if(sym == SYM_OR){ 
        gen(OPR, 0, OPR_NOT);
        gen(OPR, 0, OPR_NOT); // 两次取反，将表达式的值转变成bool值
    }   
    while (sym == SYM_OR)
    {
        sign_logic_or++; // 记录逻辑或的计算次数
        cx_logic_or[sign_logic_or] = cx;
        gen(JNZ, 0, 0);
        getsym();
        logic_and_expression(set, gen_basic_type());
        gen(OPR, 0, OPR_OR);
    } // while
    destroyset(set);
    while (sign_logic_or > sign)
    {
        code[cx_logic_or[sign_logic_or--]].a = cx;
    }
    return res;
}

// 总的表达式，add by wy
p_type_array expression(symset fsys, p_type_array array){
    p_type_array res = logic_or_expression(fsys, array);
    return res;
}


// 增加对连续赋值表达式的支持
// 简单起见，无法对非简单变量进行连续赋值
// 比如不能写 a := b[1][2] = 0;
// 但是最后一个右值可以是任意表达式
// 比如 a := b := c[2][1];
mask *assign_sequence[10];
int	  curr_assign_index = 0;
int   curr_read_index = 0; // 记录当前读取的位置，避免一行多个语句 by wdy
/////////////////////////////////////////////////
int	get_assign_num() {
    int assign_num = 0;
    for (int i = curr_read_index; i < ll; i++) {
	    if (line[i] == ':' && line[i + 1] == '=') { assign_num++; }
        else if(line[i] == ';'){
            if(i == ll - 1) curr_read_index = 0;
            else curr_read_index = i + 1;
            break;
        }
	}
	return assign_num;
}
//////////////////////////////////////////////////////////////////////
void assign_statement(symset fsys) { // 生成赋值语句
	int i;
	int assign_num = get_assign_num();
	if (assign_num >= 10) { error(33); }
	for (int i = 1; i < assign_num; i++) {
		if (sym == SYM_IDENTIFIER) {
			mask *mk;
			if (!(i = position(id, start_level))) {
				error(11); // Undeclared identifier.
			}
			if (sym == ID_VARIABLE) {
				mk									 = (mask *)&table[i];
				assign_sequence[curr_assign_index++] = mk;
			} else {
				error(12); // Illegal assignment.
			}
			getsym();
			if (sym == SYM_BECOMES) { getsym(); }
		} else {
			error(12);
		}
	}
	// 处理最后一个右值
    expression(fsys, gen_basic_type()); // 此时栈顶为最后一个表达式的值
	for (int j = curr_assign_index - 1; j >= 0; j--) {
		mask* mk = assign_sequence[j];
		gen(STO, level - mk->level, mk->address);
	}
	curr_assign_index = 0;  // 恢复现场，准备处理下一个赋值语句
}

void call_procedure(symset fsys ,int i)
{
    ptr2param para = table[i].para_procedure;
    getsym();
    if (sym != SYM_LPAREN)
        error(35);
    else
    {
        int param_num = para->n; 
        int index;
        getsym();
        while (sym != SYM_RPAREN && param_num--)
        {
            expression(fsys, gen_basic_type());
            if (sym == SYM_COMMA)
            {
                getsym();
            }
        }
        gen(LIT, 0, para->n);
        if (sym == SYM_RPAREN && param_num == 0)
        {
            mask *mk;
            mk = (mask *)&table[i];
            gen(CAL, level - mk->level, mk->address);
        }
        else if (param_num == 0)
        {
            error(36);
        }
        else if (sym == SYM_RPAREN)
        {
            error(37);
        }
    }
}

//////////////////////////////////////////////////////////////////////
void statement(symset fsys) // 语句,加入了指针和数组的赋值，modified by Lin
{
    int i, cx1, cx2, cx3;
    symset set1, set;
    if (sym == SYM_IDENTIFIER) // 修改，加入数组的赋值，modified by Lin
    {                          // variable assignment
        while (sym == SYM_NULL)
            getsym();
        mask *mk;
        if (!(i = position(id, start_level)))
        {
            error(11); // Undeclared identifier.
        }
        else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY && table[i].kind != ID_POINTER)
        {
            error(12); // Illegal assignment.
            i = 0;
        }
        getsym();
        mk = (mask *)&table[i];
        if (sym == SYM_BECOMES) {
			getsym();
			// shift(fsys);
			assign_statement(fsys);
			if (i) { 
                gen(STO, level - mk->level, mk->address); 
                gen(POP, 0, 0); // 所有调用结束之后，恢复栈顶
            }
		}
        else if (sym == SYM_LEFTBRACKET) // 调用了数组元素
        {
            gen(LEA, level - mk->level, mk->address); // 将数组基地址置于栈顶
            int dim = 0;                              // 当前维度
            while (sym == SYM_LEFTBRACKET)
            {
                if (table[i].dimension[dim++] == 0)
                {
                    error(42); // 数组不具备该维度
                }
                else
                {
                    getsym();
                    // shift(fsys);
                    expression(fsys, gen_basic_type());
                }
                int space = 1;                        // 当前维度的数组空间
                for (int j = dim; j < ARRAY_DIM; j++) // 计算当前维的空间
                {
                    if (table[i].dimension[j] != 0)
                    {
                        space *= table[i].dimension[j];
                    }
                    else
                        break;
                }
                gen(LIT, 0, space);
                gen(OPR, 0, OPR_MUL);
                gen(OPR, 0, OPR_ADD); // 更新地址
                if (sym != SYM_RIGHTBRACKET)
                {
                    error(43); // 没有对应的']'
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
            // shift(fsys);
            expression(fsys, gen_basic_type());
            gen(STOA, 0, 0);
        }
        else
        {
            error(13); // ':=' expected.
        }
    }
    else if (sym == SYM_TIMES) // 给指针指向内容赋值，add by Lin
    {
        getsym();
		expression(fsys, gen_basic_type()); // 此时栈顶为一个地址
		if (sym == SYM_BECOMES) {
			getsym();
		} else {
			error(13); // ':=' expected.
		}
		expression(fsys, gen_basic_type()); // 栈顶为一个表达式的值
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
            if (!(i = position(id, start_level)))
            {
                error(11); // Undeclared identifier.
            }
            else if (table[i].kind == ID_PROCEDURE)
            {
                // mask *mk;
                // mk = (mask *)&table[i];
                // gen(CAL, level - mk->level, mk->address);
                call_procedure(fsys,i); // 调用函数, add by wy
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
        expression(set, gen_basic_type());
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
        expression(set, gen_basic_type());
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
        loop_level++; // 循环层次+1
        break_mark[loop_level][0] = 0;
        continue_mark[loop_level][0] = 0;
        statement(fsys);
        loop_level--; // 循环层次-1
        gen(JMP, 0, cx1);
        code[cx2].a = cx;
        for (int i = 1; i <= continue_mark[loop_level + 1][0]; i++)
            code[continue_mark[loop_level + 1][i]].a = cx1; // continue的回填，jmp至循环开始的条件判断处
        for (int i = 1; i <= break_mark[loop_level + 1][0]; i++)
            code[break_mark[loop_level + 1][i]].a = cx; // break的回填，jmp至循环结束后的下一行代码
    }
    else if (sym == SYM_DO)
    { // do-while statement, add by tq
        cx1 = cx;
        getsym();
        set1 = createset(SYM_WHILE, SYM_NULL);
        set = uniteset(set1, fsys);
        loop_level++;
        break_mark[loop_level][0] = 0;
        continue_mark[loop_level][0] = 0;
        statement(set);
        loop_level--;
        destroyset(set1);
        destroyset(set);
        if (sym != SYM_WHILE)
        {
            error(40);
        }
        else
        {
            getsym();
            cx2 = cx;
            set1 = createset(SYM_SEMICOLON, SYM_NULL);
            set = uniteset(set1, fsys);
            expression(set, gen_basic_type());
            destroyset(set1);
            destroyset(set);
            gen(JPC, 0, cx+2);
            gen(JMP, 0, cx1);
        }
        for (int i = 1; i <= continue_mark[loop_level + 1][0]; i++)
            code[continue_mark[loop_level + 1][i]].a = cx2; // continue的回填，jmp至条件判断处
        for (int i = 1; i <= break_mark[loop_level + 1][0]; i++)
            code[break_mark[loop_level + 1][i]].a = cx; // break的回填，jmp至循环结束后的下一行代码
        if(sym != SYM_SEMICOLON)
        {
            error(17);
        }
    }
    else if (sym == SYM_FOR)
    { // for statement, add by tq
        getsym();
        if(sym != SYM_LPAREN)
        {
            error(26);
        }
        getsym();
        statement(fsys);
        if(sym != SYM_SEMICOLON)
        {
            error(5);
        }
        else
        {
            cx1 = cx;
            getsym();
            condition_expr(fsys, gen_basic_type());
            cx3 = cx;
            gen(JPC, 0, 0);
        }
        if(sym != SYM_SEMICOLON)
        {
            error(5);
        }
        else
        {
            getsym();
            cx2 = cx;
            gen(JMP, 0, 0);
            set1 = createset(SYM_RPAREN, SYM_NULL);
            set = uniteset(set1, fsys);
            loop_level++;
        break_mark[loop_level][0] = 0;
        continue_mark[loop_level][0] = 0;
            statement(set);
            loop_level--;
            destroyset(set);
            destroyset(set1);
            gen(JMP, 0, cx1);
            code[cx2].a = cx;
        }
        if(sym != SYM_RPAREN)
        {
            error(22);
        }
        getsym();
        loop_level++; // 循环层次+1
        break_mark[loop_level][0] = 0;
        continue_mark[loop_level][0] = 0;
        statement(fsys);
        loop_level--; // 循环层次-1
        gen(JMP, 0, cx2+1);
        code[cx3].a = cx;
        for (int i = 1; i <= continue_mark[loop_level + 1][0]; i++)
            code[continue_mark[loop_level + 1][i]].a = cx2; // continue的回填，jmp至循环结束的循环执行语句
        for (int i = 1; i <= break_mark[loop_level + 1][0]; i++)
            code[break_mark[loop_level + 1][i]].a = cx; // break的回填，jmp至循环结束后的下一行代码
    }
    else if (sym == SYM_BREAK) // break by tian
    {
        getsym();
        if (loop_level == 0)
        {
            error(33);
        }
        else
        {
            break_mark[loop_level][++break_mark[loop_level][0]] = cx;
            gen(JMP, 0, 0);
        }
    }
    else if (sym == SYM_CONTINUE) // continue by tian
    {
        getsym();
        if (loop_level == 0)
        {
            error(34);
        }
        else
        {
            continue_mark[loop_level][++continue_mark[loop_level][0]] = cx;
            gen(JMP, 0, 0);
        }
    }
    else if (sym == SYM_PRINT) // 内置函数 print()的实现 by wu
    {
        getsym();
        if (sym != SYM_LPAREN)
        {
            error(26); // 没找到左括号
        }
        getsym(); // 跳过左括号
        if (sym == SYM_RPAREN)
        {
            gen(LOD, 0, 0); // 无参数，打印换行符
            getsym();       // 跳过右括号
        }
        else
        {
            int param_count = 0;
            // shift(fsys);
            expression(fsys, gen_basic_type());
            param_count += 1;
            while (sym != SYM_RPAREN)
            {
                if (sym != SYM_COMMA)
                {
                    error(27);
                    break;
                }
                else
                {
                    getsym();
                    // shift(fsys);
                    expression(fsys, gen_basic_type());
                    param_count += 1;
                }
            }
            getsym();
            gen(LIT, 0, param_count);
        }
        gen(CAL, 0, PRINT_ADDR);
    }
    else if (sym == SYM_CALLSTACK) // 内置函数 CALLSTACK的实现 by wu
    {
        getsym();
        gen(CAL, 0, CALLSTACK_ADDR);
    }
    else if (sym == SYM_RANDOM) ////内置函数 random() 的实现 by wu
    {
        getsym();
        if (sym != SYM_LPAREN)
        {
            error(26); // 没找到左括号
        }
        getsym();
        if (sym == SYM_RPAREN)
        {
            gen(LOD, 0, 0); // 无参数，随机生成 0~100 的自然数
            getsym();       // 跳过右括号
        }
        else
        {
            int param_count = 0;
            // shift(fsys);
            expression(fsys, gen_basic_type());
            param_count += 1;
            while (sym != SYM_RPAREN)
            {
                if (sym != SYM_COMMA)
                {
                    error(27);
                    break;
                }
                else
                {
                    getsym();
                    // shift(fsys);
                    expression(fsys, gen_basic_type());
                    param_count += 1;
                    while (sym == SYM_NULL)
                        getsym();
                }
            }
            if (param_count > 2)
                error(29);
            getsym();
            gen(LIT, 0, param_count);
        }
        gen(CAL, 0, RANDOM_ADDR);
    }
    // 错误检测
    test(fsys, phi, 19);
} // statement

//////////////////////////////////////////////////////////////////////
void block(symset fsys, int para_number) // 生成一个程序体, para_number为当前程序体参数个数
{
    int cx0; // initial code index
    mask *mk;
    int block_dx;
    int savedTx, savedDx;
    int savedlevel;
    symset set1, set;

    dx = 3;
    block_dx = dx;
    // int temp_para_num;
    // for(temp_para_num = 0; temp_para_num < para_number; temp_para_num ++){
    //     mk = (mask *)&table[tx - temp_para_num];
    //     gen(STO, level - mk->level, mk->address);
    // }
    mk = (mask *)&table[tx - para_number]; // modify by wy
    mk->address = cx;
    gen(JMP, 0, 0); // 产生第一条指令JMP 0, 0
    if (level > MAXLEVEL)
    {
        error(32); // There are too many levels.
    }
    do // 循环读完整个PL0代码产生汇编程序
    {
        if (sym == SYM_CONST) // 常量声明，无需产生指令
        {                     // constant declarations
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
            } while (sym == SYM_IDENTIFIER);
        } // if

        if (sym == SYM_VAR) // 变量声明，无需产生指令
        {                   // variable declarations
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
            } while (sym == SYM_IDENTIFIER);
        }              // if
        block_dx = dx; // save dx before handling procedure call!
        while (sym == SYM_PROCEDURE)
        { // procedure declarations
            int para_num = 0;
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
            level++;
            savedTx = tx;
            ptr2param parameter;
            if (sym == SYM_LPAREN) // get parameters
            {
                savedDx = dx;
                dx = 3;     //重新分配数据地址
                getsym();
                while (sym != SYM_RPAREN)
                {
                    if (sym == SYM_VAR)
                    {
                        getsym();
                        para_num++;
                        if (sym == SYM_IDENTIFIER)
                        {
                            enter(ID_VARIABLE, NULL, 0);    //提前给函数实参占用位置，调用时往里填
                            getsym();
                        }
                    }
                    else
                    {
                        printf("%d", sym);
                        error(38);
                        getsym();
                    }
                    if (sym == SYM_COMMA)
                    {
                        getsym();
                        if (sym == SYM_RPAREN)
                        {
                            error(39);
                        }
                    }
                }
                mask *mk_p = (mask *)&table[tx - para_num];
                parameter = (ptr2param)malloc(sizeof(procedure_params));
                parameter->n = para_num;
                parameter->kind = (int *)malloc(para_num * sizeof(int));
                parameter->name = (char **)malloc(para_number * sizeof(char*));
                mk_p->para_procedure = parameter;
                int i = 0;
                for (i = 0; i < para_num; i++)
                {
                    mask *mk_p = (mask *)&table[tx - i];
                    // mk_p->address = -1 - i;
                    parameter->kind[i] = mk_p->kind;
                    parameter->name[i] = mk_p->name;
                }
                getsym();
            }
            else 
            {
                error(35);
            }
            dx = savedDx; // 恢复dx


            if (sym == SYM_SEMICOLON)
            {
                getsym();
            }
            else
            {
                error(5); // Missing ',' or ';'.
            }

            savedTx = tx;
            set1 = createset(SYM_SEMICOLON, SYM_NULL);
            set = uniteset(set1, fsys);
            block(set, para_num); // 生成子程序体
            destroyset(set1);
            destroyset(set);
            tx = savedTx - para_num; // 子程序分析完后将子程序的变量从符号表中“删除”
            level--;

            if (sym == SYM_SEMICOLON)
            {
                getsym();
                set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
                set = uniteset(statbegsys, set1);
                // test(set, fsys, 6);
                destroyset(set1);
                destroyset(set);
            }
            else
            {
                error(5); // Missing ',' or ';'.
            }
        }              // while
        dx = block_dx; // restore dx after handling procedure call!

        // 这里的检测不知道为什么有bug，先注释了,by Lin
        set1 = createset(SYM_IDENTIFIER, SYM_NULL, SYM_VAR, SYM_SEMICOLON);
        set = uniteset(statbegsys, set1);
        // test(set, declbegsys, 7);
        destroyset(set1);
        destroyset(set);
    } while (inset(sym, declbegsys));

    code[mk->address].a = cx;
    mk->address = cx;
    cx0 = cx;
    gen(INT, 0, block_dx);
    set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
    set = uniteset(set1, fsys);
    statement(set); // 生成语句
    destroyset(set1);
    destroyset(set);
    gen(OPR, 0, OPR_RET); // return
    test(fsys, phi, 8);   // test for error: Follow the statement is an incorrect symbol.
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
// 执行汇编语言程序
void interpret()
{
    int pc; // program counter
    int stack[STACKSIZE] = {0}; // 栈初始化为0
    int top;       // top of stack,栈顶寄存器
    int b;         // program, base, and top-stack register, 函数栈bp
    int temp_b;
    int temp_pc;
    int tran_i;
    instruction i; // instruction register,指令寄存器

    printf("Begin executing PL/0 program.\n");

    pc = 0;
    b = 1;
    top = 3;
    stack[0] = stack[1] = stack[2] = stack[3] = 0;
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
            // 增加逻辑运算符, by wy
            case OPR_AND:
                top--;
                stack[top] = stack[top] && stack[top + 1];
                break;
            case OPR_OR:
                top--;
                stack[top] = stack[top] || stack[top + 1];
                break;
            // 增加非运算符实现, by wdy
            case OPR_NOT:
                stack[top] = !stack[top];
                break;
            } // switch
            break;
        case LOD:
            stack[++top] = stack[base(stack, b, i.l) + i.a];
            break;

        // 添加了LODA和SOT和LEA指令，modified by Lin
        case LODA:
            stack[top] = stack[stack[top]];
            break;
        case STOA:
            stack[stack[top - 1]] = stack[top];
            // printf("%d\n", stack[top]);
            top -= 2; // STOA 之后弹栈
            break;
        case LEA:
            stack[++top] = base(stack, b, i.l) + i.a;
            break;

        case STO:
            stack[base(stack, b, i.l) + i.a] = stack[top];
            // printf("%d\n", stack[top]);
            // top--; 使用 POP 代替
            break;
        case CAL:
            switch (i.a) // 先判断是不是内置函数 by wu
            {
            case PRINT_ADDR:
                param_num = stack[top];
                printf("system call: PRINT\n");
                if (param_num == 0)
                {
                    printf("\n");
                }
                else
                {
                    while (param_num)
                    {
                        printf("%d ", stack[top - param_num]); // 逆序打印
                        param_num -= 1;
                    }
                    printf("\n");
                }
                printf("\n");
                break;
            case RANDOM_ADDR:
                printf("system call: RANDOM\n");
                param_num = stack[top];
                int random_num;
                if (param_num == 0)
                {
                    random_num = (int)rand();
                    printf("%d ", random_num);
                }
                else if (param_num == 1)
                {
                    random_num = (int)rand() % stack[top - 1];
                    printf("%d ", random_num);
                }
                else if (param_num == 2)
                {
                    random_num = (int)rand() % (stack[top - 1] - stack[top - 2]) + stack[top - 2];
                    printf("%d ", random_num);
                }
                stack[++top] = random_num;
                printf("\n\n");
                break;
            case CALLSTACK_ADDR:
                printf("system call: CALLSTACK\n");
                temp_b = b;
                temp_pc = pc;
                while(temp_b > 0){
                    printf("control link:%d\n",temp_b+1);
                    printf("access link:%d\n", temp_b);
                    printf("program counter: %d\n",temp_pc);
                    printf("parameters count:%d\n",stack[temp_b-1]);
                    param_num = stack[temp_b-1];
                    if(param_num != 0) printf("parameter list:");
                    for(tran_i=0; tran_i<param_num; tran_i++){
                        printf("%d ",stack[temp_b-1-param_num+tran_i]);
                    }
                    printf("\n\n");
                    temp_pc = stack[temp_b+2];
                    temp_b = stack[temp_b+1];
                }
                printf("\n");
                break;
            default:
                if (i.a >= 0)
                {
                    param_num = stack[top];
                    while(param_num){
                        stack[top+3+param_num] = stack[top-stack[top]+param_num-1];
                        param_num--;
                    }
                    // printf("%d",stack[top]);
                    stack[top + 1] = base(stack, b, i.l); // 访问链
                    // generate new block mark
                    stack[top + 2] = b;  // 控制链
                    stack[top + 3] = pc; // 返回地址
                    b = top + 1;
                    pc = i.a;
                }
                else
                {
                    error(28); // 程序地址错误
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
        case POP:
            top--;
            break;
        // 添加JZ和JNZ指令
        case JZ:
            if (stack[top] == 0)
                pc = i.a;
            break;
        case JNZ:
            if (stack[top] != 0)
                pc = i.a;
            break;
        } // switch
    } while (pc);

    printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    FILE *hbin;
    char* file_path = "test/test_procedure_para.txt";
    int i;
    symset set, set1, set2;

    if(argc > 1){
        if(strcmp(argv[1], "-h") == 0){
            printf("Usage: ./pl0.exe -f [filename]\n");
            return 0;
        }
        else if(strcmp(argv[1], "-f") == 0){
            if(argc > 2){
                file_path = argv[2];
            }
            else{
                printf("Usage: ./pl0.exe -f [filename]\n");
                return 0;
            }
        }
    }

    if ((infile = fopen(file_path, "r")) == NULL)
    {
        printf("File %s can't be opened.\n", file_path);
        exit(1);
    }

    phi = createset(SYM_NULL); // 空链表
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
    block(set, 0); // 对应实验文档中程序体生成
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
            fwrite(&code[i], sizeof(instruction), 1, hbin);
            // fprintf(hbin, "%d %d %d\n", code[i].f, code[i].l, code[i].a);
        fclose(hbin);
    }
    if (err == 0)
        interpret();
    else
        printf("There are %d error(s) in PL/0 program.\n", err);
    // listcode(0, cx);

    return 0;
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
// print(random(100),random(100),random(20,50));