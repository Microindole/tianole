#include <stdint.h>

#include <tianole/errno.h>
#include <tianole/mm.h>
#include <tianole/panic.h>
#include <tianole/printk.h>

#include "arch/x86/mm/page_table.h"

#define TEST_VIRTUAL_PAGE 0xffffff0000000000ull

void page_table_selftest(void)
{
	phys_addr_t page = alloc_page();
	phys_addr_t resolved;
	volatile uint64_t *mapped =
		(volatile uint64_t *)(uintptr_t)TEST_VIRTUAL_PAGE;

	if (page == 0) {
		panic("page table selftest allocation failed");
	}

	page_tables_init();

	if (map_page(TEST_VIRTUAL_PAGE + 1, page, PAGE_WRITABLE) != -EINVAL) {
		panic("page table selftest unaligned map errno failed");
	}

	if (virt_to_phys(TEST_VIRTUAL_PAGE, 0) != -EINVAL) {
		panic("page table selftest null output errno failed");
	}

	if (virt_to_phys(TEST_VIRTUAL_PAGE, &resolved) != -ENOENT) {
		panic("page table selftest missing resolve errno failed");
	}

	if (map_page(TEST_VIRTUAL_PAGE, page, PAGE_WRITABLE) != 0) {
		panic("page table selftest map failed");
	}

	if (map_page(TEST_VIRTUAL_PAGE, page, PAGE_WRITABLE) != -EEXIST) {
		panic("page table selftest duplicate map errno failed");
	}

	if (virt_to_phys(TEST_VIRTUAL_PAGE, &resolved) != 0 ||
		resolved != page) {
		panic("page table selftest resolve failed");
	}

	*mapped = 0x54494f4c45504d4dull;
	if (*mapped != 0x54494f4c45504d4dull) {
		panic("page table selftest access failed");
	}

	if (unmap_page(TEST_VIRTUAL_PAGE) != 0) {
		panic("page table selftest unmap failed");
	}

	if (unmap_page(TEST_VIRTUAL_PAGE) != -ENOENT) {
		panic("page table selftest missing unmap errno failed");
	}

	free_page(page);
	pr_info("page table selftest ok\n");
}
