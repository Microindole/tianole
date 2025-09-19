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
    // 只保留调用具体处理函数的逻辑
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}