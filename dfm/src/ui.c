#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "security.h"
#include "ui.h"
#include "utility.h"

static int input_closed;
static const char *student_file = FILENAME_A;
static const char *grade_file = FILENAME_B;
static const char *user_file = "../data/users.txt";
static const char *login_log_file = "../data/login.log";
static const char *delete_log_file = "../data/deleted_students.log";

static void configure_data_paths(void)
{
    FILE *probe = fopen(FILENAME_A, "r");

    if (probe != NULL) {
        fclose(probe);
        return;
    }
    probe = fopen("data/a.txt", "r");
    if (probe != NULL) {
        fclose(probe);
        student_file = "data/a.txt";
        grade_file = "data/b.txt";
        user_file = "data/users.txt";
        login_log_file = "data/login.log";
        delete_log_file = "data/deleted_students.log";
    }
}

/*
 * 函数：read_required
 * 功能：循环提示用户输入，直到获得一个满足条件的有效字符串；
 *       如果输入流被关闭（如 EOF），则返回失败。
 *
 * 参数：
 *   prompt     - 提示用户输入的字符串，例如 "请输入用户名: "
 *   buffer     - 用于存放用户输入内容的字符数组
 *   capacity   - buffer 的总容量（字节数），包括结尾的 '\0'
 *   token_only - 若为非 0，表示输入中不能包含任何空白字符（空格、制表符等）；
 *                若为 0，则允许包含空格
 *
 * 返回值：
 *   1 - 成功读取到符合要求的输入
 *   0 - 输入流已关闭（EOF），无法继续读取
 *
 * 说明：
 *   - 输入为空或长度超过 capacity - 1 时，会提示错误并要求重新输入。
 *   - 当 token_only 为真时，输入中若出现任何空白字符，也会提示错误并重新输入。
 *   - 函数内部依赖全局变量 input_closed ，用于标记输入流是否已关闭。
 *   - 调用者需保证 capacity > 1，否则任何输入都会被判为“过长”。
 */
static int read_required(const char *prompt, char *buffer, int capacity, int token_only, int ispassword)
{
    int length;

    for (;;) 
    {
        if (ispassword)
        {
            length = prompt_input_ex(prompt, buffer, capacity, '*');
        }
        else
        {
            length = prompt_input_ex(prompt, buffer, capacity, 0);
        }
        
        if (length < 0) 
        {
            input_closed = 1;
            return 0;
        }
        if (length == 0 || length >= capacity - 1) 
        {
            puts("输入不能为空或过长。");
            continue;
        }
        if (token_only) 
        {
            int i;
            for (i = 0; i < length; i++) 
            {
                if (isspace((unsigned char)buffer[i])) 
                {
                    break;
                }
            }
            if (i != length) 
            {
                puts("该字段不能包含空格。");
                continue;
            }
        }
        return 1;
    }
}

static int read_choice(const char *prompt, long minimum, long maximum, long *choice)
{
    char buffer[64];
    char *end;
    long value;

    for (;;) {
        if (!read_required(prompt, buffer, sizeof(buffer), 1, 0)) {
            return 0;
        }
        errno = 0;
        value = strtol(buffer, &end, 10);
        if (errno == 0 && end != buffer && *end == '\0' &&
            value >= minimum && value <= maximum) {
            *choice = value;
            return 1;
        }
        printf("请输入 %ld 到 %ld 之间的整数。\n", minimum, maximum);
    }
}

static int read_float(const char *prompt, float minimum, float maximum,
                      int lab_score, float *value)
{
    char buffer[64];
    char *end;
    float parsed;

    for (;;) {
        if (!read_required(prompt, buffer, sizeof(buffer), 1, 0)) {
            return 0;
        }
        errno = 0;
        parsed = strtof(buffer, &end);
        if (errno == 0 && end != buffer && *end == '\0' && isfinite(parsed) &&
            parsed >= minimum && parsed <= maximum &&
            (!lab_score || parsed >= 0.0f || parsed == -1.0f)) {
            *value = parsed;
            return 1;
        }
        puts("数值格式或范围无效，请重新输入。");
    }
}

static void show_result(UtilityResult result)
{
    switch (result) {
        case UTILITY_OK: puts("操作成功。"); break;
        case UTILITY_INVALID_ARGUMENT: puts("参数无效。"); break;
        case UTILITY_IO_ERROR: puts("文件读写失败，请检查数据文件和运行目录。"); break;
        case UTILITY_FORMAT_ERROR: puts("数据文件格式错误或包含无效记录。"); break;
        case UTILITY_CAPACITY_ERROR: puts("记录数量超过系统容量。"); break;
        case UTILITY_DUPLICATE: puts("结果不唯一或记录重复。"); break;
        case UTILITY_AUTH_FAILED: puts("用户名或密码错误。"); break;
        case UTILITY_CRYPTO_ERROR: puts("密码加密服务不可用。"); break;
        case UTILITY_INVALID_RECORD: puts("记录内容无效。"); break;
        case UTILITY_NOT_FOUND: puts("没有找到对应记录。"); break;
        default: puts("发生未知业务错误。"); break;
    }
}

static void print_student(const Student *student)
{
    printf("学号：%s  姓名：%s  性别：%s  宿舍：%s  电话：%s\n",
           student->id, student->name, student->gender,
           student->dorm, student->phone);
}

static void print_grade(const GradeRecord *grade)
{
    printf("学号：%s  课程：%s %s  学分：%.2f  平时：%.2f  实验：%.2f  "
           "卷面：%.2f  综合：%.2f  实得：%.2f\n",
           grade->student_id, grade->course_id, grade->course_name,
           grade->credit, grade->usual_score, grade->lab_score,
           grade->exam_score, grade->total_score, grade->earned_credit);
}

static void find_student(void)
{
    static Student students[UTILITY_MAX_RECORDS];
    Student student;
    int count = 0;
    char key[UTILITY_NAME_SIZE + 1];
    UtilityResult result;

    result = utility_load_students(student_file, students, UTILITY_MAX_RECORDS, &count);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    if (!read_required("输入学号或姓名：", key, sizeof(key), 1, 0)) {
        return;
    }
    result = utility_find_student(students, count, key, &student);
    if (result == UTILITY_OK) {
        print_student(&student);
    } else {
        show_result(result);
    }
}

static void find_students_by_dorm(void)
{
    static Student students[UTILITY_MAX_RECORDS];
    static Student matches[UTILITY_MAX_RECORDS];
    int student_count = 0;
    int match_count = 0;
    int i;
    char dorm[UTILITY_DORM_SIZE + 1];
    UtilityResult result;

    result = utility_load_students(student_file, students, UTILITY_MAX_RECORDS,
                                   &student_count);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    if (!read_required("输入宿舍号：", dorm, sizeof(dorm), 1, 0)) {
        return;
    }
    result = utility_find_students_by_dorm(students, student_count, dorm,
                                            matches, UTILITY_MAX_RECORDS,
                                            &match_count);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    for (i = 0; i < match_count; i++) {
        print_student(&matches[i]);
    }
    printf("共找到 %d 名学生。\n", match_count);
}

static void query_student_grades(void)
{
    static GradeRecord grades[UTILITY_MAX_RECORDS];
    static GradeRecord matches[UTILITY_MAX_RECORDS];
    int grade_count = 0;
    int match_count = 0;
    int i;
    double earned_credit_total = 0.0;
    char student_id[UTILITY_ID_SIZE + 1];
    UtilityResult result;

    result = utility_load_grades(grade_file, grades, UTILITY_MAX_RECORDS,
                                 &grade_count);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    if (!read_required("输入学号：", student_id, sizeof(student_id), 1, 0)) {
        return;
    }
    result = utility_get_student_grades(grades, grade_count, student_id, matches,
                                         UTILITY_MAX_RECORDS, &match_count,
                                         &earned_credit_total);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    for (i = 0; i < match_count; i++) {
        print_grade(&matches[i]);
    }
    printf("共修 %d 科，实得总学分：%.2f\n", match_count, earned_credit_total);
}

static void record_grade(void)
{
    GradeRecord grade = {0};
    char student_id[UTILITY_ID_SIZE + 1];
    char course_id[UTILITY_COURSE_ID_SIZE + 1];
    char course_name[UTILITY_COURSE_NAME_SIZE + 1];
    UtilityResult result;

    if (!read_required("学号：", student_id, sizeof(student_id), 1, 0) ||
        !read_required("课程编号：", course_id, sizeof(course_id), 1, 0) ||
        !read_required("课程名称：", course_name, sizeof(course_name), 1, 0) ||
        !read_float("学分：", 0.0f, FLT_MAX, 0, &grade.credit) ||
        !read_float("平时成绩（0-100）：", 0.0f, 100.0f, 0, &grade.usual_score) ||
        !read_float("实验成绩（无实验输入 -1）：", -1.0f, 100.0f, 1,
                    &grade.lab_score) ||
        !read_float("卷面成绩（0-100）：", 0.0f, 100.0f, 0, &grade.exam_score)) {
        return;
    }
    strcpy(grade.student_id, student_id);
    strcpy(grade.course_id, course_id);
    strcpy(grade.course_name, course_name);
    result = utility_record_grade(student_file, grade_file, &grade);
    if (result == UTILITY_NOT_FOUND) {
        printf("学号 %s 不存在于学生信息文件 %s，请先手动补充学生记录后再录入成绩。\n",
               student_id, student_file);
        return;
    }
    show_result(result);
    if (result == UTILITY_OK) {
        print_grade(&grade);
    }
}

static void delete_student(void)
{
    char student_id[UTILITY_ID_SIZE + 1];
    char confirmation[16];

    if (!read_required("输入要删除的学生学号：", student_id,
                       sizeof(student_id), 1, 0) ||
        !read_required("确认删除？输入 y 确认，其它输入取消：",
                       confirmation, sizeof(confirmation), 1, 0)) {
        return;
    }
    if (strcmp(confirmation, "y") != 0 && strcmp(confirmation, "Y") != 0) {
        puts("已取消。");
        return;
    }
    show_result(utility_delete_student(student_file, grade_file,
                                       delete_log_file, student_id));
}

static void sort_grades(void)
{
    static GradeRecord grades[UTILITY_MAX_RECORDS];
    int count = 0;
    int i;
    long key;
    long order;
    UtilityResult result;

    result = utility_load_grades(grade_file, grades, UTILITY_MAX_RECORDS, &count);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    if (!read_choice("排序字段：1 综合成绩，2 实得学分：", 1, 2, &key) ||
        !read_choice("排序方向：1 升序，2 降序：", 1, 2, &order)) {
        return;
    }
    result = utility_sort_grades(grades, count,
        key == 1 ? GRADE_SORT_TOTAL_SCORE : GRADE_SORT_EARNED_CREDIT,
        order == 1 ? GRADE_SORT_ASCENDING : GRADE_SORT_DESCENDING);
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    for (i = 0; i < count; i++) {
        print_grade(&grades[i]);
    }
}

static void show_course_statistics(void)
{
    static GradeRecord grades[UTILITY_MAX_RECORDS];
    static CourseStatistic statistics[UTILITY_MAX_RECORDS];
    int grade_count = 0;
    int statistic_count = 0;
    int i;
    UtilityResult result;

    result = utility_load_grades(grade_file, grades, UTILITY_MAX_RECORDS,
                                 &grade_count);
    if (result == UTILITY_OK) {
        result = utility_course_statistics(grades, grade_count, statistics,
                                            UTILITY_MAX_RECORDS,
                                            &statistic_count);
    }
    if (result != UTILITY_OK) {
        show_result(result);
        return;
    }
    for (i = 0; i < statistic_count; i++) {
        printf("课程：%s %s  人数：%d  平均综合成绩：%.2f\n",
               statistics[i].course_id, statistics[i].course_name,
               statistics[i].student_count, statistics[i].average_score);
    }
}

static int run_main_menu(void)
{
    long choice;

    while (!input_closed) {
        puts("\n======== 学生信息管理系统 ========");
        puts("1. 按学号或姓名查询学生");
        puts("2. 按宿舍查询学生");
        puts("3. 查询学生成绩");
        puts("4. 录入成绩");
        puts("5. 删除学生及其成绩");
        puts("6. 成绩排序");
        puts("7. 课程平均分统计");
        puts("0. 注销");
        if (!read_choice("请选择：", 0, 7, &choice)) {
            break;
        }
        switch (choice) {
            case 1: find_student(); break;
            case 2: find_students_by_dorm(); break;
            case 3: query_student_grades(); break;
            case 4: record_grade(); break;
            case 5: delete_student(); break;
            case 6: sort_grades(); break;
            case 7: show_course_statistics(); break;
            case 0: return 1;
        }
    }
    return 0;
}

static void register_account(void)
{
    char username[UTILITY_ID_SIZE + 2];
    char password[130];
    UtilityResult result;

    if (!read_required("用户名（3-32 位字母、数字或下划线）：",
                       username, sizeof(username), 1, 0) ||
        !read_required("密码（8-128 字节）：", password, sizeof(password), 0, 1)) 
    {
        return;
    }
    result = utility_auth_register(user_file, username, password);
    show_result(result);
}

static void login_account(void)
{
    char username[UTILITY_ID_SIZE + 2];
    char password[130];
    UtilityResult result;

    if (!read_required("用户名：", username, sizeof(username), 1, 0) ||
        !read_required("密码：", password, sizeof(password), 0, 1)) 
    {
        return;
    }
    result = utility_auth_login(user_file, login_log_file, username, password);
    if (result == UTILITY_OK) {
        puts("登录成功。");
        run_main_menu();
    } else {
        show_result(result);
    }
}

int ui_run(void)
{
    long choice;

    configure_data_paths();
    while (!input_closed) {
        puts("\n======== DFM 学生管理 ========");
        puts("1. 登录");
        puts("2. 注册");
        puts("0. 退出");
        if (!read_choice("请选择：", 0, 2, &choice)) {
            break;
        }
        switch (choice) {
            case 1: login_account(); break;
            case 2: register_account(); break;
            case 0: input_closed = 1; break;
        }
    }
    puts("程序结束。");
    return 0;
}