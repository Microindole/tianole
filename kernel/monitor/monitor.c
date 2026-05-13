#include <stddef.h>

#include <tianole/console.h>
#include <tianole/early_log.h>
#include <tianole/input.h>
#include <tianole/monitor.h>
#include <tianole/sched.h>
#include <tianole/timer.h>

#define MONITOR_LINE_LENGTH 128u
#define MONITOR_PROMPT "tianole> "

static int monitor_streq(const char *left, const char *right)
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

static int monitor_starts_with(const char *text, const char *prefix)
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

static const char *monitor_skip_spaces(const char *line)
{
	while (*line == ' ' || *line == '\t') {
		line++;
	}

	return line;
}

static void monitor_print_help(void)
{
	early_log_puts("commands:\n");
	early_log_puts("  help        show this help\n");
	early_log_puts("  ticks       show timer ticks\n");
	early_log_puts("  drops       show input and line drops\n");
	early_log_puts("  echo TEXT   print TEXT\n");
}

static void monitor_print_ticks(void)
{
	early_log_puts("ticks=");
	early_log_u64_decimal(timer_ticks());
	early_log_puts("\n");
}

static void monitor_print_drops(void)
{
	early_log_puts("input_drops=");
	early_log_u64_decimal(input_dropped_events());
	early_log_puts(" line_drops=");
	early_log_u64_decimal(console_dropped_lines());
	early_log_puts("\n");
}

static void monitor_run_command(const char *line)
{
	const char *command = monitor_skip_spaces(line);

	if (*command == '\0') {
		return;
	}

	if (monitor_streq(command, "help")) {
		monitor_print_help();
		return;
	}

	if (monitor_streq(command, "ticks")) {
		monitor_print_ticks();
		return;
	}

	if (monitor_streq(command, "drops")) {
		monitor_print_drops();
		return;
	}

	if (monitor_starts_with(command, "echo")) {
		const char *text = command + 4;

		if (*text != '\0' && *text != ' ' && *text != '\t') {
			early_log_puts("unknown command: ");
			early_log_puts(command);
			early_log_puts("\n");
			return;
		}

		early_log_puts(monitor_skip_spaces(text));
		early_log_puts("\n");
		return;
	}

	early_log_puts("unknown command: ");
	early_log_puts(command);
	early_log_puts("\n");
}

static void monitor_thread(void *arg)
{
	char line[MONITOR_LINE_LENGTH];

	(void)arg;

	early_log_puts("kernel monitor ready\n");
	early_log_puts(MONITOR_PROMPT);
	for (;;) {
		int ret = console_read_line(line, sizeof(line));

		if (ret < 0) {
			panic("monitor read failed");
		}

		monitor_run_command(line);
		early_log_puts(MONITOR_PROMPT);
	}
}

void monitor_init(void)
{
	if (kernel_thread_create("monitor", monitor_thread, 0) == 0) {
		panic("monitor thread creation failed");
	}

	early_log_puts("monitor initialized\n");
}
