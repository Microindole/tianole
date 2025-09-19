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

    // 如果这是一个新创建的任务，则为其构建启动栈
    if (current_task->kernel_stack_ptr == 0 && current_task->initial_regs != NULL) {
        
        uint32_t stack_top = (uint32_t)kmalloc(4096) + 4096;
        uint32_t esp = stack_top;

        // 1. 在栈底放置 iret 帧 (使用之前保存的 initial_regs)
        esp -= sizeof(registers_t);
        memcpy((void*)esp, current_task->initial_regs, sizeof(registers_t));

        // --- 劫持 iret 的返回地址 ---
        registers_t* regs_on_stack = (registers_t*)esp;
        regs_on_stack->eip = (uint32_t)child_entry_point; // 将 EIP 指向新的入口函数
        regs_on_stack->eax = 0; // 确保子进程代码如果需要，可以访问 eax=0

        // 2. 释放 initial_regs，因为它是一次性的
        kfree(current_task->initial_regs);
        current_task->initial_regs = NULL;

        // 3. 在 iret 帧上构建 C 调用帧，返回到 trampoline
        esp -= 4;
        *(uint32_t*)esp = (uint32_t)fork_trampoline;
        // 4. 安全地构建 pusha 帧，避免内存重叠
        //    首先，将 GPRs 从 iret_frame 复制到一个临时缓冲区
        uint32_t tmp_gprs[8];
        memcpy(tmp_gprs, (void*)&((registers_t*)(stack_top - sizeof(registers_t)))->edi, 32);
        
        //    然后，将临时缓冲区的内容复制到最终的栈顶
        esp -= 32;
        memcpy((void*)esp, tmp_gprs, 32);

        // 4. 设置任务的最终 esp
        current_task->kernel_stack_ptr = esp;
    }

    if (current_task->directory != current_directory) {
        load_page_directory(current_task->directory);
        current_directory = current_task->directory;
    }

    switch_task(old_task, current_task);
}

