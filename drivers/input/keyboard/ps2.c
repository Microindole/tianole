#include <arch/io.h>

#include <tianole/errno.h>
#include <tianole/input.h>
#include <tianole/irq.h>
#include <tianole/keyboard.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/spinlock.h>
#include <tianole/workqueue.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_KEYBOARD_IRQ 1u
#define PS2_RAW_QUEUE_CAPACITY 32u
#define PS2_SCANCODE_RELEASE 0x80u
#define PS2_SCANCODE_EXTENDED 0xe0u
#define PS2_SCANCODE_PAUSE 0xe1u
#define PS2_SCANCODE_MASK 0x7fu
#define PS2_SET1_KEYMAP_SIZE 128u

/**
 * struct ps2_keyboard - Minimal PS/2 set-1 keyboard state.
 * @lock: Protects raw scancode queue from IRQ and worker context.
 * @work: Deferred decoder work item.
 * @raw: Raw scancode ring filled by IRQ1.
 * @head: Next raw scancode to decode.
 * @tail: Next raw slot to write.
 * @count: Number of queued raw scancodes.
 * @dropped: Raw scancodes dropped because the ring was full.
 * @modifiers: Current modifier state snapshot.
 * @extended_pending: Non-zero after an 0xe0 scancode prefix.
 * @pause_bytes: Number of remaining Pause/Break sequence bytes to ignore.
 * @initialized: Non-zero after driver registration.
 *
 * IRQ context only reads hardware and queues raw bytes. Decoding and input
 * event reporting run from workqueue thread context.
 */
struct ps2_keyboard {
	struct spinlock lock;
	struct work_struct work;
	uint8_t raw[PS2_RAW_QUEUE_CAPACITY];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
	uint64_t dropped;
	uint32_t modifiers;
	int extended_pending;
	uint8_t pause_bytes;
	int initialized;
};

static struct ps2_keyboard ps2_keyboard;
static struct input_dev ps2_input_dev = {
	.name = "ps2-keyboard",
	.bus = INPUT_BUS_I8042,
	.capabilities = INPUT_DEVICE_CAP_KEY,
};

static const enum input_key_code ps2_set1_keycode[PS2_SET1_KEYMAP_SIZE] = {
	[0x01] = INPUT_KEY_ESC,
	[0x02] = INPUT_KEY_1,
	[0x03] = INPUT_KEY_2,
	[0x04] = INPUT_KEY_3,
	[0x05] = INPUT_KEY_4,
	[0x06] = INPUT_KEY_5,
	[0x07] = INPUT_KEY_6,
	[0x08] = INPUT_KEY_7,
	[0x09] = INPUT_KEY_8,
	[0x0a] = INPUT_KEY_9,
	[0x0b] = INPUT_KEY_0,
	[0x0c] = INPUT_KEY_MINUS,
	[0x0d] = INPUT_KEY_EQUAL,
	[0x0e] = INPUT_KEY_BACKSPACE,
	[0x0f] = INPUT_KEY_TAB,
	[0x10] = INPUT_KEY_Q,
	[0x11] = INPUT_KEY_W,
	[0x12] = INPUT_KEY_E,
	[0x13] = INPUT_KEY_R,
	[0x14] = INPUT_KEY_T,
	[0x15] = INPUT_KEY_Y,
	[0x16] = INPUT_KEY_U,
	[0x17] = INPUT_KEY_I,
	[0x18] = INPUT_KEY_O,
	[0x19] = INPUT_KEY_P,
	[0x1a] = INPUT_KEY_LEFTBRACE,
	[0x1b] = INPUT_KEY_RIGHTBRACE,
	[0x1c] = INPUT_KEY_ENTER,
	[0x1d] = INPUT_KEY_LEFTCTRL,
	[0x1e] = INPUT_KEY_A,
	[0x1f] = INPUT_KEY_S,
	[0x20] = INPUT_KEY_D,
	[0x21] = INPUT_KEY_F,
	[0x22] = INPUT_KEY_G,
	[0x23] = INPUT_KEY_H,
	[0x24] = INPUT_KEY_J,
	[0x25] = INPUT_KEY_K,
	[0x26] = INPUT_KEY_L,
	[0x27] = INPUT_KEY_SEMICOLON,
	[0x28] = INPUT_KEY_APOSTROPHE,
	[0x29] = INPUT_KEY_GRAVE,
	[0x2a] = INPUT_KEY_LEFTSHIFT,
	[0x2b] = INPUT_KEY_BACKSLASH,
	[0x2c] = INPUT_KEY_Z,
	[0x2d] = INPUT_KEY_X,
	[0x2e] = INPUT_KEY_C,
	[0x2f] = INPUT_KEY_V,
	[0x30] = INPUT_KEY_B,
	[0x31] = INPUT_KEY_N,
	[0x32] = INPUT_KEY_M,
	[0x33] = INPUT_KEY_COMMA,
	[0x34] = INPUT_KEY_DOT,
	[0x35] = INPUT_KEY_SLASH,
	[0x36] = INPUT_KEY_RIGHTSHIFT,
	[0x37] = INPUT_KEY_KPASTERISK,
	[0x38] = INPUT_KEY_LEFTALT,
	[0x39] = INPUT_KEY_SPACE,
	[0x3a] = INPUT_KEY_CAPSLOCK,
	[0x3b] = INPUT_KEY_F1,
	[0x3c] = INPUT_KEY_F2,
	[0x3d] = INPUT_KEY_F3,
	[0x3e] = INPUT_KEY_F4,
	[0x3f] = INPUT_KEY_F5,
	[0x40] = INPUT_KEY_F6,
	[0x41] = INPUT_KEY_F7,
	[0x42] = INPUT_KEY_F8,
	[0x43] = INPUT_KEY_F9,
	[0x44] = INPUT_KEY_F10,
	[0x45] = INPUT_KEY_NUMLOCK,
	[0x46] = INPUT_KEY_SCROLLLOCK,
	[0x47] = INPUT_KEY_KP7,
	[0x48] = INPUT_KEY_KP8,
	[0x49] = INPUT_KEY_KP9,
	[0x4a] = INPUT_KEY_KPMINUS,
	[0x4b] = INPUT_KEY_KP4,
	[0x4c] = INPUT_KEY_KP5,
	[0x4d] = INPUT_KEY_KP6,
	[0x4e] = INPUT_KEY_KPPLUS,
	[0x4f] = INPUT_KEY_KP1,
	[0x50] = INPUT_KEY_KP2,
	[0x51] = INPUT_KEY_KP3,
	[0x52] = INPUT_KEY_KP0,
	[0x53] = INPUT_KEY_KPDOT,
	[0x57] = INPUT_KEY_F11,
	[0x58] = INPUT_KEY_F12,
};

static const enum input_key_code ps2_set1_e0_keycode[PS2_SET1_KEYMAP_SIZE] = {
	[0x1c] = INPUT_KEY_KPENTER,
	[0x1d] = INPUT_KEY_RIGHTCTRL,
	[0x35] = INPUT_KEY_KPSLASH,
	[0x38] = INPUT_KEY_RIGHTALT,
	[0x47] = INPUT_KEY_HOME,
	[0x48] = INPUT_KEY_UP,
	[0x49] = INPUT_KEY_PAGEUP,
	[0x4b] = INPUT_KEY_LEFT,
	[0x4d] = INPUT_KEY_RIGHT,
	[0x4f] = INPUT_KEY_END,
	[0x50] = INPUT_KEY_DOWN,
	[0x51] = INPUT_KEY_PAGEDOWN,
	[0x52] = INPUT_KEY_INSERT,
	[0x53] = INPUT_KEY_DELETE,
};

enum input_key_code ps2_keyboard_keycode_from_set1(uint8_t code, int extended)
{
	code &= PS2_SCANCODE_MASK;
	if (extended != 0) {
		return ps2_set1_e0_keycode[code];
	}
	return ps2_set1_keycode[code];
}

static void ps2_update_modifier(enum input_key_code key, int pressed)
{
	uint32_t mask = 0;

	if (key == INPUT_KEY_LEFTSHIFT || key == INPUT_KEY_RIGHTSHIFT) {
		mask = INPUT_MODIFIER_SHIFT;
	} else if (key == INPUT_KEY_LEFTCTRL || key == INPUT_KEY_RIGHTCTRL) {
		mask = INPUT_MODIFIER_CTRL;
	} else if (key == INPUT_KEY_LEFTALT || key == INPUT_KEY_RIGHTALT) {
		mask = INPUT_MODIFIER_ALT;
	} else if (pressed != 0 && key == INPUT_KEY_CAPSLOCK) {
		ps2_keyboard.modifiers ^= INPUT_MODIFIER_CAPSLOCK;
		return;
	} else if (pressed != 0 && key == INPUT_KEY_NUMLOCK) {
		ps2_keyboard.modifiers ^= INPUT_MODIFIER_NUMLOCK;
		return;
	} else if (pressed != 0 && key == INPUT_KEY_SCROLLLOCK) {
		ps2_keyboard.modifiers ^= INPUT_MODIFIER_SCROLLLOCK;
		return;
	}

	if (mask == 0) {
		return;
	}

	if (pressed != 0) {
		ps2_keyboard.modifiers |= mask;
	} else {
		ps2_keyboard.modifiers &= ~mask;
	}
}

static int ps2_raw_pop(uint8_t *scancode)
{
	uint64_t flags;

	spin_lock_irqsave(&ps2_keyboard.lock, &flags);
	if (ps2_keyboard.count == 0) {
		spin_unlock_irqrestore(&ps2_keyboard.lock, flags);
		return 0;
	}

	*scancode = ps2_keyboard.raw[ps2_keyboard.head];
	ps2_keyboard.head = (ps2_keyboard.head + 1) % PS2_RAW_QUEUE_CAPACITY;
	ps2_keyboard.count--;
	spin_unlock_irqrestore(&ps2_keyboard.lock, flags);
	return 1;
}

static void ps2_keyboard_work(struct work_struct *work)
{
	uint8_t scancode;

	(void)work;

	while (ps2_raw_pop(&scancode) != 0) {
		enum input_key_code key;
		uint8_t code = scancode & PS2_SCANCODE_MASK;
		int extended = ps2_keyboard.extended_pending;
		int pressed = (scancode & PS2_SCANCODE_RELEASE) == 0;

		if (ps2_keyboard.pause_bytes != 0) {
			ps2_keyboard.pause_bytes--;
			continue;
		}

		if (scancode == PS2_SCANCODE_PAUSE) {
			ps2_keyboard.pause_bytes = 5;
			continue;
		}

		if (scancode == PS2_SCANCODE_EXTENDED) {
			ps2_keyboard.extended_pending = 1;
			continue;
		}

		ps2_keyboard.extended_pending = 0;
		key = ps2_keyboard_keycode_from_set1(code, extended);
		if (key == INPUT_KEY_RESERVED) {
			pr_warn("keyboard unknown scancode=0x%x extended=%u\n",
				(unsigned int)code,
				(unsigned int)extended);
			continue;
		}

		ps2_update_modifier(key, pressed);
		if (input_report_key(&ps2_input_dev,
			    (uint16_t)key,
			    pressed,
			    ps2_keyboard.modifiers) != 0) {
			pr_warn("keyboard input event dropped\n");
		}
	}
}

static void ps2_keyboard_irq(uint8_t irq, void *data)
{
	uint8_t status;
	uint8_t scancode;
	uint64_t flags;

	(void)irq;
	(void)data;

	status = inb(PS2_STATUS_PORT);
	if ((status & PS2_STATUS_OUTPUT_FULL) == 0) {
		return;
	}

	scancode = inb(PS2_DATA_PORT);
	spin_lock_irqsave(&ps2_keyboard.lock, &flags);
	if (ps2_keyboard.count == PS2_RAW_QUEUE_CAPACITY) {
		ps2_keyboard.dropped++;
		spin_unlock_irqrestore(&ps2_keyboard.lock, flags);
		return;
	}

	ps2_keyboard.raw[ps2_keyboard.tail] = scancode;
	ps2_keyboard.tail = (ps2_keyboard.tail + 1) % PS2_RAW_QUEUE_CAPACITY;
	ps2_keyboard.count++;
	spin_unlock_irqrestore(&ps2_keyboard.lock, flags);

	(void)queue_work(&ps2_keyboard.work);
}

void ps2_keyboard_init(void)
{
	if (ps2_keyboard.initialized != 0) {
		return;
	}

	ps2_keyboard.lock.locked = 0;
	ps2_keyboard.head = 0;
	ps2_keyboard.tail = 0;
	ps2_keyboard.count = 0;
	ps2_keyboard.dropped = 0;
	ps2_keyboard.extended_pending = 0;
	ps2_keyboard.pause_bytes = 0;
	ps2_keyboard.modifiers = 0;
	work_init(&ps2_keyboard.work, ps2_keyboard_work, 0);
	if (input_register_device(&ps2_input_dev) != 0) {
		panic("keyboard input device registration failed");
	}
	if (irq_register(PS2_KEYBOARD_IRQ, ps2_keyboard_irq, 0) != 0) {
		panic("keyboard irq registration failed");
	}
	ps2_keyboard.initialized = 1;
	pr_info("ps2 keyboard initialized\n");
}
