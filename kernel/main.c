#include <stdint.h>

#include "tianole/boot_info.h"

static inline void debug_putc(char ch) {
    __asm__ volatile("outb %0, $0xe9" : : "a"(ch));
}

static void debug_puts(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            debug_putc('\r');
        }
        debug_putc(*text++);
    }
}

static void debug_put_u64_decimal(uint64_t value) {
    char digits[20];
    uint32_t index = 0;

    if (value == 0) {
        debug_putc('0');
        return;
    }

    while (value != 0) {
        digits[index++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (index != 0) {
        debug_putc(digits[--index]);
    }
}

static void log_memory_map_summary(const tianole_boot_info_t *boot_info) {
    uint64_t offset;
    uint64_t descriptors = 0;
    uint64_t conventional_pages = 0;

    if (boot_info == 0 || boot_info->memory_map == 0 || boot_info->memory_descriptor_size == 0) {
        debug_puts("memory map metadata missing\n");
        return;
    }

    for (offset = 0; offset + sizeof(tianole_efi_memory_descriptor_t) <= boot_info->memory_map_size;
         offset += boot_info->memory_descriptor_size) {
        const tianole_efi_memory_descriptor_t *descriptor =
            (const tianole_efi_memory_descriptor_t *)(uintptr_t)(boot_info->memory_map + offset);

        descriptors++;
        if (descriptor->type == TIANOLE_EFI_MEMORY_TYPE_CONVENTIONAL) {
            conventional_pages += descriptor->number_of_pages;
        }
    }

    debug_puts("memory map descriptors=");
    debug_put_u64_decimal(descriptors);
    debug_puts("\n");
    debug_puts("conventional memory pages=");
    debug_put_u64_decimal(conventional_pages);
    debug_puts("\n");
}

void kernel_main(const tianole_boot_info_t *boot_info) {
    debug_puts("kernel_main entered\n");
    if (boot_info != 0 && boot_info->version == TIANOLE_BOOT_INFO_VERSION) {
        debug_puts("boot_info.version ok\n");
    } else {
        debug_puts("boot_info.version invalid\n");
    }

    if (boot_info != 0 && (boot_info->boot_flags & TIANOLE_BOOT_FLAG_BOOT_SERVICES_ACTIVE) != 0) {
        debug_puts("boot services still active\n");
    }

    log_memory_map_summary(boot_info);

    for (;;) {
        __asm__ volatile("hlt");
    }
}
