#include "gdt.h"
#include "common.h"
#include "string.h"
#include "tss.h"

#define GDT_ENTRIES 6

struct gdt_entry gdt_entries[GDT_ENTRIES];
struct gdt_ptr   gdt_p;
tss_entry_t tss_entry;

// 外部汇编函数
extern void gdt_flush(struct gdt_ptr*);
extern void tss_flush();

// 设置一个 GDT 条目
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// 初始化 TSS 的函数
static void tss_init(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    // 在 GDT 中添加 TSS 段描述符
    // Access byte: 0x89 = Present(1) | DPL=0(00) | Type=9 (32-bit TSS, not busy)
    gdt_set_gate(num, base, limit, 0x89, 0x40);

    // 初始化 TSS 结构体
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    
    // 将 CS 设置为内核代码段，其他段设置为内核数据段
    tss_entry.cs = 0x08;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x10;
}

// 更新 TSS 中的内核栈指针
void tss_set_stack(uint32_t esp0) {
   tss_entry.esp0 = esp0;
}

// 初始化 GDT 和 TSS
void init_gdt() {
    gdt_p.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_p.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // 0x00: Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 0x08: Kernel Code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 0x10: Kernel Data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // 0x18: User Code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // 0x20: User Data
    tss_init(5, 0x10, 0);                       // 0x28: TSS segment

    gdt_flush(&gdt_p);
    tss_flush();
}