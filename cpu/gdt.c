#include "gdt.h"
#include "string.h"

// GDT Entry 结构
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// GDTR
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// TSS Entry 结构
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0, ss0; // Ring 0 的栈指针和段选择子
    // ... 其他我们用不到的字段
} __attribute__((packed));


struct gdt_entry gdt_entries[5];
struct gdt_ptr   gdt_p;
struct tss_entry tss;

extern void gdt_flush(uint32_t);
extern void tss_flush();

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt_tss() {
    gdt_p.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_p.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

    // 设置 TSS
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss);
    gdt_set_gate(5, base, limit, 0x89, 0x40); // TSS Segment
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = 0x10; // Kernel Data Segment Selector
    // esp0 将在每次任务切换时更新

    gdt_flush((uint32_t)&gdt_p);
    tss_flush();
}

void tss_set_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}