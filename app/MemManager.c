#include "MemManager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/************************************************************
 *                          内存管理
 *************************************************************/
// 统计变量
static size_t g_total_allocated = 0;
static size_t g_max_allocated = 0;

void* my_malloc(size_t size, const char* file, int line)
{
    size_t total_size = size + sizeof(size_t);
    size_t* ptr = (size_t*)malloc(total_size);
    if (ptr != NULL)
    {
        *ptr = size; // 记录大小
        MEM_STAT_ENTER_CRITICAL();
        g_total_allocated += size;
        if (g_total_allocated > g_max_allocated)
        {
            g_max_allocated = g_total_allocated;
        }
        MEM_STAT_EXIT_CRITICAL();

        printf("[MEM] Alloc %zu bytes at %p (%s:%d), total allocated: %zu.\n", size, (void*)(ptr + 1), file, line, g_total_allocated);
        return (void*)(ptr + 1);
    }
    else
    {
        printf("[MEM] Alloc failed %zu bytes (%s:%d)\n", size, file, line);
        return NULL;
    }
}

void my_free(void* ptr, const char* file, int line)
{
    if (ptr != NULL)
    {
        size_t* real_ptr = (size_t*)ptr - 1;
        size_t size = *real_ptr;
        free(real_ptr);

        MEM_STAT_ENTER_CRITICAL();
        if (g_total_allocated >= size)
        {
            g_total_allocated -= size;
        }
        else
        {
            g_total_allocated = 0;
        }
        MEM_STAT_EXIT_CRITICAL();

        printf("[MEM] Free %zu bytes at %p (%s:%d), total allocated: %zu\n", size, ptr, file, line, g_total_allocated);
    }
}