#ifndef __SECURITY_DEFINED
#define __SECURITY_DEFINED

#define FILENAME_A "../data/a.txt"                            // 学生信息存储文件路径
#define FILENAME_B "../data/b.txt"                            // 成绩信息存储文件路径
#define DELETED_INFORMATION "../data/deleted_information.txt" // 已删除的学生信息存储文件路径
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
 * sanitize 函数用于原地去掉首尾空白（空格/制表/\r/\n）
 * confirm_dangerous 函数用于危险操作二次确认（action 取 DangerousAction 枚举）
 */

void backupGhostFiles();
void input_trim(char *s);
int confirm_dangerous(const char *path, DangerousAction action);

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

#endif
