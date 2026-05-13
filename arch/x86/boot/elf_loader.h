#ifndef X86_BOOT_ELF_LOADER_H
#define X86_BOOT_ELF_LOADER_H

#include <stdint.h>
#include <tianole/boot_info.h>

#include "efi.h"

/**
 * typedef kernel_entry_fn_t - Loaded kernel entry point ABI.
 * @boot_info: Boot handoff structure prepared by the UEFI loader.
 *
 * The UEFI bootloader is compiled with the Microsoft x64 ABI, while the kernel
 * uses the SysV ABI. The function pointer type makes that boundary explicit.
 */
typedef void __attribute__((sysv_abi)) (*kernel_entry_fn_t)(
	const boot_info_t *boot_info);

/**
 * boot_load_kernel_elf() - Load an x86_64 executable ELF kernel image.
 * @system_table: UEFI system table used for page allocation.
 * @kernel_image: In-memory ELF file contents.
 * @kernel_size: Size of @kernel_image in bytes.
 * @entry_out: Receives the SysV kernel entry point on success.
 *
 * Loadable segments are copied to their physical addresses from the ELF program
 * headers. This early loader intentionally supports only the kernel format that
 * Tianole links today: 64-bit little-endian x86_64 ET_EXEC.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
efi_status boot_load_kernel_elf(efi_system_table_t *system_table,
	const void *kernel_image,
	uint64_t kernel_size,
	kernel_entry_fn_t *entry_out);

#endif
