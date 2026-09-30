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
