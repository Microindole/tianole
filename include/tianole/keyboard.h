#ifndef TIANOLE_KEYBOARD_H
#define TIANOLE_KEYBOARD_H

#include <stdint.h>

#include <tianole/input.h>

/**
 * ps2_keyboard_init() - Register the initial PS/2 keyboard IRQ handler.
 *
 * The handler keeps interrupt work short: it reads raw scancodes and defers
 * decoding into the system workqueue.
 */
void ps2_keyboard_init(void);

/**
 * ps2_keyboard_keycode_from_set1() - Map a PS/2 set-1 scancode to a key code.
 * @code: Set-1 make/break scancode with the release bit already stripped.
 * @extended: Non-zero for an 0xe0-prefixed scancode.
 *
 * This mirrors Linux atkbd's table-driven scancode-to-keycode boundary. It is
 * exposed so boot selftests can verify the protocol table without injecting
 * hardware IRQs.
 *
 * Return: Hardware-independent input key code, or INPUT_KEY_RESERVED.
 */
enum input_key_code ps2_keyboard_keycode_from_set1(uint8_t code, int extended);

#endif
