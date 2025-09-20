#include "task.h"
#include "common.h"
#include "../mm/kheap.h"
#include "string.h"
#include <stddef.h>
#include "../cpu/tss.h"

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

    // 为一个全新的任务创建并设置内核栈
    if (current_task->kernel_stack_ptr == 0 && current_task->initial_regs != NULL) {
        
        // 分配4K内存作为内核栈
        uint32_t stack_addr = (uint32_t)kmalloc(4096);
        // 【关键修正】保存栈的最高地址
        current_task->kernel_stack_top = stack_addr + 4096;
        
        // 从栈顶开始构建 iret 帧
        uint32_t esp = current_task->kernel_stack_top;

        // 1. 在栈底放置 iret 帧
        esp -= sizeof(registers_t);
        memcpy((void*)esp, current_task->initial_regs, sizeof(registers_t));
        
        // 2. 放置 trampoline 的地址
        esp -= 4;
        *(uint32_t*)esp = (uint32_t)fork_trampoline;
        
        // 3. 伪造一个 pusha 帧
        esp -= 32;
        memset((void*)esp, 0, 32);

        // 释放一次性的 initial_regs
        kfree(current_task->initial_regs);
        current_task->initial_regs = NULL;

        // 设置任务的最终 esp
        current_task->kernel_stack_ptr = esp;
    }

    // 【关键修正】TSS的esp0必须永远指向栈的最高处、最干净的位置
    tss_set_stack(current_task->kernel_stack_top);
    
    if (current_task->directory != current_directory) {
        load_page_directory(current_task->directory);
        current_directory = current_task->directory;
    }

    switch_task(old_task, current_task);
}