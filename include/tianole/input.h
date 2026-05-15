#ifndef TIANOLE_INPUT_H
#define TIANOLE_INPUT_H

#include <stdint.h>

/**
 * enum input_event_type - Generic input event class.
 * @INPUT_EVENT_KEY: Keyboard-style key press or release.
 */
enum input_event_type {
	INPUT_EVENT_KEY = 1,
};

/**
 * enum input_modifier - Modifier bits carried by input events.
 * @INPUT_MODIFIER_SHIFT: Either shift key is currently active.
 * @INPUT_MODIFIER_CTRL: Either control key is currently active.
 * @INPUT_MODIFIER_ALT: Either alt key is currently active.
 * @INPUT_MODIFIER_CAPSLOCK: Caps lock is toggled on.
 * @INPUT_MODIFIER_NUMLOCK: Num lock is toggled on.
 * @INPUT_MODIFIER_SCROLLLOCK: Scroll lock is toggled on.
 */
enum input_modifier {
	INPUT_MODIFIER_SHIFT = 1u << 0,
	INPUT_MODIFIER_CTRL = 1u << 1,
	INPUT_MODIFIER_ALT = 1u << 2,
	INPUT_MODIFIER_CAPSLOCK = 1u << 3,
	INPUT_MODIFIER_NUMLOCK = 1u << 4,
	INPUT_MODIFIER_SCROLLLOCK = 1u << 5,
};

/**
 * enum input_key_code - Hardware-independent key identity.
 *
 * The initial numeric range intentionally follows the common Linux KEY_* PC
 * keyboard values for the subset Tianole supports. This keeps the input layer
 * shaped like a real keycode namespace rather than a toy enum whose values
 * shift whenever a new key is inserted.
 *
 * @INPUT_KEY_UNKNOWN: Key is not decoded yet.
 * @INPUT_KEY_A: Letter A key.
 * @INPUT_KEY_B: Letter B key.
 * @INPUT_KEY_C: Letter C key.
 * @INPUT_KEY_D: Letter D key.
 * @INPUT_KEY_E: Letter E key.
 * @INPUT_KEY_F: Letter F key.
 * @INPUT_KEY_G: Letter G key.
 * @INPUT_KEY_H: Letter H key.
 * @INPUT_KEY_I: Letter I key.
 * @INPUT_KEY_J: Letter J key.
 * @INPUT_KEY_K: Letter K key.
 * @INPUT_KEY_L: Letter L key.
 * @INPUT_KEY_M: Letter M key.
 * @INPUT_KEY_N: Letter N key.
 * @INPUT_KEY_O: Letter O key.
 * @INPUT_KEY_P: Letter P key.
 * @INPUT_KEY_Q: Letter Q key.
 * @INPUT_KEY_R: Letter R key.
 * @INPUT_KEY_S: Letter S key.
 * @INPUT_KEY_T: Letter T key.
 * @INPUT_KEY_U: Letter U key.
 * @INPUT_KEY_V: Letter V key.
 * @INPUT_KEY_W: Letter W key.
 * @INPUT_KEY_X: Letter X key.
 * @INPUT_KEY_Y: Letter Y key.
 * @INPUT_KEY_Z: Letter Z key.
 * @INPUT_KEY_0: Number row 0 key.
 * @INPUT_KEY_1: Number row 1 key.
 * @INPUT_KEY_2: Number row 2 key.
 * @INPUT_KEY_3: Number row 3 key.
 * @INPUT_KEY_4: Number row 4 key.
 * @INPUT_KEY_5: Number row 5 key.
 * @INPUT_KEY_6: Number row 6 key.
 * @INPUT_KEY_7: Number row 7 key.
 * @INPUT_KEY_8: Number row 8 key.
 * @INPUT_KEY_9: Number row 9 key.
 * @INPUT_KEY_ENTER: Enter key.
 * @INPUT_KEY_ESCAPE: Escape key.
 * @INPUT_KEY_BACKSPACE: Backspace key.
 * @INPUT_KEY_TAB: Tab key.
 * @INPUT_KEY_SPACE: Space key.
 * @INPUT_KEY_MINUS: Minus key.
 * @INPUT_KEY_EQUAL: Equal key.
 * @INPUT_KEY_LEFT_BRACKET: Left bracket key.
 * @INPUT_KEY_RIGHT_BRACKET: Right bracket key.
 * @INPUT_KEY_BACKSLASH: Backslash key.
 * @INPUT_KEY_SEMICOLON: Semicolon key.
 * @INPUT_KEY_APOSTROPHE: Apostrophe key.
 * @INPUT_KEY_GRAVE: Grave accent key.
 * @INPUT_KEY_COMMA: Comma key.
 * @INPUT_KEY_DOT: Dot key.
 * @INPUT_KEY_SLASH: Slash key.
 * @INPUT_KEY_LEFT_SHIFT: Left shift modifier key.
 * @INPUT_KEY_RIGHT_SHIFT: Right shift modifier key.
 * @INPUT_KEY_LEFT_CTRL: Left control modifier key.
 * @INPUT_KEY_RIGHT_CTRL: Right control modifier key.
 * @INPUT_KEY_LEFT_ALT: Left alt modifier key.
 * @INPUT_KEY_RIGHT_ALT: Right alt modifier key.
 * @INPUT_KEY_CAPSLOCK: Caps lock key.
 * @INPUT_KEY_F1: Function key F1.
 * @INPUT_KEY_F2: Function key F2.
 * @INPUT_KEY_F3: Function key F3.
 * @INPUT_KEY_F4: Function key F4.
 * @INPUT_KEY_F5: Function key F5.
 * @INPUT_KEY_F6: Function key F6.
 * @INPUT_KEY_F7: Function key F7.
 * @INPUT_KEY_F8: Function key F8.
 * @INPUT_KEY_F9: Function key F9.
 * @INPUT_KEY_F10: Function key F10.
 * @INPUT_KEY_F11: Function key F11.
 * @INPUT_KEY_F12: Function key F12.
 * @INPUT_KEY_NUMLOCK: Num lock key.
 * @INPUT_KEY_SCROLLLOCK: Scroll lock key.
 * @INPUT_KEY_KP0: Keypad 0 key.
 * @INPUT_KEY_KP1: Keypad 1 key.
 * @INPUT_KEY_KP2: Keypad 2 key.
 * @INPUT_KEY_KP3: Keypad 3 key.
 * @INPUT_KEY_KP4: Keypad 4 key.
 * @INPUT_KEY_KP5: Keypad 5 key.
 * @INPUT_KEY_KP6: Keypad 6 key.
 * @INPUT_KEY_KP7: Keypad 7 key.
 * @INPUT_KEY_KP8: Keypad 8 key.
 * @INPUT_KEY_KP9: Keypad 9 key.
 * @INPUT_KEY_KP_DOT: Keypad dot key.
 * @INPUT_KEY_KP_SLASH: Keypad slash key.
 * @INPUT_KEY_KP_ASTERISK: Keypad asterisk key.
 * @INPUT_KEY_KP_MINUS: Keypad minus key.
 * @INPUT_KEY_KP_PLUS: Keypad plus key.
 * @INPUT_KEY_KP_ENTER: Keypad enter key.
 * @INPUT_KEY_HOME: Home key.
 * @INPUT_KEY_UP: Up arrow key.
 * @INPUT_KEY_PAGEUP: Page up key.
 * @INPUT_KEY_LEFT: Left arrow key.
 * @INPUT_KEY_RIGHT: Right arrow key.
 * @INPUT_KEY_END: End key.
 * @INPUT_KEY_DOWN: Down arrow key.
 * @INPUT_KEY_PAGEDOWN: Page down key.
 * @INPUT_KEY_INSERT: Insert key.
 * @INPUT_KEY_DELETE: Delete key.
 */
enum input_key_code {
	INPUT_KEY_UNKNOWN = 0,
	INPUT_KEY_ESCAPE = 1,
	INPUT_KEY_1 = 2,
	INPUT_KEY_2 = 3,
	INPUT_KEY_3 = 4,
	INPUT_KEY_4 = 5,
	INPUT_KEY_5 = 6,
	INPUT_KEY_6 = 7,
	INPUT_KEY_7 = 8,
	INPUT_KEY_8 = 9,
	INPUT_KEY_9 = 10,
	INPUT_KEY_0 = 11,
	INPUT_KEY_MINUS = 12,
	INPUT_KEY_EQUAL = 13,
	INPUT_KEY_BACKSPACE = 14,
	INPUT_KEY_TAB = 15,
	INPUT_KEY_Q = 16,
	INPUT_KEY_W = 17,
	INPUT_KEY_E = 18,
	INPUT_KEY_R = 19,
	INPUT_KEY_T = 20,
	INPUT_KEY_Y = 21,
	INPUT_KEY_U = 22,
	INPUT_KEY_I = 23,
	INPUT_KEY_O = 24,
	INPUT_KEY_P = 25,
	INPUT_KEY_LEFT_BRACKET = 26,
	INPUT_KEY_RIGHT_BRACKET = 27,
	INPUT_KEY_ENTER = 28,
	INPUT_KEY_LEFT_CTRL = 29,
	INPUT_KEY_A = 30,
	INPUT_KEY_S = 31,
	INPUT_KEY_D = 32,
	INPUT_KEY_F = 33,
	INPUT_KEY_G = 34,
	INPUT_KEY_H = 35,
	INPUT_KEY_J = 36,
	INPUT_KEY_K = 37,
	INPUT_KEY_L = 38,
	INPUT_KEY_SEMICOLON = 39,
	INPUT_KEY_APOSTROPHE = 40,
	INPUT_KEY_GRAVE = 41,
	INPUT_KEY_LEFT_SHIFT = 42,
	INPUT_KEY_BACKSLASH = 43,
	INPUT_KEY_Z = 44,
	INPUT_KEY_X = 45,
	INPUT_KEY_C = 46,
	INPUT_KEY_V = 47,
	INPUT_KEY_B = 48,
	INPUT_KEY_N = 49,
	INPUT_KEY_M = 50,
	INPUT_KEY_COMMA = 51,
	INPUT_KEY_DOT = 52,
	INPUT_KEY_SLASH = 53,
	INPUT_KEY_RIGHT_SHIFT = 54,
	INPUT_KEY_KP_ASTERISK = 55,
	INPUT_KEY_LEFT_ALT = 56,
	INPUT_KEY_SPACE = 57,
	INPUT_KEY_CAPSLOCK = 58,
	INPUT_KEY_F1 = 59,
	INPUT_KEY_F2 = 60,
	INPUT_KEY_F3 = 61,
	INPUT_KEY_F4 = 62,
	INPUT_KEY_F5 = 63,
	INPUT_KEY_F6 = 64,
	INPUT_KEY_F7 = 65,
	INPUT_KEY_F8 = 66,
	INPUT_KEY_F9 = 67,
	INPUT_KEY_F10 = 68,
	INPUT_KEY_NUMLOCK = 69,
	INPUT_KEY_SCROLLLOCK = 70,
	INPUT_KEY_KP7 = 71,
	INPUT_KEY_KP8 = 72,
	INPUT_KEY_KP9 = 73,
	INPUT_KEY_KP_MINUS = 74,
	INPUT_KEY_KP4 = 75,
	INPUT_KEY_KP5 = 76,
	INPUT_KEY_KP6 = 77,
	INPUT_KEY_KP_PLUS = 78,
	INPUT_KEY_KP1 = 79,
	INPUT_KEY_KP2 = 80,
	INPUT_KEY_KP3 = 81,
	INPUT_KEY_KP0 = 82,
	INPUT_KEY_KP_DOT = 83,
	INPUT_KEY_F11 = 87,
	INPUT_KEY_F12 = 88,
	INPUT_KEY_KP_ENTER = 96,
	INPUT_KEY_RIGHT_CTRL = 97,
	INPUT_KEY_KP_SLASH = 98,
	INPUT_KEY_RIGHT_ALT = 100,
	INPUT_KEY_HOME = 102,
	INPUT_KEY_UP = 103,
	INPUT_KEY_PAGEUP = 104,
	INPUT_KEY_LEFT = 105,
	INPUT_KEY_RIGHT = 106,
	INPUT_KEY_END = 107,
	INPUT_KEY_DOWN = 108,
	INPUT_KEY_PAGEDOWN = 109,
	INPUT_KEY_INSERT = 110,
	INPUT_KEY_DELETE = 111,
};

/**
 * struct input_event - Hardware-independent input event.
 * @type: Event class from enum input_event_type.
 * @code: Event code, normally enum input_key_code for key events.
 * @value: Event value, 1 for press and 0 for release for key events.
 * @modifiers: Snapshot of active modifier state.
 * @device: Input device identifier assigned by the producing driver.
 * @timestamp: Kernel timer tick when the event was reported.
 */
struct input_event {
	uint16_t type;
	uint16_t code;
	int32_t value;
	uint32_t modifiers;
	uint32_t device;
	uint64_t timestamp;
};

/**
 * input_init() - Initialize the global input event queue.
 */
void input_init(void);

/**
 * input_report_event() - Add one input event to the global queue.
 * @event: Event produced by an input driver.
 *
 * Return: 0 on success, -EINVAL for invalid input, or -ENOSPC if the queue
 * was full and the event was dropped.
 */
int input_report_event(const struct input_event *event);

/**
 * input_read_event() - Read one input event from the global queue.
 * @event: Destination event storage.
 *
 * Sleeps until an event is available. Callers that need nonblocking behavior
 * should use input_try_read_event().
 *
 * Return: 0 on success, or -EINVAL for invalid input.
 */
int input_read_event(struct input_event *event);

/**
 * input_try_read_event() - Try to read one input event without sleeping.
 * @event: Destination event storage.
 *
 * Return: 0 on success, -EAGAIN if no event is available, or -EINVAL for
 * invalid input.
 */
int input_try_read_event(struct input_event *event);

/**
 * input_dropped_events() - Return the number of dropped input events.
 *
 * Return: Count of events dropped because the fixed input queue was full.
 */
uint64_t input_dropped_events(void);

#endif
