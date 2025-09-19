#include "syscall.h"
#include "common.h"
#include "task.h"
#include "string.h"
#include "../mm/kheap.h"
#include <stddef.h>

// 声明外部变量和函数
extern volatile task_t* current_task;
extern uint32_t next_pid;
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
extern void load_page_directory(page_directory_t*);
// extern void fork_trampoline();
extern void child_test_func();

// fork 的C语言包装函数
int fork() {
    int pid;
    asm volatile (
        "int $0x80"
        : "=a" (pid)  // output: eax -> pid
        : "a" (1)     // input: 1 -> eax (syscall number)
    );
    return pid;
}

// 克隆页目录 (只共享，不复制)
page_directory_t* clone_directory(page_directory_t* src) {
    // 1. 为子进程分配一个新的、页对齐的页目录
    page_directory_t* dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    if (!dir) {
        kprint("clone_directory: kmalloc_a failed!\n");
        return NULL;
    }
    memset(dir, 0, sizeof(page_directory_t));

    // 2. 关键修正：不要复制页表和物理页，而是共享它们。
    //    我们只需复制父进程的页目录项，让子进程的页目录指向和父进程相同的页表。
    //    这样父子进程就共享了同一个地址空间。
    for (int i = 0; i < 1024; i++) {
        if (src->entries[i] != 0) {
            dir->entries[i] = src->entries[i];
        }
    }
    return dir;
}


// void syscall_fork(registers_t* regs) {
//     asm volatile("cli");

//     task_t* parent_task = (task_t*)current_task;

//     // --- 创建子进程 TCB (不变) ---
//     task_t* child_task = (task_t*)kmalloc(sizeof(task_t));
//     child_task->id = next_pid++;
//     child_task->state = TASK_READY;
//     child_task->directory = clone_directory(parent_task->directory);

//     // --- 为子进程构建一个兼容 switch_task 的新内核栈 ---

//     // 1. 分配新内核栈
//     child_task->kernel_stack = (uint32_t)kmalloc(4096);
//     uint32_t child_stack_top = child_task->kernel_stack + 4096;

//     // 2. 在栈的最“深处”放置 `iret` 需要的完整中断帧
//     registers_t* child_regs_for_iret = (registers_t*)(child_stack_top - sizeof(registers_t));
//     memcpy(child_regs_for_iret, regs, sizeof(registers_t));
//     child_regs_for_iret->eax = 0; // 子进程返回值为 0

//     // 3. 在 `iret` 帧的上方，放置 `switch_task` 的 `ret` 指令需要的返回地址
//     uint32_t esp = (uint32_t)child_regs_for_iret;
//     esp -= 4; 
//     *(uint32_t*)esp = (uint32_t)fork_trampoline;

//     // 4. 在返回地址上方，伪造一个 `pusha` 帧（32字节）
//     esp -= 32;
//     // 将父进程的通用寄存器值复制到这个伪造帧中
//     memcpy((void*)esp, (void*)&regs->edi, 32); 
//     // 再次确保 eax 为 0
//     *(uint32_t*)(esp + 28) = 0; // eax 在 pusha 帧中偏移量为 28

//     // 5. 设置子进程的 ESP，让它指向这个伪造的 `pusha` 帧的顶部
//     child_task->registers.esp = esp;

//     // --- 父进程返回值设定 ---
//     regs->eax = child_task->id;

//     // --- 加入调度队列 (不变) ---
//     child_task->next = parent_task->next;
//     parent_task->next = child_task;

//     asm volatile("sti");
// }

void syscall_fork(registers_t* regs) {
    asm volatile("cli");

    task_t* parent_task = (task_t*)current_task;

    // ... 创建 TCB 和克隆页目录的代码保持不变 ...
    task_t* child_task = (task_t*)kmalloc(sizeof(task_t));
    if (!child_task) { /* 错误处理 */ return; }
    child_task->id = next_pid++;
    child_task->state = TASK_READY;
    child_task->directory = clone_directory(parent_task->directory);
    if (!child_task->directory) { /* 错误处理 */ return; }


    // --- 为子进程构建一个正确的 C 函数调用栈 ---
    child_task->kernel_stack = (uint32_t)kmalloc(4096);
    if (!child_task->kernel_stack) { /* 错误处理 */ return; }
    uint32_t esp = child_task->kernel_stack + 4096;

    // 1. 在栈上压入 "返回地址"。当 switch_task 的 'ret' 指令执行时，会跳转到这里。
    esp -= 4;
    *(uint32_t*)esp = (uint32_t)child_test_func;

    // 2. 在返回地址的 "上方" (更低地址) 伪造一个 pusha 帧（32字节）。
    esp -= 32;
    // 从父进程的中断帧里复制通用寄存器的值。
    memcpy((void*)esp, (void*)&regs->edi, 32);
    // 确保子进程的 eax (返回值) 为 0。eax 在 pusha 帧中的偏移量是 28。
    *(uint32_t*)(esp + 28) = 0;

    // 3. 设置子进程的 ESP，让它指向这个伪造的 pusha 帧的顶部。
    //    这才是 switch_task 期望看到的 ESP。
    child_task->registers.esp = esp;

    // --- 父进程返回值设定 ---
    regs->eax = child_task->id;

    // --- 加入调度队列 (不变) ---
    child_task->next = parent_task->next;
    parent_task->next = child_task;

    asm volatile("sti");
}


// 系统调用处理函数的分发表
// 数组的索引就是系统调用号 (eax 寄存器里的值)
static syscall_handler_t syscall_handlers[256];

// 系统调用的总入口 (由 isr_handler 调用)
void syscall_dispatcher(registers_t* regs) {
    // 从 eax 寄存器中获取系统调用号
    uint32_t syscall_num = regs->eax;

    // 检查处理函数是否存在
    if (syscall_handlers[syscall_num]) {
        syscall_handler_t handler = syscall_handlers[syscall_num];
        handler(regs); // 调用具体的处理函数
    } else {
        kprint("Unknown syscall: ");
        char num_str[10];
        itoa(syscall_num, num_str);
        kprint(num_str);
        kprint("\n");
    }
}

// 注册一个系统调用
void register_syscall(uint8_t num, syscall_handler_t handler) {
    syscall_handlers[num] = handler;
}

// 初始化系统调用
void init_syscalls() {
    // 注册 128 号中断的处理函数为我们的分发器
    register_interrupt_handler(128, &syscall_dispatcher);

    // 注册具体的系统调用
    // 我们把 fork 定义为 1 号系统调用
    register_syscall(1, &syscall_fork);
    
    kprint("Syscalls initialized.\n");
}