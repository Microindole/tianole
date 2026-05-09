#include <stdint.h>

#include <arch/io.h>
#include <arch/traps.h>

#include <tianole/early_log.h>
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

static void enable_interrupts(void)
{
	__asm__ volatile("sti");
}

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

static void handle_timer_irq(void)
{
	timer_tick();
	pic_send_eoi(IRQ_TIMER);
}

void handle_irq(struct trap_frame *frame)
{
	uint64_t irq = frame->vector - IRQ_BASE;

	if (irq == IRQ_TIMER) {
		handle_timer_irq();
		return;
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
	pit_init();
	early_log_puts("timer initialized\n");
	enable_interrupts();
}
