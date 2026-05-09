#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/mm.h>

struct page_node {
	struct page_node *next;
};

extern char __kernel_start[];
extern char __kernel_end[];

static struct page_node *free_pages;
static uint64_t free_page_count;

static uint64_t align_down(uint64_t value, uint64_t alignment)
{
	return value & ~(alignment - 1);
}

static uint64_t align_up(uint64_t value, uint64_t alignment)
{
	return align_down(value + alignment - 1, alignment);
}

static int ranges_overlap(uint64_t start,
	uint64_t end,
	uint64_t reserved_start,
	uint64_t reserved_end)
{
	return start < reserved_end && reserved_start < end;
}

static int page_is_reserved(const boot_info_t *boot_info, uint64_t page)
{
	uint64_t page_end = page + PAGE_SIZE;
	uint64_t kernel_start =
		align_down((uint64_t)(uintptr_t)__kernel_start, PAGE_SIZE);
	uint64_t kernel_end =
		align_up((uint64_t)(uintptr_t)__kernel_end, PAGE_SIZE);
	uint64_t boot_info_start =
		align_down((uint64_t)(uintptr_t)boot_info, PAGE_SIZE);
	uint64_t boot_info_end = align_up(
		(uint64_t)(uintptr_t)boot_info + sizeof(*boot_info), PAGE_SIZE);
	uint64_t map_start = align_down(boot_info->memory_map, PAGE_SIZE);
	uint64_t map_end = align_up(
		boot_info->memory_map + boot_info->memory_map_size, PAGE_SIZE);

	if (ranges_overlap(page, page_end, kernel_start, kernel_end)) {
		return 1;
	}

	if (ranges_overlap(page, page_end, boot_info_start, boot_info_end)) {
		return 1;
	}

	if (ranges_overlap(page, page_end, map_start, map_end)) {
		return 1;
	}

	if (arch_page_table_uses_page(page)) {
		return 1;
	}

	return 0;
}

static void add_free_page(uint64_t page)
{
	struct page_node *node = (struct page_node *)(uintptr_t)page;

	node->next = free_pages;
	free_pages = node;
	free_page_count++;
}

static void add_conventional_range(
	const boot_info_t *boot_info, uint64_t start, uint64_t pages)
{
	uint64_t page;
	uint64_t first = align_up(start, PAGE_SIZE);
	uint64_t end = start + pages * PAGE_SIZE;
	uint64_t last;

	if (first + PAGE_SIZE > end) {
		return;
	}

	last = align_down(end - PAGE_SIZE, PAGE_SIZE);

	for (page = last;; page -= PAGE_SIZE) {
		if (!page_is_reserved(boot_info, page)) {
			add_free_page(page);
		}

		if (page == first) {
			break;
		}
	}
}

static void init_free_pages(const boot_info_t *boot_info)
{
	uint64_t offset;

	for (offset = 0; offset + sizeof(boot_memory_descriptor_t) <=
		boot_info->memory_map_size;
		offset += boot_info->memory_descriptor_size) {
		const boot_memory_descriptor_t *descriptor =
			(const boot_memory_descriptor_t
					*)(uintptr_t)(boot_info->memory_map +
				offset);

		if (descriptor->type != BOOT_MEMORY_TYPE_CONVENTIONAL) {
			continue;
		}

		add_conventional_range(boot_info,
			descriptor->physical_start,
			descriptor->number_of_pages);
	}
}

phys_addr_t alloc_page(void)
{
	struct page_node *node = free_pages;

	if (node == 0) {
		return 0;
	}

	free_pages = node->next;
	free_page_count--;

	return (phys_addr_t)(uintptr_t)node;
}

void free_page(phys_addr_t page)
{
	if ((page & (PAGE_SIZE - 1)) != 0 || page == 0) {
		panic("invalid physical page free");
	}

	add_free_page(page);
}

static void page_allocator_selftest(void)
{
	phys_addr_t first = alloc_page();
	phys_addr_t second = alloc_page();

	if (first == 0 || second == 0 || first == second) {
		panic("physical page allocator selftest failed");
	}

	free_page(second);
	free_page(first);

	early_log_puts("physical page allocator selftest ok\n");
}

void mm_init(const boot_info_t *boot_info)
{
	if (boot_info == 0 || boot_info->memory_map == 0 ||
		boot_info->memory_descriptor_size == 0) {
		panic("memory map unavailable");
	}

	free_pages = 0;
	free_page_count = 0;

	init_free_pages(boot_info);

	early_log_puts("physical pages free=");
	early_log_u64_decimal(free_page_count);
	early_log_puts("\n");

	page_allocator_selftest();
	page_table_selftest();
}
