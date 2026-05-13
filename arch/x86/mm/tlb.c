#include <stdint.h>

#include "page_table.h"

/**
 * active_pml4() - Return the currently loaded top-level page table.
 *
 * Return: Virtual address of the active PML4, derived from CR3. Early boot
 * still relies on identity mappings established by firmware and then copied
 * into the Tianole-owned root.
 */
uint64_t *active_pml4(void)
{
	uint64_t cr3;

	__asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
	return (uint64_t *)(uintptr_t)(cr3 & X86_PAGE_MASK);
}

/**
 * flush_tlb_page() - Invalidate one virtual page from the local TLB.
 * @virt: Virtual address whose mapping changed.
 */
void flush_tlb_page(virt_addr_t virt)
{
	__asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)virt) : "memory");
}

/**
 * load_cr3() - Load a new top-level x86 page table.
 * @root: Physical address of the PML4 page to load into CR3.
 *
 * Reloading CR3 also flushes non-global TLB entries, which is sufficient for
 * the early single-CPU page-table handoff.
 */
void load_cr3(phys_addr_t root)
{
	__asm__ volatile("movq %0, %%cr3" : : "r"(root) : "memory");
}
