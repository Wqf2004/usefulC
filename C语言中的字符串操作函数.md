# C语言中的字符串操作函数

C语言中跟字符有关的变量数据类型有三种，分别是：
char（字符类型）、`char []`（字符数组）、`char *`（字符指针）

为了方便介绍，我将`char []` 和 `char *` 统称为字符串。原因是，在C语言中，字符相关处理函数（如 strlen、strcpy、strcmp 等）并不区分 `char[]` 和 `char *`，它们的底层实现是基于指针操作的，因此对这两种类型的行为是相同的。

但是注意，这些函数无法操作字符串字面量，因为字符串字面量是常量，无法修改。

C语言提供了丰富的字符串操作函数，这些函数定义在<string.h>头文件中。以下是一些常用的字符串操作函数：

1. **字符串复制**
   - `strcpy(char *dest, const char *src)`: 将字符串src复制到dest
   - `strncpy(char *dest, const char *src, size_t n)`: 最多复制n个字符

2. **字符串连接**
   - `strcat(char *dest, const char *src)`: 将字符串src连接到dest末尾
   - `strncat(char *dest, const char *src, size_t n)`: 最多连接n个字符

3. **字符串比较**
   - `strcmp(const char *str1, const char *str2)`: 比较两个字符串
   - `strncmp(const char *str1, const char *str2, size_t n)`: 比较前n个字符

4. **字符串查找**
   - `strchr(const char *str, int c)`: 查找字符c首次出现位置
   - `strstr(const char *haystack, const char *needle)`: 查找子字符串

5. **字符串长度**
   - `strlen(const char *str)`: 计算字符串长度

6. **字符串转换**
   - `atoi(const char *str)`: 字符串转整数
   - `itoa(int value, char *str, int base)`: 整数转字符串

这些函数在处理字符串时非常方便，但使用时需要注意字符串缓冲区溢出等问题。
