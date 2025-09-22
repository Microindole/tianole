// 文件: cpu/isr.c

#include "isr.h"
#include "common.h"
#include <stddef.h>

// isr_t 和 register_interrupt_handler 不变
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
        // --- 关键修正 ---
        // 1. 打印更详细的异常信息，包含异常号
        kprint("\nUnhandled exception: ");
        char buf[8];
        itoa(regs->int_no, buf, 8, 10); // 将整数转换为字符串
        kprint(buf);

        // 2. 打印导致错误指令的地址 (EIP)
        kprint("\nEIP: ");
        itoa_hex(regs->eip, buf, 8);
        kprint(buf);

        // 3. 停机，防止无限循环
        kprint("\nSystem Halted!");
        for(;;);
    }
}

// C-level IRQ handler (处理硬件中断)
void irq_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}

// 统一的中断处理入口
void interrupt_handler(registers_t* regs) {
    if (regs->int_no < 32) {
        isr_handler(regs);
    } else {
        irq_handler(regs);
    }
}