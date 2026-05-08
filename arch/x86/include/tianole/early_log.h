#ifndef X86_EARLY_LOG_H
#define X86_EARLY_LOG_H

#include <stdint.h>

static inline void early_log_putc(char ch)
{
	__asm__ volatile("outb %0, $0xe9" : : "a"(ch));
}

static inline void early_log_puts(const char *text)
{
	while (*text != '\0') {
		if (*text == '\n') {
			early_log_putc('\r');
		}
		early_log_putc(*text++);
	}
}

static inline void early_log_u64_decimal(uint64_t value)
{
	char digits[20];
	uint32_t index = 0;

	if (value == 0) {
		early_log_putc('0');
		return;
	}

	while (value != 0) {
		digits[index++] = (char)('0' + (value % 10));
		value /= 10;
	}

	while (index != 0) {
		early_log_putc(digits[--index]);
	}
}

#endif
