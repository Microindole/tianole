#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include "common.h" // for uint32_t

// --- 声明内存设置函数 ---
void* memset(void* ptr, int value, uint32_t num);
void* memcpy(void* dest, const void* src, uint32_t num);

// 声明字符串比较函数
int strcmp(const char* s1, const char* s2);

// 声明字符串复制函数
void strcpy(char* dest, const char* src);

// 声明字符串长度函数
uint32_t strlen(const char* str);

// --- 声明字符串拼接和反转函数 ---
void strcat(char* dest, const char* src);
void strrev(char* str);

char toupper(char c);

// 比较两块内存区域的内容
int memcmp(const void* ptr1, const void* ptr2, uint32_t num);

#endif