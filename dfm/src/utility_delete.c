#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utility.h"
#include "file_op.h"

static int collect_matching_lines(const char *path, const char *student_id,
                                  int *line_numbers,
                                  char lines[][LINE_BUF_SIZE],
                                  size_t capacity, size_t *count)
{
    /* 保存匹配记录的原始内容和行号，供日志记录及后续删除使用。 */
    FILE *file = fopen(path, "r");
    char line[LINE_BUF_SIZE];
    int line_number = 0;
    size_t found = 0;

    if (file == NULL) {
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        char id[UTILITY_ID_SIZE];
        size_t length = strlen(line);
        line_number++;
        if (length == sizeof(line) - 1 && line[length - 1] != '\n' &&
            fgetc(file) != EOF) {
            fclose(file);
            return -1;
        }
        if (sscanf(line, "%31s", id) == 1 && strcmp(id, student_id) == 0) {
            if (found == capacity || length >= LINE_BUF_SIZE) {
                fclose(file);
                return -1;
            }
            line_numbers[found] = line_number;
            memcpy(lines[found], line, length + 1);
            found++;
        }
    }
    int read_error = ferror(file);
    int close_error = fclose(file);
    if (read_error || close_error != 0) {
        return -1;
    }
    *count = found;
    return 0;
}

static int valid_student_id(const char *student_id)
{
    /* 限定编号字符，避免把表头或其他文本当成学号删除。 */
    const unsigned char *cursor = (const unsigned char *)student_id;
    if (student_id == NULL || *student_id == '\0') {
        return 0;
    }
    while (*cursor != '\0') {
        if (!isalnum(*cursor) && *cursor != '_' && *cursor != '-') {
            return 0;
        }
        cursor++;
    }
    return 1;
}

static int append_deleted_data(const char *log_path, const char *student_line,
                               char grade_lines[][LINE_BUF_SIZE],
                               size_t grade_count)
{
    /* 先把学生和成绩原始记录追加到删除日志。 */
    FILE *log = fopen(log_path, "a");
    size_t i;

    if (log == NULL) {
        return -1;
    }
    if (fprintf(log, "[student]\n%s", student_line) < 0) {
        fclose(log);
        return -1;
    }
    if (strchr(student_line, '\n') == NULL && fputc('\n', log) == EOF) {
        fclose(log);
        return -1;
    }
    for (i = 0; i < grade_count; i++) {
        if (fprintf(log, "[grade]\n%s", grade_lines[i]) < 0 ||
            (strchr(grade_lines[i], '\n') == NULL && fputc('\n', log) == EOF)) {
            fclose(log);
            return -1;
        }
    }
    return fclose(log) == 0 ? 0 : -1;
}

UtilityResult utility_delete_student(const char *student_file,
                                       const char *grade_file,
                                       const char *delete_log_file,
                                       const char *student_id)
{
    /* 先收集并记录数据，再从末行向前删除成绩，避免行号偏移。 */
    static int student_line_numbers[UTILITY_MAX_RECORDS];
    static int grade_line_numbers[UTILITY_MAX_RECORDS];
    static char student_lines[UTILITY_MAX_RECORDS][LINE_BUF_SIZE];
    static char grade_lines[UTILITY_MAX_RECORDS][LINE_BUF_SIZE];
    size_t student_count = 0;
    size_t grade_count = 0;
    size_t i;

    if (student_file == NULL || grade_file == NULL || delete_log_file == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    if (!valid_student_id(student_id)) {
        return UTILITY_INVALID_RECORD;
    }
    if (collect_matching_lines(student_file, student_id, student_line_numbers,
                               student_lines, UTILITY_MAX_RECORDS,
                               &student_count) != 0 ||
        collect_matching_lines(grade_file, student_id, grade_line_numbers,
                               grade_lines, UTILITY_MAX_RECORDS,
                               &grade_count) != 0) {
        return UTILITY_IO_ERROR;
    }
    if (student_count == 0) {
        return UTILITY_NOT_FOUND;
    }
    if (student_count != 1) {
        return UTILITY_DUPLICATE;
    }
    if (append_deleted_data(delete_log_file, student_lines[0], grade_lines,
                            grade_count) != 0) {
        return UTILITY_IO_ERROR;
    }

    for (i = grade_count; i > 0; i--) {
        if (file_delete_line(grade_file, grade_line_numbers[i - 1]) != 0) {
            return UTILITY_IO_ERROR;
        }
    }
    if (file_delete_line(student_file, student_line_numbers[0]) != 0) {
        return UTILITY_IO_ERROR;
    }
    return UTILITY_OK;
}