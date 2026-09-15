
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
int main()
{
    /* 初始化的方式 */
    char str1[] = "Hello";  // 数组的长度是固定的，不能动态的修改数组的长度，但是可以修改数组的值
    char *str2 = "World";   // "World" 是一个字符串字面量（string literal），它被编译器放置在程序的只读数据段（.rodata）中

    printf("str1: %s\n", str1);
    printf("str2: %s\n", str2);

    /* 基于数组构建字符串，函数内部的存在栈上 */
    char name[100];
    strcpy(name, "Zara Ali");
    printf("Name = %s\n", name );

    /* 动态分配内存的方式，存在堆上 */
    char *description;
    description = (char *)malloc( 200 * sizeof(char) );
    if( description == NULL )
    {
        fprintf(stderr, "Error - unable to allocate required memory\n");
    }
    else
    {
        strcpy( description, "Zara ali a DPS student in class 10th");
    }
    
    printf("Description: %s\n", description );
    
    // 释放动态分配的内存
    free(description);
    
    return 0;
}