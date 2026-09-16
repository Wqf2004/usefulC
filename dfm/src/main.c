#include <stdio.h>
#include "file_op.h"

int main()
{
    // 向文件data/b.txt中追加一行内容
    if (file_add("../data/b.txt", "文奇峰", 0) == 0)
    {
        printf("向b.txt中追加了内容");
    }
    if (file_view("../data/b.txt") == 0)
    {
        printf("查看b.txt中的内容");
    }
    // 删除这一行
    if (file_delete_line("../data/b.txt", 2) == 0)
    {
        printf("删除b.txt中刚追加的内容");
    }
    // 再次查看
    if (file_view("../data/b.txt") == 0)
    {
        printf("查看b.txt中的内容");
    }

    getchar();
    return 0;
}