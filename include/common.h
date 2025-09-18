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

// 内核打印函数声明 (来自 kernel.c)
void kprint(const char* str);
void kputc(char c);
void move_cursor();
void clear_screen();

#endif