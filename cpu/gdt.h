#ifndef GDT_H
#define GDT_H

#include "common.h"

// GDT 条目结构
struct gdt_entry_struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// GDTR 寄存器结构
struct gdt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

// TSS 条目结构
struct tss_entry_struct {
    uint32_t prev_tss;
    uint32_t esp0, ss0; // 我们最关心的：Ring 0 的栈指针和段选择子
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;

// GDT 初始化函数
void init_gdt();

#endif