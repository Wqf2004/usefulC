#include <stdio.h>

// 入参为两个int，返回值为int的函数指针
typedef int (*FuncPtr)(int, int);

// 两个int类型变量相加
int add(int a, int b) {
    return a + b;
}

// 两个int类型变量相乘
int multiply(int a, int b) {
    return a * b;
}

// 使用函数指针
int main() 
{
    FuncPtr fp = add;           
    printf("%d\n", fp(3, 4));   // 输出 7

    fp = multiply;              
    printf("%d\n", fp(3, 4));   // 输出 12

    return 0;
}