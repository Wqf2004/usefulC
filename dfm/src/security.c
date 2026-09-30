#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "security.h"
#include "file_op.h"

static int g_deleted_inited = 0; /* 本会话是否已开局（清残留+注册退出钩子）*/

/**
 * @brief 备份ghost文件以进入无痕模式
 */
void backupGhostFiles()
{
    if (file_exists(FILENAME_A))
    {
        file_copy(FILENAME_A, FILENAME_A_GHOST);
    }
    else if (file_exists(FILENAME_A_GHOST))
    {
        remove(FILENAME_A_GHOST);
    }

    if (file_exists(FILENAME_B))
    {
        file_copy(FILENAME_B, FILENAME_B_GHOST);
    }
    else if (file_exists(FILENAME_B_GHOST))
    {
        remove(FILENAME_B_GHOST);
    }
}

/**
 * @brief 还原原业务文件以退出无痕模式
 */
void restoreGhostFiles()
{
    // 学生信息 a.txt
    if (file_exists(FILENAME_A_GHOST))
    {
        remove(FILENAME_A);
        rename(FILENAME_A_GHOST, FILENAME_A);
    }
    else
    {
        remove(FILENAME_A);
    }

    // 成绩信息 b.txt
    if (file_exists(FILENAME_B_GHOST))
    {
        remove(FILENAME_B);
        rename(FILENAME_B_GHOST, FILENAME_B);
    }
    else
    {
        remove(FILENAME_B);
    }
}

/**
 * @brief 原地去除字符串的首尾空白（字符）
 * @param s 需要处理的字符串
 */
void input_trim(char *s)
{
    char *start = s;
    char *end;
    size_t len;

    if (s == NULL)
    {
        return;
    }
    while (*start && isspace((unsigned char)*start))
    {
        start++;
    }
    if (start != s)
    {
        memmove(s, start, strlen(start) + 1);
    }
    len = strlen(s);
    end = s + len;
    while (end > s && isspace((unsigned char)*(end - 1)))
    {
        end--;
    }
    *end = '\0';
}

/* 动作枚举 -> 中文名：下标顺序必须与 security.h 中的 ACT_* 严格一致。
 * 数组大小锁定 ACT_COUNT，一旦漏写或多写名称，编译器会在该行告警。 */
static const char *const ACTION_NAMES[ACT_COUNT] = {
    "删除", /* ACT_DELETE */
    "覆盖", /* ACT_OVERWRITE */
    "清空", /* ACT_CLEAR */
    "改名"  /* ACT_RENAME */
};

/**
 * @brief 危险操作二次确认，仅用于提示
 * @param path   目标文件路径，如 "../data/a.txt"
 * @param action 操作类型，取 DangerousAction 枚举值（如 ACT_DELETE）
 * @return 1 用户已确认（输入 y/Y）；
 *         0 用户取消，或读取输入失败（EOF）
 */
int confirm_dangerous(const char *path, DangerousAction action)
{
    char buf[16];
    const char *name;

    /* 防御非法枚举值，避免越界读取名称表 */
    name = (action >= 0 && action < ACT_COUNT) ? ACTION_NAMES[action]
                                               : "未知操作";

    printf("\n  [警告] 即将【%s】文件：%s\n", name, path);
    printf("  [警告] 此操作不可恢复！输入 y 确认，其它任意键取消：");

    if (fgets(buf, sizeof(buf), stdin) == NULL)
    {
        return 0; /* EOF：视为取消，避免误删 */
    }

    return (buf[0] == 'y' || buf[0] == 'Y') ? 1 : 0;
}

/*
 * 四个安全绑定包装：先过 confirm_dangerous 闸门，放行才调 file_op 原语。
 * 统一返回：0 成功 / -1 已确认但执行失败 / -2 用户取消。
 */
int safe_file_delete(const char *path)
{
    if (!confirm_dangerous(path, ACT_DELETE)) {
        return -2;
    }
    return file_del(path) == 0 ? 0 : -1;
}

int safe_file_rename(const char *old_path, const char *new_path)
{
    if (!confirm_dangerous(old_path, ACT_RENAME)) {
        return -2;
    }
    return file_rename(old_path, new_path) == 0 ? 0 : -1;
}

int safe_file_overwrite(const char *path, const char *content)
{
    if (!confirm_dangerous(path, ACT_OVERWRITE)) {
        return -2;
    }
    return file_overwrite(path, content) == 0 ? 0 : -1;
}

int safe_file_clear(const char *path)
{
    if (!confirm_dangerous(path, ACT_CLEAR)) {
        return -2;
    }
    return file_clear(path) == 0 ? 0 : -1;
}


/* 判断是否为 GBK 双字节字符的【首字节】（前导字节），范围 0x81~0xFE */
static int is_gbk_lead(unsigned char c) { return c >= 0x81 && c <= 0xFE; }

/* 判断是否为 GBK 双字节字符的【尾字节】（后继字节），范围 0x40~0xFE 且排除 0x7F */
static int is_gbk_tail(unsigned char c) { return (c >= 0x40 && c <= 0xFE && c != 0x7F); }

/**
 * @brief 初始化默认安全规则
 * @param rule  需要一个InputRule对象的地址
 */
void rule_init(InputRule *rule)
{
    if (rule == NULL)
    {
        return;
    }
    rule->allowed = CT_ALNUM;
    rule->min_len = 0;
    rule->max_len = 0;
    rule->allow_space = 0;
    rule->trim = 1;
}

/**
 * @brief 计算GBK 字符串的“字符数”
 * @param s GBK 字符串，可包含汉字
 * @return 当字符串为空时，返回0
 *         返回字符串的长度，合法双字节汉字计 1
 */
int gbk_strlen(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    int n = 0;

    if (s == NULL)
    {
        return 0;
    }
    while (*p)
    {
        if (is_gbk_lead(p[0]) && p[1] != '\0' && is_gbk_tail(p[1]))
        {
            p += 2;
        }
        else
        {
            p += 1;
        }
        n++;
    }
    return n;
}

/* 单个 ASCII 字符 -> 所属类型位；不属于任何类别返回 0 */
static unsigned char_type(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return CT_DIGIT;
    if (c >= 'a' && c <= 'z')
        return CT_LOWER;
    if (c >= 'A' && c <= 'Z')
        return CT_UPPER;
    if (c == '_')
        return CT_UNDER;
    if (c == '-')
        return CT_DASH;
    if (c == '.')
        return CT_DOT;
    return 0;
}

/**
 * @brief 进行输入校验，先按规则裁剪，再依次检查 空 -> 长度 -> 内部空格 -> 逐字符
 * @param s GBK 字符串，可包含汉字
 * @param rule 一个InputRule对象的地址（函数内为指针）
 * @return 返回一个VResult结果码
 */
VResult input_validate(char *s, const InputRule *rule)
{
    const unsigned char *p;
    int len;

    if (s == NULL || rule == NULL)
    {
        return V_ERR_ILLEGAL;
    }

    if (rule->trim)
    {
        input_trim(s);
    }
    if (s[0] == '\0')
    {
        return V_ERR_EMPTY;
    }

    len = gbk_strlen(s);
    if (rule->min_len > 0 && len < rule->min_len)
    {
        return V_ERR_TOO_SHORT;
    }
    if (rule->max_len > 0 && len > rule->max_len)
    {
        return V_ERR_TOO_LONG;
    }

    p = (const unsigned char *)s;
    while (*p)
    {
        /* 空格独立判断 */
        if (*p == ' ' || *p == '\t')
        {
            if (!rule->allow_space)
            {
                return V_ERR_SPACE;
            }
            p++;
            continue;
        }

        /* GBK 汉字 */
        if (is_gbk_lead(p[0]) && p[1] != '\0' && is_gbk_tail(p[1]))
        {
            if (!(rule->allowed & CT_CHINESE))
            {
                return V_ERR_ILLEGAL;
            }
            p += 2;
            continue;
        }

        /* 普通 ASCII 字符 */
        if ((char_type(*p) & rule->allowed) == 0u)
        {
            return V_ERR_ILLEGAL;
        }
        p++;
    }

    return V_OK;
}

/**
 * @brief 将VResult结果码转义成字符串
 * @param r 一个VResult结果码
 * @return 返回一个字符串
 */
const char *validate_msg(VResult r)
{
    switch (r)
    {
    case V_OK:
        return "校验通过";
    case V_ERR_EMPTY:
        return "输入不能为空";
    case V_ERR_TOO_SHORT:
        return "内容长度不足（短于最小长度）";
    case V_ERR_TOO_LONG:
        return "内容长度超出（超过最大长度）";
    case V_ERR_SPACE:
        return "内容内部不允许出现空格";
    case V_ERR_ILLEGAL:
        return "含有规则不允许的字符";
    default:
        return "未知校验结果";
    }
}

/**
 * @brief 通用安全输入入口（普通行输入 / 掩码密码输入二合一）
 *
 *        行为：
 *          1) 先打印 tip 提示语并立即刷新；
 *          2) mask == 0 时，走 fgets 路径（普通行输入）：
 *               - 读入一整行，最多 size-1 字节，自动补 '\0'；
 *               - 剥除行末 '\n' 与 Windows CRLF 的 '\r'；
 *               - 若一行超长装不下，排空该行剩余字符直到 '\n'/EOF，
 *                 防止残留内容污染下一次输入。
 *          3) mask != 0 时，走逐字符掩码路径（密码输入）：
 *               - 使用 _getch() 不回显读取；
 *               - 支持退格删除（'\b'）；
 *               - 输入达到 size-1 后不再接收新字符；
 *               - 回车（'\r'）结束，输出一个换行。
 *
 * @param tip  提示语字符串（如 "请选择："）；传 NULL 表示不打印任何提示
 * @param buf  输出缓冲区，返回时存放不含行末换行符的内容
 * @param size 缓冲区总容量（字节，含结尾 '\0'），实际最多读入 size-1 字节
 * @param mask 掩码字符：0 表示普通输入；非 0（如 '*'）表示密码输入
 * @return 成功：读入内容的字节长度（不含结尾 '\0'，用户直接回车时为 0）；
 *         失败：buf 为 NULL / size <= 0，或读取到 EOF/出错时返回 -1
 *               （EOF 场景会把 buf 置为空串 ""）
 */
int prompt_input_ex(const char *tip, char *buf, int size, char mask)
{
    if (buf == NULL || size <= 0)
    {
        return -1;
    }
    buf[0] = '\0';

    if (tip != NULL)
    {
        printf("%s", tip);
        fflush(stdout);
    }

    /* ---------- 普通行输入：fgets 路径 ---------- */
    if (mask == 0)
    {
        int ch;
        size_t len;

        if (fgets(buf, size, stdin) == NULL)
        {
            buf[0] = '\0';
            return -1;
        }

        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            len--;
        }
        else
        {
            /* 一行没读完，排空残留 */
            while ((ch = getchar()) != '\n' && ch != EOF)
            {
            }
        }
        if (len > 0 && buf[len - 1] == '\r')
        { /* 兼容 CRLF */
            buf[len - 1] = '\0';
            len--;
        }
        return (int)len;
    }

    /* ---------- 掩码输入：逐字符 _getch 路径 ---------- */
    {
        int i = 0;
        int ch;

        while ((ch = getch()) != '\r') /* Windows 回车为 '\r' */
        {
            if (ch == '\b') /* 退格 */
            {
                if (i > 0)
                {
                    i--;
                    printf("\b \b"); /* 擦除一个掩码字符 */
                    fflush(stdout);
                }
            }
            else if (i < size - 1)
            {
                buf[i++] = (char)ch;
                putchar(mask);
                fflush(stdout);
            }
            /* 超出容量则忽略，不写入 */
        }

        buf[i] = '\0';
        putchar('\n');
        fflush(stdout);
        return i;
    }
}

/* 新建暂存目录，已存在不算错 */
static void deleted_ensure_dir(void)
{
    DELETED_MKDIR();
}

/* 会话开局：清空上轮残留，并注册进程退出时自动清空 */
/* atexit 只接受 void(*)(void)，包一层 */
static void deleted_atexit_hook(void)
{
    (void)deleted_session_cleanup();
}

static void deleted_session_init(void)
{
    if (g_deleted_inited)
    {
        return;
    }
    deleted_session_cleanup(); /* 先把可能残留的旧暂存清掉 */
    atexit(deleted_atexit_hook);
    g_deleted_inited = 1;
}

/* 读入整张映射表（栈底->栈顶），返回条数 */
static int deleted_map_load(int seq[], char paths[][LINE_BUF_SIZE],
                            int lines_no[], char stash[][LINE_BUF_SIZE])
{
    FILE *fp = fopen(DELETED_MAP, "r");
    char row[LINE_BUF_SIZE];
    int n = 0;
    if (fp == NULL)
    {
        return 0;
    }
    while (n < DELETED_MAX && fgets(row, sizeof(row), fp) != NULL)
    {
        char *p = row;
        char *f1 = strtok(p, "|");
        char *f2 = strtok(NULL, "|");
        char *f3 = strtok(NULL, "|");
        char *f4 = strtok(NULL, "|");
        if (f1 == NULL || f2 == NULL || f3 == NULL || f4 == NULL)
        {
            continue; /* 坏行跳过 */
        }
        seq[n] = atoi(f1);
        strncpy(paths[n], f2, LINE_BUF_SIZE - 1);
        paths[n][LINE_BUF_SIZE - 1] = '\0';
        lines_no[n] = atoi(f3);
        /* 暂存路径是末段，strtok 已把行尾 \n 切成独立 token 之外，手动去换行 */
        strncpy(stash[n], f4, LINE_BUF_SIZE - 1);
        stash[n][LINE_BUF_SIZE - 1] = '\0';
        size_t L = strlen(stash[n]);
        while (L > 0 && (stash[n][L - 1] == '\n' || stash[n][L - 1] == '\r'))
        {
            stash[n][--L] = '\0';
        }
        n++;
    }
    fclose(fp);
    return n;
}

/* 删一行成功后入栈：原文写暂存文件，映射追加一行 */
static void deleted_stack_push(const char *path, int line_no, const char *content)
{
    static int seq_buf[DELETED_MAX];
    static char paths[DELETED_MAX][LINE_BUF_SIZE];
    static int ln_buf[DELETED_MAX];
    static char stash[DELETED_MAX][LINE_BUF_SIZE];
    char stash_path[64];
    FILE *fp;
    int cnt, next_seq;

    deleted_session_init();
    deleted_ensure_dir();

    cnt = deleted_map_load(seq_buf, paths, ln_buf, stash);
    next_seq = (cnt > 0) ? seq_buf[cnt - 1] + 1 : 1;

    snprintf(stash_path, sizeof(stash_path),
#ifdef _WIN32
             "DELETED_INFORMATION\\%04d.txt", next_seq);
#else
             "DELETED_INFORMATION/%04d.txt", next_seq);
#endif

    /* 暂存被删行原文（保留其行尾换行，回退时原样还原） */
    fp = fopen(stash_path, "w");
    if (fp == NULL)
    {
        return;
    }
    fputs(content, fp);
    fclose(fp);

    /* 追加映射：序号|原路径|原行号|暂存路径 */
    fp = fopen(DELETED_MAP, "a");
    if (fp == NULL)
    {
        remove(stash_path);
        return;
    }
    fprintf(fp, "%d|%s|%d|%s\n", next_seq, path, line_no, stash_path);
    fclose(fp);
}

/* 读出暂存文件中的被删原文（至多一行） */
static int stash_read_content(const char *stash_path, char *out, size_t outsz)
{
    FILE *fp = fopen(stash_path, "r");
    if (fp == NULL)
    {
        return -1;
    }
    if (fgets(out, (int)outsz, fp) == NULL)
    {
        out[0] = '\0'; /* 暂存空串 */
    }
    fclose(fp);
    return 0;
}

/* LIFO 回退最后一次删行；0 成功，-1 栈空/无法还原 */
int file_undo_delete(void)
{
    static int seq_buf[DELETED_MAX];
    static char paths[DELETED_MAX][LINE_BUF_SIZE];
    static int ln_buf[DELETED_MAX];
    static char stash[DELETED_MAX][LINE_BUF_SIZE];
    static char work[MAX_LINES][LINE_BUF_SIZE];
    char content[LINE_BUF_SIZE];
    FILE *fp;
    int cnt, top, line_no, n, i;

    cnt = deleted_map_load(seq_buf, paths, ln_buf, stash);
    if (cnt <= 0)
    {
        printf("undo stack empty\n");
        return -1;
    }
    top = cnt - 1;
    line_no = ln_buf[top];

    if (stash_read_content(stash[top], content, sizeof(content)) < 0)
    {
        printf("stash missing: %s\n", stash[top]);
        return -1;
    }

    /* 原文件必须还在；空文件则直接写回这一行 */
    fp = fopen(paths[top], "r");
    if (fp == NULL)
    {
        printf("original file gone: %s\n", paths[top]);
        return -1;
    }
    fclose(fp);

    n = read_all_lines(paths[top], work);
    if (n <= 0)
    { /* 原文件为空 */
        fp = fopen(paths[top], "w");
        if (fp == NULL)
        {
            return -1;
        }
        fputs(content, fp);
        fclose(fp);
    }
    else
    {
        if (line_no < 1)
        {
            line_no = 1;
        }
        if (line_no > n + 1)
        {
            line_no = n + 1;
        }
        if (n >= MAX_LINES)
        {
            printf("too many lines, cannot undo\n");
            return -1;
        }
        for (i = n; i >= line_no; i--)
        {
            memcpy(work[i], work[i - 1], LINE_BUF_SIZE);
        }
        strncpy(work[line_no - 1], content, LINE_BUF_SIZE - 1);
        work[line_no - 1][LINE_BUF_SIZE - 1] = '\0';
        n++;
        if (write_all_lines(paths[top], work, n) != 0)
        {
            return -1;
        }
    }

    /* 弹栈：删暂存文件，映射表重写为去掉栈顶后的内容 */
    remove(stash[top]);
    fp = fopen(DELETED_MAP, "w");
    if (fp != NULL)
    {
        for (i = 0; i < cnt - 1; i++)
        {
            fprintf(fp, "%d|%s|%d|%s\n", seq_buf[i], paths[i], ln_buf[i], stash[i]);
        }
        fclose(fp);
    }
    printf("undone: %s line %d\n", paths[top], line_no);
    return 0;
}

/* 当前可回退条数 */
int deleted_stack_count(void)
{
    static int seq_buf[DELETED_MAX];
    static char paths[DELETED_MAX][LINE_BUF_SIZE];
    static int ln_buf[DELETED_MAX];
    static char stash[DELETED_MAX][LINE_BUF_SIZE];
    return deleted_map_load(seq_buf, paths, ln_buf, stash);
}

/* 清空整个暂存目录（映射驱动，无需遍历目录，跨平台）*/
int deleted_session_cleanup(void)
{
    static int seq_buf[DELETED_MAX];
    static char paths[DELETED_MAX][LINE_BUF_SIZE];
    static int ln_buf[DELETED_MAX];
    static char stash[DELETED_MAX][LINE_BUF_SIZE];
    int cnt, i;

    cnt = deleted_map_load(seq_buf, paths, ln_buf, stash);
    for (i = 0; i < cnt; i++)
    {
        remove(stash[i]);
    }
    remove(DELETED_MAP);
#ifdef _WIN32
    _rmdir(DELETED_DIR);
#else
    rmdir(DELETED_DIR);
#endif
    g_deleted_inited = 0;
    return 0;
}

/**
 * @brief 删除第 line_no 行（行号从 1 开始），并记录到撤销栈
 *
 * 与 file_delete_line 语义完全一致，只是在删除成功时
 * 额外把被删内容压入 deleted_stack，用于后续撤销。
 *
 * @param path    文件路径，如 "b.txt"
 * @param line_no 行号（从 1 开始）；传 -1 表示删除最末尾的行
 * @return 0 成功，-1 行号不存在或写入失败
 */
int file_delete_line_safe(const char *path, int line_no)
{
    /* 1. 先读一遍文件，找到实际要被删除的行号，并截留其内容 */
    static char lines[MAX_LINES][LINE_BUF_SIZE];
    int n = read_all_lines(path, lines);

    /* 文件为空 / 行号非法：不记录，直接交给原函数去打印错误并返回 -1 */
    if (n <= 0)
    {
        return file_delete_line(path, line_no);
    }

    int real_no = (line_no == -1) ? n : line_no;
    if (real_no < 1 || real_no > n)
    {
        return file_delete_line(path, line_no); /* 让原函数统一处理错误输出 */
    }

    char deleted_content[LINE_BUF_SIZE];
    memcpy(deleted_content, lines[real_no - 1], LINE_BUF_SIZE);

    /* 2. 调用原函数真正执行删除（原函数逻辑完全不动） */
    int ret = file_delete_line(path, line_no);
    if (ret != 0)
    {
        return ret; /* 删除失败，不入栈 */
    }

    /* 3. 只有删除成功才压栈，保证栈与磁盘状态一致 */
    deleted_stack_push(path, real_no, deleted_content);
    return 0;
}