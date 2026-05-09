#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>

static int early_log_ready;

void early_log_init(void)
{
	if (early_log_ready != 0) {
		return;
	}

	arch_early_log_init();
	early_log_ready = 1;
}

void early_log_putc(char ch)
{
	if (early_log_ready == 0) {
		early_log_init();
	}

	arch_early_log_putc(ch);
}

void early_log_puts(const char *text)
{
	if (text == 0) {
		return;
	}

	while (*text != '\0') {
		if (*text == '\n') {
			early_log_putc('\r');
		}
		early_log_putc(*text++);
	}
}

static void early_log_hex_digit(uint8_t digit)
{
	if (digit < 10) {
		early_log_putc((char)('0' + digit));
		return;
	}

	early_log_putc((char)('a' + digit - 10));
}

void early_log_u64_hex(uint64_t value)
{
	int shift;

	early_log_puts("0x");
	for (shift = 60; shift >= 0; shift -= 4) {
		early_log_hex_digit((uint8_t)((value >> shift) & 0x0f));
	}
}

void early_log_u64_decimal(uint64_t value)
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

void panic(const char *message)
{
	early_log_puts("panic: ");
	early_log_puts(message);
	early_log_puts("\n");

	arch_halt_forever();
}
