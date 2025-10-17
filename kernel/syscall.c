#include "syscall.h"
#include "common.h"
#include "task.h"
#include "string.h"
#include "../mm/kheap.h"
#include <stddef.h>

// 外部变量
extern volatile task_t* current_task;
extern uint32_t next_pid;

// 外部函数
extern void fork_trampoline();
page_directory_t* clone_directory(page_directory_t* src);

// exit 的C语言包装函数
void exit() {
    asm volatile ("int $0x80" : : "a" (0)); // 0 号系统调用：exit
}

// fork 的C语言包装函数
int fork() {
    int pid;
    asm volatile (
        "int $0x80"
        : "=a" (pid)
        : "a" (1)
    );
    return pid;
}

// waitpid 的C语言包装函数
int waitpid(int pid) {
    int status;
    asm volatile (
        "int $0x80"
        : "=a" (status)
        : "a" (2), "b" (pid) // 2号系统调用：waitpid
    );
    return status;
}

// 克隆页目录
page_directory_t* clone_directory(page_directory_t* src) {
    page_directory_t* dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    if (!dir) { return NULL; }
    memset(dir, 0, sizeof(page_directory_t));

    for (int i = 0; i < 1024; i++) {
        if (src->entries[i] != 0) {
            dir->entries[i] = src->entries[i];
        }
    }
    return dir;
}


// fork 的系统调用实现
void syscall_fork(registers_t* regs) {
    asm volatile("cli");
    
    task_t* parent_task = (task_t*)current_task;

    task_t* child_task = (task_t*)kmalloc(sizeof(task_t));
    child_task->id = next_pid++;
    child_task->state = TASK_READY;
    child_task->directory = clone_directory(parent_task->directory);
    child_task->kernel_stack_ptr = 0;

    child_task->parent = parent_task;

    child_task->initial_regs = (registers_t*)kmalloc(sizeof(registers_t));
    memcpy(child_task->initial_regs, regs, sizeof(registers_t));
    child_task->initial_regs->eax = 0; // 子进程的 fork 返回 0

    regs->eax = child_task->id; // 父进程的 fork 返回子进程 ID

    // 将子进程插入到任务队列中
    child_task->next = parent_task->next;
    parent_task->next = child_task;
    
    asm volatile("sti");
}

// --- waitpid 的系统调用实现 ---
void syscall_waitpid(registers_t* regs) {
    int pid_to_wait_for = regs->ebx;

    // 简单起见，我们暂时只支持等待任意子进程 (-1)
    // 并且我们在这里只实现阻塞等待
    
    // 检查是否有任何子进程已经是僵尸(DEAD)状态
    _Bool child_exists = 0;
    volatile task_t* iter = current_task->next;
    while (iter != current_task) {
        if (iter->parent == current_task) {
            child_exists = 1;
            if (iter->state == TASK_DEAD) {
                // 如果已经有子进程结束了，直接清理并返回
                // (此处应有更完善的清理逻辑，暂时简化)
                regs->eax = iter->id;
                return;
            }
        }
        iter = iter->next;
    }

    if (!child_exists) {
        regs->eax = -1; // 没有子进程可以等待
        return;
    }

    // 如果有子进程但都还在运行，则将自己设置为等待状态
    current_task->state = TASK_WAITING;
    schedule(); // 主动放弃CPU
}

// --- exit 的系统调用实现 ---
void syscall_exit(registers_t* regs) {
    asm volatile("cli");

    task_t* task_to_exit = (task_t*)current_task;

    // 标记为死亡
    task_to_exit->state = TASK_DEAD;

    if (task_to_exit->parent && task_to_exit->parent->state == TASK_WAITING) {
        task_to_exit->parent->state = TASK_READY;
    }

    kprint("\nProcess ");
    char buf[8];
    itoa(current_task->id, buf, 8, 10);
    kprint(buf);
    kprint(" exited.\n");

    asm volatile("sti");
    schedule(); // 主动放弃CPU，让调度器处理后事
}

// 系统调用处理函数的分发表
static syscall_handler_t syscall_handlers[256];

void syscall_dispatcher(registers_t* regs) {
    uint32_t syscall_num = regs->eax;
    if (syscall_handlers[syscall_num]) {
        syscall_handlers[syscall_num](regs);
    }
}

void register_syscall(uint8_t num, syscall_handler_t handler) {
    syscall_handlers[num] = handler;
}

void init_syscalls() {
    register_interrupt_handler(128, &syscall_dispatcher);
    register_syscall(0, &syscall_exit);
    register_syscall(1, &syscall_fork);
    register_syscall(2, &syscall_waitpid);
    kprint("Syscalls initialized.\n");
}

