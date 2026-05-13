#include <stddef.h>

#include <tianole/console.h>
#include <tianole/early_log.h>
#include <tianole/input.h>
#include <tianole/kdb.h>
#include <tianole/sched.h>
#include <tianole/timer.h>

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
	early_log_puts("commands:\n");
	early_log_puts("  help        show this help\n");
	early_log_puts("  ticks       show timer ticks\n");
	early_log_puts("  drops       show input and line drops\n");
	early_log_puts("  echo TEXT   print TEXT\n");
}

static void kdb_print_ticks(void)
{
	early_log_puts("ticks=");
	early_log_u64_decimal(timer_ticks());
	early_log_puts("\n");
}

static void kdb_print_drops(void)
{
	early_log_puts("input_drops=");
	early_log_u64_decimal(input_dropped_events());
	early_log_puts(" line_drops=");
	early_log_u64_decimal(console_dropped_lines());
	early_log_puts("\n");
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
			early_log_puts("unknown command: ");
			early_log_puts(command);
			early_log_puts("\n");
			return;
		}

		early_log_puts(kdb_skip_spaces(text));
		early_log_puts("\n");
		return;
	}

	early_log_puts("unknown command: ");
	early_log_puts(command);
	early_log_puts("\n");
}

static void kdb_thread(void *arg)
{
	char line[KDB_LINE_LENGTH];

	(void)arg;

	early_log_puts("early kdb ready\n");
	early_log_puts(KDB_PROMPT);
	for (;;) {
		int ret = console_read_line(line, sizeof(line));

		if (ret < 0) {
			panic("kdb read failed");
		}

		kdb_run_command(line);
		early_log_puts(KDB_PROMPT);
	}
}

void kdb_init(void)
{
	if (kernel_thread_create("kdb", kdb_thread, 0) == 0) {
		panic("kdb thread creation failed");
	}

	early_log_puts("kdb initialized\n");
}
