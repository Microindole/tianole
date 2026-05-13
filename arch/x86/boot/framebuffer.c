#include <tianole/boot_info.h>

#include "efi.h"
#include "framebuffer.h"

/**
 * boot_capture_framebuffer() - Snapshot the firmware GOP framebuffer mode.
 * @system_table: UEFI system table used to locate GOP.
 * @boot_info: Boot handoff structure receiving framebuffer fields.
 *
 * The kernel treats framebuffer logging as an optional early console backend.
 * Failure here should not stop boot as long as serial/debug-port logging works.
 *
 * Return: EFI_SUCCESS on success or an EFI error status.
 */
efi_status boot_capture_framebuffer(
	efi_system_table_t *system_table, boot_info_t *boot_info)
{
	efi_graphics_output_protocol_t *gop;
	efi_graphics_output_mode_information_t *info;
	efi_guid_t gop_guid = efi_graphics_output_protocol_guid();
	efi_status status;

	if (system_table == 0 || boot_info == 0) {
		return EFI_INVALID_PARAMETER;
	}

	status = system_table->boot_services->locate_protocol(
		&gop_guid, 0, (void **)&gop);
	if (status != EFI_SUCCESS || gop == 0 || gop->mode == 0 ||
		gop->mode->info == 0) {
		return status;
	}

	info = gop->mode->info;
	boot_info->framebuffer_base = gop->mode->frame_buffer_base;
	boot_info->framebuffer_size = gop->mode->frame_buffer_size;
	boot_info->framebuffer_width = info->horizontal_resolution;
	boot_info->framebuffer_height = info->vertical_resolution;
	boot_info->framebuffer_pixels_per_scan_line =
		info->pixels_per_scan_line;
	boot_info->framebuffer_pixel_format = info->pixel_format;

	return EFI_SUCCESS;
}
