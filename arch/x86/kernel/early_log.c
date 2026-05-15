#include <stdint.h>

#include <arch/io.h>

#include <tianole/arch.h>
#include <tianole/console.h>

#include "screen.h"

#define X86_QEMU_DEBUG_PORT 0xe9

#define X86_COM1_BASE 0x3f8
#define X86_COM_DATA 0
#define X86_COM_INTERRUPT_ENABLE 1
#define X86_COM_FIFO_CONTROL 2
#define X86_COM_LINE_CONTROL 3
#define X86_COM_MODEM_CONTROL 4
#define X86_COM_LINE_STATUS 5

#define X86_COM_LCR_DLAB 0x80
#define X86_COM_LCR_8N1 0x03
#define X86_COM_LSR_THRE 0x20

static void debug_console_write(
	struct console *console, const char *text, size_t length);
static void serial_console_write(
	struct console *console, const char *text, size_t length);
static void screen_console_write(
	struct console *console, const char *text, size_t length);

static struct console debug_console = {
	.name = "debugcon",
	.write = debug_console_write,
};

static struct console serial_console = {
	.name = "ttyS0",
	.write = serial_console_write,
};

static struct console screen_boot_console = {
	.name = "screen",
	.write = screen_console_write,
};

static int x86_early_consoles_registered;

/**
 * debug_port_putc() - Emit one byte to QEMU debugcon port 0xe9.
 * @ch: Byte to write.
 */
static void debug_port_putc(char ch)
{
	outb(X86_QEMU_DEBUG_PORT, (uint8_t)ch);
}

/**
 * serial_putc() - Emit one byte through the legacy COM1 UART.
 * @ch: Byte to write.
 *
 * The bounded polling loop keeps panic-time logging from hanging forever if the
 * virtual UART stops reporting transmitter readiness.
 */
static void serial_putc(char ch)
{
	uint32_t timeout;

	for (timeout = 0; timeout < 100000; timeout++) {
		if ((inb(X86_COM1_BASE + X86_COM_LINE_STATUS) &
			    X86_COM_LSR_THRE) != 0) {
			break;
		}
	}

	outb(X86_COM1_BASE + X86_COM_DATA, (uint8_t)ch);
}

static void debug_console_write(
	struct console *console, const char *text, size_t length)
{
	size_t index;

	(void)console;

	for (index = 0; index < length; index++) {
		debug_port_putc(text[index]);
	}
}

static void serial_console_write(
	struct console *console, const char *text, size_t length)
{
	size_t index;

	(void)console;

	for (index = 0; index < length; index++) {
		serial_putc(text[index]);
	}
}

static void screen_console_write(
	struct console *console, const char *text, size_t length)
{
	size_t index;

	(void)console;

	for (index = 0; index < length; index++) {
		screen_console_putc(text[index]);
	}
}

static void register_x86_early_consoles(void)
{
	if (x86_early_consoles_registered != 0) {
		return;
	}

	(void)register_console(&debug_console);
	(void)register_console(&serial_console);
	(void)register_console(&screen_boot_console);
	x86_early_consoles_registered = 1;
}

/**
 * arch_early_log_init() - Initialize x86 early log backends.
 * @boot_info: Boot handoff data used to attach the framebuffer backend.
 *
 * Serial and debugcon remain available even if no framebuffer was captured.
 */
void arch_early_log_init(const boot_info_t *boot_info)
{
	outb(X86_COM1_BASE + X86_COM_INTERRUPT_ENABLE, 0x00);
	outb(X86_COM1_BASE + X86_COM_LINE_CONTROL, X86_COM_LCR_DLAB);
	outb(X86_COM1_BASE + X86_COM_DATA, 0x03);
	outb(X86_COM1_BASE + X86_COM_INTERRUPT_ENABLE, 0x00);
	outb(X86_COM1_BASE + X86_COM_LINE_CONTROL, X86_COM_LCR_8N1);
	outb(X86_COM1_BASE + X86_COM_FIFO_CONTROL, 0xc7);
	outb(X86_COM1_BASE + X86_COM_MODEM_CONTROL, 0x0b);
	screen_console_init(boot_info);
	register_x86_early_consoles();
}

/**
 * arch_early_log_putc() - Fan one log byte out to all x86 early consoles.
 * @ch: Byte emitted by the generic early_log frontend.
 */
void arch_early_log_putc(char ch)
{
	debug_port_putc(ch);
	serial_putc(ch);
	screen_console_putc(ch);
}

/**
 * arch_halt_forever() - Stop the CPU after a fatal kernel path.
 *
 * Interrupts are disabled first so panic does not return to partially updated
 * scheduler or device state.
 */
void arch_halt_forever(void)
{
	__asm__ volatile("cli");

	for (;;) {
		__asm__ volatile("hlt");
	}
}
