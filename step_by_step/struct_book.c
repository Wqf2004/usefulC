#include <stdio.h>
#include <string.h>

struct { // 这里先使用struct声明了一个匿名结构体，然后再使用这个匿名结构体定义了一个结构体变量book0
    char title[100];
    char author[100];
    char subject[100];
    int book_id;
} book0;

struct book // 这里的book是这个结构体的一个tag（命名标签），使用时用 struct book
{
    char title[50];
    char author[50];
    char subject[100];
    int book_id;
};

struct book_
{
    char title[50];
    char author[50];
    char subject[100];
    int book_id;
} book1, book2, book3; // 这里的book1~3表示用 struct book定义出来的三个结构体变量

typedef struct
{
    char title[50];
    char author[50];
    char subject[100];
    int book_id;
} Book_; // 这里的Book表示这个结构体的一个别名，使用时用 Book表示这个结构体数据类型

typedef struct book__
{
    char title[50];
    char author[50];
    char subject[100];
    int book_id;
} Book; // 可以在结构体内部使用 struct book * 指针（比如嵌套结构体或自引用）；

int main()
{
    // 初始化结构体变量
    struct book book4 = {"C语言入门", "王博士", "C语言编程", 123456};
    Book_ book5 = {.title = "C语言进阶", .author = "李教授", .subject = "编程进阶", .book_id = 123789};
    Book books[] = {
        {"数据结构（C语言）", "唐讲师", "数据结构", 456123},
        {"算法实训（C语言）", "张老师", "算法实践", 890457}
    };

    // 访问结构体变量
    printf("book4.title: %s\n", book4.title);
    printf("book5.author: %s\n", book5.author);
    printf("books[0].subject: %s\n", books[0].subject);
    
    // 给结构体变量的数据成员赋值/修改
    book0.book_id = 123450;
    strcpy(book0.title, "keil C51编程基础");
    strcpy(book0.author, "宋博士");
    strcpy(book0.subject, "嵌入式编程");
    printf("book0.title: %s\n", book0.title);
    
    
    return 0;
}
