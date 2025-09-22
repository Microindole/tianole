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

    child_task->initial_regs = (registers_t*)kmalloc(sizeof(registers_t));
    memcpy(child_task->initial_regs, regs, sizeof(registers_t));
    child_task->initial_regs->eax = 0;

    regs->eax = child_task->id;

    child_task->next = parent_task->next;
    parent_task->next = child_task;
    
    asm volatile("sti");
}

// --- exit 的系统调用实现 ---
void syscall_exit(registers_t* regs) {
    current_task->state = TASK_DEAD;
    kprint("\nProcess ");
    char buf[8];
    itoa(current_task->id, buf, 8, 10);
    kprint(buf);
    kprint(" exited.\n");
    schedule(); // 主动放弃CPU，让调度器来处理后事
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
    kprint("Syscalls initialized.\n");
}

