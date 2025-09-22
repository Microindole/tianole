#ifndef COMMON_H
#define COMMON_H

// 定义一些标准类型
typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

// 端口 I/O 函数
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t value);

// 内核打印函数声明 (来自 kernel.c)
void kprint(const char* str);
void kputc(char c);
void move_cursor();
void clear_screen();

// --- 最终修正：使用绝对安全的、带有缓冲区大小参数的 itoa ---
void itoa(int n, char* str, int len, int base);
void itoa_hex(uint32_t n, char* str, int len);

#endif

