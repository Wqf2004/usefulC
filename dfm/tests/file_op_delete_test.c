#include <stdio.h>
#include <string.h>
#include "file_op.h"

#define TEST_FILE "dfm/build/file_op_delete_test.tmp"

int main(void)
{
    static const char original[] = "first\nlast";
    static const char expected[] = "last";
    static char lines[MAX_LINES][LINE_BUF_SIZE];
    char actual[16];
    FILE *fp = fopen(TEST_FILE, "wb");
    if (fp == NULL) {
        perror("create fixture");
        return 1;
    }
    if (fwrite(original, 1, sizeof(original) - 1, fp) != sizeof(original) - 1 ||
        fclose(fp) != 0) {
        perror("write fixture");
        remove(TEST_FILE);
        return 1;
    }

    if (file_delete_line(TEST_FILE, 1) != 0) {
        fprintf(stderr, "file_delete_line failed\n");
        remove(TEST_FILE);
        return 1;
    }

    fp = fopen(TEST_FILE, "rb");
    if (fp == NULL) {
        perror("open result");
        remove(TEST_FILE);
        return 1;
    }
    size_t actual_size = fread(actual, 1, sizeof(actual), fp);
    int read_error = ferror(fp);
    fclose(fp);
    remove(TEST_FILE);

    if (read_error || actual_size != sizeof(expected) - 1 ||
        memcmp(actual, expected, sizeof(expected) - 1) != 0) {
        fprintf(stderr, "expected 4 bytes without a trailing newline; got %zu bytes\n",
                actual_size);
        return 1;
    }

    fp = fopen(TEST_FILE, "wb");
    if (fp == NULL || fclose(fp) != 0) {
        perror("create empty fixture");
        remove(TEST_FILE);
        return 1;
    }
    if (read_all_lines(TEST_FILE, lines) != 0) {
        fprintf(stderr, "empty file should contain zero lines\n");
        remove(TEST_FILE);
        return 1;
    }
    remove(TEST_FILE);

    fp = fopen(TEST_FILE, "wb");
    if (fp == NULL || fwrite("tail", 1, 4, fp) != 4 || fclose(fp) != 0) {
        perror("create insertion fixture");
        remove(TEST_FILE);
        return 1;
    }
    if (file_insert_line(TEST_FILE, 2, "next") != 0) {
        fprintf(stderr, "file_insert_line failed\n");
        remove(TEST_FILE);
        return 1;
    }
    fp = fopen(TEST_FILE, "r");
    if (fp == NULL) {
        perror("open inserted result");
        remove(TEST_FILE);
        return 1;
    }
    actual_size = fread(actual, 1, sizeof(actual), fp);
    read_error = ferror(fp);
    fclose(fp);
    if (read_error || actual_size != sizeof("tail\nnext\n") - 1 ||
        memcmp(actual, "tail\nnext\n", sizeof("tail\nnext\n") - 1) != 0) {
        fprintf(stderr, "appending a line should separate it from an unterminated last line\n");
        remove(TEST_FILE);
        return 1;
    }

    fp = fopen(TEST_FILE, "wb");
    if (fp == NULL || fwrite("tail", 1, 4, fp) != 4 || fclose(fp) != 0) {
        perror("create replacement fixture");
        remove(TEST_FILE);
        return 1;
    }
    if (file_replace_line(TEST_FILE, 1, "changed") != 0) {
        fprintf(stderr, "file_replace_line failed\n");
        remove(TEST_FILE);
        return 1;
    }
    fp = fopen(TEST_FILE, "rb");
    if (fp == NULL) {
        perror("open replaced result");
        remove(TEST_FILE);
        return 1;
    }
    actual_size = fread(actual, 1, sizeof(actual), fp);
    read_error = ferror(fp);
    fclose(fp);
    remove(TEST_FILE);
    if (read_error || actual_size != sizeof("changed") - 1 ||
        memcmp(actual, "changed", sizeof("changed") - 1) != 0) {
        fprintf(stderr, "replacing the last line should preserve its missing newline\n");
        return 1;
    }

    puts("line operations preserve existing line endings and handle empty files");
    return 0;
}