#ifndef X86_BOOT_FILE_H
#define X86_BOOT_FILE_H

#include <stdint.h>

#include "efi.h"

efi_status boot_read_file(efi_handle image_handle,
	efi_system_table_t *system_table,
	efi_char16_t *path,
	void **buffer,
	uint64_t *size);

#endif
