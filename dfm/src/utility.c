#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utility.h"
#include "file_op.h"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

#define AUTH_SALT_SIZE 16
#define AUTH_HASH_SIZE 32
#define AUTH_ITERATIONS 120000u
#define AUTH_LINE_SIZE 256

static int split_fields(char *line, char **fields, size_t max_fields)
{
    /* 按空白字符切分字段，并直接在原字符串中插入结束符。 */
    size_t count = 0;
    char *cursor = line;

    while (*cursor != '\0') {
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        if (count == max_fields) {
            return -1;
        }
        fields[count++] = cursor;
        while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor != '\0') {
            *cursor++ = '\0';
        }
    }
    return (int)count;
}

static int copy_field(char *destination, size_t size, const char *source)
{
    /* 字段过长或为空时拒绝复制，避免静默截断。 */
    size_t length = strlen(source);
    if (length == 0 || length >= size) {
        return 0;
    }
    memcpy(destination, source, length + 1);
    return 1;
}

static int parse_float(const char *text, float *value)
{
    /* 要求整段文本都是有限的浮点数。 */
    char *end = NULL;
    float parsed;

    errno = 0;
    parsed = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(parsed)) {
        return 0;
    }
    *value = parsed;
    return 1;
}

static int valid_record_id(const char *id)
{
    /* 文件中的编号只允许字母、数字、下划线和连字符。 */
    if (*id == '\0') {
        return 0;
    }
    for (; *id != '\0'; id++) {
        unsigned char ch = (unsigned char)*id;
        if (!isalnum(ch) && ch != '_' && ch != '-') {
            return 0;
        }
    }
    return 1;
}

int utility_validate_phone(const char *phone)
{
    /* 先检查长度和号段，再逐位确认都是数字。 */
    size_t i;
    if (phone == NULL || strlen(phone) != 11 || phone[0] != '1' || phone[1] < '3' || phone[1] > '9') {
        return 0;
    }
    for (i = 0; i < 11; i++) {
        if (!isdigit((unsigned char)phone[i])) {
            return 0;
        }
    }
    return 1;
}

static int line_is_header(const char *line)
{
    /* 表头通常以非编号字符开头，用此规则兼容中英文表头。 */
    const unsigned char *cursor = (const unsigned char *)line;
    while (*cursor != '\0' && isspace(*cursor)) {
        cursor++;
    }
    return *cursor != '\0' && !isalnum(*cursor) && *cursor != '_' && *cursor != '-';
}

static int read_line(FILE *file, char *line, size_t size)
{
    /* 拒绝超出缓冲区的行，避免把一条记录误读成多条。 */
    size_t length;
    if (fgets(line, (int)size, file) == NULL) {
        return feof(file) ? 0 : -1;
    }
    length = strlen(line);
    if (length == size - 1 && line[length - 1] != '\n' && !feof(file)) {
        int ch;
        while ((ch = fgetc(file)) != '\n' && ch != EOF) {
        }
        return -1;
    }
    return 1;
}

UtilityResult utility_load_students(const char *path, Student *students,
                                      size_t capacity, size_t *count)
{
    /* 逐行解析学生记录，并校验字段、手机号和数组容量。 */
    FILE *file;
    char line[LINE_BUF_SIZE];
    size_t loaded = 0;
    int first_nonempty = 1;
    int status;

    if (path == NULL || students == NULL || count == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    *count = 0;
    file = fopen(path, "r");
    if (file == NULL) {
        return UTILITY_IO_ERROR;
    }

    while ((status = read_line(file, line, sizeof(line))) > 0) {
        char *fields[5];
        Student student = {0};
        int field_count;

        field_count = split_fields(line, fields, 5);
        if (field_count == 0) {
            continue;
        }
        if (first_nonempty && line_is_header(line)) {
            first_nonempty = 0;
            continue;
        }
        first_nonempty = 0;
        if (field_count != 5 ||
            !copy_field(student.id, sizeof(student.id), fields[0]) ||
            !copy_field(student.name, sizeof(student.name), fields[1]) ||
            !copy_field(student.gender, sizeof(student.gender), fields[2]) ||
            !copy_field(student.dorm, sizeof(student.dorm), fields[3]) ||
            !copy_field(student.phone, sizeof(student.phone), fields[4]) ||
            !valid_record_id(student.id) || !utility_validate_phone(student.phone)) {
            status = -1;
            break;
        }
        if (loaded == capacity) {
            fclose(file);
            return UTILITY_CAPACITY_ERROR;
        }
        students[loaded++] = student;
    }

    if (fclose(file) != 0 || status < 0) {
        return status < 0 ? UTILITY_FORMAT_ERROR : UTILITY_IO_ERROR;
    }
    *count = loaded;
    return UTILITY_OK;
}

static int parse_grade_fields(char **fields, GradeRecord *grade)
{
    /* 解析九个成绩字段，并检查分数和学分范围。 */
    if (!copy_field(grade->student_id, sizeof(grade->student_id), fields[0]) ||
        !copy_field(grade->course_id, sizeof(grade->course_id), fields[1]) ||
        !copy_field(grade->course_name, sizeof(grade->course_name), fields[2]) ||
        !valid_record_id(grade->student_id) || !valid_record_id(grade->course_id) ||
        !parse_float(fields[3], &grade->credit) ||
        !parse_float(fields[4], &grade->usual_score) ||
        !parse_float(fields[5], &grade->lab_score) ||
        !parse_float(fields[6], &grade->exam_score) ||
        !parse_float(fields[7], &grade->total_score) ||
        !parse_float(fields[8], &grade->earned_credit)) {
        return 0;
    }
    return grade->credit >= 0.0f &&
           grade->usual_score >= 0.0f && grade->usual_score <= 100.0f &&
           grade->lab_score >= -1.0f && grade->lab_score <= 100.0f &&
           grade->exam_score >= 0.0f && grade->exam_score <= 100.0f &&
           grade->total_score >= 0.0f && grade->total_score <= 100.0f &&
           grade->earned_credit >= 0.0f && grade->earned_credit <= grade->credit;
}

UtilityResult utility_load_grades(const char *path, GradeRecord *grades,
                                    size_t capacity, size_t *count)
{
    /* 逐行读取成绩记录，跳过可选表头并验证每一列。 */
    FILE *file;
    char line[LINE_BUF_SIZE];
    size_t loaded = 0;
    int first_nonempty = 1;
    int status;

    if (path == NULL || grades == NULL || count == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    *count = 0;
    file = fopen(path, "r");
    if (file == NULL) {
        return UTILITY_IO_ERROR;
    }

    while ((status = read_line(file, line, sizeof(line))) > 0) {
        char *fields[9];
        GradeRecord grade = {0};
        int field_count = split_fields(line, fields, 9);

        if (field_count == 0) {
            continue;
        }
        if (first_nonempty && line_is_header(line)) {
            first_nonempty = 0;
            continue;
        }
        first_nonempty = 0;
        if (field_count != 9 || !parse_grade_fields(fields, &grade)) {
            status = -1;
            break;
        }
        if (loaded == capacity) {
            fclose(file);
            return UTILITY_CAPACITY_ERROR;
        }
        grades[loaded++] = grade;
    }

    if (fclose(file) != 0 || status < 0) {
        return status < 0 ? UTILITY_FORMAT_ERROR : UTILITY_IO_ERROR;
    }
    *count = loaded;
    return UTILITY_OK;
}

UtilityResult utility_calculate_grade(GradeRecord *grade)
{
    /* 输入有效后复用 grade 模块计算综合成绩和实得学分。 */
    if (grade == NULL || !isfinite(grade->credit) || grade->credit < 0.0f ||
        !isfinite(grade->usual_score) || grade->usual_score < 0.0f ||
        grade->usual_score > 100.0f || !isfinite(grade->lab_score) ||
        grade->lab_score < -1.0f || grade->lab_score > 100.0f ||
        !isfinite(grade->exam_score) || grade->exam_score < 0.0f ||
        grade->exam_score > 100.0f) {
        return UTILITY_INVALID_RECORD;
    }
    grade->total_score = grade_calc_total(grade->usual_score, grade->lab_score,
                                          grade->exam_score);
    grade->earned_credit = grade_calc_credit(grade->credit, grade->total_score);
    return UTILITY_OK;
}

UtilityResult utility_sort_grades(GradeRecord *grades, size_t count,
                                    GradeSortKey key, GradeSortOrder order)
{
    /* 插入排序会保持相同排序值的记录原有顺序。 */
    size_t i;
    if ((grades == NULL && count != 0) ||
        (key != GRADE_SORT_TOTAL_SCORE && key != GRADE_SORT_EARNED_CREDIT) ||
        (order != GRADE_SORT_ASCENDING && order != GRADE_SORT_DESCENDING)) {
        return UTILITY_INVALID_ARGUMENT;
    }

    for (i = 1; i < count; i++) {
        GradeRecord current = grades[i];
        float current_value = key == GRADE_SORT_TOTAL_SCORE
            ? current.total_score : current.earned_credit;
        size_t position = i;

        while (position > 0) {
            float previous_value = key == GRADE_SORT_TOTAL_SCORE
                ? grades[position - 1].total_score : grades[position - 1].earned_credit;
            int should_move = order == GRADE_SORT_ASCENDING
                ? previous_value > current_value : previous_value < current_value;
            if (!should_move) {
                break;
            }
            grades[position] = grades[position - 1];
            position--;
        }
        grades[position] = current;
    }
    return UTILITY_OK;
}

UtilityResult utility_course_statistics(const GradeRecord *grades,
                                          size_t grade_count,
                                          CourseStatistic *statistics,
                                          size_t capacity,
                                          size_t *count)
{
    /* 按课程编号汇总综合成绩，最后计算每门课的平均分。 */
    size_t course_count = 0;
    size_t i;

    if ((grades == NULL && grade_count != 0) || statistics == NULL || count == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    *count = 0;

    for (i = 0; i < grade_count; i++) {
        size_t j;
        for (j = 0; j < course_count; j++) {
            if (strcmp(statistics[j].course_id, grades[i].course_id) == 0) {
                break;
            }
        }
        if (j == course_count) {
            if (course_count == capacity) {
                return UTILITY_CAPACITY_ERROR;
            }
            if (!copy_field(statistics[j].course_id, sizeof(statistics[j].course_id),
                            grades[i].course_id) ||
                !copy_field(statistics[j].course_name, sizeof(statistics[j].course_name),
                            grades[i].course_name)) {
                return UTILITY_INVALID_RECORD;
            }
            statistics[j].student_count = 0;
            statistics[j].average_score = 0.0;
            course_count++;
        }
        statistics[j].average_score += grades[i].total_score;
        statistics[j].student_count++;
    }

    for (i = 0; i < course_count; i++) {
        statistics[i].average_score /= (double)statistics[i].student_count;
    }
    *count = course_count;
    return UTILITY_OK;
}

static int valid_username(const char *username)
{
    /* 用户名限制为规定长度内的 ASCII 字母、数字和下划线。 */
    size_t length;
    const unsigned char *cursor;
    if (username == NULL) {
        return 0;
    }
    length = strlen(username);
    if (length < 3 || length > UTILITY_ID_SIZE) {
        return 0;
    }
    for (cursor = (const unsigned char *)username; *cursor != '\0'; cursor++) {
        if (!isalnum(*cursor) && *cursor != '_') {
            return 0;
        }
    }
    return 1;
}

static int valid_password(const char *password)
{
    /* 注册和登录使用相同的密码长度规则。 */
    size_t length;
    if (password == NULL) {
        return 0;
    }
    length = strlen(password);
    return length >= 8 && length <= 128;
}

static void bytes_to_hex(const unsigned char *bytes, size_t length, char *hex)
{
    /* 将随机盐或摘要转换为便于写入文本文件的十六进制。 */
    static const char digits[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < length; i++) {
        hex[i * 2] = digits[bytes[i] >> 4];
        hex[i * 2 + 1] = digits[bytes[i] & 0x0f];
    }
    hex[length * 2] = '\0';
}

static int hex_to_bytes(const char *hex, unsigned char *bytes, size_t length)
{
    /* 读取摘要前先检查长度和每个十六进制字符。 */
    size_t i;
    if (strlen(hex) != length * 2) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        int high = isdigit((unsigned char)hex[i * 2])
            ? hex[i * 2] - '0' : tolower((unsigned char)hex[i * 2]) - 'a' + 10;
        int low = isdigit((unsigned char)hex[i * 2 + 1])
            ? hex[i * 2 + 1] - '0' : tolower((unsigned char)hex[i * 2 + 1]) - 'a' + 10;
        if (high < 0 || high > 15 || low < 0 || low > 15) {
            return 0;
        }
        bytes[i] = (unsigned char)((high << 4) | low);
    }
    return 1;
}

static int derive_password_hash(const char *password,
                                const unsigned char salt[AUTH_SALT_SIZE],
                                unsigned char hash[AUTH_HASH_SIZE])
{
    /* 使用 PBKDF2-SHA256 和随机盐派生口令摘要。 */
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                                   NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status >= 0) {
        status = BCryptDeriveKeyPBKDF2(algorithm, (PUCHAR)password,
                                       (ULONG)strlen(password), (PUCHAR)salt,
                                       AUTH_SALT_SIZE, AUTH_ITERATIONS,
                                       hash, AUTH_HASH_SIZE, 0);
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
    return status >= 0;
#else
    (void)password;
    (void)salt;
    (void)hash;
    return 0;
#endif
}

static int secure_random(unsigned char *buffer, size_t length)
{
    /* 通过操作系统随机数接口生成盐值。 */
#ifdef _WIN32
    return BCryptGenRandom(NULL, buffer, (ULONG)length,
                           BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
#else
    (void)buffer;
    (void)length;
    return 0;
#endif
}

static int username_exists(FILE *file, const char *username)
{
    /* 从文件开头逐条检查用户名，防止重复注册。 */
    char line[AUTH_LINE_SIZE];
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        char *separator = strchr(line, '$');
        if (separator != NULL) {
            *separator = '\0';
            if (strcmp(line, username) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

UtilityResult utility_auth_register(const char *user_file,
                                      const char *username,
                                      const char *password)
{
    /* 生成独立盐值和摘要，只把摘要信息写入账号文件。 */
    FILE *file;
    unsigned char salt[AUTH_SALT_SIZE];
    unsigned char hash[AUTH_HASH_SIZE];
    char salt_hex[AUTH_SALT_SIZE * 2 + 1];
    char hash_hex[AUTH_HASH_SIZE * 2 + 1];
    int close_status;

    if (user_file == NULL || !valid_username(username) || !valid_password(password)) {
        return UTILITY_INVALID_ARGUMENT;
    }
    file = fopen(user_file, "a+");
    if (file == NULL) {
        return UTILITY_IO_ERROR;
    }
    if (username_exists(file, username)) {
        fclose(file);
        return UTILITY_DUPLICATE;
    }
    if (!secure_random(salt, sizeof(salt)) || !derive_password_hash(password, salt, hash)) {
        fclose(file);
        return UTILITY_CRYPTO_ERROR;
    }
    bytes_to_hex(salt, sizeof(salt), salt_hex);
    bytes_to_hex(hash, sizeof(hash), hash_hex);
    if (fseek(file, 0, SEEK_END) != 0 ||
        fprintf(file, "%s$%s$%s$%u\n", username, salt_hex, hash_hex,
                AUTH_ITERATIONS) < 0) {
        fclose(file);
        return UTILITY_IO_ERROR;
    }
    close_status = fclose(file);
    return close_status == 0 ? UTILITY_OK : UTILITY_IO_ERROR;
}

static int constant_time_equal(const unsigned char *left,
                               const unsigned char *right, size_t length)
{
    /* 遍历完整个摘要再判断，避免按首个差异提前退出。 */
    unsigned char difference = 0;
    size_t i;
    for (i = 0; i < length; i++) {
        difference |= left[i] ^ right[i];
    }
    return difference == 0;
}

static int verify_password(FILE *file, const char *username, const char *password)
{
    /* 读取账号记录，重新派生摘要后与保存值比较。 */
    char line[AUTH_LINE_SIZE];
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[4];
        unsigned char salt[AUTH_SALT_SIZE];
        unsigned char expected_hash[AUTH_HASH_SIZE];
        unsigned char actual_hash[AUTH_HASH_SIZE];
        char *newline = strpbrk(line, "\r\n");
        char *cursor;
        size_t field_count = 1;

        if (newline != NULL) {
            *newline = '\0';
        }
        fields[0] = line;
        for (cursor = line; *cursor != '\0'; cursor++) {
            if (*cursor == '$') {
                *cursor++ = '\0';
                if (field_count == 4) {
                    field_count = 0;
                    break;
                }
                fields[field_count++] = cursor;
            }
        }
        if (field_count != 4 || strcmp(fields[0], username) != 0 ||
            !hex_to_bytes(fields[1], salt, sizeof(salt)) ||
            !hex_to_bytes(fields[2], expected_hash, sizeof(expected_hash)) ||
            strtoul(fields[3], NULL, 10) != AUTH_ITERATIONS) {
            continue;
        }
        return derive_password_hash(password, salt, actual_hash) &&
               constant_time_equal(expected_hash, actual_hash, sizeof(actual_hash));
    }
    return 0;
}

static UtilityResult append_login_log(const char *log_file, const char *username,
                                       int authenticated)
{
    /* 将登录时间、用户名和结果追加到日志文件。 */
    FILE *file = fopen(log_file, "a");
    time_t now;
    struct tm *local_time;
    char timestamp[32];
    const char *safe_name = valid_username(username) ? username : "invalid-user";

    if (file == NULL) {
        return UTILITY_IO_ERROR;
    }
    now = time(NULL);
    local_time = localtime(&now);
    if (local_time == NULL || strftime(timestamp, sizeof(timestamp),
                                       "%Y-%m-%d %H:%M:%S", local_time) == 0 ||
        fprintf(file, "%s %s %s\n", timestamp, safe_name,
                authenticated ? "SUCCESS" : "FAILURE") < 0) {
        fclose(file);
        return UTILITY_IO_ERROR;
    }
    return fclose(file) == 0 ? UTILITY_OK : UTILITY_IO_ERROR;
}

UtilityResult utility_auth_login(const char *user_file, const char *log_file,
                                   const char *username, const char *password)
{
    /* 完成凭据校验后，无论成功或失败都记录本次尝试。 */
    FILE *file = NULL;
    int authenticated = 0;
    UtilityResult log_result;

    if (user_file == NULL || log_file == NULL) {
        return UTILITY_INVALID_ARGUMENT;
    }
    if (valid_username(username) && valid_password(password)) {
        file = fopen(user_file, "r");
        if (file != NULL) {
            authenticated = verify_password(file, username, password);
            fclose(file);
        }
    }
    log_result = append_login_log(log_file, username, authenticated);
    if (log_result != UTILITY_OK) {
        return log_result;
    }
    return authenticated ? UTILITY_OK : UTILITY_AUTH_FAILED;
}