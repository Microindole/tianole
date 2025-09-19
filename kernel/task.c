#include "task.h"
#include "common.h"
#include "../mm/kheap.h" // 我们需要 kmalloc
#include "string.h"   // 我们需要 strcpy/memset 等
#include <stddef.h> // 定义NULL

// --- 声明外部变量和函数 ---
extern page_directory_t* current_directory;
extern void load_page_directory(page_directory_t*);
extern page_directory_t* kernel_directory; // 声明全局的内核页目录

// --- 全局变量 ---

// 指向当前正在运行的任务
volatile task_t* current_task;

// 任务队列的头指针（一个简单的链表）
volatile task_t* ready_queue;

// 全局任务 ID 计数器
uint32_t next_pid = 1;


// --- 函数实现 ---

// --- 任务 B 的入口函数 ---
void task_b_main() {
    asm volatile("sti"); // 手动开启中断
    while(1) {
        kprint("B");
        for (volatile int i = 0; i < 10000000; i++);
    }
}

// 初始化任务系统
void init_tasking() {
    asm volatile("cli");

    // 创建内核任务
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    memset(kernel_task, 0, sizeof(task_t));
    kernel_task->id = next_pid++;
    kernel_task->state = TASK_RUNNING;
    kernel_task->kernel_stack = 0;
    kernel_task->directory = kernel_directory;
    current_task = kernel_task;
    ready_queue = kernel_task;

    /* --- 暂时禁用任务 B ---
    // 创建任务 B
    task_t* task_b = (task_t*)kmalloc(sizeof(task_t));
    memset(task_b, 0, sizeof(task_t));
    task_b->id = next_pid++;
    task_b->state = TASK_READY;

    // 为任务 B 分配内核栈
    uint32_t stack = (uint32_t)kmalloc(4096);
    task_b->kernel_stack = stack;

    // 伪造初始上下文
    uint32_t esp = stack + 4096;
    esp -= 4;
    *(uint32_t*)esp = (uint32_t)task_b_main;
    esp -= 32;
    task_b->registers.esp = esp;

    // 设置循环链表
    kernel_task->next = task_b;
    task_b->next = kernel_task;
    */
   
    // --- 内核任务自己形成一个循环 ---
    kernel_task->next = kernel_task;


    asm volatile("sti");
    kprint("Tasking system initialized.\n");
}

// 实现调度器函数
void schedule() {
    volatile task_t* next_task = current_task->next;

    if (next_task == current_task) {
        return;
    }
    
    volatile task_t* old_task = current_task;
    current_task = next_task;

    // --- 切换页目录 ---
    // 如果新任务的页目录与当前的不同，就加载新的
    if (current_task->directory != current_directory) {
        load_page_directory(current_task->directory);
    }

    // 调用汇编实现的上下文切换
    switch_task((registers_t*)&old_task->registers, (registers_t*)&current_task->registers);
}