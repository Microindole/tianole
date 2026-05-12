#include <stddef.h>
#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/mm.h>

#define HEAP_BASE 0xffffff2000000000ull
#define HEAP_ALIGNMENT 16u

struct heap_block {
	size_t size;
	uint8_t free;
	struct heap_block *prev;
	struct heap_block *next;
};

static struct heap_block *heap_first;
static struct heap_block *heap_last;
static virt_addr_t heap_end = HEAP_BASE;
static int heap_ready;

static size_t align_up_size(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

static virt_addr_t align_up_addr(virt_addr_t value, uint64_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

static void block_insert_after(
	struct heap_block *block, struct heap_block *next)
{
	next->prev = block;
	next->next = block->next;

	if (block->next != 0) {
		block->next->prev = next;
	} else {
		heap_last = next;
	}

	block->next = next;
}

static void block_merge_next(struct heap_block *block)
{
	struct heap_block *next = block->next;

	if (next == 0 || next->free == 0) {
		return;
	}

	block->size += sizeof(*next) + next->size;
	block->next = next->next;

	if (next->next != 0) {
		next->next->prev = block;
	} else {
		heap_last = block;
	}
}

static void block_split(struct heap_block *block, size_t size)
{
	struct heap_block *next;

	if (block->size < size + sizeof(*next) + HEAP_ALIGNMENT) {
		return;
	}

	next = (struct heap_block *)((uint8_t *)(block + 1) + size);
	next->size = block->size - size - sizeof(*next);
	next->free = 1;

	block->size = size;
	block_insert_after(block, next);
}

static int map_heap_range(virt_addr_t start, size_t bytes)
{
	virt_addr_t current;
	virt_addr_t end = start + bytes;

	for (current = start; current < end; current += PAGE_SIZE) {
		phys_addr_t page = alloc_page();
		int ret;

		if (page == 0) {
			return -ENOMEM;
		}

		ret = map_page(current, page, PAGE_WRITABLE | PAGE_NO_EXECUTE);
		if (ret != 0) {
			free_page(page);
			return ret;
		}
	}

	return 0;
}

static int heap_extend(size_t min_size)
{
	size_t bytes =
		align_up_size(min_size + sizeof(struct heap_block), PAGE_SIZE);
	struct heap_block *block = (struct heap_block *)(uintptr_t)heap_end;
	int ret;

	ret = map_heap_range(heap_end, bytes);
	if (ret != 0) {
		return ret;
	}

	heap_end += bytes;

	block->size = bytes - sizeof(*block);
	block->free = 1;
	block->prev = heap_last;
	block->next = 0;

	if (heap_last != 0) {
		heap_last->next = block;
	} else {
		heap_first = block;
	}

	heap_last = block;

	if (block->prev != 0 && block->prev->free != 0) {
		block_merge_next(block->prev);
	}

	return 0;
}

static struct heap_block *find_free_block(size_t size)
{
	struct heap_block *block;

	for (block = heap_first; block != 0; block = block->next) {
		if (block->free != 0 && block->size >= size) {
			return block;
		}
	}

	return 0;
}

void *kmalloc(size_t size)
{
	struct heap_block *block;

	if (size == 0) {
		return 0;
	}

	size = align_up_size(size, HEAP_ALIGNMENT);
	block = find_free_block(size);

	if (block == 0) {
		if (heap_extend(size) != 0) {
			return 0;
		}
		block = find_free_block(size);
	}

	if (block == 0) {
		return 0;
	}

	block_split(block, size);
	block->free = 0;

	return block + 1;
}

void kfree(void *ptr)
{
	struct heap_block *block;

	if (ptr == 0) {
		return;
	}

	block = (struct heap_block *)ptr - 1;
	block->free = 1;

	block_merge_next(block);
	if (block->prev != 0 && block->prev->free != 0) {
		block_merge_next(block->prev);
	}
}

static void heap_selftest(void)
{
	uint64_t *first = kmalloc(sizeof(*first));
	uint8_t *second = kmalloc(128);
	uint64_t *third;

	if (first == 0 || second == 0) {
		panic("kernel heap selftest allocation failed");
	}

	*first = 0x54494f4c45484541ull;
	second[0] = 0x31;
	second[127] = 0x32;

	if (*first != 0x54494f4c45484541ull || second[0] != 0x31 ||
		second[127] != 0x32) {
		panic("kernel heap selftest memory corrupted");
	}

	kfree(first);
	third = kmalloc(sizeof(*third));
	if (third == 0) {
		panic("kernel heap selftest reuse failed");
	}

	kfree(third);
	kfree(second);

	early_log_puts("kernel heap selftest ok\n");
}

void heap_init(void)
{
	if (heap_ready != 0) {
		return;
	}

	heap_end = align_up_addr(heap_end, PAGE_SIZE);
	if (heap_extend(PAGE_SIZE - sizeof(struct heap_block)) != 0) {
		panic("kernel heap initialization failed");
	}

	heap_ready = 1;
	early_log_puts("kernel heap initialized\n");
	heap_selftest();
}
