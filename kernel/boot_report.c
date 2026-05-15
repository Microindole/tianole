#include <stdint.h>

#include <tianole/boot_info.h>
#include <tianole/kernel_init.h>
#include <tianole/printk.h>

static void log_memory_map_summary(const boot_info_t *boot_info)
{
	uint64_t offset;
	uint64_t descriptors = 0;
	uint64_t conventional_pages = 0;

	if (boot_info == 0 || boot_info->memory_map == 0 ||
		boot_info->memory_descriptor_size == 0) {
		pr_warn("memory map metadata missing\n");
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

	pr_info("memory map descriptors=%llu\n",
		(unsigned long long)descriptors);
	pr_info("conventional memory pages=%llu\n",
		(unsigned long long)conventional_pages);
}

void kernel_report_boot_state(const boot_info_t *boot_info)
{
	if (boot_info != 0 && boot_info->version == BOOT_INFO_VERSION) {
		pr_info("boot_info.version ok\n");
	} else {
		pr_warn("boot_info.version invalid\n");
	}

	if (boot_info != 0 &&
		(boot_info->boot_flags & BOOT_FLAG_SERVICES_ACTIVE) == 0) {
		pr_info("boot services exited\n");
	} else {
		pr_warn("boot services still active\n");
	}

	log_memory_map_summary(boot_info);
}
