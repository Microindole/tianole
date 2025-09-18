#ifndef IDT_H
#define IDT_H

#include "common.h"

// IDT 条目结构
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

// IDTR 寄存器结构
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// 初始化 IDT
void init_idt();

#endif