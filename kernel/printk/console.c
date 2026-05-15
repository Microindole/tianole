#include <stddef.h>

#include <tianole/console.h>
#include <tianole/early_log.h>
#include <tianole/errno.h>

static struct console *console_list;

int register_console(struct console *console)
{
	struct console *cursor;

	if (console == 0 || console->write == 0) {
		return -EINVAL;
	}

	for (cursor = console_list; cursor != 0; cursor = cursor->next) {
		if (cursor == console) {
			return 0;
		}
	}

	console->next = console_list;
	console_list = console;
	return 0;
}

int console_write_all(const char *text, size_t length)
{
	struct console *console;
	int count = 0;

	if (text == 0 || length == 0) {
		return 0;
	}

	for (console = console_list; console != 0; console = console->next) {
		console->write(console, text, length);
		count++;
	}

	if (count == 0) {
		size_t index;

		for (index = 0; index < length; index++) {
			early_log_putc(text[index]);
		}
	}

	return count;
}
