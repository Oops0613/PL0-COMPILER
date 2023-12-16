# PL/0 Compiler (implemented by C)
> 这是一个用C语言实现的⼩语⾔PL/0的简单编译器
## 项目结构
- **util.h** 存放所有用到的全局变量和宏定义
- **lexer.h** 词法分析器
- **parser.h** 语法分析器
- **main.c** 调用函数，负责源代码的头尾处理和结果输出
## 项目运行
1.在main.c中添加源文件路径
```c
fp = fopen("D:\\VSCodeProjects\\compiler\\source.txt", "r");
```
2.终端编译程序
```
> gcc ./complete/main.c
```
3.运行可执行文件
```
> ./a
```
## 使用的构造方法
### 词法分析
用**if-else**结构实现的简单DFA
![DFA.png](https://s2.loli.net/2023/12/01/lAuRfM7cUwhiOoX.png)
PS：a=字母，n=数字
### 语法分析
*递归下降分析法*</br>
根据语法规则可将编译程序**自顶向下**拆分成</br>
- 分程序block
- 常量说明constant
- 变量说明variable
- 语句部分statement
- 条件condition
- 表达式expression
- 项term
- 因子factor
### 中间代码生成
*语法制导的翻译技术*</br>
本程序使用一个**假象栈**保存翻译过程中参与运算的参数和产生的中间结果，例如：
- 四则运算的结果
- 关系运算的结果
- 布尔运算的结果</br>

并采用一些全局变量存储跳转指令的地址等  
在进行语法分析的同时，程序会产生相应的类汇编代码</br>
获取这些代码后，程序即可进入最后一步：四元式的产生</br>
#### 以下是一个例子</br>
源码：WHILE x<y DO …… </br>
> 当x小于y时,执行……内的代码
> 
类汇编码：(8,LOD,3)  (9,LOD,5) (10,OPR,8) (11,JMC,17)</br>
> 第一位=汇编码的序号</br>
> 第二位=汇编指令</br>
> 第三位=操作数的来源/操作类型/跳转的地址
>
四元式：(jn<,t1,t2,8)
