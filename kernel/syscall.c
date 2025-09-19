#include "syscall.h"
#include "common.h"
#include "task.h"
#include "string.h"
#include "../mm/kheap.h"

// 声明外部变量和函数
extern volatile task_t* current_task;
extern uint32_t next_pid;
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;
extern void load_page_directory(page_directory_t*);

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

// 克隆页目录 (这个函数无需改动)
page_directory_t* clone_directory(page_directory_t* src) {
    page_directory_t* dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(dir, 0, sizeof(page_directory_t));

    for (int pde_idx = 0; pde_idx < 1024; pde_idx++) {
        if (src->entries[pde_idx] != 0) {
            page_table_t* src_pt = (page_table_t*)(src->entries[pde_idx] & 0xFFFFF000);
            page_table_t* dest_pt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
            dir->entries[pde_idx] = (uint32_t)dest_pt | 0x3;

            for (int pte_idx = 0; pte_idx < 1024; pte_idx++) {
                if (src_pt->entries[pte_idx] != 0) {
                    void* child_phys_page = kmalloc_a(4096);
                    void* parent_virt_page = (void*)((pde_idx << 22) | (pte_idx << 12));
                    memcpy(child_phys_page, parent_virt_page, 4096);
                    dest_pt->entries[pte_idx] = (uint32_t)child_phys_page | 0x3;
                }
            }
        }
    }
    return dir;
}


// --- 这是修正后的 fork 实现 ---
void syscall_fork(registers_t* regs) {
    asm volatile("cli");

    task_t* parent_task = (task_t*)current_task;

    // --- 创建子进程 TCB ---
    task_t* child_task = (task_t*)kmalloc(sizeof(task_t));
    child_task->id = next_pid++;
    child_task->state = TASK_READY;
    child_task->directory = clone_directory(parent_task->directory);

    // --- 内核栈克隆 ---
    // 1. 为子进程分配一个新的、独立的内核栈
    child_task->kernel_stack = (uint32_t)kmalloc(4096);

    // 2. 将父进程的整个中断上下文（registers_t 结构体）
    //    复制到子进程新内核栈的顶部。
    //    这是子进程被调度时，CPU需要看到的样子。
    uint32_t child_stack_top = child_task->kernel_stack + 4096;
    registers_t* child_regs_on_stack = (registers_t*)(child_stack_top - sizeof(registers_t));
    memcpy(child_regs_on_stack, regs, sizeof(registers_t));

    // 3. 设置子进程的 ESP，让它指向这个新复制出来的上下文。
    //    这是 switch_task 将要加载到 ESP 寄存器的值。
    child_task->registers.esp = (uint32_t)child_regs_on_stack;

    // --- 返回值设定 ---
    // 4. 修改子进程上下文中的 eax，让 fork() 在子进程中返回 0
    child_regs_on_stack->eax = 0;

    // 5. 修改父进程上下文中的 eax，让 fork() 在父进程中返回子进程 PID
    regs->eax = child_task->id;

    // --- 加入调度队列 ---
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