#include <stddef.h>
#include <stdint.h>

#include <tianole/input.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/tty.h>

static char input_key_to_ascii(uint16_t code, uint32_t modifiers)
{
	int shifted = (modifiers & INPUT_MODIFIER_SHIFT) != 0;

	switch (code) {
	case INPUT_KEY_A:
		return shifted ? 'A' : 'a';
	case INPUT_KEY_B:
		return shifted ? 'B' : 'b';
	case INPUT_KEY_C:
		return shifted ? 'C' : 'c';
	case INPUT_KEY_D:
		return shifted ? 'D' : 'd';
	case INPUT_KEY_E:
		return shifted ? 'E' : 'e';
	case INPUT_KEY_F:
		return shifted ? 'F' : 'f';
	case INPUT_KEY_G:
		return shifted ? 'G' : 'g';
	case INPUT_KEY_H:
		return shifted ? 'H' : 'h';
	case INPUT_KEY_I:
		return shifted ? 'I' : 'i';
	case INPUT_KEY_J:
		return shifted ? 'J' : 'j';
	case INPUT_KEY_K:
		return shifted ? 'K' : 'k';
	case INPUT_KEY_L:
		return shifted ? 'L' : 'l';
	case INPUT_KEY_M:
		return shifted ? 'M' : 'm';
	case INPUT_KEY_N:
		return shifted ? 'N' : 'n';
	case INPUT_KEY_O:
		return shifted ? 'O' : 'o';
	case INPUT_KEY_P:
		return shifted ? 'P' : 'p';
	case INPUT_KEY_Q:
		return shifted ? 'Q' : 'q';
	case INPUT_KEY_R:
		return shifted ? 'R' : 'r';
	case INPUT_KEY_S:
		return shifted ? 'S' : 's';
	case INPUT_KEY_T:
		return shifted ? 'T' : 't';
	case INPUT_KEY_U:
		return shifted ? 'U' : 'u';
	case INPUT_KEY_V:
		return shifted ? 'V' : 'v';
	case INPUT_KEY_W:
		return shifted ? 'W' : 'w';
	case INPUT_KEY_X:
		return shifted ? 'X' : 'x';
	case INPUT_KEY_Y:
		return shifted ? 'Y' : 'y';
	case INPUT_KEY_Z:
		return shifted ? 'Z' : 'z';
	case INPUT_KEY_0:
		return shifted ? ')' : '0';
	case INPUT_KEY_1:
		return shifted ? '!' : '1';
	case INPUT_KEY_2:
		return shifted ? '@' : '2';
	case INPUT_KEY_3:
		return shifted ? '#' : '3';
	case INPUT_KEY_4:
		return shifted ? '$' : '4';
	case INPUT_KEY_5:
		return shifted ? '%' : '5';
	case INPUT_KEY_6:
		return shifted ? '^' : '6';
	case INPUT_KEY_7:
		return shifted ? '&' : '7';
	case INPUT_KEY_8:
		return shifted ? '*' : '8';
	case INPUT_KEY_9:
		return shifted ? '(' : '9';
	case INPUT_KEY_SPACE:
		return ' ';
	case INPUT_KEY_TAB:
		return '\t';
	case INPUT_KEY_ENTER:
		return '\n';
	case INPUT_KEY_BACKSPACE:
		return '\b';
	default:
		return 0;
	}
}

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

		ch = input_key_to_ascii(event.code, event.modifiers);
		if (ch == 0) {
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
