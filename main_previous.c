/************* 导入需要的C语言库（可以去依赖介绍.md中查看相关说明） *************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>  // 可用于显示访问的当前时间
#include <conio.h>
#include <windows.h>

/************* 宏定义区（可以去C语言中宏定义的基础知识.md中查看相关说明） *************/
#define MAX_STUDENTS 30  // 最大学生数量
#define MAX_GRADES 100   // 最大成绩数量
#define FILENAME_A "../dataset/a.txt" // 学生信息存储文件路径
#define FILENAME_B "../dataset/b.txt"  // 成绩信息存储文件路径
#define PASSWORD_FILE "../dataset/password.txt" // 密码文件（用户名在前）路径
#define JOURNAL_FILE "../dataset/journal.txt" // 日志文件路径
#define DELETED_INFORMATION "../dataset/deleted_information.txt" // 已删除的学生信息存储文件路径

// 无痕模式下的业务数据备份文件（进入无痕操作时生成，退出时还原并删除）
#define FILENAME_A_GHOST "../dataset/a.txt.ghost"
#define FILENAME_B_GHOST "../dataset/b.txt.ghost"
#define DELETED_INFORMATION_GHOST "../dataset/deleted_information.txt.ghost"

#define MAX_USERS 50         // 最大用户数量
#define ENCRYPT_KEY 5        // 密码加密偏移量（凯撒式移位）
#define INACTIVE_DAYS 3      // 超过该天数未登录的用户将被清理

// 菜单边框（内部宽度 = 等号数量），标题在此宽度内居中显示
#define BOX_BORDER "============================================="
#define BOX_INNER_WIDTH (sizeof(BOX_BORDER) - 1)

// 学生信息结构体
typedef struct student
{
	char id[10];  // 学号，为了方便学号位数的扩展，不使用int类型
	char name[10];
	char sex[8];
	char room[8];
	char phone[20];
}Student;

// 成绩结构体
typedef struct grades
{
    char id[10];         // 学生学号
    char courseId[20];   // 课程代码
    char courseName[80]; // 课程名字
    float credits;       // 课程学分
    float usualScore;    // 平时成绩
    float labScore;      // 实验成绩
    float examScore;     // 卷面成绩
    float totalScore;    // 总评成绩
    float creditScore;   // 实得学分
} Grades;

/************* 函数声明区（同时对函数在程序中的作用进行分类） *************/
/* 信息打印类 */
void printStudent();
void showMenu();   
void showGradeEntryMenu(); // 显示学生成绩录入子菜单
void showStudentMenu();   // 显示学生基本情况查询子菜单
void showScoreMenu();     // 显示成绩查询子菜单
void deleteStudentMenU(); // 显示删除子菜单
void showSortMenu();      // 显示排序子菜单
void showAverageMenu();   // 显示统计某门课程平均分子菜单
void printMenuHeader(const char *title);  // 打印菜单头部
void printMenuFooter();  // 打印菜单底部

/* 文件操作类 */
int readStudentsFromFile(char *FileName, Student *pStu);  // 读取学生信息文件
int readGradesFromFile(char *FileName, Grades *pGrd);
int checkStudentId(Student students[], int count, const char *id);
char* returnStudentName(Student students[], int count, const char *id);
void addGradesRecord(char *FileName_A, char* FileName_B, Student *student);
/* 算法类 */
float calculateTotalScore(float usual, float lab, float exam);
float calculateCredits(float credit, float totalScore);
void sortGrades(int option);
void calculateCourseAverage(); // 统计某门课程平均分（按课程代码或课程名称）
int compareTotalScoreAsc(const void *a, const void *b);
int compareTotalScoreDesc(const void *a, const void *b);
int compareCreditsAsc(const void *a, const void *b);
int compareCreditsDesc(const void *a, const void *b);
/* 认证类 */
void encryptPassword(const char *plain, char *out);   // 密码加密
void decryptPassword(const char *cipher, char *out);  // 密码解密
void readPassword(char *password);                    // 掩码方式读取密码输入
int loadUsers(char usernames[][50], char encPwds[][50], int maxUsers); // 读取用户名及加密密码
int registerUser();                                   // 用户注册（密码加密存储）
void updateJournal(const char *username);             // 记录用户登录时间
void cleanupInactiveUsers();                          // 清理超过INACTIVE_DAYS天未登录的用户

/* 无痕模式 */
int isGhostMode;                                      // 是否处于无痕操作模式（1=是）
void backupGhostFiles();                              // 进入无痕模式时备份业务数据文件
void restoreGhostFiles();                             // 退出无痕模式时还原业务数据文件

/* 业务类 */
int login();     // 登录：用户输入用户名和密码（带简单的文本菜单，密码存储进行了加密处理）
void queryStudentSmart();
void queryStudentsByDorm();
void queryScoresById();
void deleteStudent();
int validatePhoneNumber(const char *phone);


/* 显示主菜单
 * L - 学生成绩数据录入
 * A - 学生基本信息查询
 * B - 学生成绩信息查询
 * S - 学生成绩排序
 * C - 统计某门课程平均分
 * D - 学生信息删除
 * Q - 退出系统
*/
void showMenu() 
{
    printMenuHeader("学生信息管理系统(SMMS)");
    printf("  L - 学生成绩数据录入\n");
    printf("  A - 学生基本信息查询\n");
    printf("  B - 学生成绩信息查询\n");
    printf("  S - 学生成绩排序\n");
    printf("  C - 统计某门课程平均分\n");
    printf("  D - 学生信息删除\n");
    printf("  Q - 退出系统\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void showGradeEntryMenu() {
    printMenuHeader("学生成绩录入");
    printf("  0 - 返回上一级\n");
    printf("  1 - 继续(开始)录入\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void showStudentMenu() {
    printMenuHeader("学生基本信息查询");
    printf("  0 - 返回上一级\n");
    printf("  1 - 按学号(或姓名)查询\n");
    printf("  2 - 按宿舍号查询\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void showScoreMenu() {
    printMenuHeader("学生成绩信息查询");
    printf("  0 - 返回上一级\n");
    printf("  1 - 按学号查询所有课程成绩\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void showSortMenu() {
    printMenuHeader("学生成绩排序");
    printf("  0 - 返回上一级\n");
    printf("  1 - 按综合成绩升序排列\n");
    printf("  2 - 按综合成绩降序排列\n");
    printf("  3 - 按课程学分升序排列\n");
    printf("  4 - 按课程学分降序排列\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void showAverageMenu(){
    printMenuHeader("统计某门课程平均分");
    printf("  0 - 返回上一级\n");
    printf("  1 - 按课程代码（或课程名称）统计\n");
    printMenuFooter();
    printf("  请选择操作：");
}

void deleteStudentMenU(){
    printMenuHeader("学生信息删除");
    printf("  0 - 返回上一级\n");
    printf("  1 - 按学号删除某个学生的全部信息\n");
    printMenuFooter();
    printf("  请选择操作：");
}

// 计算字符串的终端显示宽度：中文等双字节字符占2格，ASCII占1格
// 说明：本工程源文件为 GBK 编码（无 BOM），中文以双字节存储，
//       故每个 >=0x80 的字节视为一个双字节双宽字符并跳过2字节。
static int displayWidth(const char *str)
{
    int width = 0;
    const unsigned char *s = (const unsigned char *)str;
    while (*s) {
        if (*s < 0x80) {
            width += 1;   // ASCII 字符占1格
            s += 1;
        } else {
            width += 2;   // GBK 中文等双字节字符占2格
            s += 2;       // 跳过整个双字节字符
        }
    }
    return width;
}

void printMenuHeader(const char *title)
{
    printf("\n");
    // 显示当前时间
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("  当前时间：%04d-%02d-%02d %02d:%02d:%02d\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);

    // 上边框
    printf("  " BOX_BORDER "\n");

    // 标题在内部宽度内居中（中文按2格计算）
    int titleW = displayWidth(title);
    int padTotal = BOX_INNER_WIDTH - titleW;
    if (padTotal < 0) padTotal = 0;
    int padLeft = padTotal / 2;
    int padRight = padTotal - padLeft;

    printf("  |");
    for (int i = 0; i < padLeft; i++) printf(" ");
    printf("%s", title);
    for (int i = 0; i < padRight; i++) printf(" ");
    printf("|\n");

    // 下边框
    printf("  " BOX_BORDER "\n");
}

void printMenuFooter()
{
    printf("  " BOX_BORDER "\n");
}

/* 密码加密：对可打印字符做凯撒式循环移位，保持结果仍为可打印字符且不含空格 */
void encryptPassword(const char *plain, char *out)
{
    int i;
    for (i = 0; plain[i] != '\0'; i++) {
        char c = plain[i];
        if (c >= 33 && c <= 126) {
            c = 33 + (c - 33 + ENCRYPT_KEY) % 94;
        }
        out[i] = c;
    }
    out[i] = '\0';
}

/* 密码解密：加密的逆运算 */
void decryptPassword(const char *cipher, char *out)
{
    int i;
    for (i = 0; cipher[i] != '\0'; i++) {
        char c = cipher[i];
        if (c >= 33 && c <= 126) {
            c = 33 + (c - 33 - ENCRYPT_KEY + 94) % 94;
        }
        out[i] = c;
    }
    out[i] = '\0';
}

/* 以掩码（*）方式读取密码，支持退格删除 */
void readPassword(char *password)
{
    int i = 0;
    char ch;
    while ((ch = getch()) != '\r') { // 回车结束
        if (ch == '\b') { // 退格键
            if (i > 0) {
                i--;
                printf("\b \b"); // 擦除原理：\b 退格，空格覆盖原字符，再用\b将光标移回
            }
        } else if (i < 49) {
            password[i++] = ch;
            printf("*");
        }
    }
    password[i] = '\0';
    printf("\n");
}

/* 从password.txt读取所有用户名及其加密密码，返回用户数量
 * 文件格式：每行 = 用户名 + 空格 + 加密密码 */
int loadUsers(char usernames[][50], char encPwds[][50], int maxUsers)
{
    FILE *fp = fopen(PASSWORD_FILE, "r");
    if (fp == NULL) {
        return 0;
    }
    int count = 0;
    while (count < maxUsers &&
           fscanf(fp, "%49s %49s", usernames[count], encPwds[count]) == 2) {
        count++;
    }
    fclose(fp);
    return count;
}

/* 用户注册：校验用户名是否重复，密码加密后追加写入password.txt */
int registerUser()
{
    char usernames[MAX_USERS][50];
    char encPwds[MAX_USERS][50];
    int count = loadUsers(usernames, encPwds, MAX_USERS);

    if (count >= MAX_USERS) {
        printf("  用户数量已达上限，无法注册！\n");
        return 0;
    }

    char username[50];
    char password[50];
    char encrypted[50];

    printf("  请输入要注册的用户名：");
    scanf("%s", username);

    // 用户名查重
    for (int i = 0; i < count; i++) {
        if (strcmp(usernames[i], username) == 0) {
            printf("  用户名已存在，注册失败！\n");
            return 0;
        }
    }

    printf("  请设置密码：");
    readPassword(password);

    if (strlen(password) == 0) {
        printf("  密码不能为空，注册失败！\n");
        return 0;
    }

    encryptPassword(password, encrypted);

    FILE *fp = fopen(PASSWORD_FILE, "a");
    if (fp == NULL) {
        printf("  无法打开密码文件，注册失败！\n");
        return 0;
    }
    fprintf(fp, "%s %s\n", username, encrypted);
    fclose(fp);

    updateJournal(username); // 注册即记录一次登录时间
    printf("  用户 %s 注册成功！\n", username);
    return 1;
}

/* 更新登录日志：记录用户名及本次登录时间（Unix时间戳）
 * 文件格式：每行 = 用户名 + 空格 + 时间戳 */
void updateJournal(const char *username)
{
    char names[MAX_USERS][50];
    long times[MAX_USERS];
    int count = 0;

    FILE *fp = fopen(JOURNAL_FILE, "r");
    if (fp != NULL) {
        while (count < MAX_USERS &&
               fscanf(fp, "%49s %ld", names[count], &times[count]) == 2) {
            count++;
        }
        fclose(fp);
    }

    long now = (long)time(NULL);
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], username) == 0) {
            times[i] = now;
            found = 1;
            break;
        }
    }
    if (!found && count < MAX_USERS) {
        strcpy(names[count], username);
        times[count] = now;
        count++;
    }

    fp = fopen(JOURNAL_FILE, "w");
    if (fp != NULL) {
        for (int i = 0; i < count; i++) {
            fprintf(fp, "%s %ld\n", names[i], times[i]);
        }
        fclose(fp);
    }
}

/* 清理长期未登录的用户：根据journal.txt中的最后登录时间，
 * 将超过INACTIVE_DAYS天未登录的用户从password.txt和journal.txt中移除 */
void cleanupInactiveUsers()
{
    char names[MAX_USERS][50];
    long times[MAX_USERS];
    int jcount = 0;

    FILE *fp = fopen(JOURNAL_FILE, "r");
    if (fp == NULL) {
        return; // 无日志文件，无需清理
    }
    while (jcount < MAX_USERS &&
           fscanf(fp, "%49s %ld", names[jcount], &times[jcount]) == 2) {
        jcount++;
    }
    fclose(fp);

    long now = (long)time(NULL);
    long threshold = (long)INACTIVE_DAYS * 24 * 60 * 60; // 秒数阈值

    // 读取全部用户
    char usernames[MAX_USERS][50];
    char encPwds[MAX_USERS][50];
    int ucount = loadUsers(usernames, encPwds, MAX_USERS);
    if (ucount == 0) {
        return;
    }

    // 重写password.txt，仅保留活跃用户
    fp = fopen(PASSWORD_FILE, "w");
    if (fp == NULL) {
        return;
    }
    int removed = 0;
    for (int i = 0; i < ucount; i++) {
        long last = -1;
        for (int j = 0; j < jcount; j++) {
            if (strcmp(names[j], usernames[i]) == 0) {
                last = times[j];
                break;
            }
        }
        // 无日志记录（视为新用户）或在阈值内，则保留
        if (last == -1 || (now - last) <= threshold) {
            fprintf(fp, "%s %s\n", usernames[i], encPwds[i]);
        } else {
            removed++;
        }
    }
    fclose(fp);

    // 同步重写journal.txt，移除已过期记录
    fp = fopen(JOURNAL_FILE, "w");
    if (fp != NULL) {
        for (int j = 0; j < jcount; j++) {
            if ((now - times[j]) <= threshold) {
                fprintf(fp, "%s %ld\n", names[j], times[j]);
            }
        }
        fclose(fp);
    }

    if (removed > 0) {
        printf("  已清理 %d 个超过%d天未登录的用户账户。\n", removed, INACTIVE_DAYS);
        Sleep(1000);
    }
}

/* 判断文件是否存在（用于无痕备份还原） */
static int fileExists(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp != NULL) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/* 二进制复制文件：src -> dst，src 不存在则失败返回0 */
static int copyFile(const char *src, const char *dst)
{
    FILE *fin = fopen(src, "rb");
    if (fin == NULL) {
        return 0;
    }
    FILE *fout = fopen(dst, "wb");
    if (fout == NULL) {
        fclose(fin);
        return 0;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        fwrite(buf, 1, n, fout);
    }
    fclose(fin);
    fclose(fout);
    return 1;
}

/* 进入无痕模式：把当前业务数据文件备份为 .ghost，
 * 若某文件不存在则不产生对应备份（退出时据此删除无痕期间新建的文件） */
void backupGhostFiles()
{
    if (fileExists(FILENAME_A)) {
        copyFile(FILENAME_A, FILENAME_A_GHOST);
    } else if (fileExists(FILENAME_A_GHOST)) {
        remove(FILENAME_A_GHOST);
    }

    if (fileExists(FILENAME_B)) {
        copyFile(FILENAME_B, FILENAME_B_GHOST);
    } else if (fileExists(FILENAME_B_GHOST)) {
        remove(FILENAME_B_GHOST);
    }

    if (fileExists(DELETED_INFORMATION)) {
        copyFile(DELETED_INFORMATION, DELETED_INFORMATION_GHOST);
    } else if (fileExists(DELETED_INFORMATION_GHOST)) {
        remove(DELETED_INFORMATION_GHOST);
    }
}

/* 退出无痕模式：将业务数据还原到进入前状态并清理备份
 * - 进入时存在（有.ghost备份）：用备份覆盖当前文件
 * - 进入时不存在（无.ghost备份）：删除无痕期间新建的文件 */
void restoreGhostFiles()
{
    // 学生信息 a.txt
    if (fileExists(FILENAME_A_GHOST)) {
        remove(FILENAME_A);
        rename(FILENAME_A_GHOST, FILENAME_A);
    } else {
        remove(FILENAME_A);
    }

    // 成绩信息 b.txt
    if (fileExists(FILENAME_B_GHOST)) {
        remove(FILENAME_B);
        rename(FILENAME_B_GHOST, FILENAME_B);
    } else {
        remove(FILENAME_B);
    }

    // 已删除学生信息
    if (fileExists(DELETED_INFORMATION_GHOST)) {
        remove(DELETED_INFORMATION);
        rename(DELETED_INFORMATION_GHOST, DELETED_INFORMATION);
    } else {
        remove(DELETED_INFORMATION);
    }
}

/* 打印已注册用户名列表，便于用户选择登录 */
static void printUserList(char usernames[][50], int userCount)
{
    printf("  已注册用户：");
    for (int i = 0; i < userCount; i++) {
        printf("%s%s", usernames[i], (i < userCount - 1) ? "、" : "");
    }
    printf("\n\n");
}

int login()
{
    char username[50];
    char password[50];
    char correctPassword[50] = {0};
    int attempts = 3; // 尝试次数

    system("cls");  // 清屏，进入smms登录菜单
    printMenuHeader("系统登录");

    char usernames[MAX_USERS][50];
    char encPwds[MAX_USERS][50];
    int userCount = loadUsers(usernames, encPwds, MAX_USERS);

    // 密码文件为空或不存在：提示无痕操作或注册
    if (userCount == 0) {
        printf("  当前密码文件中没有任何账户信息，你可以：\n");
        printf("  0. 进入无痕操作（对数据的所有操作在退出后均失效）\n");
        printf("  1. 注册（创建新用户名和密码）\n");
        printf("  请选择：");
        int choice_login = 0;
        scanf("%d", &choice_login);

        if (choice_login == 0) {
            printMenuFooter();
            printf("\n  已进入无痕操作模式，本次操作在退出后均会还原\n");
            isGhostMode = 1;        // 标记为无痕模式
            backupGhostFiles();     // 备份当前业务数据，供退出时还原
            Sleep(1000);
            return 1;
        } else {
            // 进行注册，注册成功后要求正式登录
            system("cls");
            printMenuHeader("用户注册");
            if (!registerUser()) {
                printMenuFooter();
                Sleep(1500);
                return 0;
            }
            Sleep(1000);
            userCount = loadUsers(usernames, encPwds, MAX_USERS); // 重新加载
            system("cls");
            printMenuHeader("系统登录");
        }
    }

    // 列出已注册用户名，供用户选择登录
    printUserList(usernames, userCount);

    while (attempts > 0) {
        printf("  0 - 进入无痕操作（退出后所有改动还原）\n");
        printf("  1 - 注册新用户\n");
        printf("  用户名（或输入 0/1 选择上方操作）：");
        scanf("%s", username);

        // 特殊选项：0-进入无痕操作，1-注册新用户
        if (strcmp(username, "0") == 0) {
            printMenuFooter();
            printf("\n  已进入无痕操作模式，本次操作在退出后均会还原\n");
            isGhostMode = 1;        // 标记为无痕模式
            backupGhostFiles();     // 备份当前业务数据，供退出时还原
            Sleep(1000);
            return 1;
        }
        if (strcmp(username, "1") == 0) {
            system("cls");
            printMenuHeader("用户注册");
            if (!registerUser()) {
                printMenuFooter();
                Sleep(1500);
            } else {
                Sleep(1000);
                userCount = loadUsers(usernames, encPwds, MAX_USERS); // 重新加载用户列表
            }
            system("cls");
            printMenuHeader("系统登录");
            printUserList(usernames, userCount);
            continue; // 重新展示登录菜单
        }

        // 用户名校验：必须存在于已注册用户中
        int idx = -1;
        for (int i = 0; i < userCount; i++) {
            if (strcmp(usernames[i], username) == 0) {
                idx = i;
                break;
            }
        }
        if (idx == -1) {
            printf("  用户名不存在，请重新输入！\n\n");
            continue; // 用户名错误不消耗尝试次数
        }

        // 解密该用户存储的密码用于比对
        decryptPassword(encPwds[idx], correctPassword);

        printf("  密码：");
        readPassword(password);

        if (strcmp(password, correctPassword) == 0) {
            printMenuFooter();
            printf("\n  登录成功！正在进入系统...\n");
            updateJournal(username); // 记录本次登录时间
            Sleep(1000);
            return 1;
        } else {
            attempts--;
            printf("\n  密码错误！剩余尝试次数：%d\n\n", attempts);
            if (attempts > 0) {
                printf("  按任意键继续...");
                getch();
                system("cls");
                printMenuHeader("系统登录");
                printUserList(usernames, userCount);
            }
        }
    }

    printMenuFooter();
    return 0;
}

// 主函数
int main()
{
    // 启动时清理超过INACTIVE_DAYS天未登录的用户（依据journal.txt的最后登录时间）
    cleanupInactiveUsers();

    // 登录验证（登录成功后会在journal.txt中记录用户名与登录时间）
    if (!login()) {
        printf("登录失败，系统退出！\n");
        return 0;
    }

    char choice;
    while (1) {
        system("cls"); // 清除前面的登录输入信息，正式进行smms操作菜单
        showMenu();
        scanf(" %c", &choice);

        switch (choice) {
            case 'L':
            case 'l':
                do {
                    system("cls");
                    showGradeEntryMenu();
                    scanf(" %c", &choice); //前面留一个空格的目的是为了跳过前面大的换行符\n

                    switch(choice){
                        case '1':
                        {  // case标签后面的语句也需要放到花括号下
                            printStudent();
                            Student *pStu = (Student *)malloc(MAX_GRADES * sizeof(Student));
                            if (pStu == NULL) {
                                printf("内存分配竟然失败！！\n");
                                getch();
                                break;
                            }
                            addGradesRecord(FILENAME_A, FILENAME_B, pStu); // 录入学生成绩
                            free(pStu); // 释放内存
                            break;
                        }
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环

            case 'A':
            case 'a':
                do {
                    system("cls");
                    showStudentMenu();
                    scanf(" %c", &choice);

                    switch (choice) {
                        case '1':
                            queryStudentSmart();
                            break;
                        case '2':
                            queryStudentsByDorm();
                            break;
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环

            case 'B':
            case 'b':
                do {
                    system("cls");
                    showScoreMenu();
                    scanf(" %c", &choice);

                    switch (choice) {
                        case '1':
                            queryScoresById();
                            break;
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环

            case 'S':
            case 's':
                do {
                    system("cls");
                    showSortMenu();
                    scanf(" %c", &choice);

                    switch(choice){
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                            sortGrades(choice - '0');
                            printf("\n按任意键返回...");
                            getch();
                            break;
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环
            
            case 'C':
            case 'c':
                do {
                    system("cls");
                    showAverageMenu();
                    scanf(" %c", &choice);

                    switch(choice){
                        case '1': // 按课程代码（或课程名称）统计平均分
                            calculateCourseAverage();
                            printf("\n按任意键返回...");
                            getch();
                            break;
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环

            case 'D':
            case 'd':
                do {
                    system("cls");
                    deleteStudentMenU();
                    scanf(" %c", &choice);

                    switch(choice){
                        case '1':
                            deleteStudent();
                            break;
                        case '0':
                            break; // 返回主菜单
                        default:
                            printf("无效选择，请重新输入\n");
                            getch();
                    }
                } while (choice != '0');
                break; // 返回主菜单循环

            case 'Q':
            case 'q':
                if (isGhostMode) {
                    restoreGhostFiles(); // 还原无痕期间的所有业务数据改动
                    printf("无痕操作已结束，本次所有改动已还原。\n");
                }
                printf("感谢使用，再见！\n");
                return 0;

            default:
                printf("  无效选择，请重新输入\n");
                Sleep(1000);
        }
    }
}

/* 从文本文件读取学生信息 */
int readStudentsFromFile(char* FileName, Student* pStu)
{
	FILE* fp; // 文件指针
	fp = fopen(FileName, "r");
	if (fp == NULL)
	{
		printf("The file is error.\n");
		exit(0);
	}
    
	/*跳过表头信息（同时判断EOF，防止空文件或无换行文件导致死循环）*/ 
	int ch;
	while ((ch = fgetc(fp)) != '\n' && ch != EOF); // 跳过第一行
    
	/*读入到学生数组中*/
	int i = 0;
	while (fscanf(fp, "%s %s %s %s %s\n", pStu[i].id, pStu[i].name, pStu[i].sex, pStu[i].room, pStu[i].phone) == 5) 
	{
		i++; // 每读取一名学生的信息计数加一
	}
    printf("学生的学号及姓名信息：\n");
	fclose(fp);

	int sum = i;
	return sum;
}

/*打印出学生的信息*/
void printStudent()
{
    // 读取学生数据
    Student students[MAX_STUDENTS];
    int sum = readStudentsFromFile(FILENAME_A, students);
    printf("  当前的学生学号及姓名信息：\n");
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < sum; i++)
    {
        printf("  %s %s", students[i].id, students[i].name);
        if ((i + 1) % 4 == 0) printf("\n");
    }
    printf("\n  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n");
}

// 检查学号信息是否存在于学生信息表中
int checkStudentId(Student students[], int count, const char *id) 
{
    for (int i = 0; i < count; i++) {
        if (strcmp(students[i].id, id) == 0) {
            return 1; // 存在
        }
    }
    return 0; // 不存在
}

// 根据学号信息返回学生姓名
char* returnStudentName(Student students[], int count, const char *id)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(students[i].id, id) == 0) {
            return students[i].name; 
        }
    }
    return ""; 
}

// 计算综合成绩
float calculateTotalScore(float usual, float lab, float exam) 
{
	if(lab != -1) 
	{
		return usual * 0.15 + lab * 0.15 + exam * 0.7;
	}
    return usual * 0.3 + exam * 0.7;
}

// 采用等级学分制
float calculateCredits(float credit, float totalScore) {	
	if (totalScore >= 90) {
		return credit * 1;
	} else if (totalScore >= 80) {
		return credit * 0.8;
	} else if (totalScore >= 70) {
		return credit * 0.75;
	} else if( totalScore >= 60) {
		return credit * 0.6;
	}
	else {
		return 0; 
	}
	return -1;
}

// 录入
void addGradesRecord(char *FileName_A, char* FileName_B, Student *student)
{
    system("cls");
    printMenuHeader("学生成绩录入");
    int studentCount = readStudentsFromFile(FileName_A, student);
    printf("\n");
    if (studentCount == 0) {
        printf("  没有学生信息，请先在 %s 中添加学生数据\n", FileName_A);
        printf("\n按任意键返回...");
        getch();
        return;
    }

    printf("  当前系统中的学生信息：\n");
    for (int i = 0; i < studentCount; i++) {
        printf("  %s %s  ", student[i].id, student[i].name);
        if ((i + 1) % 3 == 0) printf("\n");
    }
    printf("\n\n");

    Grades grades;
    char id[20];

    // 写入文件
    FILE *file = fopen(FileName_B, "a+");
    if (file == NULL) {
        printf("  无法打开文件 %s\n", FileName_B);
        return;
    }

    // 当文件为空时添加表头
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0) {
        fprintf(file, "学号 课程编号 课程名称 学分 平时成绩 实验成绩 卷面成绩 综合成绩\n");
    }

    // 智能插入：检测文件末尾是否已有换行
    // - 若结尾无换行（如只有表头且末尾无\n）：写数据前先补一个换行，避免与上一行黏连
    // - 若结尾已有换行：直接写数据即可
    // 只需在首条记录写入前判断一次，之后每条记录都会写入尾随换行
    int needLeadingNewline = 0;
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize > 0) {
        fseek(file, -1, SEEK_END);
        if (fgetc(file) != '\n') {
            needLeadingNewline = 1;
        }
    }

    while (1) {
        fseek(file, 0, SEEK_END);
        printf("  请输入学号（输入0退出）：");
        scanf("%s", id);

        if (strcmp(id, "0") == 0) {
            fclose(file);
            return;
        }

        // 检查学号是否存在
        if (!checkStudentId(student, studentCount, id)) {
            printf("  错误：该学号不存在于 %s 中！\n\n", FileName_A);
            continue;
        }

        strcpy(grades.id, id);

        printf("  请输入课程编号：");
        scanf("%s", grades.courseId);

        printf("  请输入课程名称：");
        scanf("%s", grades.courseName);

        printf("  请输入学分：");
        scanf("%f", &grades.credits);

        printf("  请输入平时成绩：");
        scanf("%f", &grades.usualScore);

        printf("  请输入实验成绩（若无实验请输入-1）：");
        scanf("%f", &grades.labScore);

        printf("  请输入卷面成绩：");
        scanf("%f", &grades.examScore);

        // 计算总评成绩
        grades.totalScore = calculateTotalScore(grades.usualScore, grades.labScore, grades.examScore);

        if (file == NULL) {
            printf("  无法打开文件 %s\n", FileName_B);
            return;
        }

        float creditScore = calculateCredits(grades.credits, grades.totalScore);

        // 若文件末尾缺少换行，先补一个换行再写数据（只需补一次）
        if (needLeadingNewline) {
            fprintf(file, "\n");
            needLeadingNewline = 0;
        }

        fprintf(file, "%s %s %s %.1f %.1f %.1f %.1f %.1f %.1f\n",
                grades.id, grades.courseId, grades.courseName,
                grades.credits, grades.usualScore, grades.labScore,
                grades.examScore, grades.totalScore, creditScore);

        printf("\n  成绩记录添加成功！\n");
        printf("  综合成绩：%.1f，实得学分：%.1f\n\n", grades.totalScore, creditScore);
    }
    fclose(file);
}

int readGradesFromFile(char *FileName, Grades *pGrd)
{
    FILE* fp; // 文件指针
	fp = fopen(FileName, "r");
	if (fp == NULL)
	{
		printf("The file is error.\n");
		exit(0);
	}

	/*跳过表头信息（同时判断EOF，防止空文件或无换行文件导致死循环）*/ 
	int ch;
	while ((ch = fgetc(fp)) != '\n' && ch != EOF); // 跳过第一行

	/*读入到成绩数组中*/
	int i = 0;
	while (fscanf(fp, "%s %s %s %f %f %f %f %f %f\n", pGrd[i].id, pGrd[i].courseId, pGrd[i].courseName, 
        &pGrd[i].credits, &pGrd[i].usualScore, &pGrd[i].labScore, &pGrd[i].examScore, &pGrd[i].totalScore, &pGrd[i].creditScore) == 9) 
	{

		i++; // 每读取一名学生的信息计数加一
	}
	fclose(fp);
    int sum = i;
	return sum;
}

// 智能查询学生基本信息（支持学号和姓名查询）
void queryStudentSmart()
{
    system("cls");
    printMenuHeader("学生基本信息查询");
    char input[50];
    printf("  请输入要查询的学号或姓名：");
    scanf("%s", input);

    // 读取学生数据
    Student students[MAX_STUDENTS];
    int studentCount = readStudentsFromFile(FILENAME_A, students);

    // 智能判断：如果输入全是数字，按学号查询；否则按姓名查询
    int isNumber = 1;
    for (int i = 0; input[i] != '\0'; i++) {
        if (!isdigit(input[i])) {
            isNumber = 0;
            break;
        }
    }

    int found = 0;
    printf("\n");
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    if (isNumber) {
        // 按学号查询
        for (int i = 0; i < studentCount; i++) {
            if (strcmp(students[i].id, input) == 0) {
                printf("  学号：%s\n", students[i].id);
                printf("  姓名：%s\n", students[i].name);
                printf("  性别：%s\n", students[i].sex);
                printf("  宿舍号：%s\n", students[i].room);
                printf("  电话号码：%s\n", students[i].phone);
                // 验证手机号码格式
                if (validatePhoneNumber(students[i].phone)) {
                    printf("  手机号码格式：? 有效\n");
                } else {
                    printf("  手机号码格式：? 无效\n");
                }
                found = 1;
                break;
            }
        }
        if (!found) printf("  未找到该学号的学生信息\n");
    } else {
        // 按姓名查询
        for (int i = 0; i < studentCount; i++) {
            if (strcmp(students[i].name, input) == 0) {
                printf("  学号：%s\n", students[i].id);
                printf("  姓名：%s\n", students[i].name);
                printf("  性别：%s\n", students[i].sex);
                printf("  宿舍号：%s\n", students[i].room);
                printf("  电话号码：%s\n", students[i].phone);
                // 验证手机号码格式
                if (validatePhoneNumber(students[i].phone)) {
                    printf("  手机号码格式：? 有效\n");
                } else {
                    printf("  手机号码格式：? 无效\n");
                }
                found = 1;
                break;
            }
        }
        if (!found) printf("  未找到该姓名的学生信息\n");
    }
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n按任意键返回...");
    getch();
}

// 按宿舍号查询学生基本信息
void queryStudentsByDorm()
{
    system("cls");
    printMenuHeader("按宿舍号查询");
    char dorm[20];
    printf("  请输入要查询的宿舍号：");
    scanf("%s", dorm);

    // 读取学生数据
    Student students[MAX_STUDENTS];
    int studentCount = readStudentsFromFile(FILENAME_A, students);

    int found = 0;
    printf("\n");
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < studentCount; i++) {
        if (strcmp(students[i].room, dorm) == 0) {
            printf("  学号：%s\n", students[i].id);
            printf("  姓名：%s\n", students[i].name);
            printf("  性别：%s\n", students[i].sex);
            printf("  宿舍号：%s\n", students[i].room);
            printf("  电话号码：%s\n", students[i].phone);
            printf("  ------------------------\n");
            found = 1;
        }
    }

    if (!found) {
        printf("  未找到该宿舍的学生信息\n");
    }
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n按任意键返回...");
    getch();
}

// 按学号查询成绩信息
void queryScoresById()
{
    system("cls");
    printMenuHeader("按学号查询成绩");
    char id[20];
    printf("  请输入要查询的学号：");
    scanf("%s", id);

    // 读取成绩数据
    Grades scores[MAX_GRADES];
    int scoreCount = readGradesFromFile(FILENAME_B, scores);
    // 读取学生信息
    Student students[MAX_STUDENTS];
    int studentCount = readStudentsFromFile(FILENAME_A, students);
    char *name = returnStudentName(students, studentCount, id);

    // 查询并显示成绩
    int found = 0;
    float allCredit = 0;

    printf("\n");
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < scoreCount; i++) {
        if (strcmp(scores[i].id, id) == 0) {
            printf("  学号：%s  姓名：%s\n", id, name);
            printf("  课程编号：%s\n", scores[i].courseId);
            printf("  课程名称：%s\n", scores[i].courseName);
            printf("  综合成绩：%.1f\n", scores[i].totalScore);
            printf("  实得学分：%.2f\n", scores[i].credits);
            printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            found++;
            allCredit = allCredit + scores[i].credits;
        }
    }

    if (found) {
        printf("  共修：%d科，实得总学分为：%.2f\n", found, allCredit);
    } else {
        printf("  未找到该学生的成绩记录\n");
    }
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n按任意键返回...");
    getch();
}


// 删除学生及其成绩信息
void deleteStudent()
{
    system("cls");
    printMenuHeader("删除学生信息");
    char id[20];
    printf("  请输入要删除的学生学号：");
    scanf("%s", id);

    // 读取学生文件
    Student students[MAX_STUDENTS];
    int count = readStudentsFromFile(FILENAME_A, students);
    if (count == 0) {
        printf("\n  没有学生信息可删除\n");
        printf("\n按任意键返回...");
        getch();
        return;
    }

    // 查找学生
    int found = 0;
    int index = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(students[i].id, id) == 0) {
            found = 1;
            index = i;
            break;
        }
    }

    if (!found) {
        printf("\n  未找到学号为 %s 的学生\n", id);
        printf("\n按任意键返回...");
        getch();
        return;
    }

    // 显示学生信息
    printf("\n  找到以下学生信息：\n");
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  学号：%s\n", students[index].id);
    printf("  姓名：%s\n", students[index].name);
    printf("  性别：%s\n", students[index].sex);
    printf("  宿舍号：%s\n", students[index].room);
    printf("  电话号码：%s\n", students[index].phone);
    printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    // 确认删除
    char confirm;
    printf("\n  确定要删除学号为 %s 的学生及其所有成绩吗？(y/n) ", id);
    scanf(" %c", &confirm);
    if (confirm != 'y' && confirm != 'Y') {
        printf("\n  已取消删除操作\n");
        printf("\n按任意键返回...");
        getch();
        return;
    }

    // 1. 从a.txt中删除学生
    FILE *tempFile = fopen("temp_a.txt", "w");
    if (tempFile == NULL) {
        printf("\n  无法创建临时文件\n");
        return;
    }

    // 写入表头
    fprintf(tempFile, "学号 姓名 性别 宿舍号码 电话号码\n");

    for (int i = 0; i < count; i++) {
        if (strcmp(students[i].id, id) != 0) {
            // 写入除要删除学生外的所有记录
            fprintf(tempFile, "%s %s %s %s %s\n",
                    students[i].id,
                    students[i].name,
                    students[i].sex,
                    students[i].room,
                    students[i].phone);
        }
    }
    fclose(tempFile);

    // 替换原文件
    remove(FILENAME_A);
    rename("temp_a.txt", FILENAME_A);

    // 2. 从b.txt中删除对应成绩
    Grades scores[MAX_GRADES];
    int scoreCount = readGradesFromFile(FILENAME_B, scores);

    tempFile = fopen("temp_b.txt", "w");
    if (tempFile == NULL) {
        printf("\n  无法创建临时文件\n");
        return;
    }

    // 写入表头
    fprintf(tempFile, "学号 课程编号 课程名称 学分 平时成绩 实验成绩 卷面成绩 综合成绩\n");

    for (int i = 0; i < scoreCount; i++) {
        if (strcmp(scores[i].id, id) != 0) {
            // 写入除要删除学生外的所有成绩记录
            fprintf(tempFile, "%s %s %s %.1f %.1f %.1f %.1f %.1f\n",
                    scores[i].id,
                    scores[i].courseId,
                    scores[i].courseName,
                    scores[i].credits,
                    scores[i].usualScore,
                    scores[i].labScore,
                    scores[i].examScore,
                    scores[i].totalScore);
        }
    }
    fclose(tempFile);

    // 替换原文件
    remove(FILENAME_B);
    rename("temp_b.txt", FILENAME_B);

    printf("\n  学号为 %s 的学生及其所有成绩已成功删除\n", id);
    printf("\n按任意键返回...");
    getch();
}


// 按综合成绩升序
int compareTotalScoreAsc(const void *a, const void *b) {
    Grades *scoreA = (Grades *)a;
    Grades *scoreB = (Grades *)b;
    return (scoreA->totalScore - scoreB->totalScore) * 100; // 乘以100避免浮点数误差
}

// 按综合成绩降序
int compareTotalScoreDesc(const void *a, const void *b) {
    return compareTotalScoreAsc(b, a); // 反转参数顺序实现降序
}

// 按学分升序
int compareCreditsAsc(const void *a, const void *b) {
    Grades *scoreA = (Grades *)a;
    Grades *scoreB = (Grades *)b;
    return scoreA->credits - scoreB->credits;
}

// 按学分降序
int compareCreditsDesc(const void *a, const void *b) {
    return compareCreditsAsc(b, a); // 反转参数顺序实现降序
}

// 手机号码验证
int validatePhoneNumber(const char *phone)
{
    int len = strlen(phone);

    // 中国手机号码必须是11位，且以1开头
    if (len != 11) {
        return 0;
    }

    if (phone[0] != '1') {
        return 0;
    }

    // 检查第二位是否在3-9之间
    if (phone[1] < '3' || phone[1] > '9') {
        return 0;
    }

    // 检查其余位是否都是数字
    for (int i = 0; i < len; i++) {
        if (!isdigit(phone[i])) {
            return 0;
        }
    }

    return 1;
}

// 排序功能
void sortGrades(int option)
{
    Grades grades[MAX_GRADES];
    int count = readGradesFromFile(FILENAME_B, grades);
    Student students[MAX_STUDENTS];
    int studentCount = readStudentsFromFile(FILENAME_A, students);

    if (count == 0) {
        printf("\n没有成绩记录可排序！\n");
        return;
    }

    // 根据选择的排序方式进行排序
    switch (option) {
        case 1: // 按综合成绩升序
            qsort(grades, count, sizeof(Grades), compareTotalScoreAsc);
            printf("\n--- 按综合成绩升序排列 ---\n");
            break;
        case 2: // 按综合成绩降序
            qsort(grades, count, sizeof(Grades), compareTotalScoreDesc);
            printf("\n--- 按综合成绩降序排列 ---\n");
            break;
        case 3: // 按实得学分升序
            qsort(grades, count, sizeof(Grades), compareCreditsAsc);
            printf("\n--- 按实得学分升序排列 ---\n");
            break;
        case 4: // 按实得学分降序
            qsort(grades, count, sizeof(Grades), compareCreditsDesc);
            printf("\n--- 按实得学分降序排列 ---\n");
            break;
        default:
            return;
    }

    // 显示排序后的结果
    printf("=====================================================================================\n");
    printf("学号      姓名        课程编号  课程名称              实得学分  综合成绩\n");
    printf("=====================================================================================\n");
    for (int i = 0; i < count; i++) {
        char *name = returnStudentName(students, studentCount, grades[i].id);
        printf("%-10s %-10s %-8s %-20s %-9.2f %.1f\n",
               grades[i].id, name, grades[i].courseId,
               grades[i].courseName, grades[i].credits, grades[i].totalScore);
    }
    printf("=====================================================================================\n");
    printf("共 %d 条记录\n", count);
}

// 统计某门课程的平均综合成绩
// byWhat: 1 - 按课程代码统计；2 - 按课程名称统计
void calculateCourseAverage()
{
    Grades grades[MAX_GRADES];
    int count = readGradesFromFile(FILENAME_B, grades);

    if (count == 0) {
        printf("\n没有成绩记录可供统计！\n");
        return;
    }

    // 清空菜单选择后残留的换行符，避免被 fgets 直接读走
    int tmp;
    while ((tmp = getchar()) != '\n' && tmp != EOF);

    // 输入要查询的课程代码或课程名称
    char target[80];
    printf("\n请输入要查询的课程代码或课程名称：");
    if (fgets(target, sizeof(target), stdin) == NULL) {
        return;
    }
    target[strcspn(target, "\n")] = '\0'; // 去除结尾换行符
    if (strlen(target) == 0) {
        printf("\n未输入课程信息！\n");
        return;
    }

    // 按课程代码或课程名称匹配，累计该课程的总分与人数
    float sum = 0;
    int num = 0;
    char matchedId[20] = "";
    char matchedName[80] = "";
    for (int i = 0; i < count; i++) {
        if (strcmp(grades[i].courseId, target) == 0 ||
            strcmp(grades[i].courseName, target) == 0) {
            if (num == 0) {
                strcpy(matchedId, grades[i].courseId);
                strcpy(matchedName, grades[i].courseName);
            }
            sum += grades[i].totalScore;
            num++;
        }
    }

    if (num == 0) {
        printf("\n未找到课程「%s」的成绩记录！\n", target);
        return;
    }

    float avg = sum / num;
    printf("\n--- 课程平均综合成绩 ---\n");
    printf("  课程代码：%s\n", matchedId);
    printf("  课程名称：%s\n", matchedName);
    printf("  平均分：  %.1f\n", avg);
    printf("  人数：    %d\n", num);
    printf("  ------------------------------------------------------------\n");
}
