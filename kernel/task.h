#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "../cpu/isr.h" // 我们需要 registers_t 结构体

// 定义任务的状态
typedef enum {
    TASK_RUNNING,   // 正在运行
    TASK_READY,     // 准备就绪，可以被调度
    TASK_SLEEPING,  // 正在休眠
    TASK_DEAD       // 已结束，等待被清理
} task_state_t;

// 任务（进程）控制块 (TCB)
typedef struct task {
    uint32_t id;                // 任务的唯一 ID (PID)
    task_state_t state;         // 任务当前的状态

    registers_t registers;      // 任务被切换出时保存的寄存器状态
    
    uint32_t kernel_stack;      // 指向任务内核栈顶的指针 (ESP)

    struct task* next;          // 指向任务队列中的下一个任务
} task_t;


// --- 函数声明 ---

// 初始化任务系统
void init_tasking();

// 调度器函数
void schedule();

// 创建一个新任务
int create_task(void (*entry)(), uint32_t flags);

// 声明汇编实现的上下文切换函数
extern void switch_task(registers_t* old, registers_t* new);

#endif