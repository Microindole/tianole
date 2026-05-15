#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/mm.h>
#include <tianole/panic.h>
#include <tianole/printk.h>

#include "page_table.h"

#define MAX_RESERVED_TABLE_PAGES 4096

static phys_addr_t reserved_table_pages[MAX_RESERVED_TABLE_PAGES];
static uint64_t reserved_table_page_count;
static int reserved_table_pages_ready;
static uint64_t kernel_pml4[X86_PAGE_TABLE_ENTRIES]
	__attribute__((aligned(PAGE_SIZE)));
static int page_tables_ready;

static uint64_t *entry_table(uint64_t entry)
{
	return (uint64_t *)(uintptr_t)(entry & X86_PAGE_MASK);
}

/**
 * add_reserved_table_page() - Mark a page table page as allocator-reserved.
 * @page: Physical address of a page currently used as a page-table page.
 *
 * The physical page allocator asks this module whether a firmware page can be
 * reused. Recording active page-table pages prevents reclaiming tables that are
 * still needed to preserve early identity mappings.
 */
static void add_reserved_table_page(phys_addr_t page)
{
	uint64_t index;

	for (index = 0; index < reserved_table_page_count; index++) {
		if (reserved_table_pages[index] == page) {
			return;
		}
	}

	if (reserved_table_page_count >= MAX_RESERVED_TABLE_PAGES) {
		panic("too many active page table pages");
	}

	reserved_table_pages[reserved_table_page_count++] = page;
}

/**
 * collect_active_page_tables() - Walk current page tables and reserve tables.
 *
 * The walk records only page-table pages, not the mapped leaf pages. Large
 * pages are treated as leaves and therefore do not add a lower-level table.
 */
static void collect_active_page_tables(void)
{
	uint64_t pml4_index;
	uint64_t pdpt_index;
	uint64_t pd_index;
	uint64_t *pml4 = active_pml4();

	add_reserved_table_page((phys_addr_t)(uintptr_t)pml4);

	for (pml4_index = 0; pml4_index < X86_PAGE_TABLE_ENTRIES;
		pml4_index++) {
		uint64_t *pdpt;

		if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
			continue;
		}

		add_reserved_table_page(pml4[pml4_index] & X86_PAGE_MASK);

		pdpt = entry_table(pml4[pml4_index]);
		for (pdpt_index = 0; pdpt_index < X86_PAGE_TABLE_ENTRIES;
			pdpt_index++) {
			uint64_t *pd;

			if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0 ||
				(pdpt[pdpt_index] & X86_PAGE_SIZE_FLAG) != 0) {
				continue;
			}

			add_reserved_table_page(
				pdpt[pdpt_index] & X86_PAGE_MASK);

			pd = entry_table(pdpt[pdpt_index]);
			for (pd_index = 0; pd_index < X86_PAGE_TABLE_ENTRIES;
				pd_index++) {
				if ((pd[pd_index] & PAGE_PRESENT) == 0 ||
					(pd[pd_index] & X86_PAGE_SIZE_FLAG) !=
						0) {
					continue;
				}

				add_reserved_table_page(
					pd[pd_index] & X86_PAGE_MASK);
			}
		}
	}
}

/**
 * arch_page_table_uses_page() - Test whether a page backs an active table.
 * @page: Page-aligned physical address to test.
 *
 * Return: Non-zero if @page must stay reserved for page-table ownership.
 */
int arch_page_table_uses_page(uint64_t page)
{
	uint64_t index;

	if ((page & (PAGE_SIZE - 1)) != 0) {
		return 0;
	}

	if (reserved_table_pages_ready == 0) {
		collect_active_page_tables();
		reserved_table_pages_ready = 1;
	}

	for (index = 0; index < reserved_table_page_count; index++) {
		if (reserved_table_pages[index] == page) {
			return 1;
		}
	}

	return 0;
}

/**
 * page_tables_init() - Take ownership of the kernel page-table root.
 *
 * The initial kernel root is a full copy of the firmware PML4. That keeps UEFI
 * identity mappings and the GOP framebuffer usable while later page-table
 * operations mutate only Tianole-owned top-level storage.
 */
void page_tables_init(void)
{
	uint64_t index;
	uint64_t *firmware_pml4;

	if (page_tables_ready != 0) {
		return;
	}

	firmware_pml4 = active_pml4();
	for (index = 0; index < X86_PAGE_TABLE_ENTRIES; index++) {
		kernel_pml4[index] = firmware_pml4[index];
	}

	load_cr3((phys_addr_t)(uintptr_t)kernel_pml4);
	page_tables_ready = 1;
	pr_info("kernel page table root active\n");
}
