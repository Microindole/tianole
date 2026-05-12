#ifndef TIANOLE_MM_H
#define TIANOLE_MM_H

#include <stddef.h>
#include <stdint.h>

#include <tianole/boot_info.h>

/**
 * PAGE_SIZE - Base page size used by the current memory manager.
 */
#define PAGE_SIZE 4096u

/**
 * typedef phys_addr_t - Physical address value.
 *
 * Represents addresses in physical memory, not directly dereferenceable by C.
 */
typedef uint64_t phys_addr_t;

/**
 * typedef virt_addr_t - Virtual address value.
 *
 * Represents addresses in the active virtual address space.
 */
typedef uint64_t virt_addr_t;

/**
 * PAGE_PRESENT - Mapping flag that marks a page table entry present.
 */
#define PAGE_PRESENT (1ull << 0)

/**
 * PAGE_WRITABLE - Mapping flag that allows writes through a page mapping.
 */
#define PAGE_WRITABLE (1ull << 1)

/**
 * PAGE_NO_EXECUTE - Mapping flag that blocks instruction fetches.
 */
#define PAGE_NO_EXECUTE (1ull << 63)

/**
 * mm_init() - Initialize the generic memory manager.
 * @boot_info: Bootloader-provided memory map and handoff data.
 *
 * Converts boot memory information into Tianole-owned page allocator state.
 */
void mm_init(const boot_info_t *boot_info);

/**
 * alloc_page() - Allocate one physical page.
 *
 * Return: Physical page base address, or 0 when no page is available.
 */
phys_addr_t alloc_page(void);

/**
 * free_page() - Return one physical page to the allocator.
 * @page: Physical page base address previously returned by alloc_page().
 *
 * The page must not still be mapped or owned by another subsystem.
 */
void free_page(phys_addr_t page);

/**
 * kmalloc() - Allocate small kernel heap memory.
 * @size: Number of bytes requested.
 *
 * Return: Kernel virtual pointer, or NULL when allocation fails.
 */
void *kmalloc(size_t size);

/**
 * kfree() - Free memory allocated by kmalloc().
 * @ptr: Pointer returned by kmalloc(), or NULL.
 *
 * Releases the allocation back to the kernel heap.
 */
void kfree(void *ptr);

/**
 * map_page() - Map one virtual page to one physical page.
 * @virt: Virtual page address.
 * @phys: Physical page address.
 * @flags: Generic page flags such as PAGE_WRITABLE.
 *
 * Return: 0 on success, -EINVAL, -ENOMEM or -EEXIST on failure.
 */
int map_page(virt_addr_t virt, phys_addr_t phys, uint64_t flags);

/**
 * unmap_page() - Remove one virtual page mapping.
 * @virt: Virtual page address to unmap.
 *
 * Return: 0 on success, -EINVAL or -ENOENT on failure.
 */
int unmap_page(virt_addr_t virt);

/**
 * virt_to_phys() - Resolve a virtual address to a physical address.
 * @virt: Virtual address to query.
 * @phys: Output storage for the resolved physical address.
 *
 * Return: 0 when a mapping exists, -EINVAL or -ENOENT otherwise.
 */
int virt_to_phys(virt_addr_t virt, phys_addr_t *phys);

/**
 * heap_init() - Initialize the kernel heap.
 *
 * Sets up kmalloc()/kfree() after page allocation and page tables are ready.
 */
void heap_init(void);

/**
 * page_table_selftest() - Run boot-time page table checks.
 *
 * Verifies map, unmap and address translation behavior during early boot.
 */
void page_table_selftest(void);

#endif
