#include <stdint.h>

#include <tianole/errno.h>
#include <tianole/input.h>
#include <tianole/keyboard.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/tty.h>

static void input_assert_unicode(
	uint16_t code, uint32_t modifiers, uint32_t expected)
{
	struct tty_keysym sym;

	if (tty_key_event_to_keysym(code, modifiers, &sym) == 0 ||
		sym.type != TTY_KEYSYM_UNICODE || sym.value != expected) {
		panic("input keymap selftest failed");
	}
}

static void input_assert_no_char(uint16_t code)
{
	struct tty_keysym sym;

	if (tty_key_event_to_keysym(code, 0, &sym) != 0) {
		panic("input non-character keymap selftest failed");
	}
}

static void input_assert_function(
	uint16_t code, uint32_t expected, const char *expected_string)
{
	struct tty_keysym sym;
	const char *function;
	size_t length;
	size_t index;

	if (tty_key_event_to_keysym(code, 0, &sym) == 0 ||
		sym.type != TTY_KEYSYM_FUNCTION || sym.value != expected) {
		panic("input function keymap selftest failed");
	}

	function = tty_keysym_function_string(&sym, &length);
	if (function == 0) {
		panic("input function string selftest failed");
	}

	for (index = 0; index < length && expected_string[index] != '\0';
		index++) {
		if (function[index] != expected_string[index]) {
			panic("input function string content selftest failed");
		}
	}
	if (expected_string[index] != '\0' || index != length) {
		panic("input function string length selftest failed");
	}
}

static void input_assert_utf8(uint32_t codepoint, const char *expected)
{
	struct tty_keysym sym = {
		.type = TTY_KEYSYM_UNICODE,
		.value = codepoint,
	};
	char buffer[4];
	size_t length;
	size_t index;

	length = tty_keysym_to_utf8(&sym, buffer, sizeof(buffer));
	for (index = 0; index < length && expected[index] != '\0'; index++) {
		if (buffer[index] != expected[index]) {
			panic("input utf8 selftest failed");
		}
	}
	if (expected[index] != '\0' || index != length) {
		panic("input utf8 length selftest failed");
	}
}

static void input_assert_set1(
	uint8_t code, int extended, enum input_key_code expected)
{
	if (ps2_keyboard_keycode_from_set1(code, extended) != expected) {
		panic("ps2 set1 keycode selftest failed");
	}
}

static void input_assert_last_event(void)
{
	struct input_event event = {
		.type = INPUT_EVENT_KEY,
		.code = INPUT_KEY_A,
		.value = 1,
		.modifiers = 0,
		.device = 99,
		.timestamp = 0,
	};
	struct input_event last;

	if (input_last_event(&last) != -EAGAIN) {
		panic("input last-event empty selftest failed");
	}

	if (input_report_event(&event) != 0 || input_last_event(&last) != 0 ||
		last.code != INPUT_KEY_A || last.modifiers != 0) {
		panic("input last-event selftest failed");
	}

	if (input_try_read_event(&last) != 0) {
		panic("input last-event drain selftest failed");
	}
}

void input_selftest(void)
{
	input_assert_unicode(INPUT_KEY_A, 0, 'a');
	input_assert_unicode(INPUT_KEY_A, INPUT_MODIFIER_SHIFT, 'A');
	input_assert_unicode(INPUT_KEY_A, INPUT_MODIFIER_CAPSLOCK, 'A');
	input_assert_unicode(INPUT_KEY_A,
		INPUT_MODIFIER_SHIFT | INPUT_MODIFIER_CAPSLOCK,
		'a');
	input_assert_unicode(INPUT_KEY_1, 0, '1');
	input_assert_unicode(INPUT_KEY_1, INPUT_MODIFIER_SHIFT, '!');
	input_assert_unicode(INPUT_KEY_MINUS, 0, '-');
	input_assert_unicode(INPUT_KEY_MINUS, INPUT_MODIFIER_SHIFT, '_');
	input_assert_unicode(INPUT_KEY_TAB, 0, '\t');
	input_assert_unicode(INPUT_KEY_ENTER, 0, '\n');
	input_assert_utf8(0x00e9, "\xc3\xa9");
	input_assert_function(INPUT_KEY_F1, TTY_FUNC_F1, "\033[[A");
	input_assert_function(INPUT_KEY_UP, TTY_FUNC_UP, "\033[A");
	input_assert_function(INPUT_KEY_DELETE, TTY_FUNC_DELETE, "\033[3~");
	input_assert_no_char(INPUT_KEY_LEFTCTRL);

	input_assert_set1(0x1e, 0, INPUT_KEY_A);
	input_assert_set1(0x3b, 0, INPUT_KEY_F1);
	input_assert_set1(0x48, 1, INPUT_KEY_UP);
	input_assert_set1(0x1d, 1, INPUT_KEY_RIGHTCTRL);
	input_assert_set1(0x7f, 0, INPUT_KEY_RESERVED);
	input_assert_last_event();

	pr_info("input selftest ok\n");
}
