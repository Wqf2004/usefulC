# 宏定义基础知识

C语言编译流程简述：

1. 预处理阶段（Preprocessing）：将源代码中的宏定义展开，处理`#include`指令，处理`#define`指令，处理`#ifdef`、`#ifndef`、`#if`、`#else`、`#elif`、`#endif`等条件编译指令。
2. 编译阶段（Compilation）：将预处理后的源代码转换为汇编代码。
3. 汇编阶段（Assembly）：将汇编代码转换为机器代码。
4. 链接阶段（Linking）：将多个模块的机器代码和库函数链接在一起，形成可执行文件。

`#define`宏定义的作用是什么？
在C语言中，`#define` 是一个预处理器指令，用于定义宏（macro）。
注意：用大写字母命名宏常量，以区别于变量。

- 最常见的用法是用来定义符号常量，使代码更易读、更易维护。
- 用于构造条件编译。

观察到 C语言中的stdio.h和stdlib.h中都定义了size_t类型，那么在导入时会不会出现重复定义？
原来标准库内部实现类似这样

```c
#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef unsigned long size_t;
#endif
```

带内部守卫宏。

1. 如果你先 `#include <stdio.h>` ，stdio内部碰到未定义 __SIZE_T_DEFINED ，执行 typedef ，标记宏。
​
2. 后续再 `#include <stdlib.h>` ，stdlib内部判断宏已经存在，跳过typedef语句。

所以不管include顺序、不管先后包含哪个头，typedef只会执行一次，不会报重复定义编译错误。
