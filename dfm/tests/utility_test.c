#include <stdio.h>
#include <string.h>
#include "utility.h"

#define STUDENT_FIXTURE "dfm/build/utility-students.tmp"
#define GRADE_FIXTURE "dfm/build/utility-grades.tmp"
#define DUPLICATE_FIXTURE "dfm/build/utility-duplicate-students.tmp"
#define INVALID_FIXTURE "dfm/build/utility-invalid-students.tmp"
#define USER_FIXTURE "dfm/build/utility-users.tmp"
#define LOGIN_LOG "dfm/build/utility-login.tmp"
#define DELETE_LOG "dfm/build/utility-delete.tmp"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int write_fixture(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }
    if (fputs(content, file) == EOF) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int main(void)
{
    Student students[4];
    Student student_matches[4];
    GradeRecord grades[4];
    GradeRecord grade_matches[4];
    GradeRecord stable_grades[3] = {0};
    CourseStatistic statistics[4];
    GradeRecord calculated = {0};
    double earned_credit_total = 0.0;
    size_t student_count = 0;
    size_t grade_count = 0;
    size_t statistic_count = 0;
    FILE *file;
    char line[512];
    size_t login_log_lines = 0;

    remove(STUDENT_FIXTURE);
    remove(GRADE_FIXTURE);
    remove(DUPLICATE_FIXTURE);
    remove(INVALID_FIXTURE);
    remove(USER_FIXTURE);
    remove(LOGIN_LOG);
    remove(DELETE_LOG);

    CHECK(write_fixture(STUDENT_FIXTURE,
        "# student header\n"
        "1001 Alice F 101 13800138000\n"
        "1002 Bob M 101 13900139000\n"
        "1003 Carol F 102 13700137000\n"));
    CHECK(utility_load_students(STUDENT_FIXTURE, students, 2, &student_count) ==
          UTILITY_CAPACITY_ERROR);
    CHECK(student_count == 0);
    CHECK(utility_validate_phone("13800138000"));
    CHECK(!utility_validate_phone("12800138000"));
    CHECK(!utility_validate_phone("1380013800x"));
    CHECK(!utility_validate_phone(NULL));
    CHECK(utility_load_students(STUDENT_FIXTURE, students, 4, &student_count) == UTILITY_OK);
    CHECK(student_count == 3);
    CHECK(strcmp(students[0].id, "1001") == 0);
    CHECK(strcmp(students[0].name, "Alice") == 0);
    CHECK(strcmp(students[0].dorm, "101") == 0);
    CHECK(student_count == 3);
    CHECK(utility_find_student(students, student_count, "1002", &student_matches[0]) == UTILITY_OK);
    CHECK(strcmp(student_matches[0].name, "Bob") == 0);
    CHECK(utility_find_student(students, student_count, "100", &student_matches[0]) ==
          UTILITY_NOT_FOUND);
    CHECK(write_fixture(DUPLICATE_FIXTURE,
        "2001 Alex F 201 13800138000\n"
        "2002 Alex M 202 13900139000\n"));
    CHECK(utility_load_students(DUPLICATE_FIXTURE, student_matches, 4,
                                &student_count) == UTILITY_OK);
    CHECK(utility_find_student(student_matches, student_count, "Alex",
                                &students[0]) == UTILITY_DUPLICATE);
    CHECK(write_fixture(INVALID_FIXTURE,
        "3001 Dana F 301 12800138000\n"));
    CHECK(utility_load_students(INVALID_FIXTURE, students, 4, &student_count) ==
          UTILITY_FORMAT_ERROR);
    CHECK(utility_load_students(STUDENT_FIXTURE, students, 4, &student_count) == UTILITY_OK);
    CHECK(utility_find_student(students, student_count, "Carol", &student_matches[0]) == UTILITY_OK);
    CHECK(utility_find_students_by_dorm(students, student_count, "101",
                                         student_matches, 4, &statistic_count) == UTILITY_OK);
    CHECK(statistic_count == 2);
    CHECK(strcmp(student_matches[1].name, "Bob") == 0);
    CHECK(utility_find_students_by_dorm(students, student_count, "101",
                                         student_matches, 1, &statistic_count) ==
          UTILITY_CAPACITY_ERROR);
    CHECK(statistic_count == 2);

    CHECK(write_fixture(GRADE_FIXTURE,
        "# grade header\n"
        "1001 MTH Math 3 80 -1 90 87 2.4\n"
        "1002 MTH Math 3 70 -1 80 77 2.25\n"
        "1001 SCI Science 2 90 90 90 90 2\n"));
    CHECK(utility_load_grades(GRADE_FIXTURE, grades, 4, &grade_count) == UTILITY_OK);
    CHECK(grade_count == 3);

    calculated.credit = 3.0f;
    calculated.usual_score = 80.0f;
    calculated.lab_score = -1.0f;
    calculated.exam_score = 90.0f;
    CHECK(utility_calculate_grade(&calculated) == UTILITY_OK);
    CHECK(calculated.total_score > 86.99f && calculated.total_score < 87.01f);
    CHECK(calculated.earned_credit > 2.39f && calculated.earned_credit < 2.41f);

    CHECK(utility_sort_grades(grades, grade_count, GRADE_SORT_TOTAL_SCORE,
                               GRADE_SORT_DESCENDING) == UTILITY_OK);
    CHECK(strcmp(grades[0].course_id, "SCI") == 0);
    CHECK(utility_sort_grades(grades, grade_count, GRADE_SORT_TOTAL_SCORE,
                               GRADE_SORT_ASCENDING) == UTILITY_OK);
    CHECK(strcmp(grades[0].student_id, "1002") == 0);
    CHECK(utility_course_statistics(grades, grade_count, statistics, 4,
                                     &statistic_count) == UTILITY_OK);
    CHECK(statistic_count == 2);
    CHECK(strcmp(statistics[0].course_id, "MTH") == 0);
    CHECK(statistics[0].student_count == 2);
    CHECK(statistics[0].average_score > 81.99 && statistics[0].average_score < 82.01);
    CHECK(utility_course_statistics(grades, grade_count, statistics, 1,
                                     &statistic_count) == UTILITY_CAPACITY_ERROR);

    strcpy(stable_grades[0].student_id, "first");
    strcpy(stable_grades[1].student_id, "second");
    strcpy(stable_grades[2].student_id, "third");
    stable_grades[0].total_score = 80.0f;
    stable_grades[1].total_score = 80.0f;
    stable_grades[2].total_score = 70.0f;
    CHECK(utility_sort_grades(stable_grades, 3, GRADE_SORT_TOTAL_SCORE,
                               GRADE_SORT_DESCENDING) == UTILITY_OK);
    CHECK(strcmp(stable_grades[0].student_id, "first") == 0);
    CHECK(strcmp(stable_grades[1].student_id, "second") == 0);
    CHECK(utility_sort_grades(stable_grades, 3, (GradeSortKey)99,
                               GRADE_SORT_ASCENDING) == UTILITY_INVALID_ARGUMENT);

    CHECK(utility_get_student_grades(grades, grade_count, "1001", grade_matches,
                                      4, &statistic_count,
                                      &earned_credit_total) == UTILITY_OK);
    CHECK(statistic_count == 2);
    CHECK(earned_credit_total > 4.39 && earned_credit_total < 4.41);
    CHECK(utility_get_student_grades(grades, grade_count, "1001", grade_matches,
                                      1, &statistic_count,
                                      &earned_credit_total) == UTILITY_CAPACITY_ERROR);
    CHECK(statistic_count == 2);
    CHECK(utility_get_student_grades(grades, grade_count, "9999", grade_matches,
                                      4, &statistic_count,
                                      &earned_credit_total) == UTILITY_OK);
    CHECK(statistic_count == 0 && earned_credit_total == 0.0);

    calculated.usual_score = 101.0f;
    CHECK(utility_calculate_grade(&calculated) == UTILITY_INVALID_RECORD);
    calculated.usual_score = 80.0f;

    memset(&calculated, 0, sizeof(calculated));
    strcpy(calculated.student_id, "1001");
    strcpy(calculated.course_id, "ART");
    strcpy(calculated.course_name, "Art");
    calculated.credit = 2.0f;
    calculated.usual_score = 100.0f;
    calculated.lab_score = -1.0f;
    calculated.exam_score = 100.0f;
    CHECK(utility_record_grade(STUDENT_FIXTURE, GRADE_FIXTURE, &calculated) == UTILITY_OK);
    CHECK(calculated.total_score == 100.0f && calculated.earned_credit == 2.0f);
    CHECK(utility_load_grades(GRADE_FIXTURE, grades, 4, &grade_count) == UTILITY_OK);
    CHECK(grade_count == 4);
    strcpy(calculated.student_id, "9999");
    CHECK(utility_record_grade(STUDENT_FIXTURE, GRADE_FIXTURE, &calculated) ==
          UTILITY_NOT_FOUND);
    strcpy(calculated.student_id, "1001");

    CHECK(utility_delete_student(STUDENT_FIXTURE, GRADE_FIXTURE,
                                  DELETE_LOG, "9999") == UTILITY_NOT_FOUND);
    CHECK(utility_delete_student(STUDENT_FIXTURE, GRADE_FIXTURE,
                                  DELETE_LOG, "bad id") == UTILITY_INVALID_RECORD);

    CHECK(utility_delete_student(STUDENT_FIXTURE, GRADE_FIXTURE,
                                  DELETE_LOG, "1001") == UTILITY_OK);
    CHECK(utility_load_students(STUDENT_FIXTURE, students, 4, &student_count) == UTILITY_OK);
    CHECK(student_count == 2);
    CHECK(strcmp(students[0].id, "1002") == 0);
    CHECK(utility_load_grades(GRADE_FIXTURE, grades, 4, &grade_count) == UTILITY_OK);
    CHECK(grade_count == 1);
    CHECK(strcmp(grades[0].student_id, "1002") == 0);
    file = fopen(DELETE_LOG, "r");
    CHECK(file != NULL);
    CHECK(fgets(line, sizeof(line), file) != NULL);
    CHECK(fgets(line, sizeof(line), file) != NULL);
    CHECK(strstr(line, "1001") != NULL);
    fclose(file);

    CHECK(utility_auth_register(USER_FIXTURE, "alice_01", "correct horse battery") == UTILITY_OK);
    CHECK(utility_auth_register(USER_FIXTURE, "ab", "correct horse battery") ==
          UTILITY_INVALID_ARGUMENT);
    CHECK(utility_auth_register(USER_FIXTURE, "alice_01", "correct horse battery") == UTILITY_DUPLICATE);
    CHECK(utility_auth_login(USER_FIXTURE, LOGIN_LOG, "alice_01",
                              "correct horse battery") == UTILITY_OK);
    CHECK(utility_auth_login(USER_FIXTURE, LOGIN_LOG, "alice_01",
                              "wrong password") == UTILITY_AUTH_FAILED);

    file = fopen(USER_FIXTURE, "r");
    CHECK(file != NULL);
    CHECK(fgets(line, sizeof(line), file) != NULL);
    fclose(file);
    CHECK(strstr(line, "correct horse battery") == NULL);

    file = fopen(LOGIN_LOG, "r");
    CHECK(file != NULL);
    while (fgets(line, sizeof(line), file) != NULL) {
        login_log_lines++;
    }
    fclose(file);
    CHECK(login_log_lines == 2);

    remove(STUDENT_FIXTURE);
    remove(GRADE_FIXTURE);
    remove(DUPLICATE_FIXTURE);
    remove(INVALID_FIXTURE);
    remove(USER_FIXTURE);
    remove(LOGIN_LOG);
    remove(DELETE_LOG);
    puts("utility business tests passed");
    return 0;
}