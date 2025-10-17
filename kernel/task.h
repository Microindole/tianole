#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "../cpu/isr.h" 
#include "../mm/paging.h"

// 定义任务的状态
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_DEAD
} task_state_t;

typedef struct task {
    uint32_t id;
    task_state_t state;
    
    // 指向任务内核栈顶的指针 (ESP)
    uint32_t kernel_stack_ptr;

    page_directory_t* directory;
    struct task* next;
    
    // 只用于新创建的进程，用来存放 fork 时保存的父进程中断状态
    registers_t* initial_regs; 
    
    struct task* parent;
    
} task_t;


void init_tasking();
void schedule();

extern void switch_task(volatile task_t* old, volatile task_t* new);

void list_processes();

#endif

