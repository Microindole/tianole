#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/sched.h>

#include "cpu.h"

static const char *const exception_names[32] = {
	"divide error",
	"debug",
	"non-maskable interrupt",
	"breakpoint",
	"overflow",
	"bound range exceeded",
	"invalid opcode",
	"device not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid tss",
	"segment not present",
	"stack segment fault",
	"general protection fault",
	"page fault",
	"reserved",
	"x87 floating-point exception",
	"alignment check",
	"machine check",
	"simd floating-point exception",
	"virtualization exception",
	"control protection exception",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"hypervisor injection exception",
	"vmm communication exception",
	"security exception",
	"reserved",
};

/**
 * interrupted_rsp() - Derive the interrupted stack pointer for diagnostics.
 * @frame: Trap frame built by the assembly entry path.
 *
 * The current same-ring entry path does not store RSP as a named field. Until
 * trap frames grow a stable saved-RSP slot, this reports the stack location
 * just beyond the saved RFLAGS value.
 */
static uint64_t interrupted_rsp(const struct trap_frame *frame)
{
	return (uint64_t)(uintptr_t)&frame->rflags + sizeof(frame->rflags);
}

/**
 * arch_traps_init() - Initialize x86 descriptor tables for traps and IRQs.
 */
void arch_traps_init(void)
{
	gdt_init();
	idt_init();
	early_log_puts("traps initialized\n");
}

/**
 * trap_dispatch() - Route x86 exceptions and external IRQs.
 * @frame: Register snapshot from the assembly trap entry.
 *
 * External IRQs are dispatched first and may request a scheduler decision at
 * the common IRQ-exit boundary. CPU exceptions remain diagnostic-only for now
 * and end in panic unless a specific handler consumes them.
 */
void trap_dispatch(struct trap_frame *frame)
{
	uint64_t vector = frame->vector;
	const char *name = "unknown exception";

	if (vector >= 32 && vector < 48) {
		sched_irq_enter();
		handle_irq(frame);
		sched_irq_exit();
		return;
	}

	if (vector < 32) {
		name = exception_names[vector];
	}

	early_log_puts("exception: ");
	early_log_puts(name);
	early_log_puts("\n");
	early_log_puts("vector=");
	early_log_u64_decimal(vector);
	early_log_puts(" error=");
	early_log_u64_hex(frame->error_code);
	early_log_puts("\n");
	early_log_puts("rip=");
	early_log_u64_hex(frame->rip);
	early_log_puts(" rsp=");
	early_log_u64_hex(interrupted_rsp(frame));
	early_log_puts(" rflags=");
	early_log_u64_hex(frame->rflags);
	early_log_puts("\n");

	if (vector == 14) {
		handle_page_fault(frame);
	}

	panic("unhandled CPU exception");
}
