#ifndef ARCH_X86_MM_PAGE_TABLE_H
#define ARCH_X86_MM_PAGE_TABLE_H

#include <stdint.h>

#include <tianole/mm.h>

#define X86_PAGE_TABLE_ENTRIES 512u
#define X86_PAGE_MASK 0x000ffffffffff000ull
#define X86_PAGE_SIZE_FLAG (1ull << 7)

/**
 * active_pml4() - Return the currently loaded top-level page table.
 *
 * Return: Virtual address of the active PML4, derived from CR3.
 */
uint64_t *active_pml4(void);

/**
 * load_cr3() - Load a new x86 page-table root.
 * @root: Physical address of the PML4 page to load into CR3.
 */
void load_cr3(phys_addr_t root);

/**
 * flush_tlb_page() - Invalidate one virtual page from the local TLB.
 * @virt: Virtual address whose translation changed.
 */
void flush_tlb_page(virt_addr_t virt);

/**
 * page_tables_init() - Switch from firmware page tables to the kernel root.
 *
 * The first kernel root starts as a copy of the active UEFI PML4 so the kernel
 * keeps early identity mappings, including the boot framebuffer, while later
 * map/unmap operations are owned by Tianole.
 */
void page_tables_init(void);

#endif
