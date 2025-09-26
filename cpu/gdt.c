#include "gdt.h"
#include "common.h"
#include "string.h" // For memset

#define GDT_ENTRIES 6 // 从 5 增加到 6

gdt_entry_t gdt_entries[GDT_ENTRIES];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);
extern void tss_flush(); // 声明新的汇编函数

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

// 写入 TSS 条目到 GDT
static void write_tss(int32_t num) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry);

    gdt_set_gate(num, base, limit, 0xE9, 0x00); // 0xE9: Present, DPL=3, TSS
}

// 初始化 GDT 和 TSS
void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code (Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data (Ring 0)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code (Ring 3)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data (Ring 3)
    write_tss(5);                               // TSS Segment

    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0 = 0x10;  // 0x10 是内核数据段的选择子
    tss_entry.esp0 = 0x0;  // esp0 会在 exec 时被正确设置，这里先清零

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
    
    kprint("GDT and TSS initialized.\n");
}

// 由 exec 调用，用于设置中断发生时要切换到的内核栈
void set_kernel_stack(uint32_t stack) {
   tss_entry.esp0 = stack;

   // --- 调试打印 ---
   serial_print("TSS.esp0 set to: ");
   char buf[12];
   itoa_hex(stack, buf, 12);
   serial_print(buf);
   serial_print("\n");
}