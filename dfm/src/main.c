#include <stdio.h>
#include "file_op.h"

int main()
{
    if (file_add("../data/b.txt", "\n大家好") == 0)
    {
        printf("向data/b.txt中追加大家好");
    }
    getchar();
    return 0;
}