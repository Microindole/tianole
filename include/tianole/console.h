#ifndef TIANOLE_CONSOLE_H
#define TIANOLE_CONSOLE_H

#include <stddef.h>

/**
 * struct console - Kernel log console backend.
 * @name: Human-readable backend name.
 * @write: Raw byte output callback.
 * @next: Console list link owned by printk console core.
 *
 * This mirrors the small useful part of Linux's struct console. Console
 * backends receive printk output; interactive terminal input/output belongs to
 * the later tty/terminal layer.
 */
struct console {
	const char *name;
	void (*write)(struct console *console, const char *text, size_t length);
	struct console *next;
};

/**
 * register_console() - Add a backend to the printk console list.
 * @console: Static console descriptor owned by the backend.
 *
 * Return: 0 on success, or a negative errno value.
 */
int register_console(struct console *console);

/**
 * console_write_all() - Write bytes to every registered printk console.
 * @text: Bytes to write.
 * @length: Number of bytes to write.
 *
 * Return: Number of registered consoles written.
 */
int console_write_all(const char *text, size_t length);

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
