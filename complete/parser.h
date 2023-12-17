#include "util.h"
#include "lexer.h"

// 将标识符填入符号表
// 从下标 1 开始
void enter(int kind)
{
    mask *mk;
    tx++;
    strcpy(table[tx].name, id);//名字
    table[tx].kind = kind;//类型

    switch (kind)
    {
    case ID_CONSTANT:
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        table[tx].value = num;
        break;
    case ID_VARIABLE:
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = dx++;
        break;
    }
}

// 在符号表table中查找id标识符
int locate(char *id)
{
    int i;
    strcpy(table[0].name, id);
    i = tx + 1;
    while (strcmp(table[--i].name, id) != 0)
        ;
    return i;
}

// <常量定义>→<标识符>=<⽆符号整数>
void constant()
{
    if (sym == SYM_IDENTIFIER)
    { //全局变量id中存有已识别的标识符
        getsym();
        if (sym == SYM_EQU || sym == SYM_BECOMES)
        {
            if (sym == SYM_BECOMES)//语法错误，多了:
                error(1); // Found ':=' when expecting '='.
            getsym();
            if (sym == SYM_NUMBER)
            {
                enter(ID_CONSTANT); // 将标识符填入符号表
                getsym();
            }
            else//常量声明缺少数字
            {
                error(2); // There must be a number to follow '='.
            }
        }
        else//缺少=
        {
            error(3); // There must be an '=' to follow the identifier.
        }
    }
    else
    {
        error(4); // There must be an identifier to follow 'const'.
    }
}

// <变量说明>→VAR<标识符>{，<标识符>}
void variable(void)
{
    if (sym == SYM_IDENTIFIER)
    {
        enter(ID_VARIABLE); // 将变量填入符号表
        getsym();
    }
    else
    {
        error(4); // There must be an identifier to follow 'var'.
    }
}

// 生成类汇编指令（基于栈）
void gen(int x, int y, int z)
{
    if (cx > CXMAX)//指令条数太多
    { // cx > 500
        printf("Fatal Error: Program too long.\n");
        exit(1);
    }
    // printf("gen code[%d] f=%s l=%d a=%d\n",cx,mnemonic[x],y,z);
    code[cx].f = x;
    code[cx].l = y;
    code[cx].a = z;
    //strcpy(code[cx].name,c);
    cx++;
}

// 因子，把待运算的数放到栈中
// 把项和因子独立开处理解决了加减号与乘除号的优先级问题
// <因⼦>→<标识符>|<常量>|(<表达式>)
void factor()
{
    void expression();
    int i;
    if (sym == SYM_IDENTIFIER)
    {
        if ((i = locate(id)) == 0)
        {
            error(11); // Undeclared identifier.
        }
        else
        {
            mask *mk;
            switch (table[i].kind)
            {
            case ID_CONSTANT:
                gen(LIT, 0, table[i].value); // 把常数放到栈顶
                printf("%s\n",id);
                break;
            case ID_VARIABLE:
                mk = (mask *)&table[i];
                gen(LOD, level - mk->level, mk->address); // 把变量放到栈顶
                break;
            }
        }
        getsym();
    }
    else if (sym == SYM_NUMBER)
    {
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        gen(LIT, 0, num); // 把常数放到栈顶
        //int index=locate(id);
        printf("%s\n",id);
        //table[index].value=num;
        getsym();
    }
    else if (sym == SYM_LPAREN) // (
    {
        getsym();
        expression();          // 递归调用表达式
        if (sym == SYM_RPAREN) // )
        {
            getsym();
        }
        else
        {
            error(22); // Missing ')'.
        }
    }
}

// <项>→<因⼦>|<项><乘法运算符><因⼦>
void term()
{
    int mulop;
    factor();
    while (sym == SYM_TIMES || sym == SYM_SLASH)
    {
        mulop = sym; // 记录下当前运算符
        getsym();
        factor();
        if (mulop == SYM_TIMES)
        {
            gen(OPR, 0, OPR_MUL); // 将栈顶和次栈顶进行乘运算
        }
        else
        {
            gen(OPR, 0, OPR_DIV);//除
        }
    }
}

// <表达式>→[+|-]项|<表达式><加法运算符><项>
void expression()
{
    int addop;//正负号
    if (sym == SYM_PLUS || sym == SYM_MINUS)
    {
        addop = sym;
        getsym();
        factor();
        if (addop == SYM_MINUS)
        {
            gen(OPR, 0, OPR_NEG);//取相反数
        }
        term();
    }
    else
    {
        term();
    }
    while (sym == SYM_PLUS || sym == SYM_MINUS)
    {
        addop = sym;
        getsym();
        term();
        if (addop == SYM_PLUS)
        {
            gen(OPR, 0, OPR_ADD);//加
        }
        else
        {
            gen(OPR, 0, OPR_MIN);//减
        }
    }
}

// <条件>→<表达式><关系运算符><表达式>
void condition()
{
    int relop;
        expression();
        relop = sym; // 记录下当前运算符
        getsym();
        expression();
        switch (relop)
        { // 根据比较关系生成指令
        case SYM_EQU: //==
            gen(OPR, 0, OPR_EQU);
            break;
        case SYM_NEQ: //<>
            gen(OPR, 0, OPR_NEQ);
            break;
        case SYM_LES: //<
            gen(OPR, 0, OPR_LES);
            break;
        case SYM_LEQ: //<=
            gen(OPR, 0, OPR_LEQ);
            break;
        case SYM_GTR: //>
            gen(OPR, 0, OPR_GTR);
            break;
        case SYM_GEQ: //>=
            gen(OPR, 0, OPR_GEQ);
            break;
        default:
            error(20);//缺少运算符
        }
}

// <语句>→<赋值语句>|<条件语句>|<循环语句>|<复合语句>|<空语句>
void statement()
{
    int i = 0, savedCx, savedCx_;

    if (sym == SYM_IDENTIFIER)
    { // <赋值语句>→<标识符>:=<表达式>
        if ((i = locate(id)) == 0)
        {
            error(11); // Undeclared identifier.
        }
        else if (table[i].kind != ID_VARIABLE)
        {
            error(12); // Illegal assignment.
        }
        getsym();
        if (sym == SYM_BECOMES) // :=
        {
            getsym();
        }
        else
        {
            error(13); // ':=' expected.
        }
        expression(); // 算出赋值号右部表达式的值
        mask *mk;
        mk = (mask *)&table[i];
        if (i)
        {   
            gen(STO, level - mk->level, mk->address); // 将栈顶内容存到刚刚识别到的变量里
        }
    }
    else if (sym == SYM_IF)
    {//<条件语句>→IF<条件>THEN<语句>
        getsym();
        condition();
        if (sym == SYM_THEN)
        {
            getsym();
        }
        else
        {
            error(16); // 'then' expected.
        }
        savedCx = cx;
        gen(JPC, 0, 0);   // 条件转移指令，栈顶为非真时跳转到a
        statement();      // 递归调用
        code[savedCx].a = cx; // 设置刚刚那个条件转移指令的跳转位置（回填）
    }
    else if (sym == SYM_BEGIN)
    { //<复合语句>→BEGIN<语句>{；<语句>}END
        getsym();
        statement(); // 递归调用
        while (sym == SYM_SEMICOLON)
        {
            if (sym == SYM_SEMICOLON)
            {
                getsym();
            }
            else
            {
                error(10);
            }
            statement();
        }
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
    {//<循环语句>→WHILE<条件>DO<语句>
        getsym();
        savedCx = cx;   // while循环的开始位置
        condition();    // 处理while里的条件
        savedCx_ = cx;  // while的do后的语句后的语句块的开始位置
        gen(JPC, 0, 0); // 条件转移指令，转移位置暂时不知

        if (sym == SYM_DO)
        {
            getsym();
        }
        else
        {
            error(18); // 'do' expected.
        }
        statement();          // 分析do后的语句块
        gen(JMP, 0, savedCx); // 无条件转移指令，跳转到cx1（while的起始），再次进行逻辑判断
        code[savedCx_].a = cx; // 回填刚才那个条件转移指令的跳转位置，while循环结束
    }
}
//供条件跳转使用
//Map<汇编代码序号,三地址代码序号>
int mapping(int map[],int source){
    int i=1;
    while(source>map[i]){
        i++;
    }
    return i;
}
void listcode(int from, int to)
{
    int c_cnt=1;//三地址代码的数量
    int map[to-from];
    char *p;
    int flag=0;
    for (int i = from; i < to; i++)
    {   
        p=mnemonic[code[i].f];
        if(strcmp(p,"JMP")==0||strcmp(p,"STO")==0||strcmp(p,"JPC")==0){
            map[c_cnt++]=i;//存储三地址代码序号到汇编代码序号的映射
            //printf("%d,%d\n",c_cnt-1,i);
        }
    }
    c_cnt=1;
    int cur=1;//arg数组的当前下标
    int v_cnt=0;//已产生的变量个数
    char* opr="=";//当前存储的运算符
    
    printf("listcode:\n");
    for (int i = from; i < to; i++)
    {
        printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
        p=mnemonic[code[i].f];
        if(strcmp(p,"JMP")==0){//无条件跳转
            fprintf(fw,"%d ",c_cnt++);
            fprintf(fw,"(j,_,_,%d)\n",mapping(map,code[i].a));
        }
        else if(strcmp(p,"LIT")==0){//把常数放到栈顶（准备运算）
            if(cur>2){
                cur=2;
                args[1].value=args[2].value;
            }
            args[cur].value=code[i].a;
            args[cur].kind=0;//是常量
            cur++;
        }
        else if(strcmp(p,"LOD")==0){//把变量原始下标放到栈顶
            args[cur].value=code[i].a;
            args[cur].kind=1;//是变量
            cur++;
            //cur_index=1;
        }
        else if(strcmp(p,"STO")==0||strcmp(p,"JPC")==0){//赋值语句或者条件语句
            cur=1;
            fprintf(fw,"%d ",c_cnt++);
            if(strcmp(mnemonic[code[i-1].f],"LIT")==0){//变量:=常量（直接赋值）
                if(temp[code[i].a]==0){//只有第一次进行复制时进行下标转换
                    temp[code[i].a]=++v_cnt;
                }
                fprintf(fw,"(%s,%d,_,t%d)\n",opr,args[1].value,temp[code[i].a]);
            }
            else if(strcmp(mnemonic[code[i-1].f],"OPR")==0){//表达式求值，赋值或条件运算
                
                fprintf(fw,strcmp(p,"JPC")==0?"(jn":"(");//JPC说明是条件跳转
                fprintf(fw,"%s,",opr);
                if((args[1].kind==1)){//变量
                    fprintf(fw,"t%d,",temp[args[1].value]);
                }
                else{//常量
                    fprintf(fw,"%d,",args[1].value);
                }
                if((args[2].kind==1)){//变量
                    fprintf(fw,"t%d,",temp[args[2].value]);
                }
                else{//常量
                    fprintf(fw,"%d,",args[2].value);
                }
                if(strcmp(p,"JPC")==0){
                    fprintf(fw,"%d)\n",mapping(map,code[i].a));
                }
                else{
                    fprintf(fw,"t%d)\n",temp[code[i].a]);
                }
                
            }
        }
        else if(strcmp(p,"OPR")==0){
            switch (code[i].a)
            {
            case OPR_ADD:
                opr="+";
                break;
            case OPR_MIN:
                opr="-";
                break;
            case OPR_MUL:
                opr="*";
                break;
            case OPR_DIV:
                opr="/";
                break;
            case OPR_EQU:
                opr="=";
                break;
            case OPR_NEG:
                opr="<>";
                break;
            case OPR_LES:
                opr="<";
                break;
            case OPR_LEQ:
                opr="<=";
                break;
            case OPR_GTR:
                opr=">";
                break;
            case OPR_GEQ:
                opr=">=";
                break;
            default:
                break;
            }
        }
    }
    fprintf(fw,"\n");
}
//代码有多层时使用
int base(int stack[], int currentLevel, int levelDiff)
{
    int b = currentLevel;
    while (levelDiff--)
        b = stack[b];
    return b;
}

void interpret()
{
    int pc = 0;           // program counter
    int stack[STACKSIZE]; // 假想栈
    int top = 0;          // 栈顶指针
    int b = 1;
    instruction i; // instruction register

    printf("Begin executing PL/0 program.\n");
    
    stack[1] = stack[2] = stack[3] = 0;
    do
    {
        printf("%d_", pc);
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
            case OPR_EQU:
                top--;
                stack[top] = stack[top] == stack[top + 1];
                break;
            case OPR_NEQ:
                top--;
                stack[top] = stack[top] != stack[top + 1];
            case OPR_LES:
                top--;
                stack[top] = stack[top] < stack[top + 1];
                break;
            case OPR_GEQ:
                top--;
                stack[top] = stack[top] >= stack[top + 1];
            case OPR_GTR:
                top--;
                stack[top] = stack[top] > stack[top + 1];
                break;
            case OPR_LEQ:
                top--;
                stack[top] = stack[top] <= stack[top + 1];
            }
            break;
        case LOD:
            stack[++top] = stack[base(stack, b, i.l) + i.a];
            break;
        case STO:
            stack[base(stack, b, i.l) + i.a] = stack[top];
            top--;
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
        }
    } while (pc);

    printf("\nEnd executing PL/0 program.\n");
}

// <分程序>→[<常量说明>][<变量说明>]<语句部分>
// 一遍扫描，语法分析、语义分析、目标代码生成 一起完成
void block()
{
    //后续变量定义主要用于代码生成
    int savedDx;
    int savedTx;
    int savedCx = cx;
    dx = 3; // 分配3个单元供运行期间存放静态链SL、动态链DL和返回地址RA
    printf("%d.gen JMP\n",savedCx);
    gen(JMP, 0, 0); // 跳转到分程序的开始位置，由于当前还没有知道在何处开始，所以jmp的目标暂时填为0
    // mask *mk = (mask *)&table[tx]; 
    // mk->address = cx; // 记录刚刚在符号表中记录的过程的address，cx是下一条指令的地址

    while (1)
    {
        if (level > MAXLEVEL)
        {
            error(32); // 层次数超过上限
        }
        // <常量说明>→CONST<常量定义>{，<常量定义>}
        if (sym == SYM_CONST)
        {
            getsym();
            if (sym == SYM_IDENTIFIER)
            {
                constant();
                while (sym == SYM_COMMA) //循环处理id1=num1,id2=num2,……
                {
                    getsym();
                    constant();
                }
                //常量定义结束
                if (sym == SYM_SEMICOLON)
                {
                    getsym();
                }
                else
                {
                    error(5); // Missing ',' or ';'.
                }
            }
        }
        // <变量说明>→VAR<标识符>{，<标识符>}
        else if (sym == SYM_VAR)
        {
            getsym();
            if (sym == SYM_IDENTIFIER)
            {
                variable();
                while (sym == SYM_COMMA)
                {
                    getsym();
                    variable();
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
        }
        else if (sym == SYM_NULL)//说明出现语法错误或者读到EOF
        {
            // getsym(); // 出错的情况下跳过出错的符号
            // 暂时不进行容错处理，遇到错误直接报错
            error(27);
            exit(1);
        }
        else
        {
            break;
        }
    }
    // 后续部分主要用于代码生成
    code[savedCx].a = cx; // 这时cx正好指向语句的开始位置，这个位置正是前面的 jmp 指令需要跳转到的位置
    printf("%d.JMP to %d\n",savedCx, cx);
    // mk->address = cx;
    gen(INT, 0, dx); // 为主程序在运行栈中开辟数据区，开辟 dx 个空间，作为这个分程序的第1条指令
    statement(); // 语句
    gen(OPR, 0, OPR_RET); // 从分程序返回（对于 0 层的程序来说，就是程序运行完成，退出）
}
