#ifndef __GRADE_DEFINED
#define __GRADE_DEFINED

/* 成绩等级枚举：综合成绩对应的五级评价 */
typedef enum Grade_Level
{
    LEVEL_EXCELLENT, /* 优秀 */
    LEVEL_GOOD,      /* 良好 */
    LEVEL_MEDIUM,    /* 中等 */
    LEVEL_PASS,      /* 及格 */
    LEVEL_FAIL       /* 不及格 */
} GradeLevel;

/* 等级 -> 中文字符串，非法值返回"未知" */
const char *grade_level_name(GradeLevel lv);

/* 综合成绩 -> 等级（纯计算，无 I/O） */
GradeLevel get_level(float total);

/* 综合成绩：有实验 平时*0.15+实验*0.15+卷面*0.70；无实验(lab==-1) 平时*0.30+卷面*0.70 */
float grade_calc_total(float usual, float lab, float exam);

/* 实得学分：按综合成绩所在的等级段折算 */
float grade_calc_credit(float credit, float total);

#endif
