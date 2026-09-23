#include <stdio.h>
#include <string.h>
#include "grade.h"
#include "file_op.h"
#include "security.h"

/* 等级 -> 中文字符串 */
const char *grade_level_name(GradeLevel lv)
{
    switch (lv) {
        case LEVEL_EXCELLENT: return "优秀";
        case LEVEL_GOOD:      return "良好";
        case LEVEL_MEDIUM:    return "中等";
        case LEVEL_PASS:      return "及格";
        case LEVEL_FAIL:      return "不及格";
        default:              return "未知";
    }
}

/* 综合成绩 -> 等级：>=90 优秀，>=80 良好，>=70 中等，>=60 及格，其余不及格 */
GradeLevel get_level(float total)
{
    if (total >= 90.0f) {
        return LEVEL_EXCELLENT;
    } else if (total >= 80.0f) {
        return LEVEL_GOOD;
    } else if (total >= 70.0f) {
        return LEVEL_MEDIUM;
    } else if (total >= 60.0f) {
        return LEVEL_PASS;
    }
    return LEVEL_FAIL;
}

/* 综合成绩：lab == -1 表示无实验，此时平时占比提高到 0.30 */
float grade_calc_total(float usual, float lab, float exam)
{
    if (lab != -1.0f) {
        return usual * 0.15f + lab * 0.15f + exam * 0.70f;
    }
    return usual * 0.30f + exam * 0.70f;
}

/* 实得学分：沿用等级学分制折算 */
float grade_calc_credit(float credit, float total)
{
    if (total >= 90.0f) {
        return credit * 1.0f;
    } else if (total >= 80.0f) {
        return credit * 0.8f;
    } else if (total >= 70.0f) {
        return credit * 0.75f;
    } else if (total >= 60.0f) {
        return credit * 0.6f;
    }
    return 0.0f;
}

/* 从缓冲区解析一个 float 并做范围校验，返回 1 有效，0 无效 */
static int parse_score(const char *buf, float *out, float low, float high)
{
    float v;

    if (sscanf(buf, "%f", &v) != 1) {
        return 0;
    }
    /* 实验成绩允许 -1（表示无实验），其余情况必须在 [low, high] 内 */
    if (low < 0.0f && v == -1.0f) {
        *out = v;
        return 1;
    }
    if (v < 0.0f || v < low || v > high) {
        return 0;
    }
    *out = v;
    return 1;
}

/* 菜单演示：成绩录入完整流程（全程零 FILE*，只调 file_op 封装） */
void demo_grade_entry(void)
{
    char id[64], courseId[32], courseName[64], in[64], line[512];
    float credit, usual, lab, exam, total, creditScore;
    GradeLevel lv;

    printf("\n======== 成绩录入（业务层 file_op，全程零 FILE*） ========\n");

    /* 学生信息文件必须存在，否则无法校验学号 */
    if (!file_exists(FILENAME_A)) {
        printf("  [提示] 学生信息文件 %s 不存在，请先录入学生信息\n", FILENAME_A);
        return;
    }
    printf("  当前学生信息清单：\n");
    file_view(FILENAME_A);

    /* 成绩文件不存在则新建并写入表头（与 data/b.txt 表头逐字一致） */
    if (!file_exists(FILENAME_B)) {
        file_create(FILENAME_B);
        file_add(FILENAME_B, "学号 课程编号 课程名称 学分 平时成绩 实验成绩 卷面成绩 综合成绩 实得学分", 0);
    }

    for (;;) {
        if (prompt_input("\n请输入学号（输入 q 返回上级菜单）：", id, sizeof(id)) < 0) {
            break;
        }
        input_trim(id);
        if (strcmp(id, "q") == 0) {
            break;
        }
        if (id[0] == '\0') {
            printf("  [提示] 学号不能为空，请重新输入\n");
            continue;
        }

        /* 学号必须已存在于学生信息文件 */
        if (file_find(FILENAME_A, id) <= 0) {
            printf("  错误：该学号不存在于学生信息文件\n");
            continue;
        }

        if (prompt_input("请输入课程编号：", courseId, sizeof(courseId)) < 0) {
            break;
        }
        input_trim(courseId);
        if (prompt_input("请输入课程名称：", courseName, sizeof(courseName)) < 0) {
            break;
        }
        input_trim(courseName);

        if (prompt_input("请输入学分：", in, sizeof(in)) < 0) {
            break;
        }
        if (sscanf(in, "%f", &credit) != 1) {
            printf("  输入无效，请重新录入本条\n");
            continue;
        }

        if (prompt_input("请输入平时成绩：", in, sizeof(in)) < 0) {
            break;
        }
        if (!parse_score(in, &usual, 0.0f, 100.0f)) {
            printf("  输入无效，请重新录入本条\n");
            continue;
        }

        if (prompt_input("请输入实验成绩（无实验请输入 -1）：", in, sizeof(in)) < 0) {
            break;
        }
        if (!parse_score(in, &lab, -1.0f, 100.0f)) {
            printf("  输入无效，请重新录入本条\n");
            continue;
        }

        if (prompt_input("请输入卷面成绩：", in, sizeof(in)) < 0) {
            break;
        }
        if (!parse_score(in, &exam, 0.0f, 100.0f)) {
            printf("  输入无效，请重新录入本条\n");
            continue;
        }

        /* 计算综合成绩、实得学分与等级（纯计算，无 I/O） */
        total = grade_calc_total(usual, lab, exam);
        creditScore = grade_calc_credit(credit, total);
        lv = get_level(total);

        /* 拼成与 b.txt 表头列顺序一致的 9 列文本（空格分隔，%.1f 对齐旧实现） */
        snprintf(line, sizeof(line),
                 "%s %s %s %.1f %.1f %.1f %.1f %.1f %.1f",
                 id, courseId, courseName,
                 credit, usual, lab, exam, total, creditScore);

        /* 追加写入成绩文件（mode=0：先写内容再补换行） */
        if (file_add(FILENAME_B, line, 0) == 0) {
            printf("  [成功] 已保存：综合成绩=%.1f，等级=%s\n", total, grade_level_name(lv));
        } else {
            printf("  [失败] 写入失败\n");
        }
    }
}
