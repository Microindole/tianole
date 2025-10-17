#ifndef ISR_H
#define ISR_H

#include "common.h"

// CPU 保存的寄存器状态结构体
typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} registers_t;

// 定义中断处理函数指针
typedef void (*isr_t)(registers_t*);

// 注册一个中断处理函数
void register_interrupt_handler(uint8_t n, isr_t handler);

// 声明那个由汇编代码调用的、统一的 C 入口函数
void interrupt_handler(registers_t* regs);

#endif