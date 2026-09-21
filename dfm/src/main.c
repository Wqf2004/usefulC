#include <stdio.h>
#include <string.h>
#include "file_op.h"
#include "security.h"

#define LINE_LEN 260

/* 演示一：文件删除流程（无痕备份 + 二次确认 + file_del） */
static void demo_file_delete(void)
{
    char path[LINE_LEN];

    backupGhostFiles();   /* 无痕模式：先备份 a/b.txt 为 .ghost */

    printf("\n======== 演示一：文件删除流程（二次确认） ========\n");
    printf("系统示例路径：%s\n", FILENAME_A);
    printf("流程：输入路径 -> 二次确认 -> 调用 file_del 删除\n");

    for (;;) {
        if (prompt_input("\n请输入要删除的文件路径（输入 q 返回上级菜单）：",
                         path, sizeof(path)) < 0) {
            break;
        }
        input_trim(path);                 /* 统一入口已去换行，这里再去首尾空白 */

        if (strcmp(path, "q") == 0) {
            break;
        }
        if (path[0] == '\0') {
            printf("  [提示] 路径不能为空，请重新输入\n");
            continue;
        }

        if (!confirm_dangerous(path, ACT_DELETE)) {
            printf("  [取消] 已放弃删除\n");
            continue;
        }
        if (file_del(path) == 0) {
            printf("  [成功] 文件已删除：%s\n", path);
        } else {
            printf("  [失败] 删除未成功：%s\n", path);
        }
    }
}

/* 根据预设编号填充一条规则；返回 1 有效，0 无效 */
static int build_preset_rule(int no, InputRule *rule, const char **desc)
{
    rule_init(rule);
    switch (no) {
        case 1: /* 纯数字：学号/手机号 */
            rule->allowed = CT_DIGIT;
            rule->min_len = 4;
            rule->max_len = 11;
            *desc = "纯数字（4~11 位，如学号/手机号）";
            break;
        case 2: /* 用户名：字母数字下划线 */
            rule->allowed = CT_ALNUM | CT_UNDER;
            rule->min_len = 3;
            rule->max_len = 16;
            *desc = "字母/数字/下划线（3~16 位用户名）";
            break;
        case 3: /* 中文姓名 */
            rule->allowed = CT_CHINESE;
            rule->min_len = 2;
            rule->max_len = 8;
            *desc = "纯中文（2~8 个汉字，如姓名）";
            break;
        case 4: /* 昵称：中文 + 字母数字混合，允许内部空格 */
            rule->allowed = CT_CHINESE | CT_ALNUM;
            rule->min_len = 1;
            rule->max_len = 16;
            rule->allow_space = 1;
            *desc = "中文/字母/数字混合（最多16字符，允许空格）";
            break;
        default:
            return 0;
    }
    return 1;
}

/* 演示二：通用可配置输入内容校验框架 */
static void demo_validator(void)
{
    char sel[16];
    char text[LINE_LEN];
    InputRule rule;
    const char *desc = NULL;
    int no;
    VResult vr;

    printf("\n======== 演示二：通用输入内容校验框架 ========\n");
    printf("可配置“允许的字符类型 + 长度区间 + 是否允许空格”\n");
    printf("  1 - 纯数字（4~11 位）\n");
    printf("  2 - 字母/数字/下划线（3~16 位）\n");
    printf("  3 - 纯中文姓名（2~8 字）\n");
    printf("  4 - 中英文混合昵称（<=16 字符，允许空格）\n");

    for (;;) {
        if (prompt_input("\n选择规则编号（输入 q 返回上级菜单）：", sel, sizeof(sel)) < 0) {
            break;
        }
        if (strcmp(sel, "q") == 0) {
            break;
        }
        if (sscanf(sel, "%d", &no) != 1 || !build_preset_rule(no, &rule, &desc)) {
            printf("  [提示] 无效的规则编号，请重新选择\n");
            continue;
        }

        printf("  当前规则：%s\n", desc);
        if (prompt_input("请输入待校验内容：", text, sizeof(text)) < 0) {
            break;
        }

        vr = input_validate(text, &rule);
        if (vr == V_OK) {
            printf("  [通过] %s（共 %d 个字符）内容：%s\n",
                   validate_msg(vr), gbk_strlen(text), text);
        } else {
            printf("  [拦截] %s\n", validate_msg(vr));
        }
    }
}

int main(void)
{
    char choice[16];

    for (;;) {
        printf("\n############ dfm 输入校验演示平台 ############\n");
        printf("  1 - 文件删除流程演示（二次确认）\n");
        printf("  2 - 通用输入内容校验框架演示\n");
        printf("  q - 退出程序\n");

        if (prompt_input("请选择功能：", choice, sizeof(choice)) < 0) {
            break;
        }
        if (strcmp(choice, "q") == 0) {
            break;
        } else if (strcmp(choice, "1") == 0) {
            demo_file_delete();
        } else if (strcmp(choice, "2") == 0) {
            demo_validator();
        } else {
            printf("  [提示] 无效选择，请重新输入\n");
        }
    }

    printf("程序结束，再见！\n");
    return 0;
}
