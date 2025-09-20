#include "paging.h"
#include "kheap.h"
#include "common.h"
#include "string.h"
#include "kheap.h"

// 全局的内核页目录
page_directory_t* kernel_directory = 0;
// 当前加载的页目录
page_directory_t* current_directory = 0;

// 外部汇编函数
extern void enable_paging();

void page_fault_handler(registers_t* regs) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    kprint("\n--- PAGE FAULT ---\n");
    
    // --- 关键修正：使用安全的 itoa_hex ---
    char hex_buf[12];
    itoa_hex(faulting_address, hex_buf, 12);
    kprint("Faulting Address: ");
    kprint(hex_buf);
    kprint("\n");

    itoa_hex(regs->eip, hex_buf, 12);
    kprint("Instruction Pointer: ");
    kprint(hex_buf);
    kprint("\n");

    kprint("System Halted!\n");
    for(;;);
}

// 创建一个新的、通用的页分配和映射函数
void alloc_and_map_page(page_directory_t* dir, uint32_t virt_addr, _Bool is_kernel, _Bool is_writeable) {
    uint32_t pde_idx = virt_addr / (1024 * 4096);
    uint32_t pte_idx = (virt_addr / 4096) % 1024;
    
    // 如果页表不存在，则创建
    if (dir->entries[pde_idx] == 0) {
        page_table_t* pt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(pt, 0, sizeof(page_table_t));
        uint32_t flags = 0x3 | (is_kernel ? 0 : 4); // Present, RW, User
        dir->entries[pde_idx] = (uint32_t)pt | flags;
    }

    page_table_t* pt = (page_table_t*)(dir->entries[pde_idx] & 0xFFFFF000);

    // 分配一个物理页帧
    void* frame = kmalloc_a(4096); // 使用 kmalloc_a 保证页对齐
    
    // 设置页表项
    uint32_t flags = 0x1 | (is_writeable ? 2 : 0) | (is_kernel ? 0 : 4); // Present, RW, User
    pt->entries[pte_idx] = ((uint32_t)frame & 0xFFFFF000) | flags;
}


void init_paging() {
    // 1. 为页目录申请一页对齐的内存
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    // 2. 创建一个临时的页表用于映射
    page_table_t* first_page_table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
    memset(first_page_table, 0, sizeof(page_table_t));

    // 3. 正确地执行恒等映射 (Identity Map)
    //    我们只需要映射内核所在的第一个页表 (前 4MB)
    for (int i = 0; i < 1024; i++) {
        // 让虚拟地址 i*4096 指向 物理地址 i*4096
        // 标志位: 0x3 = Present(1) | Read/Write(1) | User(0)
        first_page_table->entries[i] = (i * 4096) | 3;
    }

    // 4. 在页目录中设置第一项，指向我们的页表
    //    标志位: 0x3 = Present(1) | Read/Write(1) | User(0)
    kernel_directory->entries[0] = (uint32_t)first_page_table | 3;

    // 5. 注册页错误中断处理函数
    register_interrupt_handler(14, page_fault_handler);

    // 6. 加载页目录并开启分页
    load_page_directory(kernel_directory);
    enable_paging();

    kprint("Paging enabled.\n");
}
