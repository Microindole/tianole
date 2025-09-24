#include "exec.h"
#include "task.h"
#include "../mm/paging.h"
#include "string.h"
#include "../mm/kheap.h"
#include "../cpu/gdt.h" // 包含 gdt.h 以便使用 set_kernel_stack

// 使用 objcopy 创建的符号
extern uint8_t _binary_build_user_hello_bin_start[];
extern uint8_t _binary_build_user_hello_bin_end[];

// 用于切换到用户模式的汇编函数
extern void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);

void exec_init_user_program() {
    volatile task_t* parent_task = current_task;

    // 1. 创建新任务的 TCB
    task_t* app_task = (task_t*)kmalloc(sizeof(task_t));
    app_task->id = 2; // 以后会变成动态 PID
    app_task->state = TASK_RUNNING;
    app_task->parent = (task_t*)parent_task;
    
    // 为新进程创建一个内核栈
    uint32_t kernel_stack = (uint32_t)kmalloc(4096) + 4096;
    app_task->kernel_stack_ptr = kernel_stack;

    // 2. 创建新的页目录
    page_directory_t* app_dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(app_dir, 0, sizeof(page_directory_t));
    
    // --- 核心修正 #1：将页目录与任务关联 ---
    app_task->directory = app_dir;

    // 映射内核空间 (与之前相同)
    page_table_t* kernel_page_table = (page_table_t*)(parent_task->directory->entries[0] & 0xFFFFF000);
    app_dir->entries[0] = (uint32_t)kernel_page_table | 0x3;

    // 3. 映射用户程序的代码和栈 (与之前相同)
    uint32_t user_code_addr = 0x400000;
    uint32_t user_stack_addr = 0xE0000000;

    void* code_phys = kmalloc_a(4096);
    void* stack_phys = kmalloc_a(4096);
    
    // ... (映射代码和栈页的逻辑与之前完全相同) ...
    uint32_t code_pde_idx = user_code_addr / (1024 * 4096);
    page_table_t* code_table = (page_table_t*)(app_dir->entries[code_pde_idx] & 0xFFFFF000);
    if (!code_table) {
        code_table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(code_table, 0, sizeof(page_table_t));
        app_dir->entries[code_pde_idx] = (uint32_t)code_table | 0x7;
    }
    code_table->entries[(user_code_addr / 4096) % 1024] = (uint32_t)code_phys | 0x7;

    uint32_t stack_pde_idx = user_stack_addr / (1024 * 4096);
    page_table_t* stack_table = (page_table_t*)(app_dir->entries[stack_pde_idx] & 0xFFFFF000);
    if (!stack_table) {
        stack_table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(stack_table, 0, sizeof(page_table_t));
        app_dir->entries[stack_pde_idx] = (uint32_t)stack_table | 0x7;
    }
    stack_table->entries[(user_stack_addr / 4096) % 1024] = (uint32_t)stack_phys | 0x7;


    // 4. 复制程序代码 (与之前相同)
    uint32_t app_size = _binary_build_user_hello_bin_end - _binary_build_user_hello_bin_start;
    memcpy(code_phys, _binary_build_user_hello_bin_start, app_size);

    // 5. 将新任务加入任务列表并切换当前任务
    app_task->next = parent_task->next;
    parent_task->next = app_task;
    parent_task->state = TASK_READY; // 父进程(Shell)可以继续运行

    // --- 核心修正 #2：更新全局的当前任务指针 ---
    current_task = app_task;
    
    // 6. 切换地址空间并跳转
    load_page_directory(current_task->directory);
    current_directory = current_task->directory;
    
    set_kernel_stack(current_task->kernel_stack_ptr);
    
    kprint("Jumping to user mode...\n");
    switch_to_user_mode(user_code_addr, user_stack_addr + 4096);
}