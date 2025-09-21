#include "paging.h"
#include "kheap.h"
#include "common.h"
#include "string.h"

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

void init_paging() {
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    for (uint32_t addr = 0; addr < 1024 * 1024 * 4; addr += 0x1000) {
        uint32_t pde_idx = addr / (1024 * 4096);
        uint32_t pte_idx = (addr / 4096) % 1024;

        if (kernel_directory->entries[pde_idx] == 0) {
            page_table_t* pt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
            memset(pt, 0, sizeof(page_table_t));
            kernel_directory->entries[pde_idx] = (uint32_t)pt | 0x3; // Present, RW
        }
        
        page_table_t* pt = (page_table_t*)(kernel_directory->entries[pde_idx] & 0xFFFFF000);
        pt->entries[pte_idx] = (addr & 0xFFFFF000) | 0x3; // Present, RW
    }

    register_interrupt_handler(14, page_fault_handler);

    load_page_directory(kernel_directory);
    enable_paging();

    kprint("Paging enabled.\n");
}

void map_page(page_directory_t* dir, uint32_t virt, uint32_t phys, int is_kernel, int is_writeable) {
    uint32_t pde_idx = virt / (1024 * 4096);
    uint32_t pte_idx = (virt / 4096) % 1024;

    // 获取页目录项，如果对应的页表不存在，就创建一个
    uint32_t pde = dir->entries[pde_idx];
    if (!(pde & 0x1)) { // Present bit is not set
        page_table_t* pt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(pt, 0, sizeof(page_table_t));
        dir->entries[pde_idx] = (uint32_t)pt | 0x3; // Present, RW
        if (!is_kernel) {
            dir->entries[pde_idx] |= 0x4; // User access
        }
    }

    // 获取页表并设置页表项
    page_table_t* pt = (page_table_t*)(dir->entries[pde_idx] & 0xFFFFF000);

    uint32_t flags = 0x1; // Present
    if (is_writeable) flags |= 0x2; // Writable
    if (!is_kernel) flags |= 0x4; // User access

    pt->entries[pte_idx] = (phys & 0xFFFFF000) | flags;

    load_page_directory(current_directory);
}
