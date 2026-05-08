#include "debug_log.h"
#include "efi.h"
#include "elf_loader.h"
#include "file.h"
#include "memory_map.h"
#include "tianole/boot_info.h"

static efi_char16_t boot_banner_text[] = u"Tianole x86 bootloader.\r\n";
static efi_char16_t kernel_path_text[] = u"\\kernel.elf";

efi_status EFIAPI efi_main(
	efi_handle image_handle, efi_system_table_t *system_table)
{
	void *kernel_image;
	uint64_t kernel_size;
	kernel_entry_fn_t kernel_entry;
	boot_info_t boot_info = {
		.version = BOOT_INFO_VERSION,
		.boot_flags = BOOT_FLAG_SERVICES_ACTIVE,
	};
	efi_status status;

	system_table->con_out->output_string(
		system_table->con_out, boot_banner_text);
	boot_debug_log_puts("Tianole x86 bootloader loaded.\n");

	status = boot_read_file(image_handle,
		system_table,
		kernel_path_text,
		&kernel_image,
		&kernel_size);
	if (status != EFI_SUCCESS) {
		boot_debug_log_puts("failed: read kernel.elf\n");
		return status;
	}

	status = boot_load_kernel_elf(
		system_table, kernel_image, kernel_size, &kernel_entry);
	if (status != EFI_SUCCESS) {
		boot_debug_log_puts("failed: load kernel image\n");
		return status;
	}

	status = boot_exit_services_with_latest_memory_map(
		image_handle, system_table, &boot_info);
	if (status != EFI_SUCCESS) {
		boot_debug_log_puts("failed: exit boot services\n");
		return status;
	}

	boot_debug_log_puts("jumping to kernel entry\n");
	kernel_entry(&boot_info);

	boot_debug_log_puts("kernel returned unexpectedly\n");
	for (;;) {
		__asm__ volatile("hlt");
	}

	return EFI_SUCCESS;
}
