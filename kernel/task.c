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
    if (!current_task) return;

    // --- 步骤 1: 清理所有已死亡的任务 ---
    // 这是一个更安全的清理循环。我们从当前任务的下一个开始检查，
    // 确保我们永远不会在循环中释放 current_task 本身。
    volatile task_t* iter = current_task;
    while (iter->next != current_task) {
        if (iter->next->state == TASK_DEAD) {
            volatile task_t* dead_task = iter->next;
            // 从链表中移除
            iter->next = dead_task->next; 
            
            // 释放相关内存
            if (dead_task->initial_regs) {
                kfree((void*)dead_task->initial_regs);
            }
            kfree((void*)dead_task);
        } else {
            iter = iter->next;
        }
    }

    // --- 步骤 2: 寻找下一个可以运行的任务 ---
    volatile task_t* next_task = current_task;
    do {
        next_task = next_task->next;
    } while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING);

    // 如果转了一圈都没找到，或者只有当前任务自己，就不切换
    if (next_task == current_task) {
        return;
    }

    // --- 步骤 3: 执行任务切换 ---
    volatile task_t* old_task = current_task;
    current_task = next_task;

    if (old_task->state != TASK_DEAD) {
        old_task->state = TASK_READY;
    }
    current_task->state = TASK_RUNNING;

    // --- 步骤 4: 如果是新任务，为其构建内核栈 (这部分逻辑不变) ---
    if (current_task->kernel_stack_ptr == 0 && current_task->initial_regs != NULL) {
        uint32_t stack_top = (uint32_t)kmalloc(4096) + 4096;
        uint32_t esp = stack_top;
        
        esp -= sizeof(registers_t);
        memcpy((void*)esp, current_task->initial_regs, sizeof(registers_t));

        registers_t* regs_on_stack = (registers_t*)esp;
        regs_on_stack->eip = (uint32_t)child_entry_point;
        regs_on_stack->eax = 0;

        kfree(current_task->initial_regs);
        current_task->initial_regs = NULL;

        esp -= 4;
        *(uint32_t*)esp = (uint32_t)fork_trampoline;
        
        uint32_t tmp_gprs[8];
        memcpy(tmp_gprs, (void*)&((registers_t*)(stack_top - sizeof(registers_t)))->edi, 32);
        esp -= 32;
        memcpy((void*)esp, tmp_gprs, 32);

        current_task->kernel_stack_ptr = esp;
    }

    // --- 步骤 5: 切换页目录并调用汇编代码 (不变) ---
    if (current_task->directory != current_directory) {
        load_page_directory(current_task->directory);
        current_directory = current_task->directory;
    }

    switch_task(old_task, current_task);
}

// --- 实现 list_processes 函数 ---
void list_processes() {
    kprint("\nPID   STATE\n");
    kprint("-----------\n");

    if (!current_task) {
        kprint("Tasking not initialized.\n");
        return;
    }

    volatile task_t* start_task = current_task;
    volatile task_t* p = current_task;

    char pid_buf[8];
    char* state_str;

    do {
        // 打印 PID
        itoa(p->id, pid_buf, 8, 10);
        kprint(pid_buf);
        kprint("     ");

        // 打印状态
        switch(p->state) {
            case TASK_RUNNING:
                state_str = "RUNNING";
                break;
            case TASK_READY:
                state_str = "READY";
                break;
            default:
                state_str = "UNKNOWN";
                break;
        }
        kprint(state_str);
        kprint("\n");

        p = p->next;
    } while (p != start_task);
}

