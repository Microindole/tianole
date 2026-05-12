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
#define IRQ_COUNT 16u

struct irq_action {
	irq_handler_t handler;
	void *data;
};

static struct irq_action irq_actions[IRQ_COUNT];

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

static void pic_mask_all_except_timer(void)
{
	outb(PIC1_DATA, 0xfe);
	outb(PIC2_DATA, 0xff);
}

static void pic_send_eoi(uint8_t irq)
{
	if (irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}

	outb(PIC1_COMMAND, PIC_EOI);
}

static void pit_init(void)
{
	uint16_t divisor = (uint16_t)(PIT_FREQUENCY / PIT_TARGET_HZ);

	outb(PIT_COMMAND, PIT_MODE_RATE_GENERATOR);
	outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xff));
	outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

uint64_t arch_irq_save(void)
{
	uint64_t flags;

	__asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

void arch_irq_restore(uint64_t flags)
{
	if ((flags & (1ull << 9)) != 0) {
		__asm__ volatile("sti" : : : "memory");
	}
}

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

void arch_timer_init(void)
{
	pic_remap();
	pic_mask_all_except_timer();
	if (irq_register(IRQ_TIMER, timer_irq_handler, 0) != 0) {
		panic("timer irq registration failed");
	}
	pit_init();
	early_log_puts("timer initialized\n");
	arch_irq_restore(1ull << 9);
}
