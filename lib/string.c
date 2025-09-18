#include "string.h"
#include "common.h" // for uint32_t

// 字符串比较函数
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 字符串复制函数
void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// 字符串长度函数
uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}