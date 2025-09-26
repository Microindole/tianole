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

// 一个简化的虚拟地址到物理地址的转换函数
// 在当前的恒等映射下，它直接返回原地址
// 注意：这个函数没有处理页不存在的情况，因为它假设被转换的地址是有效的
uint32_t virtual_to_physical(page_directory_t* dir, uint32_t virt_addr) {
    // 在我们当前的设置中 (内核恒等映射), 虚拟地址等于物理地址
    // 当你未来实现更复杂的映射时 (比如内核在高半区), 这里就需要真正的翻译逻辑
    return virt_addr; 
}
