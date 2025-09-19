#include "timer.h"
#include "../cpu/isr.h"
#include "common.h"
#include "../kernel/task.h"

// --- 外部变量和函数 ---
extern volatile task_t* current_task;
void schedule();

uint32_t tick = 0;

static void timer_callback(registers_t* regs) {
    tick++;
    
    // --- 调用调度器 ---
    // 每 5 个 tick (大约 100ms) 进行一次任务切换
    if (tick % 5 == 0) {
        schedule();
    }
}

void init_timer(uint32_t frequency) {
    // 注册定时器中断的处理函数 (IRQ0 -> INT 32)
    register_interrupt_handler(32, &timer_callback);

    // 计算分频值
    uint32_t divisor = 1193180 / frequency;

    // 设置 PIT 的模式和频率
    outb(0x43, 0x36); // Command port
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    // 发送分频值
    outb(0x40, l);
    outb(0x40, h);
}