#include <stdio.h>
#include <string.h>
#include "utility.h"

static int is_token(const char *text)
{
    /* 空白分隔文件中的单个字段不能包含空格或制表符。 */
    const unsigned char *cursor = (const unsigned char *)text;
    if (text == NULL || *text == '\0') {
        return 0;
    }
    while (*cursor != '\0') {
        if (*cursor <= ' ' || *cursor == 127) {
            return 0;
        }
        cursor++;
    }
    return 1;
}

UtilityResult utility_find_student(const Student *students, size_t count,
                                     const char *key, Student *student)
{
    /* 学号或姓名都按完整字段匹配；重名时提示调用方结果不唯一。 */
    size_t i;
    size_t matches = 0;

    if ((students == NULL && count != 0) || key == NULL || key[0] == '\0' ||
        student == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    for (i = 0; i < count; i++) {
        if (strcmp(students[i].id, key) == 0 || strcmp(students[i].name, key) == 0) {
            *student = students[i];
            matches++;
        }
    }
    if (matches == 0) {
        return UTILITY_NOT_FOUND;
    }
    return matches == 1 ? UTILITY_OK : UTILITY_DUPLICATE;
}

UtilityResult utility_find_students_by_dorm(const Student *students,
                                              size_t student_count,
                                              const char *dorm,
                                              Student *matches,
                                              size_t capacity,
                                              size_t *count)
{
    /* 先统计匹配数量，确认输出空间足够后再复制结果。 */
    size_t i;
    size_t found = 0;

    if ((students == NULL && student_count != 0) || dorm == NULL ||
        matches == NULL || count == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    *count = 0;
    for (i = 0; i < student_count; i++) {
        if (strcmp(students[i].dorm, dorm) == 0) {
            found++;
        }
    }
    *count = found;
    if (found > capacity) {
        return UTILITY_CAPACITY_ERROR;
    }
    found = 0;
    for (i = 0; i < student_count; i++) {
        if (strcmp(students[i].dorm, dorm) == 0) {
            matches[found++] = students[i];
        }
    }
    return UTILITY_OK;
}

UtilityResult utility_get_student_grades(const GradeRecord *grades,
                                           size_t grade_count,
                                           const char *student_id,
                                           GradeRecord *matches,
                                           size_t capacity,
                                           size_t *count,
                                           double *earned_credit_total)
{
    /* 查询该学生的所有课程记录，并累加实得学分。 */
    size_t i;
    size_t found = 0;
    double total = 0.0;

    if ((grades == NULL && grade_count != 0) || student_id == NULL ||
        student_id[0] == '\0' || matches == NULL || count == NULL ||
        earned_credit_total == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    for (i = 0; i < grade_count; i++) {
        if (strcmp(grades[i].student_id, student_id) == 0) {
            found++;
            total += grades[i].earned_credit;
        }
    }
    *count = found;
    *earned_credit_total = total;
    if (found > capacity) {
        return UTILITY_CAPACITY_ERROR;
    }
    found = 0;
    for (i = 0; i < grade_count; i++) {
        if (strcmp(grades[i].student_id, student_id) == 0) {
            matches[found++] = grades[i];
        }
    }
    return UTILITY_OK;
}

UtilityResult utility_record_grade(const char *student_file,
                                     const char *grade_file,
                                     GradeRecord *grade)
{
    /* 确认学号存在并算出成绩后，再将记录追加到成绩文件。 */
    static Student students[UTILITY_MAX_RECORDS];
    size_t student_count = 0;
    size_t i;
    int student_found = 0;
    UtilityResult result;
    FILE *file;
    long end_position;
    int write_status;
    int close_status;

    if (student_file == NULL || grade_file == NULL || grade == NULL ||
        !is_token(grade->student_id) || !is_token(grade->course_id) ||
        !is_token(grade->course_name)) {
        return UTILITY_INVALID_ARGUMENT;
    }
    result = utility_load_students(student_file, students,
                                   UTILITY_MAX_RECORDS, &student_count);
    if (result != UTILITY_OK) {
        return result;
    }
    for (i = 0; i < student_count; i++) {
        if (strcmp(students[i].id, grade->student_id) == 0) {
            student_found = 1;
            break;
        }
    }
    if (!student_found) {
        return UTILITY_NOT_FOUND;
    }
    result = utility_calculate_grade(grade);
    if (result != UTILITY_OK) {
        return result;
    }

    file = fopen(grade_file, "ab+");
    if (file == NULL) {
        return UTILITY_IO_ERROR;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (end_position = ftell(file)) < 0) {
        fclose(file);
        return UTILITY_IO_ERROR;
    }
    if (end_position == 0) {
        if (fputs("# student_id course_id course_name credit usual lab exam total earned\n",
                  file) == EOF) {
            fclose(file);
            return UTILITY_IO_ERROR;
        }
    } else {
        int last_byte;
        if (fseek(file, -1, SEEK_END) != 0 || (last_byte = fgetc(file)) == EOF ||
            fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return UTILITY_IO_ERROR;
        }
        if (last_byte != '\n' && fputc('\n', file) == EOF) {
            fclose(file);
            return UTILITY_IO_ERROR;
        }
    }
    write_status = fprintf(file, "%s %s %s %.2f %.2f %.2f %.2f %.2f %.2f\n",
                           grade->student_id, grade->course_id, grade->course_name,
                           grade->credit, grade->usual_score, grade->lab_score,
                           grade->exam_score, grade->total_score, grade->earned_credit);
    close_status = fclose(file);
    if (write_status < 0 || close_status != 0) {
        return UTILITY_IO_ERROR;
    }
    return UTILITY_OK;
}