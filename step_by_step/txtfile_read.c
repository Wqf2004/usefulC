#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    /* 打开文件，获取文件指针 */
    FILE *fp = fopen("../dataset/test.txt", "r");
    if (fp == NULL)
    {
        printf("打开文件失败\n");
        return 1;
    }

    char line[32]; // 存储文件内容的一行
    /* 读取文件内容 */
    while (fgets(line, 32, fp) != NULL) // 使用这样的方式也能够读取整个文件
    {
        printf("%s", line);
    }

    /* 判断文件是否读取完成 */
    if (feof(fp))
    {
        printf("\n已经读取到文件末尾\n");
    }
    else
    {
        printf("\n未读取到文件末尾\n");
    }

    /* 关闭文件 */
    fclose(fp);
    return 0;
}
