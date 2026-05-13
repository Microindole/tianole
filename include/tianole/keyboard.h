#ifndef TIANOLE_KEYBOARD_H
#define TIANOLE_KEYBOARD_H

/**
 * ps2_keyboard_init() - Register the initial PS/2 keyboard IRQ handler.
 *
 * The handler keeps interrupt work short: it reads raw scancodes and defers
 * decoding into the system workqueue.
 */
void ps2_keyboard_init(void);

#endif
