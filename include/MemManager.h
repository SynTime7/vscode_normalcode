#ifndef __MEMMANGER_H_
#define __MEMMANGER_H_
#include <stdint.h>

// 线程安全保护宏
#define MEM_STAT_ENTER_CRITICAL()
#define MEM_STAT_EXIT_CRITICAL()

// 宏定义替换
#define test_malloc(size) my_malloc(size, __FILE__, __LINE__)
#define test_free(ptr) my_free(ptr, __FILE__, __LINE__)

void* my_malloc(size_t size, const char* file, int line);
void my_free(void* ptr, const char* file, int line);

#endif