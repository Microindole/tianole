#ifndef TIANOLE_INPUT_H
#define TIANOLE_INPUT_H

#include <stdint.h>

#include <tianole/input-event-codes.h>

/**
 * enum input_event_type - Generic input event class.
 * @INPUT_EVENT_KEY: Keyboard-style key press or release.
 */
enum input_event_type {
	INPUT_EVENT_KEY = 1,
};

/**
 * enum input_bus_type - Physical bus used by an input device.
 * @INPUT_BUS_UNKNOWN: Bus is not known or not relevant.
 * @INPUT_BUS_I8042: PC i8042/PS/2 controller path.
 * @INPUT_BUS_USB: USB input device path reserved for future HID support.
 */
enum input_bus_type {
	INPUT_BUS_UNKNOWN = 0,
	INPUT_BUS_I8042 = 1,
	INPUT_BUS_USB = 2,
};

/**
 * enum input_device_capability - Event classes produced by a device.
 * @INPUT_DEVICE_CAP_KEY: Device can produce key events.
 */
enum input_device_capability {
	INPUT_DEVICE_CAP_KEY = 1u << 0,
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
 * struct input_dev - Registered input device descriptor.
 * @name: Human-readable device name.
 * @bus: Physical bus from enum input_bus_type.
 * @capabilities: Bitmask from enum input_device_capability.
 * @id: Stable runtime id assigned by input_register_device().
 * @registered: Non-zero after successful registration.
 *
 * Device drivers own this static descriptor and report events through input
 * core. The structure is intentionally small for now but keeps the Linux-like
 * boundary where input devices are registered before they produce events.
 */
struct input_dev {
	const char *name;
	uint16_t bus;
	uint32_t capabilities;
	uint32_t id;
	int registered;
};

/**
 * input_init() - Initialize the global input event queue.
 */
void input_init(void);

/**
 * input_register_device() - Register one static input device descriptor.
 * @dev: Device descriptor owned by the input driver.
 *
 * Return: 0 on success, -EINVAL for invalid input, or -ENOSPC when the small
 * early registry is full.
 */
int input_register_device(struct input_dev *dev);

/**
 * input_report_key() - Report one key event from a registered input device.
 * @dev: Registered input device.
 * @code: Hardware-independent key code.
 * @value: 1 for press, 0 for release.
 * @modifiers: Modifier snapshot produced by the driver.
 *
 * Return: 0 on success, or a negative errno value.
 */
int input_report_key(struct input_dev *dev,
	uint16_t code,
	int32_t value,
	uint32_t modifiers);

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

/**
 * input_last_event() - Copy the most recently reported input event.
 * @event: Destination event storage.
 *
 * Return: 0 on success, -EAGAIN when no event has been reported yet, or
 * -EINVAL for invalid input.
 */
int input_last_event(struct input_event *event);

/**
 * input_selftest() - Run boot-time input and keymap selftests.
 */
void input_selftest(void);

#endif
