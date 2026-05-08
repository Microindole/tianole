#ifndef X86_BOOT_DEBUG_LOG_H
#define X86_BOOT_DEBUG_LOG_H

static inline void boot_debug_log_putc(char ch)
{
	__asm__ volatile("outb %0, $0xe9" : : "a"(ch));
}

static inline void boot_debug_log_puts(const char *text)
{
	while (*text != '\0') {
		if (*text == '\n') {
			boot_debug_log_putc('\r');
		}
		boot_debug_log_putc(*text++);
	}
}

#endif
