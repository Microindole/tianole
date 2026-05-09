#ifndef TIANOLE_MM_H
#define TIANOLE_MM_H

#include <stddef.h>
#include <stdint.h>

#include <tianole/boot_info.h>

#define PAGE_SIZE 4096u

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

#define PAGE_PRESENT (1ull << 0)
#define PAGE_WRITABLE (1ull << 1)
#define PAGE_NO_EXECUTE (1ull << 63)

void mm_init(const boot_info_t *boot_info);
phys_addr_t alloc_page(void);
void free_page(phys_addr_t page);
void *kmalloc(size_t size);
void kfree(void *ptr);
int map_page(virt_addr_t virt, phys_addr_t phys, uint64_t flags);
int unmap_page(virt_addr_t virt);
int virt_to_phys(virt_addr_t virt, phys_addr_t *phys);
void heap_init(void);
void page_table_selftest(void);

#endif
