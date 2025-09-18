#include "isr.h"
#include "common.h"
#include <stddef.h> // for NULL

// 定义一个函数指针数组来存储中断处理函数
isr_t interrupt_handlers[256];

// 注册中断处理函数
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// 所有中断最终都会调用这个 C 语言的总入口
// 这个函数由汇编代码 isr_common_stub 调用
void isr_handler(registers_t* regs) {
    // 检查是否有为这个中断注册的处理函数
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        kprint("Unhandled interrupt: ");
        // (此处需要一个整数转字符串的函数来打印 regs->int_no)
    }
}

// IRQ 处理函数，它会发送 EOI 信号给 PIC
void irq_handler(registers_t *regs) {
    // 发送 EOI (End of Interrupt) 信号给 PICs
    // 如果中断来自从片(IRQ 8-15)，也需要通知从片
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20); // 发送重置信号给从片
    }
    outb(0x20, 0x20); // 发送重置信号给主片

    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}