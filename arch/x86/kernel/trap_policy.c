#include <stdint.h>

#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>

#include "cpu.h"
#include "trap_policy.h"

enum x86_trap_origin x86_trap_origin_from_cs(uint64_t cs)
{
	if ((cs & X86_SELECTOR_RPL_MASK) == X86_RING3_RPL) {
		return X86_TRAP_FROM_USER;
	}

	return X86_TRAP_FROM_KERNEL;
}

const char *x86_trap_origin_name(enum x86_trap_origin origin)
{
	return origin == X86_TRAP_FROM_USER ? "user" : "kernel";
}

void x86_trap_exit(struct trap_frame *frame,
	enum x86_trap_origin origin,
	enum x86_trap_exit_reason reason)
{
	(void)origin;

	switch (reason) {
	case X86_TRAP_EXIT_IRQ:
		sched_irq_exit(frame);
		break;
	case X86_TRAP_EXIT_SYSCALL:
	case X86_TRAP_EXIT_USER_EXCEPTION:
		break;
	}
}

void x86_unhandled_user_exception(
	struct trap_frame *frame, enum x86_trap_origin origin, const char *name)
{
	x86_trap_exit(frame, origin, X86_TRAP_EXIT_USER_EXCEPTION);
	pr_err("user exception: %s\n", name != 0 ? name : "unknown");
	pr_err("user exception policy is not implemented\n");
	panic("unhandled user CPU exception");
}

void x86_unhandled_kernel_exception(
	struct trap_frame *frame, enum x86_trap_origin origin)
{
	(void)frame;
	(void)origin;

	panic("unhandled CPU exception");
}
