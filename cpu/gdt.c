#include "cpu/gdt.h"
#include "common.h"

// GDT表（6个条目）
static struct gdt_entry gdt_entries[6];
static struct gdt_ptr gdt_ptr_struct;

// TSS
static struct tss_entry tss;

// 设置GDT条目
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;

    gdt_entries[num].access = access;
}

// 初始化GDT
void init_gdt(void) {
    gdt_ptr_struct.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_ptr_struct.base = (uint32_t)&gdt_entries;

    // 0x00: 空描述符
    gdt_set_gate(0, 0, 0, 0, 0);

    // 0x08: 内核代码段
    // 基址=0, 界限=0xFFFFFFFF (4GB)
    // 访问字节: 1001 1010 = 0x9A
    //   Present=1, DPL=00(Ring 0), Type=1(代码段), Executable=1, Readable=1
    // 粒度: 1100 1111 = 0xCF
    //   Granularity=1(4KB页), Size=1(32位), 界限高4位=F
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 0x10: 内核数据段
    // 访问字节: 1001 0010 = 0x92
    //   Present=1, DPL=00(Ring 0), Type=0(数据段), Writable=1
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 0x18: 用户代码段
    // 访问字节: 1111 1010 = 0xFA
    //   Present=1, DPL=11(Ring 3), Type=1(代码段), Executable=1, Readable=1
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 0x20: 用户数据段
    // 访问字节: 1111 0010 = 0xF2
    //   Present=1, DPL=11(Ring 3), Type=0(数据段), Writable=1
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 0x28: TSS
    // 访问字节: 1110 1001 = 0xE9
    //   Present=1, DPL=11(Ring 3), Type=1001(32位TSS)
    // 先初始化TSS结构
    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(struct tss_entry) - 1;

    // 清零TSS
    for (int i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }

    // 设置TSS的关键字段
    tss.ss0 = KERNEL_DS;   // 内核数据段
    tss.esp0 = 0;          // 稍后通过set_kernel_stack设置
    tss.cs = KERNEL_CS | 3;   // 用户代码段（加上RPL=3）
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = USER_DS | 3;

    gdt_set_gate(5, tss_base, tss_limit, 0xE9, 0x00);

    // 加载GDT
    gdt_flush((uint32_t)&gdt_ptr_struct);

    // 加载TSS
    tss_flush();
}

// 设置TSS中的内核栈指针
// 当从用户态陷入内核态时，CPU会从TSS中读取esp0作为内核栈
void set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}
