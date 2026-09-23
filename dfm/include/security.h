#ifndef __SECURITY_DEFINED
#define __SECURITY_DEFINED

#ifdef _WIN32
#include <direct.h>      /* _mkdir, _rmdir */
#else
#include <sys/stat.h>    /* mkdir */
#include <unistd.h>      /* rmdir */
#endif

#define FILENAME_A "../data/a.txt"                            // 学生信息存储文件路径
#define FILENAME_B "../data/b.txt"                            // 成绩信息存储文件路径
#define FILENAME_A_GHOST "../data/a.txt.ghost"                // 无痕模式下的学生信息备份路径
#define FILENAME_B_GHOST "../data/b.txt.ghost"                // 无痕模式下的成绩信息备份路径

#define CT_DIGIT 0x01u                  /* 数字 0-9 */
#define CT_LOWER 0x02u                  /* 小写字母 a-z */
#define CT_UPPER 0x04u                  /* 大写字母 A-Z */
#define CT_LETTER (CT_LOWER | CT_UPPER) /* 全部字母 */
#define CT_ALNUM (CT_LETTER | CT_DIGIT) /* 字母 + 数字 */
#define CT_UNDER 0x10u                  /* 下划线 _ */
#define CT_DASH 0x20u                   /* 连字符 - */
#define CT_DOT 0x40u                    /* 点 . */
#define CT_CHINESE 0x80u                /* GBK 中文汉字 */

/* 危险操作类型：供 confirm_dangerous 的 action 参数使用 */
typedef enum Dangerous_Action
{
    ACT_DELETE = 0, /* 删除文件/记录 */
    ACT_OVERWRITE,  /* 覆盖已有内容 */
    ACT_CLEAR,      /* 清空内容 */
    ACT_RENAME,     /* 改名（可能覆盖同名目标） */
    ACT_COUNT       /* 动作总数，仅作名称表越界防御，勿当真实动作传入 */
} DangerousAction;

/* 校验结果码：0 通过，非 0 为原因 */
typedef enum Value_Result
{
    V_OK = 0,        /* 校验通过 */
    V_ERR_EMPTY,     /* 内容为空（或全是空白） */
    V_ERR_TOO_SHORT, /* 短于规则要求的最小长度 */
    V_ERR_TOO_LONG,  /* 长于规则允许的最大长度 */
    V_ERR_SPACE,     /* 内部出现了不被允许的空格 */
    V_ERR_ILLEGAL    /* 含规则不允许的字符 */
} VResult;

/* 一条可配置的校验规则 */
typedef struct Input_Rule
{
    unsigned allowed; /* 允许的字符类型位掩码（如 CT_ALNUM） */
    int min_len;      /* 最小字符数，0 表示不限制下限 */
    int max_len;      /* 最大字符数，<=0 表示不限制上限 */
    int allow_space;  /* 是否允许“内容内部”的空格：1 允许 / 0 拒绝 */
    int trim;         /* 是否自动去除首尾空白：1 去除 / 0 保留 */
} InputRule;

/*
 * 杂项功能函数
 *
 * backupGhostFiles 函数用于进入无痕模式，对相关文件备份为.ghost文件
 * restoreGhostFiles 函数用于退出无痕模式，还原业务数据文件
 * sanitize 函数用于原地去掉首尾空白（空格/制表/\r/\n）
 * confirm_dangerous 函数用于危险操作二次确认（action 取 DangerousAction 枚举）
 */

void backupGhostFiles();
void restoreGhostFiles();
void input_trim(char *s);
int confirm_dangerous(const char *path, DangerousAction action);

/*
 * 安全绑定操作（二次确认闸门 + 真实文件操作 焊死）
 *
 * 设计意图：危险动作不允许业务层"裸调" file_op 原语绕过确认。
 *           凡是删除/改名/覆盖/清空，一律走下面 safe_* 包装：
 *             内部先 confirm_dangerous() 弹闸门，
 *             用户确认(y/Y)才真正调用 file_op，取消/EOF 则原样不动。
 *
 * 返回值约定（四者统一）：
 *    0  已确认并执行成功
 *   -1  已确认但底层操作失败（见 perror 提示）
 *   -2  用户取消或确认时 EOF（文件保持原样，未做任何改动）
 */
int safe_file_delete(const char *path);                      /* ACT_DELETE -> file_del */
int safe_file_rename(const char *old_path, const char *new_path); /* ACT_RENAME -> file_rename */
int safe_file_overwrite(const char *path, const char *content);   /* ACT_OVERWRITE -> file_overwrite */
int safe_file_clear(const char *path);                       /* ACT_CLEAR -> file_clear */

/*
 * 通用可配置输入内容校验框架
 *
 * 用法三步：
 *   1) InputRule rule; rule_init(&rule);          // 先给一份安全默认值
 *   2) 按需要配置 rule.allowed / min_len / ...     // 允许什么、多长
 *   3) input_validate(buf, &rule);                 // 原地裁剪并校验
 *
 * 字符类型用“位掩码”描述，可用 | 自由组合，例如：
 *   CT_DIGIT | CT_UNDER        只允许数字和下划线
 *   CT_ALNUM                   允许字母和数字
 *   CT_CHINESE | CT_ALNUM      允许中文和字母数字混合
 * 长度一律按“字符数”计算（GBK 中文双字节算 1 个字符）。
 */

void rule_init(InputRule *rule);                        
void input_trim(char *s);                                
int gbk_strlen(const char *s);                           
VResult input_validate(char *s, const InputRule *rule); 
const char *validate_msg(VResult r);                     
int prompt_input(const char *tip, char *buf, int size);

/* 把文件所有行读进 lines 二维数组并返回行数；文件不存在返回 0，文件最后一行无换行时补 '\n' */
/* ========================================================================
 *  删除行撤销栈（会话内 LIFO）
 *  暂存目录 : DELETED_INFORMATION/
 *  映射文件 : DELETED_INFORMATION/deleted_information.txt
 *  映射格式 : 序号|原路径|原行号|暂存文件路径   （末条=栈顶）
 *  被删原文 : DELETED_INFORMATION/0001.txt ... （一删一文件，避免分隔符冲突）
 *  一次进程视为一次会话：首次删除清空残留开局，进程正常退出自动清空。
 * ======================================================================== */

#ifdef _WIN32
#define DELETED_DIR "DELETED_INFORMATION"
#define DELETED_MAP "DELETED_INFORMATION\\deleted_information.txt"
#define DELETED_MKDIR() _mkdir(DELETED_DIR)
#else
#define DELETED_DIR "DELETED_INFORMATION"
#define DELETED_MAP "DELETED_INFORMATION/deleted_information.txt"
#define DELETED_MKDIR() mkdir(DELETED_DIR, 0755)
#endif

#define DELETED_MAX 1000 /* 单次会话最多记录的删行条数 */

int file_undo_delete(void);
int deleted_stack_count(void);
int deleted_session_cleanup(void);

#endif
