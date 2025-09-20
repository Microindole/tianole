#include "gdt.h"
#include "common.h"

#define GDT_ENTRIES 5

struct gdt_entry gdt_entries[GDT_ENTRIES];
struct gdt_ptr   gdt_p;

// 外部汇编函数，用于加载新的 GDT
extern void gdt_flush(struct gdt_ptr*);

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

// 初始化 GDT
void init_gdt() {
    gdt_p.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_p.base  = (uint32_t)&gdt_entries;

    // NULL 段
    gdt_set_gate(0, 0, 0, 0, 0);
    // 内核代码段 (Ring 0)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // 内核数据段 (Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    // 用户代码段 (Ring 3)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    // 用户数据段 (Ring 3)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush(&gdt_p);
}