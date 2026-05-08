#ifndef X86_BOOT_ELF_LOADER_H
#define X86_BOOT_ELF_LOADER_H

#include <stdint.h>

#include "efi.h"
#include "tianole/boot_info.h"

typedef void
	__attribute__((sysv_abi)) (*kernel_entry_fn_t)(const boot_info_t *);

efi_status boot_load_kernel_elf(efi_system_table_t *system_table,
	const void *kernel_image,
	uint64_t kernel_size,
	kernel_entry_fn_t *entry_out);

#endif
