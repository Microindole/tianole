#include "task.h"
#include "common.h"
#include "../mm/kheap.h" // 我们需要 kmalloc
#include "string.h"   // 我们需要 strcpy/memset 等
#include <stddef.h> // 定义NULL

// --- 全局变量 ---

// 指向当前正在运行的任务
volatile task_t* current_task;

// 任务队列的头指针（一个简单的链表）
volatile task_t* ready_queue;

// 全局任务 ID 计数器
uint32_t next_pid = 1;


// --- 函数实现 ---

// --- 新增一个任务函数 ---
void task_b_main() {
    while(1) {
        kprint("B");
        // 伪造一个延迟，防止刷屏太快
        for (volatile int i = 0; i < 10000000; i++);
    }
}

// 初始化任务系统
void init_tasking() {
    asm volatile("cli");

    // 创建第一个任务，即内核本身
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->id = next_pid++;
    kernel_task->state = TASK_RUNNING;
    kernel_task->kernel_stack = 0; 
    kernel_task->next = NULL;
    current_task = kernel_task;
    ready_queue = kernel_task;

    // --- 创建第二个任务 (修改部分) ---
    task_t* task_b = (task_t*)kmalloc(sizeof(task_t));
    task_b->id = next_pid++;
    task_b->state = TASK_READY;

    // 为新任务分配内核栈
    uint32_t stack = (uint32_t)kmalloc(4096);
    task_b->kernel_stack = stack; // 保存栈底，以便未来释放

    // --- 伪造初始上下文 ---
    // 栈顶在高地址
    uint32_t esp = stack + 4096;

    // 1. 将 EIP (task_b_main 的地址) 压入栈，这是给 ret 指令使用的
    esp -= 4;
    *(uint32_t*)esp = (uint32_t)task_b_main;
    
    // 2. 将 8 个通用寄存器的初始值 (我们设为 0) 压入栈，这是给 popa 指令使用的
    esp -= 32; // 8 registers * 4 bytes
    // (我们不需要真的写入0，因为 kmalloc 返回的内存通常是脏的，
    //  第一次 popa 恢复成什么不重要)

    // 3. 将这个伪造的栈顶保存到任务结构体中
    task_b->registers.esp = esp;
    
    // 将新任务加入到就绪队列
    kernel_task->next = task_b;
    task_b->next = ready_queue; // <--- 修改：让任务 B 的下一个指向队列头部，形成循环

    asm volatile("sti");
    kprint("Tasking system initialized.\n");
}

// 实现调度器函数
void schedule() {
    if (current_task == NULL || ready_queue == NULL) {
        return;
    }

    // --- 修改：将 next_task 声明为 volatile ---
    volatile task_t* next_task = current_task->next;
    if (next_task == NULL) {
        next_task = ready_queue; // 循环回队列头部
    }
    
    if (next_task == current_task) {
        return;
    }

    // --- 修改：使用 volatile 指针 ---
    volatile task_t* old_task = current_task;
    current_task = next_task;

    switch_task((registers_t*)&old_task->registers, (registers_t*)&next_task->registers);
}