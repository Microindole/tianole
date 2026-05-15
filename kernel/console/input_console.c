#include <stddef.h>
#include <stdint.h>

#include <tianole/input.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/tty.h>

static void input_console_thread(void *arg)
{
	(void)arg;

	for (;;) {
		struct input_event event;
		char ch;

		if (input_read_event(&event) != 0) {
			panic("input console read failed");
		}

		if (event.type != INPUT_EVENT_KEY || event.value == 0) {
			continue;
		}

		if (tty_key_event_to_char(event.code, event.modifiers, &ch) ==
			0) {
			continue;
		}

		tty_receive_char(ch);
	}
}

void input_console_init(void)
{
	tty_init();

	if (kernel_thread_create("input-console", input_console_thread, 0) ==
		0) {
		panic("input console thread creation failed");
	}

	pr_info("input console initialized\n");
}
