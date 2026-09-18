#include <stdio.h>
#include <string.h>
#include "file_op.h"

static int read_all_lines(const char *path, char lines[][LINE_BUF_SIZE], int mode)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }

    int n = 0;
    while (n < MAX_LINES && fgets(lines[n], LINE_BUF_SIZE, fp) != NULL) 
    {
        n++;
    }
    /* 文件最后一行往往没有换行，补上 */
    if (!mode)
    {
        size_t len = strlen(lines[n-1]);
        if (len > 0 && lines[n-1][len - 1] != '\n' && len < LINE_BUF_SIZE - 1) 
        {
            lines[n-1][len] = '\n';        
            lines[n-1][len + 1] = '\0';
        }
    }

    fclose(fp);
    return n;
}

static int write_all_lines(const char *path, char lines[][LINE_BUF_SIZE], int n)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("fopen failed");
        return -1;
    }

    int i;
    for (i = 0; i < n; i++) {
        fputs(lines[i], fp);
    }

    fclose(fp);
    return 0;
}

int main()
{
    static char lines[MAX_LINES][LINE_BUF_SIZE];
    int n = read_all_lines("../data/b.txt", lines, 1);
    printf("%d", n);
    write_all_lines("../data/b.txt", lines, n);
    // // 向文件data/b.txt中追加一行内容
    // if (file_add("../data/b.txt", "文奇峰", 0) == 0)
    // {
    //     printf("向b.txt中追加了内容");
    // }
    // if (file_view("../data/b.txt") == 0)
    // {
    //     printf("查看b.txt中的内容");
    // }
    // // 删除这一行
    // if (file_delete_line("../data/b.txt", -1) == 0)
    // {
    //     printf("删除b.txt中刚追加的内容");
    // }
    // // 再次查看
    // if (file_view("../data/b.txt") == 0)
    // {
    //     printf("查看b.txt中的内容");
    // }

    getchar();
    return 0;
}