#ifndef X86_BOOT_MEMORY_MAP_H
#define X86_BOOT_MEMORY_MAP_H

#include <tianole/boot_info.h>

#include "efi.h"

efi_status boot_exit_services_with_latest_memory_map(efi_handle image_handle,
	efi_system_table_t *system_table,
	boot_info_t *boot_info);

#endif
