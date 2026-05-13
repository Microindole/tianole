#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/sched.h>

#include "cpu.h"

typedef int (*exception_handler_t)(struct trap_frame *frame);

/**
 * struct exception_desc - C-level policy for one CPU exception vector.
 * @name: Diagnostic name printed before exception-specific handling.
 * @handler: Optional exception-specific handler. Return non-zero if the
 *           exception was handled and execution may resume.
 * @has_error: Whether hardware pushes an error code for this vector.
 *
 * Linux uses generated entry wrappers and DEFINE_IDTENTRY-style declarations to
 * keep entry metadata in one place. Tianole keeps the same direction with a
 * compact descriptor table shared with IDT setup.
 */
struct exception_desc {
	const char *name;
	exception_handler_t handler;
	uint8_t has_error;
};

static int x86_handle_default_exception(struct trap_frame *frame);
static int x86_handle_page_fault(struct trap_frame *frame);

static const struct exception_desc exception_descs[32] = {
#define DEFINE_EXCEPTION_DESC(vector, entry, has_error, name, handler)         \
	[vector] = {name, handler, has_error},
	X86_EXCEPTION_VECTORS(DEFINE_EXCEPTION_DESC)
#undef DEFINE_EXCEPTION_DESC
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

static const struct exception_desc *exception_desc(uint64_t vector)
{
	if (vector >= 32) {
		return 0;
	}

	return &exception_descs[vector];
}

static int x86_handle_default_exception(struct trap_frame *frame)
{
	(void)frame;

	return 0;
}

static int x86_handle_page_fault(struct trap_frame *frame)
{
	handle_page_fault(frame);
	return 0;
}

static void print_exception_frame(
	const struct trap_frame *frame, const struct exception_desc *desc)
{
	const char *name = "unknown exception";

	if (desc != 0 && desc->name != 0) {
		name = desc->name;
	}

	early_log_puts("exception: ");
	early_log_puts(name);
	early_log_puts("\n");
	early_log_puts("vector=");
	early_log_u64_decimal(frame->vector);
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
 * the common IRQ-exit boundary. CPU exceptions are described by a vector table;
 * each vector can grow its own policy without adding ad hoc checks here.
 */
void trap_dispatch(struct trap_frame *frame)
{
	const struct exception_desc *desc;

	if (frame->vector >= 32 && frame->vector < 48) {
		sched_irq_enter();
		handle_irq(frame);
		sched_irq_exit();
		return;
	}

	desc = exception_desc(frame->vector);
	print_exception_frame(frame, desc);

	if (desc != 0 && desc->handler != 0 && desc->handler(frame) != 0) {
		return;
	}

	panic("unhandled CPU exception");
}
