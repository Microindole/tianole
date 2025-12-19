#ifndef GDT_H
#define GDT_H

#include "common.h"

// GDT描述符结构
struct gdt_entry {
    uint16_t limit_low;    // 段界限 0-15位
    uint16_t base_low;     // 基址 0-15位
    uint8_t base_middle;   // 基址 16-23位
    uint8_t access;        // 访问标志
    uint8_t granularity;   // 粒度和段界限16-19位
    uint8_t base_high;     // 基址 24-31位
} __attribute__((packed));

// GDT指针结构（用于lgdt指令）
struct gdt_ptr {
    uint16_t limit;        // GDT大小-1
    uint32_t base;         // GDT基址
} __attribute__((packed));

// TSS结构（任务状态段）
struct tss_entry {
    uint32_t prev_tss;     // 前一个TSS
    uint32_t esp0;         // Ring 0栈指针
    uint32_t ss0;          // Ring 0栈段选择子
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

// 段选择子定义
#define KERNEL_CS 0x08  // 内核代码段
#define KERNEL_DS 0x10  // 内核数据段
#define USER_CS   0x18  // 用户代码段
#define USER_DS   0x20  // 用户数据段
#define TSS_SEG   0x28  // TSS段

// 初始化GDT
void init_gdt(void);

// 设置TSS的内核栈（当从用户态切换到内核态时使用）
void set_kernel_stack(uint32_t stack);

// 汇编函数（在gdt.s中实现）
extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

#endif
