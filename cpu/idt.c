#include "idt.h"
#include "common.h"
#include <stddef.h> // for NULL

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// --- 声明所有在 isr.s 中定义的汇编 ISR 入口 (无省略) ---
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr32(); extern void isr33(); extern void isr34(); extern void isr35();
extern void isr36(); extern void isr37(); extern void isr38(); extern void isr39();
extern void isr40(); extern void isr41(); extern void isr42(); extern void isr43();
extern void isr44(); extern void isr45(); extern void isr46(); extern void isr47();
extern void isr128();


// 全局 IDT
struct idt_entry idt_entries[256];
struct idt_ptr   idt_p;

// 外部汇编函数
extern void idt_load(struct idt_ptr*);

// 设置一个 IDT 条目
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

// PIC 重映射
void pic_remap() {
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20); // Master PIC remap to 32
    outb(PIC2_DATA, 0x28); // Slave PIC remap to 40
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);
}

// 初始化 IDT
void init_idt() {
    idt_p.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_p.base  = (uint32_t)&idt_entries;

    for(int i = 0; i < 256; i++) {
        idt_entries[i] = (struct idt_entry){0};
    }

    pic_remap();

    // 设置 ISRs (0-31 for exceptions, 32-47 for IRQs) (无省略)
    uint16_t sel = 0x08; // Kernel code segment
    uint8_t flags = 0x8E; // Interrupt gate
    
    idt_set_gate(0, (uint32_t)isr0, sel, flags);
    idt_set_gate(1, (uint32_t)isr1, sel, flags);
    idt_set_gate(2, (uint32_t)isr2, sel, flags);
    idt_set_gate(3, (uint32_t)isr3, sel, flags);
    idt_set_gate(4, (uint32_t)isr4, sel, flags);
    idt_set_gate(5, (uint32_t)isr5, sel, flags);
    idt_set_gate(6, (uint32_t)isr6, sel, flags);
    idt_set_gate(7, (uint32_t)isr7, sel, flags);
    idt_set_gate(8, (uint32_t)isr8, sel, flags);
    idt_set_gate(9, (uint32_t)isr9, sel, flags);
    idt_set_gate(10, (uint32_t)isr10, sel, flags);
    idt_set_gate(11, (uint32_t)isr11, sel, flags);
    idt_set_gate(12, (uint32_t)isr12, sel, flags);
    idt_set_gate(13, (uint32_t)isr13, sel, flags);
    idt_set_gate(14, (uint32_t)isr14, sel, flags);
    idt_set_gate(15, (uint32_t)isr15, sel, flags);
    idt_set_gate(16, (uint32_t)isr16, sel, flags);
    idt_set_gate(17, (uint32_t)isr17, sel, flags);
    idt_set_gate(18, (uint32_t)isr18, sel, flags);
    idt_set_gate(19, (uint32_t)isr19, sel, flags);
    idt_set_gate(20, (uint32_t)isr20, sel, flags);
    idt_set_gate(21, (uint32_t)isr21, sel, flags);
    idt_set_gate(22, (uint32_t)isr22, sel, flags);
    idt_set_gate(23, (uint32_t)isr23, sel, flags);
    idt_set_gate(24, (uint32_t)isr24, sel, flags);
    idt_set_gate(25, (uint32_t)isr25, sel, flags);
    idt_set_gate(26, (uint32_t)isr26, sel, flags);
    idt_set_gate(27, (uint32_t)isr27, sel, flags);
    idt_set_gate(28, (uint32_t)isr28, sel, flags);
    idt_set_gate(29, (uint32_t)isr29, sel, flags);
    idt_set_gate(30, (uint32_t)isr30, sel, flags);
    idt_set_gate(31, (uint32_t)isr31, sel, flags);

    idt_set_gate(32, (uint32_t)isr32, sel, flags); // Timer
    idt_set_gate(33, (uint32_t)isr33, sel, flags); // Keyboard
    idt_set_gate(34, (uint32_t)isr34, sel, flags);
    idt_set_gate(35, (uint32_t)isr35, sel, flags);
    idt_set_gate(36, (uint32_t)isr36, sel, flags);
    idt_set_gate(37, (uint32_t)isr37, sel, flags);
    idt_set_gate(38, (uint32_t)isr38, sel, flags);
    idt_set_gate(39, (uint32_t)isr39, sel, flags);
    idt_set_gate(40, (uint32_t)isr40, sel, flags);
    idt_set_gate(41, (uint32_t)isr41, sel, flags);
    idt_set_gate(42, (uint32_t)isr42, sel, flags);
    idt_set_gate(43, (uint32_t)isr43, sel, flags);
    idt_set_gate(44, (uint32_t)isr44, sel, flags);
    idt_set_gate(45, (uint32_t)isr45, sel, flags);
    idt_set_gate(46, (uint32_t)isr46, sel, flags);
    idt_set_gate(47, (uint32_t)isr47, sel, flags);

    // 为系统调用设置中断门 (int 0x80)
    // 注意：这里的 flags 是 0xEE，而不是 0x8E
    // DPL(Descriptor Privilege Level) 设置为 3，允许用户态代码通过 int 指令触发
    idt_set_gate(128, (uint32_t)isr128, sel, 0xEE);

    // 加载 IDT
    idt_load(&idt_p);
}