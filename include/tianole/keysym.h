#ifndef TIANOLE_KEYSYM_H
#define TIANOLE_KEYSYM_H

#include <stdint.h>

/**
 * enum tty_keysym_type - Symbol kind produced by a tty keymap.
 * @TTY_KEYSYM_NONE: No symbol is produced for this key combination.
 * @TTY_KEYSYM_UNICODE: Symbol value is a Unicode codepoint.
 *
 * This is the Tianole equivalent of the intermediate keysym layer used by
 * Linux vt keyboard handling. Keyboards report key identities; tty keymaps
 * translate those identities into symbols, Unicode, function strings or
 * terminal actions.
 */
enum tty_keysym_type {
	TTY_KEYSYM_NONE = 0,
	TTY_KEYSYM_UNICODE = 1,
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

#endif
