// cpu/isr.c (最终决战版)

#include "isr.h"
#include "common.h"
#include <stddef.h>

// isr_t 和 register_interrupt_handler 不变
isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// 统一的中断处理入口，由 isr.s 调用
void interrupt_handler(registers_t* regs) {
    // 首先，判断是 CPU 异常还是硬件中断(IRQ)
    if (regs->int_no < 32) {
        // --- CPU 异常处理 ---
        if (interrupt_handlers[regs->int_no] != NULL) {
            isr_t handler = interrupt_handlers[regs->int_no];
            handler(regs);
        } else {
            kprint("\nUnhandled exception: ");
            char buf[8];
            itoa(regs->int_no, buf, 8, 10);
            kprint(buf);
            kprint("\nSystem Halted!");
            for(;;);
        }
    } else {
        // --- 硬件中断(IRQ)处理 ---
        
        // 【核心】在调用具体处理函数之前，提前发送 EOI 信号
        if (regs->int_no >= 40) {
            outb(0xA0, 0x20); // 发送 EOI 到从片
        }
        outb(0x20, 0x20); // 发送 EOI 到主片

        // 现在可以安全地调用具体的中断处理函数了
        if (interrupt_handlers[regs->int_no] != NULL) {
            isr_t handler = interrupt_handlers[regs->int_no];
            handler(regs); // 这个函数内部可能会切换任务，但没关系了
        }
    }
}