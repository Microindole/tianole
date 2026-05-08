#include <stdint.h>

#include "tianole/arch.h"

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

static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static void debug_port_putc(char ch)
{
	outb(X86_QEMU_DEBUG_PORT, (uint8_t)ch);
}

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

void arch_early_log_init(void)
{
	outb(X86_COM1_BASE + X86_COM_INTERRUPT_ENABLE, 0x00);
	outb(X86_COM1_BASE + X86_COM_LINE_CONTROL, X86_COM_LCR_DLAB);
	outb(X86_COM1_BASE + X86_COM_DATA, 0x03);
	outb(X86_COM1_BASE + X86_COM_INTERRUPT_ENABLE, 0x00);
	outb(X86_COM1_BASE + X86_COM_LINE_CONTROL, X86_COM_LCR_8N1);
	outb(X86_COM1_BASE + X86_COM_FIFO_CONTROL, 0xc7);
	outb(X86_COM1_BASE + X86_COM_MODEM_CONTROL, 0x0b);
}

void arch_early_log_putc(char ch)
{
	debug_port_putc(ch);
	serial_putc(ch);
}

void arch_halt_forever(void)
{
	__asm__ volatile("cli");

	for (;;) {
		__asm__ volatile("hlt");
	}
}
