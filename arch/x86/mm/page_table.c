#include <stdint.h>

#include <tianole/errno.h>
#include <tianole/mm.h>

#include "page_table.h"

static uint64_t table_index(virt_addr_t virt, unsigned int shift)
{
	return (virt >> shift) & 0x1ffu;
}

static void clear_page(phys_addr_t page)
{
	uint64_t *words = (uint64_t *)(uintptr_t)page;
	uint64_t index;

	for (index = 0; index < PAGE_SIZE / sizeof(uint64_t); index++) {
		words[index] = 0;
	}
}

static uint64_t *entry_table(uint64_t entry)
{
	return (uint64_t *)(uintptr_t)(entry & X86_PAGE_MASK);
}

static uint64_t make_table_entry(phys_addr_t table)
{
	return table | PAGE_PRESENT | PAGE_WRITABLE;
}

/**
 * ensure_next_table() - Find or allocate the next page-table level.
 * @table: Current page-table level.
 * @index: Entry index within @table.
 * @next: Receives the next-level table address.
 *
 * Return: 0 on success or -ENOMEM if a new table page cannot be allocated.
 */
static int ensure_next_table(uint64_t *table, uint64_t index, uint64_t **next)
{
	phys_addr_t page;

	if ((table[index] & PAGE_PRESENT) != 0) {
		*next = entry_table(table[index]);
		return 0;
	}

	page = alloc_page();
	if (page == 0) {
		return -ENOMEM;
	}

	clear_page(page);
	table[index] = make_table_entry(page);
	*next = (uint64_t *)(uintptr_t)page;
	return 0;
}

/**
 * page_entry() - Resolve the leaf PTE for a virtual address.
 * @virt: Virtual address whose PTE is requested.
 * @create: Allocate missing intermediate tables when non-zero.
 * @entry: Receives the leaf PTE address.
 *
 * Return: 0 on success, -ENOENT when lookup fails, or -ENOMEM on allocation
 * failure while @create is non-zero.
 */
static int page_entry(virt_addr_t virt, int create, uint64_t **entry)
{
	uint64_t *pml4 = active_pml4();
	uint64_t *pdpt;
	uint64_t *pd;
	uint64_t *pt;
	uint64_t pml4_index = table_index(virt, 39);
	uint64_t pdpt_index = table_index(virt, 30);
	uint64_t pd_index = table_index(virt, 21);
	uint64_t pt_index = table_index(virt, 12);

	if (create != 0) {
		int ret = ensure_next_table(pml4, pml4_index, &pdpt);

		if (ret != 0) {
			return ret;
		}

		ret = ensure_next_table(pdpt, pdpt_index, &pd);
		if (ret != 0) {
			return ret;
		}

		ret = ensure_next_table(pd, pd_index, &pt);
		if (ret != 0) {
			return ret;
		}
	} else {
		if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
			return -ENOENT;
		}
		pdpt = entry_table(pml4[pml4_index]);

		if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0) {
			return -ENOENT;
		}
		pd = entry_table(pdpt[pdpt_index]);

		if ((pd[pd_index] & PAGE_PRESENT) == 0) {
			return -ENOENT;
		}
		pt = entry_table(pd[pd_index]);
	}

	*entry = &pt[pt_index];
	return 0;
}

/**
 * map_page() - Map one 4 KiB virtual page to a physical page.
 * @virt: Page-aligned virtual address.
 * @phys: Page-aligned physical address.
 * @flags: Architecture PTE flags supplied by the caller.
 *
 * Return: 0 on success, -EINVAL for unaligned input, -EEXIST if already
 * mapped, or another negative errno from page-table allocation.
 */
int map_page(virt_addr_t virt, phys_addr_t phys, uint64_t flags)
{
	uint64_t *entry;
	int ret;

	if ((virt & (PAGE_SIZE - 1)) != 0 || (phys & (PAGE_SIZE - 1)) != 0) {
		return -EINVAL;
	}

	ret = page_entry(virt, 1, &entry);
	if (ret != 0) {
		return ret;
	}

	if ((*entry & PAGE_PRESENT) != 0) {
		return -EEXIST;
	}

	*entry = (phys & X86_PAGE_MASK) | flags | PAGE_PRESENT;
	flush_tlb_page(virt);
	return 0;
}

/**
 * unmap_page() - Remove one 4 KiB mapping from the active page tables.
 * @virt: Page-aligned virtual address.
 *
 * Return: 0 on success, -EINVAL for unaligned input, or -ENOENT if unmapped.
 */
int unmap_page(virt_addr_t virt)
{
	uint64_t *entry;
	int ret;

	if ((virt & (PAGE_SIZE - 1)) != 0) {
		return -EINVAL;
	}

	ret = page_entry(virt, 0, &entry);
	if (ret != 0 || (*entry & PAGE_PRESENT) == 0) {
		return -ENOENT;
	}

	*entry = 0;
	flush_tlb_page(virt);
	return 0;
}

/**
 * virt_to_phys() - Translate a mapped virtual address to physical address.
 * @virt: Virtual address to translate.
 * @phys: Receives the translated physical address including page offset.
 *
 * Return: 0 on success, -EINVAL for invalid output storage, or -ENOENT if the
 * virtual address is not mapped.
 */
int virt_to_phys(virt_addr_t virt, phys_addr_t *phys)
{
	uint64_t *entry;
	int ret;

	if (phys == 0) {
		return -EINVAL;
	}

	ret = page_entry(virt, 0, &entry);
	if (ret != 0 || (*entry & PAGE_PRESENT) == 0) {
		return -ENOENT;
	}

	*phys = (*entry & X86_PAGE_MASK) | (virt & (PAGE_SIZE - 1));
	return 0;
}
