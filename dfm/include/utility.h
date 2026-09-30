#ifndef DFM_UTILITY_H
#define DFM_UTILITY_H

#include <stddef.h>
#include "grade.h"

#define UTILITY_ID_SIZE 32
#define UTILITY_NAME_SIZE 64
#define UTILITY_GENDER_SIZE 16
#define UTILITY_DORM_SIZE 32
#define UTILITY_PHONE_SIZE 16
#define UTILITY_COURSE_ID_SIZE 32
#define UTILITY_COURSE_NAME_SIZE 96
#define UTILITY_MAX_RECORDS 1000

typedef struct Student
{
    char id[UTILITY_ID_SIZE];
    char name[UTILITY_NAME_SIZE];
    char gender[UTILITY_GENDER_SIZE];
    char dorm[UTILITY_DORM_SIZE];
    char phone[UTILITY_PHONE_SIZE];
} Student;

typedef struct GradeRecord
{
    char student_id[UTILITY_ID_SIZE];
    char course_id[UTILITY_COURSE_ID_SIZE];
    char course_name[UTILITY_COURSE_NAME_SIZE];
    float credit;
    float usual_score;
    float lab_score;
    float exam_score;
    float total_score;
    float earned_credit;
} GradeRecord;

typedef struct CourseStatistic
{
    char course_id[UTILITY_COURSE_ID_SIZE];
    char course_name[UTILITY_COURSE_NAME_SIZE];
    int student_count;
    double average_score;
} CourseStatistic;

typedef enum UtilityResult
{
    UTILITY_OK = 0,
    UTILITY_INVALID_ARGUMENT = -1,
    UTILITY_IO_ERROR = -2,
    UTILITY_FORMAT_ERROR = -3,
    UTILITY_CAPACITY_ERROR = -4,
    UTILITY_DUPLICATE = -5,
    UTILITY_AUTH_FAILED = -6,
    UTILITY_CRYPTO_ERROR = -7,
    UTILITY_INVALID_RECORD = -8,
    UTILITY_NOT_FOUND = -9
} UtilityResult;

typedef enum GradeSortKey
{
    GRADE_SORT_TOTAL_SCORE,
    GRADE_SORT_EARNED_CREDIT
} GradeSortKey;

typedef enum GradeSortOrder
{
    GRADE_SORT_ASCENDING,
    GRADE_SORT_DESCENDING
} GradeSortOrder;

/* 校验 11 位中国大陆手机号。 */
int utility_validate_phone(const char *phone);

/* 按现有空白分隔格式读取文件，允许文件包含表头。 */
UtilityResult utility_load_students(const char *path, Student *students,
                                      size_t capacity, int *count);
UtilityResult utility_load_grades(const char *path, GradeRecord *grades,
                                    size_t capacity, int *count);

/* 按学号或姓名精确查询学生，或按宿舍号查询学生列表。 */
UtilityResult utility_find_student(const Student *students, size_t count,
                                     const char *key, Student *student);
UtilityResult utility_find_students_by_dorm(const Student *students,
                                              size_t student_count,
                                              const char *dorm,
                                              Student *matches,
                                              size_t capacity,
                                              int *count);

/* 查询学生的全部成绩，并返回实得学分总和。 */
UtilityResult utility_get_student_grades(const GradeRecord *grades,
                                           size_t grade_count,
                                           const char *student_id,
                                           GradeRecord *matches,
                                           size_t capacity,
                                           int *count,
                                           double *earned_credit_total);
/* 录入成绩前自动计算综合成绩和实得学分，且学号必须存在于学生文件中。 */
UtilityResult utility_record_grade(const char *student_file,
                                     const char *grade_file,
                                     GradeRecord *grade);
/* 删除学生及其全部成绩记录，并保存被删除的数据。 */
UtilityResult utility_delete_student(const char *student_file,
                                       const char *grade_file,
                                       const char *delete_log_file,
                                       const char *student_id);

/* 按 grade.c 中的规则计算综合成绩和实得学分。 */
UtilityResult utility_calculate_grade(GradeRecord *grade);

/* 稳定排序；排序字段相同时保持原有顺序。 */
UtilityResult utility_sort_grades(GradeRecord *grades, size_t count,
                                    GradeSortKey key, GradeSortOrder order);

/* 按课程编号统计平均综合成绩；结果空间不足时返回 UTILITY_CAPACITY_ERROR。 */
UtilityResult utility_course_statistics(const GradeRecord *grades,
                                          size_t grade_count,
                                          CourseStatistic *statistics,
                                          size_t capacity,
                                          int *count);

/* 用户名为 3 到 32 位字母、数字或下划线；密码长度为 8 到 128 字节。 */
UtilityResult utility_auth_register(const char *user_file,
                                      const char *username,
                                      const char *password);
/* 登录成功或失败都会追加记录到日志文件。 */
UtilityResult utility_auth_login(const char *user_file, const char *log_file,
                                   const char *username, const char *password);

#endif