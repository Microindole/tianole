#ifndef X86_BOOT_DEBUG_LOG_H
#define X86_BOOT_DEBUG_LOG_H

/**
 * boot_debug_log_putc() - Write one byte to QEMU debugcon during boot.
 * @ch: Byte to emit.
 *
 * The bootloader uses the QEMU 0xe9 debug port before the kernel serial and
 * framebuffer logging backends are available.
 */
static inline void boot_debug_log_putc(char ch)
{
	__asm__ volatile("outb %0, %1" : : "a"(ch), "Nd"(0xe9));
}

/**
 * boot_debug_log_puts() - Write a NUL-terminated boot debug string.
 * @text: ASCII text to emit; a NULL pointer is ignored.
 */
static inline void boot_debug_log_puts(const char *text)
{
	while (text != 0 && *text != 0) {
		boot_debug_log_putc(*text);
		text++;
	}
}

#endif
