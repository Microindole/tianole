#ifndef ARCH_X86_IO_H
#define ARCH_X86_IO_H

#include <stdint.h>

/**
 * outb() - Write one byte to an x86 I/O port.
 * @port: I/O port number.
 * @value: Byte to write.
 *
 * Port I/O is an x86-only backend primitive. Generic kernel code should reach
 * it through arch or driver interfaces instead of calling it directly.
 */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * inb() - Read one byte from an x86 I/O port.
 * @port: I/O port number.
 *
 * Return: Byte read from @port.
 */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

/**
 * io_wait() - Give legacy x86 port hardware a short ordering delay.
 *
 * The dummy write to port 0x80 follows the classic PC-compatible delay used
 * while programming slow legacy devices such as the 8259 PIC.
 */
static inline void io_wait(void)
{
	outb(0x80, 0);
}

#endif
