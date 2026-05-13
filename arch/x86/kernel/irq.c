#include <stdint.h>

#include <arch/io.h>
#include <arch/traps.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/irq.h>
#include <tianole/timer.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xa0
#define PIC2_DATA 0xa1
#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define PIT_FREQUENCY 1193182u
#define PIT_TARGET_HZ 100u
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_MODE_RATE_GENERATOR 0x36

#define IRQ_BASE 32u
#define IRQ_TIMER 0u
#define IRQ_KEYBOARD 1u
#define IRQ_COUNT 16u

/**
 * struct irq_action - Registered external IRQ callback.
 * @handler: Short interrupt handler called from the trap path.
 * @data: Opaque handler argument.
 *
 * Handlers run with normal kernel scheduling unavailable. They must do minimal
 * work, avoid sleeping, and defer policy to thread context where possible.
 */
struct irq_action {
	irq_handler_t handler;
	void *data;
};

static struct irq_action irq_actions[IRQ_COUNT];

/**
 * pic_remap() - Move legacy PIC IRQs away from CPU exception vectors.
 *
 * PC firmware commonly starts the 8259 PIC at vectors 0-15, which conflict
 * with architectural exceptions. Tianole remaps IRQ0-15 to vectors 32-47 so
 * trap dispatch can distinguish hardware interrupts from CPU faults.
 */
static void pic_remap(void)
{
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();

	outb(PIC1_DATA, IRQ_BASE);
	io_wait();
	outb(PIC2_DATA, IRQ_BASE + 8);
	io_wait();

	outb(PIC1_DATA, 4);
	io_wait();
	outb(PIC2_DATA, 2);
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
}

/**
 * pic_mask_initial_irqs() - Unmask early IRQ lines used during boot.
 *
 * Timer IRQ0 drives scheduling, and keyboard IRQ1 feeds the early input
 * pipeline. Other legacy IRQ lines stay masked until their drivers exist.
 */
static void pic_mask_initial_irqs(void)
{
	outb(PIC1_DATA, (uint8_t) ~((1u << IRQ_TIMER) | (1u << IRQ_KEYBOARD)));
	outb(PIC2_DATA, 0xff);
}

/**
 * pic_send_eoi() - Acknowledge a handled legacy PIC interrupt.
 * @irq: IRQ number in the remapped 0-15 PIC range.
 *
 * Slave PIC interrupts must be acknowledged on both controllers; master-only
 * IRQs need only the master EOI.
 */
static void pic_send_eoi(uint8_t irq)
{
	if (irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}

	outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * pit_init() - Program PIT channel 0 as the initial periodic timer.
 *
 * PIT is only the first x86 timer backend. The generic scheduler sees ticks
 * through `timer_tick()` and should not depend on PIT details.
 */
static void pit_init(void)
{
	uint16_t divisor = (uint16_t)(PIT_FREQUENCY / PIT_TARGET_HZ);

	outb(PIT_COMMAND, PIT_MODE_RATE_GENERATOR);
	outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xff));
	outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

/**
 * arch_irq_save() - Disable local interrupts and return previous RFLAGS.
 *
 * Return: Saved RFLAGS value for arch_irq_restore().
 */
uint64_t arch_irq_save(void)
{
	uint64_t flags;

	__asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

/**
 * arch_irq_restore() - Restore local interrupt enable state from RFLAGS.
 * @flags: RFLAGS value returned by arch_irq_save().
 */
void arch_irq_restore(uint64_t flags)
{
	if ((flags & (1ull << 9)) != 0) {
		__asm__ volatile("sti" : : : "memory");
	}
}

/**
 * irq_register() - Register one legacy PIC IRQ handler.
 * @irq: IRQ number in the 0-15 PIC range.
 * @handler: Callback run from interrupt context.
 * @data: Opaque callback data.
 *
 * Return: 0 on success, -EINVAL for invalid input, or -EBUSY if occupied.
 */
int irq_register(uint8_t irq, irq_handler_t handler, void *data)
{
	uint64_t flags;

	if (irq >= IRQ_COUNT || handler == 0) {
		return -EINVAL;
	}

	flags = arch_irq_save();

	if (irq_actions[irq].handler != 0) {
		arch_irq_restore(flags);
		return -EBUSY;
	}

	irq_actions[irq].handler = handler;
	irq_actions[irq].data = data;

	arch_irq_restore(flags);
	return 0;
}

static void timer_irq_handler(uint8_t irq, void *data)
{
	(void)irq;
	(void)data;

	timer_tick();
}

/**
 * handle_irq() - Dispatch a remapped external IRQ from trap context.
 * @frame: Trap frame whose vector lies in the IRQ range.
 *
 * The handler sends EOI only after a registered callback returns. Unregistered
 * interrupts are logged and acknowledged so the PIC line does not remain stuck.
 */
void handle_irq(struct trap_frame *frame)
{
	uint64_t irq = frame->vector - IRQ_BASE;
	struct irq_action *action;

	if (irq < IRQ_COUNT) {
		action = &irq_actions[irq];
		if (action->handler != 0) {
			action->handler((uint8_t)irq, action->data);
			pic_send_eoi((uint8_t)irq);
			return;
		}
	}

	early_log_puts("unexpected irq=");
	early_log_u64_decimal(irq);
	early_log_puts("\n");
	pic_send_eoi((uint8_t)irq);
}

/**
 * arch_timer_init() - Bring up the first x86 periodic timer source.
 *
 * This initializes the legacy PIC/PIT path, registers IRQ0, then enables local
 * interrupts. Later APIC or HPET support should replace this backend without
 * changing scheduler policy.
 */
void arch_timer_init(void)
{
	pic_remap();
	pic_mask_initial_irqs();
	if (irq_register(IRQ_TIMER, timer_irq_handler, 0) != 0) {
		panic("timer irq registration failed");
	}
	pit_init();
	early_log_puts("timer initialized\n");
	arch_irq_restore(1ull << 9);
}
