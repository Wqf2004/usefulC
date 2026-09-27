#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define CHANGE_DIR(path) _chdir(path)
#define CREATE_DIR(path) _mkdir(path)
#define GET_CWD(buffer, size) _getcwd(buffer, (int)(size))
#define PROCESS_ID _getpid
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define CHANGE_DIR(path) chdir(path)
#define CREATE_DIR(path) mkdir(path, 0700)
#define GET_CWD(buffer, size) getcwd(buffer, size)
#define PROCESS_ID getpid
#endif

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "UI test failed at line %d: %s\n", __LINE__, #condition); \
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

static int file_contains(const char *path, const char *text)
{
    char line[512];
    FILE *file = fopen(path, "r");
    int found = 0;

    if (file == NULL) {
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, text) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}

int main(void)
{
    char original_dir[1024];
    char fixture_dir[128];
    char data_dir[160];
    char input_path[192];
    char output_path[192];
    char student_path[192];
    char grade_path[192];
    char user_path[192];
    char login_path[192];
    unsigned long process_id = (unsigned long)PROCESS_ID();
    int result;
    FILE *user_file;
    char user_line[512];

    CHECK(GET_CWD(original_dir, sizeof(original_dir)) != NULL);
    snprintf(fixture_dir, sizeof(fixture_dir), "dfm/build/ui-test-%lu", process_id);
    snprintf(data_dir, sizeof(data_dir), "%s/data", fixture_dir);
    snprintf(input_path, sizeof(input_path), "%s/input.tmp", fixture_dir);
    snprintf(output_path, sizeof(output_path), "%s/output.tmp", fixture_dir);
    snprintf(student_path, sizeof(student_path), "%s/a.txt", data_dir);
    snprintf(grade_path, sizeof(grade_path), "%s/b.txt", data_dir);
    snprintf(user_path, sizeof(user_path), "%s/users.txt", data_dir);
    snprintf(login_path, sizeof(login_path), "%s/login.log", data_dir);
    CHECK(CREATE_DIR(fixture_dir) == 0);
    CHECK(CREATE_DIR(data_dir) == 0);
    CHECK(write_fixture(input_path,
        "\n"
        "invalid\n"
        "9\n"
        "2\n"
        "ui_test\n"
        "1234567\n"
        "2\n"
        "ui_test\n"
        "TestPassword123\n"
        "2\n"
        "ui_test\n"
        "TestPassword123\n"
        "1\n"
        "ui_test\n"
        "WrongPassword123\n"
        "1\n"
        "ui_test\n"
        "TestPassword123\n"
        "8\n"
        "4\n"
        "\n"
        "9999\n"
        "BIO WITHSPACE\n"
        "BIO\n"
        "Biology\n"
        "-1\n"
        "3\n"
        "101\n"
        "0\n"
        "-0.5\n"
        "-1\n"
        "100.1\n"
        "0\n"
        "4\n"
        "1001\n"
        "BIO\n"
        "Biology\n"
        "0\n"
        "0\n"
        "-1\n"
        "0\n"
        "4\n"
        "1001\n"
        "MAX\n"
        "MaxScore\n"
        "3\n"
        "100\n"
        "100\n"
        "100\n"
        "3\n"
        "1001\n"
        "0\n"
        "0\n"));
    CHECK(write_fixture(student_path,
        "# id name gender dorm phone\n"
        "1001 Alice F 101 13800138000\n"));
    CHECK(write_fixture(grade_path,
        "# student_id course_id course_name credit usual lab exam total earned\n"
        "1001 MTH Math 3 80 -1 90 87 2.4\n"));

    CHECK(CHANGE_DIR(fixture_dir) == 0);
    if (freopen("input.tmp", "r", stdin) == NULL ||
        freopen("output.tmp", "w", stdout) == NULL) {
        fprintf(stderr, "unable to redirect UI test streams\n");
        return 1;
    }
    result = ui_run();
    if (fflush(stdout) != 0 || fclose(stdout) != 0) {
        fprintf(stderr, "unable to finish captured UI output\n");
        return 1;
    }
    fclose(stdin);
    CHECK(CHANGE_DIR(original_dir) == 0);

    CHECK(result == 0);
    CHECK(file_contains(output_path, "9999"));
    CHECK(file_contains(output_path, "data/a.txt"));
    CHECK(file_contains(output_path, "MTH"));
    CHECK(file_contains(output_path, "Math"));
    CHECK(file_contains(output_path, "BIO Biology"));
    CHECK(file_contains(output_path, "MAX MaxScore"));
    CHECK(file_contains(output_path, "87.00"));
    CHECK(file_contains(output_path, "100.00"));
    CHECK(file_contains(output_path, "5.40"));
    CHECK(file_contains(grade_path,
                        "1001 BIO Biology 0.00 0.00 -1.00 0.00 0.00 0.00"));
    CHECK(file_contains(grade_path,
                        "1001 MAX MaxScore 3.00 100.00 100.00 100.00 100.00 3.00"));
    user_file = fopen(user_path, "r");
    CHECK(user_file != NULL);
    CHECK(fgets(user_line, sizeof(user_line), user_file) != NULL);
    CHECK(strncmp(user_line, "ui_test$", 8) == 0);
    CHECK(strstr(user_line, "TestPassword123") == NULL);
    CHECK(fgets(user_line, sizeof(user_line), user_file) == NULL);
    fclose(user_file);
    CHECK(file_contains(login_path, "ui_test SUCCESS"));
    CHECK(file_contains(login_path, "ui_test FAILURE"));

    remove(input_path);
    remove(output_path);
    remove(student_path);
    remove(grade_path);
    remove(user_path);
    remove(login_path);
#ifdef _WIN32
    _rmdir(data_dir);
    _rmdir(fixture_dir);
#else
    rmdir(data_dir);
    rmdir(fixture_dir);
#endif

    fprintf(stderr, "UI boundary and end-to-end tests passed\n");
    return 0;
}
