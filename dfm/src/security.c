#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "security.h"
#include "file_op.h"

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
 * @brief 带提示语的安全行输入（统一入口，替代各处裸 fgets）
 *
 *        行为：
 *          1) 先打印 tip 提示语并立即刷新（无缓冲场景也能正常显示）；
 *          2) 用 fgets 从标准输入读取一整行，最多写入 size-1 个字节并自动补 '\0'；
 *          3) 自动剥除行末换行符 '\n'，并兼容 Windows CRLF 的行尾 '\r'；
 *          4) 若一行超长、缓冲区装不下，则排空该行剩余字符直到 '\n'/EOF，
 *             防止残留内容污染下一次输入。
 *
 * @param tip  提示语字符串（如 "请选择："）；传 NULL 表示不打印任何提示
 * @param buf  调用方提供的输出缓冲区，返回时存放不含行末换行符的内容
 * @param size 缓冲区总容量（字节，含结尾 '\0'），实际最多读入 size-1 字节
 * @return 成功：读入内容的字节长度（不含结尾 '\0'，用户直接回车时为 0）；
 *         失败：buf 为 NULL / size<=0，或 fgets 读到 EOF/出错时返回 -1
 *               （EOF 场景会同时把 buf 置为空串 ""）
 */
int prompt_input(const char *tip, char *buf, int size)
{
    int ch;
    size_t len;

    if (buf == NULL || size <= 0)
    {
        return -1;
    }
    if (tip != NULL)
    {
        printf("%s", tip);
        fflush(stdout);
    }
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
