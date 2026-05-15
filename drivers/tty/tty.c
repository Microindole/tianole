#include <stddef.h>
#include <stdint.h>

#include <tianole/console.h>
#include <tianole/errno.h>
#include <tianole/sched.h>
#include <tianole/tty.h>

#define TTY_LINE_LENGTH 128u
#define TTY_LINE_QUEUE_SIZE 8u

/**
 * struct tty_line_queue - Completed input lines for future consumers.
 * @wait: Wait queue used for blocking readers and queue locking.
 * @lines: Fixed-size ring of NUL-terminated submitted lines.
 * @lengths: Length of each submitted line excluding the NUL terminator.
 * @head: Next line to read.
 * @tail: Next slot to write.
 * @count: Number of submitted lines.
 * @dropped: Lines dropped because the ring was full.
 *
 * This is the first terminal/tty boundary. Keyboard/input code feeds decoded
 * characters here; kdb and future shell-like consumers read completed lines.
 */
struct tty_line_queue {
	struct wait_queue wait;
	char lines[TTY_LINE_QUEUE_SIZE][TTY_LINE_LENGTH];
	size_t lengths[TTY_LINE_QUEUE_SIZE];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
	unsigned long dropped;
};

static struct tty_line_queue tty_lines;
static char edit_line[TTY_LINE_LENGTH];
static size_t edit_length;
static int tty_ready;

static const char *const tty_function_strings[] = {
	[TTY_FUNC_F1] = "\033[[A",
	[TTY_FUNC_F2] = "\033[[B",
	[TTY_FUNC_F3] = "\033[[C",
	[TTY_FUNC_F4] = "\033[[D",
	[TTY_FUNC_F5] = "\033[[E",
	[TTY_FUNC_F6] = "\033[17~",
	[TTY_FUNC_F7] = "\033[18~",
	[TTY_FUNC_F8] = "\033[19~",
	[TTY_FUNC_F9] = "\033[20~",
	[TTY_FUNC_F10] = "\033[21~",
	[TTY_FUNC_F11] = "\033[23~",
	[TTY_FUNC_F12] = "\033[24~",
	[TTY_FUNC_HOME] = "\033[1~",
	[TTY_FUNC_INSERT] = "\033[2~",
	[TTY_FUNC_DELETE] = "\033[3~",
	[TTY_FUNC_END] = "\033[4~",
	[TTY_FUNC_PAGEUP] = "\033[5~",
	[TTY_FUNC_PAGEDOWN] = "\033[6~",
	[TTY_FUNC_UP] = "\033[A",
	[TTY_FUNC_DOWN] = "\033[B",
	[TTY_FUNC_RIGHT] = "\033[C",
	[TTY_FUNC_LEFT] = "\033[D",
};

static int tty_has_line(void *arg)
{
	struct tty_line_queue *queue = arg;

	return queue->count != 0;
}

static void tty_echo_char(char ch)
{
	tty_write(&ch, 1);
}

void tty_write(const char *buffer, size_t size)
{
	size_t index;

	if (buffer == 0 || size == 0) {
		return;
	}

	for (index = 0; index < size; index++) {
		if (buffer[index] == '\n') {
			console_write_all("\r", 1);
		}
		console_write_all(&buffer[index], 1);
	}
}

void tty_write_string(const char *text)
{
	const char *cursor;
	size_t length = 0;

	if (text == 0) {
		return;
	}

	cursor = text;
	while (*cursor++ != '\0') {
		length++;
	}

	tty_write(text, length);
}

static int tty_pop_line_locked(char *buffer, size_t size)
{
	size_t length;
	size_t index;

	if (tty_lines.count == 0) {
		return -EAGAIN;
	}

	length = tty_lines.lengths[tty_lines.head];
	if (length + 1 > size) {
		length = size - 1;
	}

	for (index = 0; index < length; index++) {
		buffer[index] = tty_lines.lines[tty_lines.head][index];
	}
	buffer[length] = '\0';

	tty_lines.head = (tty_lines.head + 1) % TTY_LINE_QUEUE_SIZE;
	tty_lines.count--;
	return (int)length;
}

static void tty_submit_edit_line(void)
{
	uint64_t flags;
	size_t index;

	wait_queue_lock_irqsave(&tty_lines.wait, &flags);
	if (tty_lines.count == TTY_LINE_QUEUE_SIZE) {
		tty_lines.dropped++;
		wait_queue_unlock_irqrestore(&tty_lines.wait, flags);
		edit_length = 0;
		return;
	}

	for (index = 0; index < edit_length; index++) {
		tty_lines.lines[tty_lines.tail][index] = edit_line[index];
	}
	tty_lines.lines[tty_lines.tail][edit_length] = '\0';
	tty_lines.lengths[tty_lines.tail] = edit_length;
	tty_lines.tail = (tty_lines.tail + 1) % TTY_LINE_QUEUE_SIZE;
	tty_lines.count++;
	wait_queue_wake_one_locked(&tty_lines.wait);
	wait_queue_unlock_irqrestore(&tty_lines.wait, flags);

	edit_length = 0;
}

static void tty_handle_backspace(void)
{
	if (edit_length == 0) {
		return;
	}

	edit_length--;
	tty_echo_char('\b');
}

void tty_init(void)
{
	if (tty_ready != 0) {
		return;
	}

	wait_queue_init(&tty_lines.wait);
	tty_lines.head = 0;
	tty_lines.tail = 0;
	tty_lines.count = 0;
	tty_lines.dropped = 0;
	edit_length = 0;
	tty_ready = 1;
}

void tty_receive_char(char ch)
{
	if (tty_ready == 0) {
		tty_init();
	}

	if (ch == '\b') {
		tty_handle_backspace();
		return;
	}

	if (ch == '\n') {
		tty_echo_char('\n');
		tty_submit_edit_line();
		return;
	}

	if (edit_length + 1 >= TTY_LINE_LENGTH) {
		return;
	}

	edit_line[edit_length++] = ch;
	tty_echo_char(ch);
}

size_t tty_keysym_to_utf8(
	const struct tty_keysym *sym, char *buffer, size_t size)
{
	uint32_t codepoint;

	if (sym == 0 || buffer == 0 || sym->type != TTY_KEYSYM_UNICODE) {
		return 0;
	}

	codepoint = sym->value;
	if (codepoint <= 0x7f) {
		if (size < 1) {
			return 0;
		}
		buffer[0] = (char)codepoint;
		return 1;
	}

	if (codepoint <= 0x7ff) {
		if (size < 2) {
			return 0;
		}
		buffer[0] = (char)(0xc0u | (codepoint >> 6));
		buffer[1] = (char)(0x80u | (codepoint & 0x3fu));
		return 2;
	}

	if (codepoint <= 0xffff) {
		if (size < 3) {
			return 0;
		}
		buffer[0] = (char)(0xe0u | (codepoint >> 12));
		buffer[1] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
		buffer[2] = (char)(0x80u | (codepoint & 0x3fu));
		return 3;
	}

	if (codepoint <= 0x10ffff) {
		if (size < 4) {
			return 0;
		}
		buffer[0] = (char)(0xf0u | (codepoint >> 18));
		buffer[1] = (char)(0x80u | ((codepoint >> 12) & 0x3fu));
		buffer[2] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
		buffer[3] = (char)(0x80u | (codepoint & 0x3fu));
		return 4;
	}

	return 0;
}

const char *tty_keysym_function_string(
	const struct tty_keysym *sym, size_t *length)
{
	const char *text;
	size_t index;

	if (length != 0) {
		*length = 0;
	}
	if (sym == 0 || sym->type != TTY_KEYSYM_FUNCTION ||
		sym->value >= sizeof(tty_function_strings) /
				sizeof(tty_function_strings[0])) {
		return 0;
	}

	text = tty_function_strings[sym->value];
	if (text == 0) {
		return 0;
	}

	if (length == 0) {
		return text;
	}

	for (index = 0; text[index] != '\0'; index++) {
	}
	*length = index;
	return text;
}

void tty_receive_keysym(const struct tty_keysym *sym)
{
	char bytes[4];
	const char *function;
	size_t length;
	size_t index;

	function = tty_keysym_function_string(sym, &length);
	if (function != 0) {
		for (index = 0; index < length; index++) {
			tty_receive_char(function[index]);
		}
		return;
	}

	length = tty_keysym_to_utf8(sym, bytes, sizeof(bytes));
	for (index = 0; index < length; index++) {
		tty_receive_char(bytes[index]);
	}
}

int tty_read_line(char *buffer, size_t size)
{
	uint64_t flags;
	int ret;

	if (buffer == 0 || size == 0) {
		return -EINVAL;
	}

	if (tty_ready == 0) {
		tty_init();
	}

	ret = wait_queue_wait(&tty_lines.wait, tty_has_line, &tty_lines);
	if (ret != 0) {
		return ret;
	}

	wait_queue_lock_irqsave(&tty_lines.wait, &flags);
	ret = tty_pop_line_locked(buffer, size);
	wait_queue_unlock_irqrestore(&tty_lines.wait, flags);
	return ret;
}

int tty_try_read_line(char *buffer, size_t size)
{
	uint64_t flags;
	int ret;

	if (buffer == 0 || size == 0) {
		return -EINVAL;
	}

	if (tty_ready == 0) {
		tty_init();
	}

	wait_queue_lock_irqsave(&tty_lines.wait, &flags);
	ret = tty_pop_line_locked(buffer, size);
	wait_queue_unlock_irqrestore(&tty_lines.wait, flags);
	return ret;
}

unsigned long tty_dropped_lines(void)
{
	uint64_t flags;
	unsigned long dropped;

	if (tty_ready == 0) {
		return 0;
	}

	wait_queue_lock_irqsave(&tty_lines.wait, &flags);
	dropped = tty_lines.dropped;
	wait_queue_unlock_irqrestore(&tty_lines.wait, flags);
	return dropped;
}
