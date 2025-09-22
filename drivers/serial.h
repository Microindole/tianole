#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"

// 初始化串口
void init_serial();

// 向串口打印一个字符串
void serial_print(const char* str);

// 向串口发送一个字符
void serial_putc(char a);

#endif