#include <stdio.h>

/* 接受回调函数的冒泡排序函数
 * 参数：
 *  arr: 待排序数组
 *  n: 数组长度
 *  compare: 比较函数指针
*/
void bubblesort(int arr[], int n, int (*compare)(int, int)) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (compare(arr[i], arr[j]) > 0) // 以升序为例，如果前面的数arr[i]大于后面的数arr[j]则交换
            {
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

/* ***** 上面的排序中设置的交换时机决定了下方怎样的设计是升序，怎样的是降序 ***** */

// 升序排序调用
int compare_asc(int a, int b) {
    return a - b;
}

// 降序排序调用
int compare_desc(int a, int b) {
    return b - a;
}

int main() {
    int data[] = {64, 34, 25, 12, 22, 11, 90};
    int n = 7;

    printf("原数组: ");
    for (int i = 0; i < n; i++) printf("%d ", data[i]);
    printf("\n");

    // 使用升序回调
    bubblesort(data, n, compare_asc);
    printf("升序: ");
    for (int i = 0; i < n; i++) printf("%d ", data[i]);
    printf("\n");

    // 使用降序回调
    bubblesort(data, n, compare_desc);
    printf("降序: ");
    for (int i = 0; i < n; i++) printf("%d ", data[i]);
    printf("\n");

    return 0;
}