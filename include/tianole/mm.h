#ifndef TIANOLE_MM_H
#define TIANOLE_MM_H

#include <stdint.h>

#include <tianole/boot_info.h>

#define PAGE_SIZE 4096u

typedef uint64_t phys_addr_t;

void mm_init(const boot_info_t *boot_info);
phys_addr_t alloc_page(void);
void free_page(phys_addr_t page);

#endif
