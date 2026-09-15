/*测试char, short, unsigned int, int, long, long long, float, double等C语言类型所占的字节数*/
#include <stdio.h>

int main() 
{
    printf("char: %d byte\n", (int)sizeof (char));
    printf("short: %d bytes\n", (int)sizeof(short));
    printf("unsigned int: %d bytes\n", (int)sizeof(unsigned int));
    printf("int: %d bytes\n", (int)sizeof(int));
    printf("long: %d bytes\n", (int)sizeof(long));
    printf("long long: %d bytes\n", (int)sizeof(long long));
    printf("float: %d bytes\n", (int)sizeof(float));
    printf("double: %d bytes\n", (int)sizeof(double));

    return 0;
}
