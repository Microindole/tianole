#include <stddef.h>

#include <tianole/input.h>
#include <tianole/kdb.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/timer.h>
#include <tianole/tty.h>

#define KDB_LINE_LENGTH 128u
#define KDB_PROMPT "tianole> "

static int kdb_streq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left != *right) {
			return 0;
		}
		left++;
		right++;
	}

	return *left == '\0' && *right == '\0';
}

static int kdb_starts_with(const char *text, const char *prefix)
{
	while (*prefix != '\0') {
		if (*text != *prefix) {
			return 0;
		}
		text++;
		prefix++;
	}

	return 1;
}

static const char *kdb_skip_spaces(const char *line)
{
	while (*line == ' ' || *line == '\t') {
		line++;
	}

	return line;
}

static void kdb_print_help(void)
{
	tty_write_string("commands:\n");
	tty_write_string("  help        show this help\n");
	tty_write_string("  ticks       show timer ticks\n");
	tty_write_string("  drops       show input and line drops\n");
	tty_write_string("  echo TEXT   print TEXT\n");
}

static void kdb_print_u64_decimal(unsigned long long value)
{
	char digits[20];
	size_t index = 0;

	if (value == 0) {
		tty_write_string("0");
		return;
	}

	while (value != 0) {
		digits[index++] = (char)('0' + (value % 10));
		value /= 10;
	}

	while (index != 0) {
		char digit = digits[--index];

		tty_write(&digit, 1);
	}
}

static void kdb_print_ticks(void)
{
	tty_write_string("ticks=");
	kdb_print_u64_decimal(timer_ticks());
	tty_write_string("\n");
}

static void kdb_print_drops(void)
{
	tty_write_string("input_drops=");
	kdb_print_u64_decimal(input_dropped_events());
	tty_write_string(" line_drops=");
	kdb_print_u64_decimal(tty_dropped_lines());
	tty_write_string("\n");
}

static void kdb_run_command(const char *line)
{
	const char *command = kdb_skip_spaces(line);

	if (*command == '\0') {
		return;
	}

	if (kdb_streq(command, "help")) {
		kdb_print_help();
		return;
	}

	if (kdb_streq(command, "ticks")) {
		kdb_print_ticks();
		return;
	}

	if (kdb_streq(command, "drops")) {
		kdb_print_drops();
		return;
	}

	if (kdb_starts_with(command, "echo")) {
		const char *text = command + 4;

		if (*text != '\0' && *text != ' ' && *text != '\t') {
			tty_write_string("unknown command: ");
			tty_write_string(command);
			tty_write_string("\n");
			return;
		}

		tty_write_string(kdb_skip_spaces(text));
		tty_write_string("\n");
		return;
	}

	tty_write_string("unknown command: ");
	tty_write_string(command);
	tty_write_string("\n");
}

static void kdb_thread(void *arg)
{
	char line[KDB_LINE_LENGTH];

	(void)arg;

	tty_write_string("early kdb ready\n");
	tty_write_string(KDB_PROMPT);
	for (;;) {
		int ret = tty_read_line(line, sizeof(line));

		if (ret < 0) {
			panic("kdb read failed");
		}

		kdb_run_command(line);
		tty_write_string(KDB_PROMPT);
	}
}

void kdb_init(void)
{
	if (kernel_thread_create("kdb", kdb_thread, 0) == 0) {
		panic("kdb thread creation failed");
	}

	pr_info("kdb initialized\n");
}
