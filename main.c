#include <stdlib.h>
#include <stdio.h>
#include "top.h"

// 线程安全保护宏
// #define MEM_STAT_ENTER_CRITICAL() taskENTER_CRITICAL()
// #define MEM_STAT_EXIT_CRITICAL()  taskEXIT_CRITICAL()


void main(void)
{
    char *temp = (char *)test_malloc(12);
    test_free(temp);
    return;
}