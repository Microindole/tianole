#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/mm.h>

#include "page_table.h"

#define ENTRY_COUNT 512
#define PAGE_MASK 0x000ffffffffff000ull
#define PAGE_SIZE_FLAG (1ull << 7)
#define MAX_RESERVED_TABLE_PAGES 4096

static phys_addr_t reserved_table_pages[MAX_RESERVED_TABLE_PAGES];
static uint64_t reserved_table_page_count;
static int reserved_table_pages_ready;
static uint64_t kernel_pml4[ENTRY_COUNT] __attribute__((aligned(PAGE_SIZE)));
static int page_tables_ready;

static uint64_t *active_pml4(void)
{
	uint64_t cr3;

	__asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
	return (uint64_t *)(uintptr_t)(cr3 & PAGE_MASK);
}

static void flush_tlb_page(virt_addr_t virt)
{
	__asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)virt) : "memory");
}

static void load_cr3(phys_addr_t root)
{
	__asm__ volatile("movq %0, %%cr3" : : "r"(root) : "memory");
}

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
	return (uint64_t *)(uintptr_t)(entry & PAGE_MASK);
}

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

static void collect_active_page_tables(void)
{
	uint64_t pml4_index;
	uint64_t pdpt_index;
	uint64_t pd_index;
	uint64_t *pml4 = active_pml4();

	add_reserved_table_page((phys_addr_t)(uintptr_t)pml4);

	for (pml4_index = 0; pml4_index < ENTRY_COUNT; pml4_index++) {
		uint64_t *pdpt;

		if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
			continue;
		}

		add_reserved_table_page(pml4[pml4_index] & PAGE_MASK);

		pdpt = entry_table(pml4[pml4_index]);
		for (pdpt_index = 0; pdpt_index < ENTRY_COUNT; pdpt_index++) {
			uint64_t *pd;

			if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0 ||
				(pdpt[pdpt_index] & PAGE_SIZE_FLAG) != 0) {
				continue;
			}

			add_reserved_table_page(pdpt[pdpt_index] & PAGE_MASK);

			pd = entry_table(pdpt[pdpt_index]);
			for (pd_index = 0; pd_index < ENTRY_COUNT; pd_index++) {
				if ((pd[pd_index] & PAGE_PRESENT) == 0 ||
					(pd[pd_index] & PAGE_SIZE_FLAG) != 0) {
					continue;
				}

				add_reserved_table_page(
					pd[pd_index] & PAGE_MASK);
			}
		}
	}
}

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

static uint64_t make_table_entry(phys_addr_t table)
{
	return table | PAGE_PRESENT | PAGE_WRITABLE;
}

void page_tables_init(void)
{
	uint64_t index;
	uint64_t *firmware_pml4;

	if (page_tables_ready != 0) {
		return;
	}

	firmware_pml4 = active_pml4();
	for (index = 0; index < ENTRY_COUNT; index++) {
		kernel_pml4[index] = firmware_pml4[index];
	}

	load_cr3((phys_addr_t)(uintptr_t)kernel_pml4);
	page_tables_ready = 1;
	early_log_puts("kernel page table root active\n");
}

static int ensure_next_table(uint64_t *table, uint64_t index, uint64_t **next)
{
	phys_addr_t page;

	if ((table[index] & PAGE_PRESENT) != 0) {
		*next = entry_table(table[index]);
		return 0;
	}

	page = alloc_page();
	if (page == 0) {
		return -1;
	}

	clear_page(page);
	table[index] = make_table_entry(page);
	*next = (uint64_t *)(uintptr_t)page;
	return 0;
}

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
		if (ensure_next_table(pml4, pml4_index, &pdpt) != 0 ||
			ensure_next_table(pdpt, pdpt_index, &pd) != 0 ||
			ensure_next_table(pd, pd_index, &pt) != 0) {
			return -1;
		}
	} else {
		if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
			return -1;
		}
		pdpt = entry_table(pml4[pml4_index]);

		if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0) {
			return -1;
		}
		pd = entry_table(pdpt[pdpt_index]);

		if ((pd[pd_index] & PAGE_PRESENT) == 0) {
			return -1;
		}
		pt = entry_table(pd[pd_index]);
	}

	*entry = &pt[pt_index];
	return 0;
}

int map_page(virt_addr_t virt, phys_addr_t phys, uint64_t flags)
{
	uint64_t *entry;

	if ((virt & (PAGE_SIZE - 1)) != 0 || (phys & (PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	if (page_entry(virt, 1, &entry) != 0) {
		return -1;
	}

	if ((*entry & PAGE_PRESENT) != 0) {
		return -1;
	}

	*entry = (phys & PAGE_MASK) | flags | PAGE_PRESENT;
	flush_tlb_page(virt);
	return 0;
}

int unmap_page(virt_addr_t virt)
{
	uint64_t *entry;

	if ((virt & (PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	if (page_entry(virt, 0, &entry) != 0 || (*entry & PAGE_PRESENT) == 0) {
		return -1;
	}

	*entry = 0;
	flush_tlb_page(virt);
	return 0;
}

int virt_to_phys(virt_addr_t virt, phys_addr_t *phys)
{
	uint64_t *entry;

	if (phys == 0 || page_entry(virt, 0, &entry) != 0 ||
		(*entry & PAGE_PRESENT) == 0) {
		return -1;
	}

	*phys = (*entry & PAGE_MASK) | (virt & (PAGE_SIZE - 1));
	return 0;
}
