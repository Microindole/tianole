#include "task.h"
#include "common.h"
#include "../mm/kheap.h"
#include "string.h"
#include <stddef.h>

// 外部变量和函数
extern page_directory_t* current_directory;
extern void fork_trampoline();

// 全局变量
volatile task_t* current_task;
volatile task_t* ready_queue;
uint32_t next_pid = 1;

// 初始化任务系统
void init_tasking() {
    asm volatile("cli");

    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->id = next_pid++;
    kernel_task->state = TASK_RUNNING;
    kernel_task->kernel_stack_ptr = 0;
    kernel_task->initial_regs = NULL;
    kernel_task->directory = current_directory;

    current_task = kernel_task;
    ready_queue = kernel_task;
    kernel_task->next = kernel_task; // 形成循环链表

    asm volatile("sti");
    kprint("Tasking system initialized.\n");
}

// 外部函数声明，以便我们可以获取它的地址
extern void child_entry_point(void);

// 调度器
void schedule() {
    volatile task_t* next_task = current_task->next;
    if (next_task == current_task) {
        return;
    }

    volatile task_t* old_task = current_task;
    current_task = next_task;

    // 如果这是一个新创建的任务 (用户或fork出的)
    if (current_task->kernel_stack_ptr == 0 && current_task->initial_regs != NULL) {
        
        uint32_t stack_top = (uint32_t)kmalloc(4096) + 4096;
        uint32_t esp = stack_top;

        // --- 核心修正：正确地构建启动栈 ---
        
        // 1. 在栈底放置 iret 帧 (包含了用户态或子进程的寄存器)
        esp -= sizeof(registers_t);
        memcpy((void*)esp, current_task->initial_regs, sizeof(registers_t));
        
        // 2. 在 iret 帧之上，放置 trampoline 的地址，作为 switch_task 的返回地址
        esp -= 4;
        *(uint32_t*)esp = (uint32_t)fork_trampoline;
        
        // 3. 在 trampoline 地址之上，伪造一个 pusha 帧
        //    对于第一次运行的任务，这些寄存器的值不重要，但必须占位
        esp -= 32;
        memset((void*)esp, 0, 32);

        // 4. 释放一次性的 initial_regs
        kfree(current_task->initial_regs);
        current_task->initial_regs = NULL;

        // 5. 设置任务的最终 esp
        current_task->kernel_stack_ptr = esp;
    }

    if (current_task->directory != current_directory) {
        load_page_directory(current_task->directory);
        current_directory = current_task->directory;
    }

    switch_task(old_task, current_task);
}