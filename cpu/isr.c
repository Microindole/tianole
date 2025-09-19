#include "isr.h"
#include "common.h"
#include <stddef.h>

isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// C-level exception handler (处理 CPU 异常)
void isr_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        kprint("Unhandled exception: ");
    }
}

// C-level IRQ handler (处理硬件中断)
void irq_handler(registers_t *regs) {
    // --- 关键修复：在这里发送 EOI 信号 ---
    // 在调用具体的中断处理函数之前发送 EOI。
    // 这样，即使处理函数内部发生了任务切换，中断控制器也已经被正确重置，
    // 不会阻塞后续的键盘等其他中断。
    
    // 如果中断号来自从片 (IRQ 8-15 -> int 40-47)
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20); // 发送 EOI 到从片
    }
    // 总是需要发送 EOI 到主片
    outb(0x20, 0x20);

    // 现在可以安全地调用注册的具体处理函数了
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}