#include "string.h"
#include "common.h" // for uint32_t

// 内存复制函数
void* memcpy(void* dest, const void* src, uint32_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (uint32_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
    return dest;
}

// 内存设置函数
void* memset(void* ptr, int value, uint32_t num) {
    unsigned char* p = ptr;
    unsigned char v = (unsigned char)value;
    for (uint32_t i = 0; i < num; i++) {
        p[i] = v;
    }
    return ptr;
}

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

// --- 字符串拼接函数 ---
void strcat(char* dest, const char* src) {
    // 移动到目标字符串的末尾
    while (*dest) {
        dest++;
    }
    // 复制源字符串
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// --- 字符串反转函数 ---
void strrev(char* str) {
    int start = 0;
    int end = strlen(str) - 1;
    char temp;

    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}


char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}