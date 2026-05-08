#include <stdint.h>

#include <tianole/boot_info.h>
#include <tianole/early_log.h>
#include <tianole/kernel_init.h>

static void log_memory_map_summary(const boot_info_t *boot_info)
{
	uint64_t offset;
	uint64_t descriptors = 0;
	uint64_t conventional_pages = 0;

	if (boot_info == 0 || boot_info->memory_map == 0 ||
		boot_info->memory_descriptor_size == 0) {
		early_log_puts("memory map metadata missing\n");
		return;
	}

	for (offset = 0; offset + sizeof(boot_memory_descriptor_t) <=
		boot_info->memory_map_size;
		offset += boot_info->memory_descriptor_size) {
		const boot_memory_descriptor_t *descriptor =
			(const boot_memory_descriptor_t
					*)(uintptr_t)(boot_info->memory_map +
				offset);

		descriptors++;
		if (descriptor->type == BOOT_MEMORY_TYPE_CONVENTIONAL) {
			conventional_pages += descriptor->number_of_pages;
		}
	}

	early_log_puts("memory map descriptors=");
	early_log_u64_decimal(descriptors);
	early_log_puts("\n");
	early_log_puts("conventional memory pages=");
	early_log_u64_decimal(conventional_pages);
	early_log_puts("\n");
}

void kernel_report_boot_state(const boot_info_t *boot_info)
{
	if (boot_info != 0 && boot_info->version == BOOT_INFO_VERSION) {
		early_log_puts("boot_info.version ok\n");
	} else {
		early_log_puts("boot_info.version invalid\n");
	}

	if (boot_info != 0 &&
		(boot_info->boot_flags & BOOT_FLAG_SERVICES_ACTIVE) == 0) {
		early_log_puts("boot services exited\n");
	} else {
		early_log_puts("boot services still active\n");
	}

	log_memory_map_summary(boot_info);
}
