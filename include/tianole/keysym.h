#ifndef TIANOLE_KEYSYM_H
#define TIANOLE_KEYSYM_H

#include <stdint.h>

/**
 * enum tty_keysym_type - Symbol kind produced by a tty keymap.
 * @TTY_KEYSYM_NONE: No symbol is produced for this key combination.
 * @TTY_KEYSYM_UNICODE: Symbol value is a Unicode codepoint.
 * @TTY_KEYSYM_FUNCTION: Symbol value indexes a terminal function string.
 *
 * This is the Tianole equivalent of the intermediate keysym layer used by
 * Linux vt keyboard handling. Keyboards report key identities; tty keymaps
 * translate those identities into symbols, Unicode, function strings or
 * terminal actions.
 */
enum tty_keysym_type {
	TTY_KEYSYM_NONE = 0,
	TTY_KEYSYM_UNICODE = 1,
	TTY_KEYSYM_FUNCTION = 2,
};

/**
 * struct tty_keysym - Result of applying a tty keymap to one input key.
 * @type: Symbol kind from enum tty_keysym_type.
 * @value: Unicode codepoint or type-specific symbol value.
 */
struct tty_keysym {
	uint16_t type;
	uint32_t value;
};

/**
 * enum tty_function_key - Default terminal function string identities.
 * @TTY_FUNC_F1: Function key F1.
 * @TTY_FUNC_F2: Function key F2.
 * @TTY_FUNC_F3: Function key F3.
 * @TTY_FUNC_F4: Function key F4.
 * @TTY_FUNC_F5: Function key F5.
 * @TTY_FUNC_F6: Function key F6.
 * @TTY_FUNC_F7: Function key F7.
 * @TTY_FUNC_F8: Function key F8.
 * @TTY_FUNC_F9: Function key F9.
 * @TTY_FUNC_F10: Function key F10.
 * @TTY_FUNC_F11: Function key F11.
 * @TTY_FUNC_F12: Function key F12.
 * @TTY_FUNC_HOME: Home navigation key.
 * @TTY_FUNC_INSERT: Insert navigation key.
 * @TTY_FUNC_DELETE: Delete navigation key.
 * @TTY_FUNC_END: End navigation key.
 * @TTY_FUNC_PAGEUP: Page up navigation key.
 * @TTY_FUNC_PAGEDOWN: Page down navigation key.
 * @TTY_FUNC_UP: Up cursor key.
 * @TTY_FUNC_DOWN: Down cursor key.
 * @TTY_FUNC_LEFT: Left cursor key.
 * @TTY_FUNC_RIGHT: Right cursor key.
 */
enum tty_function_key {
	TTY_FUNC_F1 = 0,
	TTY_FUNC_F2 = 1,
	TTY_FUNC_F3 = 2,
	TTY_FUNC_F4 = 3,
	TTY_FUNC_F5 = 4,
	TTY_FUNC_F6 = 5,
	TTY_FUNC_F7 = 6,
	TTY_FUNC_F8 = 7,
	TTY_FUNC_F9 = 8,
	TTY_FUNC_F10 = 9,
	TTY_FUNC_F11 = 10,
	TTY_FUNC_F12 = 11,
	TTY_FUNC_HOME = 20,
	TTY_FUNC_INSERT = 21,
	TTY_FUNC_DELETE = 22,
	TTY_FUNC_END = 23,
	TTY_FUNC_PAGEUP = 24,
	TTY_FUNC_PAGEDOWN = 25,
	TTY_FUNC_UP = 26,
	TTY_FUNC_DOWN = 27,
	TTY_FUNC_LEFT = 28,
	TTY_FUNC_RIGHT = 29,
};

#endif
