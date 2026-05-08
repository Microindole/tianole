#include "efi.h"

static efi_char16_t banner_text[] = {
    'T', 'i', 'a', 'n', 'o', 'l', 'e', ' ',
    'b', 'o', 'o', 't', ' ', 's', 't', 'u',
    'b', ' ', 'l', 'o', 'a', 'd', 'e', 'd',
    '.', '\r', '\n', 0
};

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

efi_status EFIAPI efi_main(efi_handle image_handle, efi_system_table_t *system_table) {
    (void)image_handle;

    system_table->con_out->output_string(system_table->con_out, banner_text);
    debug_puts("Tianole boot stub loaded.\n");

    for (;;) {
        system_table->boot_services->stall(1000 * 1000);
    }
}
