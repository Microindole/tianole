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
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    // 恒等映射前 4MB 的内核空间
    for (uint32_t addr = 0; addr < 0x400000; addr += 0x1000) {
        alloc_and_map_page(kernel_directory, addr, 1, 1);
    }

    register_interrupt_handler(14, page_fault_handler);

    load_page_directory(kernel_directory);
    enable_paging();

    kprint("Paging enabled.\n");
}
