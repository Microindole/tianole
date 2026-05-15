#include <stddef.h>
#include <stdint.h>

#include <tianole/input.h>
#include <tianole/tty.h>

struct tty_keymap_entry {
	uint16_t code;
	char normal;
	char shifted;
	uint8_t flags;
};

#define TTY_KEYMAP_LETTER 1u

static const struct tty_keymap_entry us_keymap[] = {
	{INPUT_KEY_A, 'a', 'A', TTY_KEYMAP_LETTER},
	{INPUT_KEY_B, 'b', 'B', TTY_KEYMAP_LETTER},
	{INPUT_KEY_C, 'c', 'C', TTY_KEYMAP_LETTER},
	{INPUT_KEY_D, 'd', 'D', TTY_KEYMAP_LETTER},
	{INPUT_KEY_E, 'e', 'E', TTY_KEYMAP_LETTER},
	{INPUT_KEY_F, 'f', 'F', TTY_KEYMAP_LETTER},
	{INPUT_KEY_G, 'g', 'G', TTY_KEYMAP_LETTER},
	{INPUT_KEY_H, 'h', 'H', TTY_KEYMAP_LETTER},
	{INPUT_KEY_I, 'i', 'I', TTY_KEYMAP_LETTER},
	{INPUT_KEY_J, 'j', 'J', TTY_KEYMAP_LETTER},
	{INPUT_KEY_K, 'k', 'K', TTY_KEYMAP_LETTER},
	{INPUT_KEY_L, 'l', 'L', TTY_KEYMAP_LETTER},
	{INPUT_KEY_M, 'm', 'M', TTY_KEYMAP_LETTER},
	{INPUT_KEY_N, 'n', 'N', TTY_KEYMAP_LETTER},
	{INPUT_KEY_O, 'o', 'O', TTY_KEYMAP_LETTER},
	{INPUT_KEY_P, 'p', 'P', TTY_KEYMAP_LETTER},
	{INPUT_KEY_Q, 'q', 'Q', TTY_KEYMAP_LETTER},
	{INPUT_KEY_R, 'r', 'R', TTY_KEYMAP_LETTER},
	{INPUT_KEY_S, 's', 'S', TTY_KEYMAP_LETTER},
	{INPUT_KEY_T, 't', 'T', TTY_KEYMAP_LETTER},
	{INPUT_KEY_U, 'u', 'U', TTY_KEYMAP_LETTER},
	{INPUT_KEY_V, 'v', 'V', TTY_KEYMAP_LETTER},
	{INPUT_KEY_W, 'w', 'W', TTY_KEYMAP_LETTER},
	{INPUT_KEY_X, 'x', 'X', TTY_KEYMAP_LETTER},
	{INPUT_KEY_Y, 'y', 'Y', TTY_KEYMAP_LETTER},
	{INPUT_KEY_Z, 'z', 'Z', TTY_KEYMAP_LETTER},
	{INPUT_KEY_0, '0', ')', 0},
	{INPUT_KEY_1, '1', '!', 0},
	{INPUT_KEY_2, '2', '@', 0},
	{INPUT_KEY_3, '3', '#', 0},
	{INPUT_KEY_4, '4', '$', 0},
	{INPUT_KEY_5, '5', '%', 0},
	{INPUT_KEY_6, '6', '^', 0},
	{INPUT_KEY_7, '7', '&', 0},
	{INPUT_KEY_8, '8', '*', 0},
	{INPUT_KEY_9, '9', '(', 0},
	{INPUT_KEY_ENTER, '\n', '\n', 0},
	{INPUT_KEY_BACKSPACE, '\b', '\b', 0},
	{INPUT_KEY_TAB, '\t', '\t', 0},
	{INPUT_KEY_SPACE, ' ', ' ', 0},
	{INPUT_KEY_MINUS, '-', '_', 0},
	{INPUT_KEY_EQUAL, '=', '+', 0},
	{INPUT_KEY_LEFT_BRACKET, '[', '{', 0},
	{INPUT_KEY_RIGHT_BRACKET, ']', '}', 0},
	{INPUT_KEY_BACKSLASH, '\\', '|', 0},
	{INPUT_KEY_SEMICOLON, ';', ':', 0},
	{INPUT_KEY_APOSTROPHE, '\'', '"', 0},
	{INPUT_KEY_GRAVE, '`', '~', 0},
	{INPUT_KEY_COMMA, ',', '<', 0},
	{INPUT_KEY_DOT, '.', '>', 0},
	{INPUT_KEY_SLASH, '/', '?', 0},
};

int tty_key_event_to_char(uint16_t code, uint32_t modifiers, char *ch)
{
	size_t index;
	int shifted = (modifiers & INPUT_MODIFIER_SHIFT) != 0;
	int caps = (modifiers & INPUT_MODIFIER_CAPSLOCK) != 0;

	if (ch == 0) {
		return 0;
	}

	for (index = 0; index < sizeof(us_keymap) / sizeof(us_keymap[0]);
		index++) {
		if (us_keymap[index].code != code) {
			continue;
		}

		if ((us_keymap[index].flags & TTY_KEYMAP_LETTER) != 0) {
			shifted = shifted != caps;
		}

		*ch = shifted != 0 ? us_keymap[index].shifted
				   : us_keymap[index].normal;
		return 1;
	}

	return 0;
}
