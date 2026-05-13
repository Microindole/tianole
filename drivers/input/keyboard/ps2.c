#include <arch/io.h>

#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/input.h>
#include <tianole/irq.h>
#include <tianole/keyboard.h>
#include <tianole/spinlock.h>
#include <tianole/timer.h>
#include <tianole/workqueue.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_KEYBOARD_IRQ 1u
#define PS2_KEYBOARD_DEVICE 1u
#define PS2_RAW_QUEUE_CAPACITY 32u
#define PS2_SCANCODE_RELEASE 0x80u

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
	int initialized;
};

static struct ps2_keyboard ps2_keyboard;

static enum input_key_code ps2_keycode_from_set1(uint8_t code)
{
	switch (code) {
	case 0x01:
		return INPUT_KEY_ESCAPE;
	case 0x02:
		return INPUT_KEY_1;
	case 0x03:
		return INPUT_KEY_2;
	case 0x04:
		return INPUT_KEY_3;
	case 0x05:
		return INPUT_KEY_4;
	case 0x06:
		return INPUT_KEY_5;
	case 0x07:
		return INPUT_KEY_6;
	case 0x08:
		return INPUT_KEY_7;
	case 0x09:
		return INPUT_KEY_8;
	case 0x0a:
		return INPUT_KEY_9;
	case 0x0b:
		return INPUT_KEY_0;
	case 0x0e:
		return INPUT_KEY_BACKSPACE;
	case 0x0f:
		return INPUT_KEY_TAB;
	case 0x10:
		return INPUT_KEY_Q;
	case 0x11:
		return INPUT_KEY_W;
	case 0x12:
		return INPUT_KEY_E;
	case 0x13:
		return INPUT_KEY_R;
	case 0x14:
		return INPUT_KEY_T;
	case 0x15:
		return INPUT_KEY_Y;
	case 0x16:
		return INPUT_KEY_U;
	case 0x17:
		return INPUT_KEY_I;
	case 0x18:
		return INPUT_KEY_O;
	case 0x19:
		return INPUT_KEY_P;
	case 0x1c:
		return INPUT_KEY_ENTER;
	case 0x1d:
		return INPUT_KEY_LEFT_CTRL;
	case 0x1e:
		return INPUT_KEY_A;
	case 0x1f:
		return INPUT_KEY_S;
	case 0x20:
		return INPUT_KEY_D;
	case 0x21:
		return INPUT_KEY_F;
	case 0x22:
		return INPUT_KEY_G;
	case 0x23:
		return INPUT_KEY_H;
	case 0x24:
		return INPUT_KEY_J;
	case 0x25:
		return INPUT_KEY_K;
	case 0x26:
		return INPUT_KEY_L;
	case 0x2a:
		return INPUT_KEY_LEFT_SHIFT;
	case 0x2c:
		return INPUT_KEY_Z;
	case 0x2d:
		return INPUT_KEY_X;
	case 0x2e:
		return INPUT_KEY_C;
	case 0x2f:
		return INPUT_KEY_V;
	case 0x30:
		return INPUT_KEY_B;
	case 0x31:
		return INPUT_KEY_N;
	case 0x32:
		return INPUT_KEY_M;
	case 0x36:
		return INPUT_KEY_RIGHT_SHIFT;
	case 0x38:
		return INPUT_KEY_LEFT_ALT;
	case 0x39:
		return INPUT_KEY_SPACE;
	default:
		return INPUT_KEY_UNKNOWN;
	}
}

static void ps2_update_modifier(enum input_key_code key, int pressed)
{
	uint32_t mask = 0;

	if (key == INPUT_KEY_LEFT_SHIFT || key == INPUT_KEY_RIGHT_SHIFT) {
		mask = INPUT_MODIFIER_SHIFT;
	} else if (key == INPUT_KEY_LEFT_CTRL) {
		mask = INPUT_MODIFIER_CTRL;
	} else if (key == INPUT_KEY_LEFT_ALT) {
		mask = INPUT_MODIFIER_ALT;
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
		struct input_event event;
		enum input_key_code key;
		uint8_t code = scancode & ~PS2_SCANCODE_RELEASE;
		int pressed = (scancode & PS2_SCANCODE_RELEASE) == 0;

		key = ps2_keycode_from_set1(code);
		if (key == INPUT_KEY_UNKNOWN) {
			early_log_puts("keyboard unknown scancode=0x");
			early_log_u64_hex(scancode);
			early_log_puts("\n");
			continue;
		}

		ps2_update_modifier(key, pressed);
		event.type = INPUT_EVENT_KEY;
		event.code = (uint16_t)key;
		event.value = pressed;
		event.modifiers = ps2_keyboard.modifiers;
		event.device = PS2_KEYBOARD_DEVICE;
		event.timestamp = timer_ticks();
		if (input_report_event(&event) != 0) {
			early_log_puts("keyboard input event dropped\n");
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
	ps2_keyboard.modifiers = 0;
	work_init(&ps2_keyboard.work, ps2_keyboard_work, 0);
	if (irq_register(PS2_KEYBOARD_IRQ, ps2_keyboard_irq, 0) != 0) {
		panic("keyboard irq registration failed");
	}
	ps2_keyboard.initialized = 1;
	early_log_puts("ps2 keyboard initialized\n");
}
