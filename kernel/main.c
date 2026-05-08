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

    for (;;) {
        __asm__ volatile("hlt");
    }
}
