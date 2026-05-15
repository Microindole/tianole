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
 * This early consumer reads input events, decodes them to characters, and
 * feeds the temporary tty line discipline.
 */
void input_console_init(void);

#endif
