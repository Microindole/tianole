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
 * @INPUT_MODIFIER_CTRL: Left control key is currently active.
 * @INPUT_MODIFIER_ALT: Left alt key is currently active.
 */
enum input_modifier {
	INPUT_MODIFIER_SHIFT = 1u << 0,
	INPUT_MODIFIER_CTRL = 1u << 1,
	INPUT_MODIFIER_ALT = 1u << 2,
};

/**
 * enum input_key_code - Hardware-independent key identity.
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
 * @INPUT_KEY_LEFT_SHIFT: Left shift modifier key.
 * @INPUT_KEY_RIGHT_SHIFT: Right shift modifier key.
 * @INPUT_KEY_LEFT_CTRL: Left control modifier key.
 * @INPUT_KEY_LEFT_ALT: Left alt modifier key.
 */
enum input_key_code {
	INPUT_KEY_UNKNOWN = 0,
	INPUT_KEY_A,
	INPUT_KEY_B,
	INPUT_KEY_C,
	INPUT_KEY_D,
	INPUT_KEY_E,
	INPUT_KEY_F,
	INPUT_KEY_G,
	INPUT_KEY_H,
	INPUT_KEY_I,
	INPUT_KEY_J,
	INPUT_KEY_K,
	INPUT_KEY_L,
	INPUT_KEY_M,
	INPUT_KEY_N,
	INPUT_KEY_O,
	INPUT_KEY_P,
	INPUT_KEY_Q,
	INPUT_KEY_R,
	INPUT_KEY_S,
	INPUT_KEY_T,
	INPUT_KEY_U,
	INPUT_KEY_V,
	INPUT_KEY_W,
	INPUT_KEY_X,
	INPUT_KEY_Y,
	INPUT_KEY_Z,
	INPUT_KEY_0,
	INPUT_KEY_1,
	INPUT_KEY_2,
	INPUT_KEY_3,
	INPUT_KEY_4,
	INPUT_KEY_5,
	INPUT_KEY_6,
	INPUT_KEY_7,
	INPUT_KEY_8,
	INPUT_KEY_9,
	INPUT_KEY_ENTER,
	INPUT_KEY_ESCAPE,
	INPUT_KEY_BACKSPACE,
	INPUT_KEY_TAB,
	INPUT_KEY_SPACE,
	INPUT_KEY_LEFT_SHIFT,
	INPUT_KEY_RIGHT_SHIFT,
	INPUT_KEY_LEFT_CTRL,
	INPUT_KEY_LEFT_ALT,
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
