#include <stddef.h>
#include <stdint.h>

#include <tianole/console.h>
#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/input.h>
#include <tianole/panic.h>
#include <tianole/sched.h>

#define CONSOLE_LINE_LENGTH 128u
#define CONSOLE_LINE_QUEUE_SIZE 8u

/**
 * struct console_line_queue - Completed input lines for future consumers.
 * @wait: Wait queue used for blocking readers and queue locking.
 * @lines: Fixed-size ring of NUL-terminated submitted lines.
 * @lengths: Length of each submitted line excluding the NUL terminator.
 * @head: Next line to read.
 * @tail: Next slot to write.
 * @count: Number of submitted lines.
 * @dropped: Lines dropped because the ring was full.
 *
 * This is a temporary tty-like line discipline. Keyboard drivers feed generic
 * input events, this layer edits characters into lines, and future shell code
 * can read lines without depending on PS/2 scancodes.
 */
struct console_line_queue {
	struct wait_queue wait;
	char lines[CONSOLE_LINE_QUEUE_SIZE][CONSOLE_LINE_LENGTH];
	size_t lengths[CONSOLE_LINE_QUEUE_SIZE];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
	unsigned long dropped;
};

static struct console_line_queue console_lines;
static char edit_line[CONSOLE_LINE_LENGTH];
static size_t edit_length;

static int console_has_line(void *arg)
{
	struct console_line_queue *queue = arg;

	return queue->count != 0;
}

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

static int console_pop_line_locked(char *buffer, size_t size)
{
	size_t length;
	size_t index;

	if (console_lines.count == 0) {
		return -EAGAIN;
	}

	length = console_lines.lengths[console_lines.head];
	if (length + 1 > size) {
		length = size - 1;
	}

	for (index = 0; index < length; index++) {
		buffer[index] = console_lines.lines[console_lines.head][index];
	}
	buffer[length] = '\0';

	console_lines.head = (console_lines.head + 1) % CONSOLE_LINE_QUEUE_SIZE;
	console_lines.count--;
	return (int)length;
}

static void console_submit_edit_line(void)
{
	uint64_t flags;
	size_t index;

	wait_queue_lock_irqsave(&console_lines.wait, &flags);
	if (console_lines.count == CONSOLE_LINE_QUEUE_SIZE) {
		console_lines.dropped++;
		wait_queue_unlock_irqrestore(&console_lines.wait, flags);
		edit_length = 0;
		return;
	}

	for (index = 0; index < edit_length; index++) {
		console_lines.lines[console_lines.tail][index] =
			edit_line[index];
	}
	console_lines.lines[console_lines.tail][edit_length] = '\0';
	console_lines.lengths[console_lines.tail] = edit_length;
	console_lines.tail = (console_lines.tail + 1) % CONSOLE_LINE_QUEUE_SIZE;
	console_lines.count++;
	wait_queue_wake_one_locked(&console_lines.wait);
	wait_queue_unlock_irqrestore(&console_lines.wait, flags);

	edit_length = 0;
}

static void console_echo_backspace(void)
{
	if (edit_length == 0) {
		return;
	}

	edit_length--;
	early_log_putc('\b');
}

static void console_handle_char(char ch)
{
	if (ch == '\b') {
		console_echo_backspace();
		return;
	}

	if (ch == '\n') {
		early_log_putc('\n');
		console_submit_edit_line();
		return;
	}

	if (edit_length + 1 >= CONSOLE_LINE_LENGTH) {
		return;
	}

	edit_line[edit_length++] = ch;
	early_log_putc(ch);
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

		console_handle_char(ch);
	}
}

void input_console_init(void)
{
	wait_queue_init(&console_lines.wait);
	console_lines.head = 0;
	console_lines.tail = 0;
	console_lines.count = 0;
	console_lines.dropped = 0;
	edit_length = 0;

	if (kernel_thread_create("input-console", input_console_thread, 0) ==
		0) {
		panic("input console thread creation failed");
	}

	early_log_puts("input console initialized\n");
}

int console_read_line(char *buffer, size_t size)
{
	uint64_t flags;
	int ret;

	if (buffer == 0 || size == 0) {
		return -EINVAL;
	}

	ret = wait_queue_wait(
		&console_lines.wait, console_has_line, &console_lines);
	if (ret != 0) {
		return ret;
	}

	wait_queue_lock_irqsave(&console_lines.wait, &flags);
	ret = console_pop_line_locked(buffer, size);
	wait_queue_unlock_irqrestore(&console_lines.wait, flags);
	return ret;
}

int console_try_read_line(char *buffer, size_t size)
{
	uint64_t flags;
	int ret;

	if (buffer == 0 || size == 0) {
		return -EINVAL;
	}

	wait_queue_lock_irqsave(&console_lines.wait, &flags);
	ret = console_pop_line_locked(buffer, size);
	wait_queue_unlock_irqrestore(&console_lines.wait, flags);
	return ret;
}

unsigned long console_dropped_lines(void)
{
	uint64_t flags;
	unsigned long dropped;

	wait_queue_lock_irqsave(&console_lines.wait, &flags);
	dropped = console_lines.dropped;
	wait_queue_unlock_irqrestore(&console_lines.wait, flags);
	return dropped;
}
