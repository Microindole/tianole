#ifndef X86_BOOT_MEMORY_MAP_H
#define X86_BOOT_MEMORY_MAP_H

#include <tianole/boot_info.h>

#include "efi.h"

/**
 * boot_exit_services_with_latest_memory_map() - Leave UEFI boot services.
 * @image_handle: UEFI image handle passed to ExitBootServices().
 * @system_table: UEFI system table used to fetch the memory map.
 * @boot_info: Handoff structure receiving the final memory map metadata.
 *
 * UEFI invalidates the map key whenever allocations change. This helper keeps
 * fetching a fresh map and uses the matching key for ExitBootServices(). The
 * memory map buffer remains owned by the kernel after boot services exit.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
efi_status boot_exit_services_with_latest_memory_map(efi_handle image_handle,
	efi_system_table_t *system_table,
	boot_info_t *boot_info);

#endif
