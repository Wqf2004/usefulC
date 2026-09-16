#include <stdio.h>
#include <string.h>
#include "file_op.h"

/**
 * @brief 新建一个空文件（0 字节），不写任何内容
 * @param path 文件路径，如 "a.txt"
 * @return 0 成功；-1 文件已存在（不会清空它）或创建失败（目录不存在/无权限）
 */
int file_create(const char *path)
{
    FILE *fp = fopen(path, "r");   /* 先试探：文件到底在不在？ */
    if (fp != NULL) {
        fclose(fp);
        printf("file already exists: %s\n", path);
        return -1;                 /* 已存在就坚决不动，避免被 "w" 瞬间清空 */
    }

    fp = fopen(path, "w");         /* 不存在才新建，"w" 在这里等于"白纸一张" */
    if (fp == NULL) {
        perror("fopen failed");    /* 多半是上级目录不存在，或没有写权限 */
        return -1;
    }

    fclose(fp);                    /* 一个字都不写，立即关闭 -> 得到0字节空文件 */
    return 0;
}

/**
 * @brief 删除指定文件
 * @param path 文件路径，如 "a.tmp"
 * @return 0成功 -1失败，常见失败原因：文件不存在、路径是目录、权限不足
 */
int file_del(const char *path)
{
    if (remove(path) != 0) {       /* remove 是 stdio.h 提供的标准库函数 */
        perror("remove failed");
        return -1;
    }
    return 0;
}

/**
 * @brief 重命名/移动文件或目录（rename 一个函数干两件事）
 * @param old_path 原路径，如 "a.txt"
 * @param new_path 新路径，如 "b.txt" 或 "dir/b.txt"
 * @return 0成功 -1失败，常见失败原因：
 *         1) 原文件不存在；
 *         2) 新路径已存在（Windows 下 rename 不会覆盖）；
 *         3) 跨磁盘移动（如 C盘 -> D盘，Windows 下会失败）
 */
int file_rename(const char *old_path, const char *new_path)
{
    if (rename(old_path, new_path) != 0) {   /* rename 同样来自 stdio.h */
        perror("rename failed");
        return -1;
    }
    return 0;
}

/**
 * @brief 文件末尾追加一行
 * @param path    文件路径，如 "b.txt"
 * @param content 要写入的字符串
 * @return 0成功 -1失败
 */
int file_add(const char *path, const char *content)
{
    FILE *fp = fopen(path, "a");   /* "a" = 追加写入，文件不存在则创建 */
    if (fp == NULL) {
        perror("fopen failed");
        return -1;
    }

    fputs(content, fp);            /* 写内容 */
    fputc('\n', fp);               /* 每行自动换行 */

    if (fclose(fp) == EOF) {       /* 关闭文件并刷新缓冲 */
        perror("fclose failed");
        return -1;
    }
    return 0;
}

/**
 * @brief 查看文件全部内容（带行号打印）
 */
int file_view(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen failed");
        return -1;
    }

    char line[LINE_BUF_SIZE];
    int no = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        ++no;
        printf("%4d  %s", no, line);
        /* 末行没有换行时补一个，免得提示符跟在内容后面 */
        size_t n = strlen(line);
        if (n == 0 || line[n - 1] != '\n') {
            putchar('\n');
        }
    }

    fclose(fp);
    return 0;
}

/**
 * @brief 按关键字查找行，返回命中行数
 */
int file_find(const char *path, const char *keyword)
{
    if (keyword == NULL || keyword[0] == '\0') {
        fprintf(stderr, "keyword must not be empty\n");
        return -1;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen failed");
        return -1;
    }

    char line[LINE_BUF_SIZE];
    int no = 0, hits = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        ++no;
        if (strstr(line, keyword) != NULL) {   /* strstr = 字符串里找子串 */
            ++hits;
            printf("%4d: %s", no, line);
            size_t n = strlen(line);
            if (n == 0 || line[n - 1] != '\n') {
                putchar('\n');
            }
        }
    }

    fclose(fp);
    printf("---- %d hit(s) ----\n", hits);
    return hits;
}

/**
 * @brief 把文件所有行读进 lines 二维数组，返回行数
 *        文件不存在时返回 0（之后用 "w" 写回就会自动新建）
 *        顺便保证每行末尾都有 '\n'，这样读进来什么样写回就什么样
 */
static int read_all_lines(const char *path, char lines[][LINE_BUF_SIZE])
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }

    int n = 0;
    while (n < MAX_LINES && fgets(lines[n], LINE_BUF_SIZE, fp) != NULL) {
        size_t len = strlen(lines[n]);
        if (len > 0 && lines[n][len - 1] != '\n' && len < LINE_BUF_SIZE - 1) {
            lines[n][len] = '\n';        /* 文件最后一行往往没有换行，补上 */
            lines[n][len + 1] = '\0';
        }
        n++;
    }

    fclose(fp);
    return n;
}

/**
 * @brief 把 n 行整体写回文件（"w" 模式一打开就会先清空原文件）
 */
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

/**
 * @brief 在第 line_no 行之前插入一行（行号从 1 开始）
 *        line_no 超过末尾时自动追加到最后
 * @return 0 成功，-1 失败
 */
int file_insert_line(const char *path, int line_no, const char *content)
{
    static char lines[MAX_LINES][LINE_BUF_SIZE]; /* 约1MB，加static放静态区，不占用栈空间 */
    int n = read_all_lines(path, lines);

    if (line_no < 1) {
        line_no = 1;
    }
    if (line_no > n + 1) {
        line_no = n + 1;                  /* 超出末尾 -> 追加 */
    }
    if (n >= MAX_LINES) {
        fprintf(stderr, "too many lines (max %d)\n", MAX_LINES);
        return -1;
    }

    int i;
    for (i = n; i >= line_no; i--) {      /* 从最后一行往后挪，从前往后挪会覆盖 */
        strcpy(lines[i], lines[i - 1]);
    }
    snprintf(lines[line_no - 1], LINE_BUF_SIZE, "%s\n", content);
    n++;

    return write_all_lines(path, lines, n);
}

/**
 * @brief 删除第 line_no 行（行号从 1 开始）
 * @return 0 成功，-1 行号不存在
 */
int file_delete_line(const char *path, int line_no)
{
    static char lines[MAX_LINES][LINE_BUF_SIZE]; /* 约1MB，加static放静态区，不占用栈空间 */
    int n = read_all_lines(path, lines);

    if (line_no < 1 || line_no > n) {
        printf("line %d not found\n", line_no);
        return -1;
    }

    int i;
    for (i = line_no - 1; i < n - 1; i++) {   /* 后面的行全部前移一行 */
        strcpy(lines[i], lines[i + 1]);
    }
    n--;

    return write_all_lines(path, lines, n);
}

/**
 * @brief 把第 line_no 行整行替换成 content（行号从 1 开始）
 * @return 0 成功，-1 行号不存在
 */
int file_replace_line(const char *path, int line_no, const char *content)
{
    static char lines[MAX_LINES][LINE_BUF_SIZE]; /* 约1MB，加static放静态区，不占用栈空间 */
    int n = read_all_lines(path, lines);

    if (line_no < 1 || line_no > n) {
        printf("line %d not found\n", line_no);
        return -1;
    }

    snprintf(lines[line_no - 1], LINE_BUF_SIZE, "%s\n", content);

    return write_all_lines(path, lines, n);
}

/**
 * @brief 在文件每一行里查找 old_str，全部替换成 new_str
 *        new_str 比 old_str 长也没关系；new_str 传 "" 就是删除
 * @return 替换了几处；old_str 为空返回 -1
 */
int file_replace_str(const char *path, const char *old_str, const char *new_str)
{
    if (old_str == NULL || old_str[0] == '\0') {
        fprintf(stderr, "old_str must not be empty\n");
        return -1;
    }

    static char lines[MAX_LINES][LINE_BUF_SIZE]; /* 约1MB，加static放静态区，不占用栈空间 */
    int n = read_all_lines(path, lines);
    int count = 0;
    size_t old_len = strlen(old_str);

    int i;
    for (i = 0; i < n; i++) {
        char out[LINE_BUF_SIZE * 4];     /* 替换后会变长，给足临时空间 */
        size_t used = 0;
        char *s = lines[i];
        char *p;

        while ((p = strstr(s, old_str)) != NULL) {
            size_t before = (size_t)(p - s);
            memcpy(out + used, s, before);     used += before;  /* 旧串前面的部分 */
            strcpy(out + used, new_str);       used += strlen(new_str); /* 新串 */
            s = p + old_len;
            count++;
        }
        strcpy(out + used, s);                 /* 这一行剩下的尾巴（含换行） */
        strcpy(lines[i], out);
    }

    if (count > 0) {
        write_all_lines(path, lines, n);
    }
    return count;
}
