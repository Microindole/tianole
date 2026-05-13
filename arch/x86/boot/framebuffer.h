#ifndef X86_BOOT_FRAMEBUFFER_H
#define X86_BOOT_FRAMEBUFFER_H

#include <tianole/boot_info.h>

#include "efi.h"

/**
 * boot_capture_framebuffer() - Copy GOP mode data into boot_info.
 * @system_table: UEFI system table used to locate GOP.
 * @boot_info: Kernel handoff structure updated on success.
 *
 * Failure is non-fatal for boot. The kernel keeps serial/debug-port logging if
 * the firmware exposes no supported graphics output protocol.
 */
efi_status boot_capture_framebuffer(
	efi_system_table_t *system_table, boot_info_t *boot_info);

#endif
