#ifndef X86_BOOT_FILE_H
#define X86_BOOT_FILE_H

#include <stdint.h>

#include "efi.h"

/**
 * boot_read_file() - Read a file from the boot image volume.
 * @image_handle: UEFI image handle for the running bootloader.
 * @system_table: UEFI system table used for protocol and pool services.
 * @path: UTF-16 path relative to the boot volume root.
 * @buffer: Receives an allocated buffer containing the file bytes.
 * @size: Receives the number of bytes read.
 *
 * The returned buffer is allocated from UEFI loader data pool memory and is
 * intentionally kept alive until ExitBootServices(), because the kernel image
 * is consumed before firmware services are dropped.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
efi_status boot_read_file(efi_handle image_handle,
	efi_system_table_t *system_table,
	efi_char16_t *path,
	void **buffer,
	uint64_t *size);

#endif
