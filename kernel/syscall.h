#ifndef SYSCALL_H
#define SYSCALL_H

#include "../cpu/isr.h"

// 系统调用处理函数的函数指针类型
typedef void (*syscall_handler_t)(registers_t* regs);

// 初始化系统调用
void init_syscalls();

// fork 的C语言包装函数
int fork();

void exit();

// waitpid 的C语言包装函数
int waitpid(int pid);

#endif