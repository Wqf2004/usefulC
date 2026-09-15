#include <stdio.h>

#define DEBUG // 定义一个DEBUG宏，不注释表示开启调试模式，注释表示关闭调试模式

int main()
{
#ifdef DEBUG
    printf("DEBUG is defined\n");
#endif
    return 0;
}
