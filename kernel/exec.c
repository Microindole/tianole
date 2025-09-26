// kernel/exec.c (最终修正版)

#include "exec.h"
#include "task.h"
#include "../mm/paging.h"
#include "string.h"
#include "../mm/kheap.h"
#include "../cpu/gdt.h"
#include "../drivers/serial.h"

// 外部全局变量
extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;

// 使用 objcopy 创建的符号
extern uint8_t _binary_build_user_hello_bin_start[];
extern uint8_t _binary_build_user_hello_bin_end[];

// 用于切换到用户模式的汇编函数
extern void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);

// 一个安全的辅助函数，用于在指定的页目录中映射一个页面
// 这个版本正确处理了物理地址到虚拟地址的转换
// 一个安全的、实现了写时复制逻辑的辅助函数
static void map_page(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t pde_idx = virt_addr / (1024 * 4096);
    uint32_t pte_idx = (virt_addr / 4096) % 1024;

    uint32_t pde = dir->entries[pde_idx];
    page_table_t* pt_virt;

    if (!(pde & 0x1)) {
        // --- 情况 1: 页表不存在 ---
        // 这是最简单的情况，直接创建一个新的即可
        pt_virt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(pt_virt, 0, sizeof(page_table_t));
        
        uint32_t pt_phys = virtual_to_physical(kernel_directory, (uint32_t)pt_virt);
        dir->entries[pde_idx] = pt_phys | flags | 0x1; // 使用传入的flags
    } else {
        // --- 情况 2: 页表已存在 (很可能是从内核继承的共享页表) ---
        uint32_t pt_phys_original = pde & 0xFFFFF000;
        
        // !!! --- 这是最关键的修复：实现写时复制 --- !!!
        // 1. 为这个用户进程分配一个全新的、私有的页表
        pt_virt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        
        // 2. 将旧的、共享的页表内容完整复制到新的私有页表中
        //    (pt_phys_original 恰好等于其虚拟地址，因为内核是恒等映射)
        memcpy(pt_virt, (void*)pt_phys_original, sizeof(page_table_t));

        // 3. 获取新页表的物理地址
        uint32_t pt_phys_new = virtual_to_physical(kernel_directory, (uint32_t)pt_virt);
        
        // 4. 更新用户进程的页目录，让它指向这个新的、私有的页表
        dir->entries[pde_idx] = pt_phys_new | flags | 0x1;
    }

    // --- 现在，我们可以安全地在新创建的或私有的副本上进行修改 ---
    pt_virt->entries[pte_idx] = (phys_addr & 0xFFFFF000) | flags | 0x1;
}

void exec_init_user_program() {
    volatile task_t* parent_task = current_task;

    task_t* app_task = (task_t*)kmalloc(sizeof(task_t));
    memset(app_task, 0, sizeof(task_t)); // !!! 重要：清零新任务，避免野指针 !!!

    app_task->id = next_pid++;
    app_task->state = TASK_RUNNING;
    app_task->parent = (task_t*)parent_task;
    
    uint32_t kernel_stack = (uint32_t)kmalloc(4096) + 4096;
    app_task->kernel_stack_ptr = kernel_stack;

    serial_print("New kernel stack for app at: ");
    char buf[12];
    itoa_hex(kernel_stack, buf, 12);
    serial_print(buf);
    serial_print("\n");

    // 1. 创建一个新的、干净的页目录
    page_directory_t* app_dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(app_dir, 0, sizeof(page_directory_t));
    app_task->directory = app_dir;

    // 2. 复制内核空间的映射 (所有高于1MB的映射)
    //    这可以确保用户进程可以访问内核提供的系统调用等服务
    for (int i = 0; i < 1024; i++) { // 从1开始，跳过第一个4MB
        if (kernel_directory->entries[i] != 0) {
             app_dir->entries[i] = kernel_directory->entries[i];
        }
    }

    // 定义用户空间的地址和权限
    uint32_t user_code_addr = 0x400000;
    uint32_t user_stack_top = 0xE0001000;
    uint32_t user_stack_page = user_stack_top - 4096;
    uint32_t user_flags = 0x7; // Present, R/W, User Mode

    // 3. 为用户代码分配物理内存并进行映射
    void* code_page_virt = kmalloc_a(4096);
    uint32_t code_page_phys = virtual_to_physical(kernel_directory, (uint32_t)code_page_virt);
    map_page(app_dir, user_code_addr, code_page_phys, user_flags);

    // 4. 为用户栈分配物理内存并进行映射
    void* stack_page_virt = kmalloc_a(4096);
    uint32_t stack_page_phys = virtual_to_physical(kernel_directory, (uint32_t)stack_page_virt);
    map_page(app_dir, user_stack_page, stack_page_phys, user_flags);

    // 5. 将用户程序代码复制到新分配的页中
    uint32_t app_size = _binary_build_user_hello_bin_end - _binary_build_user_hello_bin_start;
    memcpy(code_page_virt, _binary_build_user_hello_bin_start, app_size);

    // 6. 更新任务链表
    app_task->next = (struct task*)current_task->next;
    current_task->next = (struct task*)app_task;

    // 7. 手动执行任务切换
    volatile task_t* old_task = current_task;
    if (old_task->state == TASK_RUNNING) {
        old_task->state = TASK_READY;
    }
    current_task = app_task;
    
    // 8. 激活新的地址空间
    uint32_t app_dir_phys = virtual_to_physical(kernel_directory, (uint32_t)current_task->directory);
    load_page_directory((page_directory_t*)app_dir_phys);
    current_directory = current_task->directory;
    
    // 9. 更新TSS
    set_kernel_stack(current_task->kernel_stack_ptr);
    
    // 10. 跳转！
    // kprint("Jumping to user mode...\n");
    switch_to_user_mode(user_code_addr, user_stack_top);
}