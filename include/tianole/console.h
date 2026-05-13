#ifndef TIANOLE_CONSOLE_H
#define TIANOLE_CONSOLE_H

#include <stddef.h>

/**
 * input_console_init() - Start the temporary input console consumer.
 *
 * This early consumer reads input events, performs minimal line editing, echoes
 * printable keys through the existing kernel log console, and queues completed
 * lines for future shell or command consumers.
 */
void input_console_init(void);

/**
 * console_read_line() - Read one completed console input line.
 * @buffer: Destination byte buffer.
 * @size: Size of @buffer in bytes.
 *
 * Sleeps until the input console submits a line. The returned line is always
 * NUL-terminated when @size is non-zero.
 *
 * Return: Number of bytes copied excluding the NUL terminator, or -EINVAL for
 * invalid input.
 */
int console_read_line(char *buffer, size_t size);

/**
 * console_try_read_line() - Try to read one completed line without sleeping.
 * @buffer: Destination byte buffer.
 * @size: Size of @buffer in bytes.
 *
 * Return: Number of bytes copied excluding the NUL terminator, -EAGAIN when no
 * line is ready, or -EINVAL for invalid input.
 */
int console_try_read_line(char *buffer, size_t size);

/**
 * console_dropped_lines() - Return line drops caused by full queues.
 *
 * Return: Number of completed lines dropped by the early console line queue.
 */
unsigned long console_dropped_lines(void);

#endif
