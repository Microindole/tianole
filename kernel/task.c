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
    // kernel_task->initial_regs = NULL;
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
    // 1. 找到下一个要运行的任务
    volatile task_t* next_task = current_task->next;
    if (next_task == current_task) {
        return; // 如果没有其他任务，什么都不做
    }

    // --- BEGIN FINAL FIX ---
    
    // 2. 如果下一个任务是第一次运行(kernel_stack_ptr为0)，那么就在这里为它构建初始内核栈
    //    关键：此时我们仍然在旧任务的上下文中，全局的 current_task 还没被修改。
    if (next_task->kernel_stack_ptr == 0) {
        
        // 分配4K内存作为新任务的内核栈
        uint32_t stack_addr = (uint32_t)kmalloc(4096);
        next_task->kernel_stack_top = stack_addr + 4096;
        
        // 从栈顶开始构建 iret 帧
        uint32_t esp = next_task->kernel_stack_top;

        // a. 在栈底放置 iret 帧 (从 next_task->initial_regs 复制)
        esp -= sizeof(registers_t);
        memcpy((void*)esp, &next_task->initial_regs, sizeof(registers_t));
        
        // b. 放置 trampoline 的地址
        esp -= 4;
        *(uint32_t*)esp = (uint32_t)fork_trampoline;
        
        // c. 伪造一个 pusha 帧
        esp -= 32;
        memset((void*)esp, 0, 32);

        // d. 设置新任务的最终 esp
        next_task->kernel_stack_ptr = esp;
    }

    // 3. 保存旧任务的指针
    volatile task_t* old_task = current_task;
    
    // 4. 至此，新任务已完全准备就绪。现在，正式更新全局的 current_task 指针
    current_task = next_task;

    // --- END FINAL FIX ---

    // 5. 更新TSS的内核栈顶指针，使其指向新任务的栈
    tss_set_stack(current_task->kernel_stack_top);
    
    // 6. 如果需要，切换页目录
    if (current_task->directory != old_task->directory) { // 注意：这里和 old_task 比较
        load_page_directory(current_task->directory);
        current_directory = current_task->directory; // 更新全局 current_directory
    }

    // 7. 执行汇编级的上下文切换
    switch_task(old_task, current_task);
}