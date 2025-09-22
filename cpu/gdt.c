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

// 完整的 TSS Entry 结构
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0; // Ring 0 的栈指针
    uint32_t ss0;  // Ring 0 的栈段选择子
    uint32_t esp1, ss1, esp2, ss2; // 其他特权级的栈（未使用）
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));


// 全局变量
struct gdt_entry gdt_entries[6];
struct gdt_ptr   gdt_p;
struct tss_entry tss;

// 外部汇编函数
extern void gdt_flush(uint32_t);
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

// 初始化 GDT 和 TSS
void init_gdt_tss() {
    gdt_p.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_p.base  = (uint32_t)&gdt_entries;

    // GDT 段描述符
    // 0x9A: P=1, DPL=0, S=1, Type=1010 (代码, 可执行, 可读)
    // 0x92: P=1, DPL=0, S=1, Type=0010 (数据, 可读写)
    // 0xFA: P=1, DPL=3, S=1, Type=1010 (用户代码)
    // 0xF2: P=1, DPL=3, S=1, Type=0010 (用户数据)
    // 0xCF: G=1, D/B=1 (4KB粒度, 32位)
    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

    // TSS 段描述符
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;
    
    // 0x89: P=1, DPL=0, S=0, Type=1001 (32-bit TSS, Available)
    // 0x00: G=0, D/B=0 (字节粒度)
    gdt_set_gate(5, base, limit, 0x89, 0x00);
    
    // 初始化 TSS
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = 0x10;  // 内核数据段选择子
    tss.esp0 = 0;    // 栈顶指针将在跳转前设置
    // iomap_base 必须大于或等于 TSS 的大小，以表示没有 I/O 权限位图
    tss.iomap_base = sizeof(tss);

    // 加载 GDT 和 TSS
    gdt_flush((uint32_t)&gdt_p);
    tss_flush();
}

// 设置内核栈顶指针
void tss_set_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}