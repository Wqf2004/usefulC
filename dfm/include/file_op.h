#ifndef __FILE_OP_DEFINED
#define __FILE_OP_DEFINED

#define LINE_BUF_SIZE 1024 /* 一行最大的存储容量，单位为字节 */
#define MAX_LINES 1000     /* 一个文件最大的存储行数 */

/* ========================================================================
 * 文件本身的创建
 * 整体覆盖：清空原内容后写入新内容
 * 清空文件：保留文件本身，截断为 0 字节
 * 文件变更路径
 * 判断是否存在及复制
 * ======================================================================== */

int file_create(const char *path);   
int file_del(const char *path);
int file_overwrite(const char *path, const char *content); 
int file_clear(const char *path);                       
int file_rename(const char *old_path, const char *new_path);
int file_exists(const char *path);
int file_copy(const char *src, const char *dst);

/* ========================================================================
 *  文件"内容"级的增删改查（文本文件，按行操作）
 *  行号约定：全部从 1 开始数，跟编辑器一致
 *  约定：文本文件每行不超过 1023 字节，总行数不超过 1000 行
 *
 *      查：file_view 看全貌 / file_find 关键字找行
 *      增：file_insert_line 插入（行号超范围就追加到末尾，和 file_add 一样）
 *      删：file_delete_line 删一行
 *      改：file_replace_line 整行替换 / file_replace_str 字符串替换
 *
 *  原理：
 *      先用 "r" 把所有行 fgets 读进一个二维数组，
 *      然后进行 数组插入/删除元素 等修改数组的操作，
 *      最后用 "w" 打开文件（会自动清空），把数组整体 fputs 写回去。
 * ======================================================================== */

int file_add(const char *path, const char *content, int mode);
int file_view(const char *path);
int file_find(const char *path, const char *keyword);
int file_insert_line(const char *path, int line_no, const char *content);
int file_delete_line(const char *path, int line_no); 
int file_replace_line(const char *path, int line_no, const char *content);
int file_replace_str(const char *path, const char *old_str, const char *new_str);

int read_all_lines(const char *path, char lines[][LINE_BUF_SIZE]);
int write_all_lines(const char *path, char lines[][LINE_BUF_SIZE], int n);

#endif