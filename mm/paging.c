// mm/paging.c
#include "paging.h"
#include "kheap.h"
#include "common.h"
#include "string.h"

// 全局的内核页目录
page_directory_t* kernel_directory = 0;

// 当前加载的页目录
page_directory_t* current_directory = 0;

// 声明外部汇编函数
extern void load_page_directory(page_directory_t*);
extern void enable_paging();

void page_fault_handler(registers_t* regs) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present   = !(regs->err_code & 0x1);
    int rw        = regs->err_code & 0x2;
    int user      = regs->err_code & 0x4;
    int reserved  = regs->err_code & 0x8;

    kprint("\n--- PAGE FAULT ---\n");
    kprint("Details:\n");

    if (present) { kprint("- Reason: Page not present\n"); } else { kprint("- Reason: Page-level protection violation\n");}
    if (rw) { kprint("- Operation: Write\n"); } else { kprint("- Operation: Read\n"); }
    if (user) { kprint("- Privilege: User-mode\n"); } else { kprint("- Privilege: Kernel-mode\n"); }
    if (reserved) { kprint("- Cause: Reserved bit overwrite\n"); }

    kprint("- Faulting Address: ");
    char hex[12];
    itoa_hex(faulting_address, hex); // 使用新的十六进制转换函数
    kprint(hex);
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

        // 如果页表还不存在，就创建一个
        if (kernel_directory->entries[pde_idx] == 0) {
            page_table_t* pt = (page_table_t*)kmalloc_a(sizeof(page_table_t));
            memset(pt, 0, sizeof(page_table_t));
            
            // 将页表的物理地址和权限位写入页目录项
            kernel_directory->entries[pde_idx] = (uint32_t)pt | 0x3; // Present, RW
        }
        
        // 获取页表 (清除权限位)
        page_table_t* pt = (page_table_t*)(kernel_directory->entries[pde_idx] & 0xFFFFF000);
        
        // 设置页表项，将虚拟地址映射到同地址的物理页
        // frame 是地址的高20位，所以要右移12位
        pt->entries[pte_idx] = (addr & 0xFFFFF000) | 0x3; // Present, RW
    }

    register_interrupt_handler(14, page_fault_handler);

    load_page_directory(kernel_directory);
    enable_paging();

    kprint("Paging enabled.\n");
}