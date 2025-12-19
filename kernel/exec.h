#ifndef EXEC_H
#define EXEC_H

#include "common.h"
#include "../cpu/isr.h"

// 用户态程序的加载地址（类似Linux的0x08048000）
#define USER_PROGRAM_BASE 0x08048000

// 用户栈的起始地址（向下增长）
#define USER_STACK_TOP 0xC0000000

// 用户栈大小（4KB）
#define USER_STACK_SIZE 0x1000

// exec系统调用的实现
// 从文件系统加载可执行文件并在当前进程中执行
void syscall_exec(registers_t* regs);

// 内部函数：加载并执行用户程序
int do_exec(const char* filename);

#endif
