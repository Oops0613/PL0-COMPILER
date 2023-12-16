#include "parser.h"
int main()
{
    fp = fopen("D:\\VSCodeProjects\\compiler\\source.txt", "r");
    fw =fopen("D:\\VSCodeProjects\\compiler\\result.txt", "w");
    if (!fp)
    {
        printf("File dosen't exist");
        exit(1);
    }
    
    // <程序>→<程序⾸部><分程序>
    getsym();
    if(sym!=SYM_PROGRAM){
        error(19);//缺失PROGRAM关键字
    }
    //<程序⾸部>→PROGRAM<标识符>
    getsym();
    if(sym!=SYM_IDENTIFIER){
        error(27);//没有程序名
    }
    getsym();
    block();
    if (sym != SYM_NULL){
        error(9); //END之后不应有字符
    }
    if (err)
        printf("There are %d error(s) in PL/0 program.\n", err);
    else
        interpret();
    listcode(1, cx); // 输出目标代码
    fclose(fp);
    fclose(fw);
    return 0;
}
