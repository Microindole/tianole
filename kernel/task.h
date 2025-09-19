#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "../cpu/isr.h" 
#include "../mm/paging.h"

// 定义任务的状态
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_SLEEPING,
    TASK_DEAD
} task_state_t;

// 任务（进程）控制块 (TCB) - 最终版
typedef struct task {
    uint32_t id;
    task_state_t state;
    
    // 指向任务内核栈顶的指针 (ESP)
    uint32_t kernel_stack_ptr;

    page_directory_t* directory;
    struct task* next;
    
    // 新增字段：只用于新创建的进程，用来存放 fork 时保存的父进程中断状态
    registers_t* initial_regs; 
    
} task_t;


// --- 函数声明 ---

void init_tasking();
void schedule();

// --- 关键修正：switch_task 现在接收 task_t 指针 ---
extern void switch_task(volatile task_t* old, volatile task_t* new);

#endif

