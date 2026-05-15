#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <tianole/console.h>
#include <tianole/printk.h>

#define PRINTK_RING_SIZE 4096u

/**
 * struct printk_ring - Early static kernel log buffer.
 * @buffer: Byte storage for formatted log output.
 * @head: Next byte position to write.
 * @count: Number of valid bytes currently retained.
 * @dropped: Number of overwritten bytes.
 *
 * This is intentionally much smaller than Linux's printk ringbuffer. The
 * important boundary is the same: normal kernel code writes log records to a
 * generic frontend, while consoles are replaceable output backends.
 */
struct printk_ring {
	char buffer[PRINTK_RING_SIZE];
	size_t head;
	size_t count;
	uint64_t dropped;
};

static struct printk_ring printk_ring;
static int printk_ready;

static void printk_ring_putc(char ch)
{
	printk_ring.buffer[printk_ring.head] = ch;
	printk_ring.head = (printk_ring.head + 1) % PRINTK_RING_SIZE;

	if (printk_ring.count < PRINTK_RING_SIZE) {
		printk_ring.count++;
	} else {
		printk_ring.dropped++;
	}
}

static void printk_console_putc(char ch)
{
	if (ch == '\n') {
		console_write_all("\r", 1);
	}
	console_write_all(&ch, 1);
}

static void printk_emit_char(char ch, int *count)
{
	if (printk_ready != 0) {
		printk_ring_putc(ch);
	}

	printk_console_putc(ch);
	(*count)++;
}

static void printk_emit_string(const char *text, int *count)
{
	if (text == 0) {
		text = "(null)";
	}

	while (*text != '\0') {
		printk_emit_char(*text++, count);
	}
}

static void printk_emit_unsigned_width(unsigned long long value,
	unsigned int base,
	unsigned int width,
	char pad,
	int *count)
{
	char digits[32];
	unsigned int index = 0;
	const char *alphabet = "0123456789abcdef";

	if (value == 0) {
		digits[index++] = '0';
	} else {
		while (value != 0) {
			digits[index++] = alphabet[value % base];
			value /= base;
		}
	}

	while (index < width) {
		printk_emit_char(pad, count);
		width--;
	}

	while (index != 0) {
		printk_emit_char(digits[--index], count);
	}
}

static void printk_emit_unsigned(
	unsigned long long value, unsigned int base, int *count)
{
	printk_emit_unsigned_width(value, base, 0, ' ', count);
}

static void printk_emit_signed(long long value, int *count)
{
	unsigned long long magnitude;

	if (value < 0) {
		printk_emit_char('-', count);
		magnitude = (unsigned long long)(-(value + 1)) + 1;
	} else {
		magnitude = (unsigned long long)value;
	}

	printk_emit_unsigned(magnitude, 10, count);
}

static int vprintk_format(const char *fmt, va_list args)
{
	int count = 0;

	if (fmt == 0) {
		return 0;
	}

	while (*fmt != '\0') {
		int long_count = 0;
		unsigned int width = 0;
		char pad = ' ';
		char spec;

		if (*fmt != '%') {
			printk_emit_char(*fmt++, &count);
			continue;
		}

		fmt++;
		if (*fmt == '%') {
			printk_emit_char(*fmt++, &count);
			continue;
		}

		if (*fmt == '0') {
			pad = '0';
			fmt++;
		}

		while (*fmt >= '0' && *fmt <= '9') {
			width = width * 10 + (unsigned int)(*fmt - '0');
			fmt++;
		}

		while (*fmt == 'l') {
			long_count++;
			fmt++;
		}

		spec = *fmt++;
		switch (spec) {
		case 'c':
			printk_emit_char((char)va_arg(args, int), &count);
			break;
		case 's':
			printk_emit_string(va_arg(args, const char *), &count);
			break;
		case 'd':
		case 'i':
			if (long_count >= 2) {
				printk_emit_signed(
					va_arg(args, long long), &count);
			} else if (long_count == 1) {
				printk_emit_signed(va_arg(args, long), &count);
			} else {
				printk_emit_signed(va_arg(args, int), &count);
			}
			break;
		case 'u':
			if (long_count >= 2) {
				printk_emit_unsigned_width(
					va_arg(args, unsigned long long),
					10,
					width,
					pad,
					&count);
			} else if (long_count == 1) {
				printk_emit_unsigned_width(
					va_arg(args, unsigned long),
					10,
					width,
					pad,
					&count);
			} else {
				printk_emit_unsigned_width(
					va_arg(args, unsigned int),
					10,
					width,
					pad,
					&count);
			}
			break;
		case 'x':
			if (long_count >= 2) {
				printk_emit_unsigned_width(
					va_arg(args, unsigned long long),
					16,
					width,
					pad,
					&count);
			} else if (long_count == 1) {
				printk_emit_unsigned_width(
					va_arg(args, unsigned long),
					16,
					width,
					pad,
					&count);
			} else {
				printk_emit_unsigned_width(
					va_arg(args, unsigned int),
					16,
					width,
					pad,
					&count);
			}
			break;
		case 'p':
			printk_emit_string("0x", &count);
			printk_emit_unsigned(
				(uintptr_t)va_arg(args, void *), 16, &count);
			break;
		default:
			printk_emit_char('%', &count);
			if (spec != '\0') {
				printk_emit_char(spec, &count);
			}
			break;
		}
	}

	return count;
}

void printk_init(void)
{
	if (printk_ready != 0) {
		return;
	}

	printk_ring.head = 0;
	printk_ring.count = 0;
	printk_ring.dropped = 0;
	printk_ready = 1;
}

int printk(const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vprintk_format(fmt, args);
	va_end(args);
	return ret;
}

int printk_level(enum printk_loglevel level, const char *fmt, ...)
{
	va_list args;
	int ret;

	(void)level;

	va_start(args, fmt);
	ret = vprintk_format(fmt, args);
	va_end(args);
	return ret;
}
