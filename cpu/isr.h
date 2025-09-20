#ifndef ISR_H
#define ISR_H

#include "common.h"

// CPU 保存的寄存器状态结构体 (最终修正版)
// 这个结构体的顺序必须与 isr.s 中 pusha 以及 CPU 自动压栈的顺序完全匹配
typedef struct registers {
    uint32_t ds;                                     // 数据段选择子
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // 由 pusha 推入
    uint32_t int_no, err_code;                       // 由我们的 ISR stub 推入
    uint32_t eip, cs, eflags, useresp, ss;           // 由 CPU 在特权级切换时自动推入
} registers_t;

// 定义中断处理函数指针
typedef void (*isr_t)(registers_t*);

// 注册一个中断处理函数
void register_interrupt_handler(uint8_t n, isr_t handler);

#endif