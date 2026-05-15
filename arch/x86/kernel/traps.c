#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>

#include "cpu.h"
#include "trap_policy.h"

struct exception_desc;

typedef int (*exception_handler_t)(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);

/**
 * struct exception_desc - C-level policy for one CPU exception vector.
 * @name: Diagnostic name printed before exception-specific handling.
 * @handler: Optional exception-specific handler. Return non-zero if the
 *           exception was handled and execution may resume.
 * @has_error: Whether hardware pushes an error code for this vector.
 * @gate_type: IDT gate type used for this vector.
 * @dpl: Descriptor privilege level installed in the IDT.
 * @ist: Interrupt stack table index used by hardware on entry.
 *
 * Linux uses generated entry wrappers and DEFINE_IDTENTRY-style declarations to
 * keep entry metadata in one place. Tianole keeps the same direction with a
 * compact descriptor table shared with IDT setup.
 */
struct exception_desc {
	const char *name;
	exception_handler_t handler;
	uint8_t has_error;
	uint8_t gate_type;
	uint8_t dpl;
	uint8_t ist;
};

static int x86_handle_default_exception(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);
static int x86_handle_double_fault(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);
static int x86_handle_general_protection(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);
static int x86_handle_invalid_opcode(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);
static int x86_handle_page_fault(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc);

static const struct exception_desc exception_descs[32] = {
#define DEFINE_EXCEPTION_DESC(                                                 \
	vector, entry, has_error, gate_type, dpl, ist, name, handler)          \
	[vector] = {name, handler, has_error, gate_type, dpl, ist},
	X86_EXCEPTION_VECTORS(DEFINE_EXCEPTION_DESC)
#undef DEFINE_EXCEPTION_DESC
};

/**
 * interrupted_rsp() - Derive the interrupted stack pointer for diagnostics.
 * @frame: Trap frame built by the assembly entry path.
 * @origin: Trap origin derived from the interrupted CS selector.
 *
 * On a ring transition x86 pushes the interrupted RSP/SS pair into the IRET
 * frame. Current kernel-mode same-ring traps do not have that hardware pair,
 * so diagnostics keep the old derived value for kernel traps.
 */
static uint64_t interrupted_rsp(
	const struct trap_frame *frame, enum x86_trap_origin origin)
{
	if (origin == X86_TRAP_FROM_USER) {
		return frame->rsp;
	}

	return (uint64_t)(uintptr_t)&frame->rflags + sizeof(frame->rflags);
}

static const struct exception_desc *exception_desc(uint64_t vector)
{
	if (x86_vector_class(vector) != X86_VECTOR_EXCEPTION) {
		return 0;
	}

	return &exception_descs[vector];
}

/**
 * trap_origin() - Classify whether a trap interrupted kernel or user mode.
 * @frame: Trap frame built by the entry path.
 *
 * x86 stores the interrupted code selector in the frame. Its RPL bits are the
 * first boundary future user-mode exception policy needs: ring 0 faults remain
 * kernel faults, while ring 3 faults should eventually be delivered to or kill
 * the current task instead of panicking the whole kernel.
 */
static enum x86_trap_origin trap_origin(const struct trap_frame *frame)
{
	return x86_trap_origin_from_cs(frame->cs);
}

/**
 * x86_handle_default_exception() - Leave an exception to common policy.
 * @frame: Register snapshot from the assembly entry path.
 * @origin: Whether the interrupted context was kernel or future user mode.
 * @desc: Metadata for this exception vector.
 *
 * Vectors that do not yet need special handling return unhandled here. The
 * common dispatch code then applies either kernel panic policy or the future
 * user-mode delivery policy.
 */
static int x86_handle_default_exception(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc)
{
	(void)frame;
	(void)origin;
	(void)desc;

	return 0;
}

/**
 * x86_handle_double_fault() - Handle a fatal double fault.
 * @frame: Register snapshot from the trap entry.
 * @origin: Interrupted privilege level.
 * @desc: Metadata for vector 8.
 *
 * Double fault stays fatal regardless of origin. The IDT metadata already
 * selects the dedicated IST stack, and this handler records that fact before
 * stopping the kernel.
 */
static int x86_handle_double_fault(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc)
{
	(void)frame;
	(void)origin;
	(void)desc;

	pr_emerg("double fault: fatal exception\n");
	pr_emerg("double fault: ist=%u\n", X86_IST_DOUBLE_FAULT);
	panic("double fault");
}

/**
 * x86_handle_general_protection() - Handle #GP policy.
 * @frame: Register snapshot from the trap entry.
 * @origin: Interrupted privilege level.
 * @desc: Metadata for vector 13.
 *
 * User-origin #GP is routed to the future user exception policy. Kernel-origin
 * #GP remains fatal until fixup/oops support exists.
 */
static int x86_handle_general_protection(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc)
{
	if (origin == X86_TRAP_FROM_USER) {
		x86_unhandled_user_exception(frame, origin, desc->name);
	}

	pr_emerg("general protection: fatal exception\n");
	pr_emerg("general protection: error=0x%016llx\n",
		(unsigned long long)frame->error_code);
	panic("general protection fault");
}

/**
 * x86_handle_invalid_opcode() - Handle #UD policy.
 * @frame: Register snapshot from the trap entry.
 * @origin: Interrupted privilege level.
 * @desc: Metadata for vector 6.
 *
 * The split mirrors Linux's user-vs-kernel trap policy. User-origin #UD will
 * later become process delivery; kernel-origin #UD is fatal today.
 */
static int x86_handle_invalid_opcode(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc)
{
	(void)frame;

	if (origin == X86_TRAP_FROM_USER) {
		x86_unhandled_user_exception(frame, origin, desc->name);
	}

	pr_emerg("invalid opcode: fatal exception\n");
	panic("invalid opcode");
}

/**
 * x86_handle_page_fault() - Route #PF to the architecture MM fault reporter.
 * @frame: Register snapshot from the trap entry.
 * @origin: Interrupted privilege level.
 * @desc: Metadata for vector 14.
 *
 * The MM fault path decodes CR2 and access bits. If it cannot recover, common
 * dispatch applies kernel or future user-mode unhandled policy.
 */
static int x86_handle_page_fault(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const struct exception_desc *desc)
{
	(void)origin;
	(void)desc;

	handle_page_fault(frame);
	return 0;
}

static void print_exception_frame(
	const struct trap_frame *frame, const struct exception_desc *desc)
{
	const char *name = "unknown exception";
	enum x86_trap_origin origin = trap_origin(frame);

	if (desc != 0 && desc->name != 0) {
		name = desc->name;
	}

	pr_err("exception: %s\n", name);
	pr_err("vector=%llu error=0x%016llx\n",
		(unsigned long long)frame->vector,
		(unsigned long long)frame->error_code);
	pr_err("rip=0x%016llx rsp=0x%016llx rflags=0x%016llx\n",
		(unsigned long long)frame->rip,
		(unsigned long long)interrupted_rsp(frame, origin),
		(unsigned long long)frame->rflags);
	pr_err("mode=%s", x86_trap_origin_name(origin));
	if (origin == X86_TRAP_FROM_USER) {
		pr_err(" ss=0x%016llx", (unsigned long long)frame->ss);
	}
	pr_err("\n");
}

/**
 * arch_traps_init() - Initialize x86 descriptor tables for traps and IRQs.
 */
void arch_traps_init(void)
{
	gdt_init();
	idt_init();
	pr_info("traps initialized\n");
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
	enum x86_trap_origin origin;

	switch (x86_vector_class(frame->vector)) {
	case X86_VECTOR_LEGACY_IRQ:
		sched_irq_enter();
		handle_irq(frame);
		x86_trap_exit(frame, trap_origin(frame), X86_TRAP_EXIT_IRQ);
		return;
	case X86_VECTOR_EXCEPTION:
		break;
	case X86_VECTOR_SYSCALL:
		pr_err("unexpected syscall vector=%llu\n",
			(unsigned long long)frame->vector);
		x86_trap_exit(frame, trap_origin(frame), X86_TRAP_EXIT_SYSCALL);
		panic("unhandled syscall vector");
	case X86_VECTOR_EXTERNAL_IRQ:
	case X86_VECTOR_SYSTEM:
	case X86_VECTOR_RESERVED:
		pr_err("unexpected vector=%llu\n",
			(unsigned long long)frame->vector);
		panic("unhandled CPU vector");
	}

	desc = exception_desc(frame->vector);
	origin = trap_origin(frame);
	print_exception_frame(frame, desc);

	if (desc != 0 && desc->handler != 0 &&
		desc->handler(frame, origin, desc) != 0) {
		return;
	}

	if (origin == X86_TRAP_FROM_USER) {
		x86_unhandled_user_exception(frame,
			origin,
			desc != 0 && desc->name != 0 ? desc->name : 0);
	}

	x86_unhandled_kernel_exception(frame, origin);
}

/**
 * arch_test_double_fault() - Exercise the double fault dispatch policy.
 *
 * This does not manufacture a real hardware double fault. It builds a minimal
 * vector-8 trap frame and routes it through the same C dispatch path so checks
 * can verify the dedicated fatal handler without corrupting the current stack.
 */
void arch_test_double_fault(void)
{
	static struct trap_frame frame;

	frame.vector = 8;
	frame.error_code = 0;
	frame.rip = (uint64_t)(uintptr_t)arch_test_double_fault;
	frame.cs = KERNEL_CODE_SELECTOR;
	frame.rflags = 1ull << 9;
	frame.rsp = 0;
	frame.ss = 0;

	trap_dispatch(&frame);
}

/**
 * arch_test_general_protection() - Exercise the #GP dispatch policy.
 *
 * This test path mirrors the double fault check: it validates that vector 13
 * reaches its dedicated policy without relying on undefined setup tricks to
 * force a hardware general-protection exception this early in boot.
 */
void arch_test_general_protection(void)
{
	static struct trap_frame frame;

	frame.vector = 13;
	frame.error_code = 0;
	frame.rip = (uint64_t)(uintptr_t)arch_test_general_protection;
	frame.cs = KERNEL_CODE_SELECTOR;
	frame.rflags = 1ull << 9;
	frame.rsp = 0;
	frame.ss = 0;

	trap_dispatch(&frame);
}

/**
 * arch_test_user_invalid_opcode() - Exercise the future user fault boundary.
 *
 * The kernel does not enter ring 3 yet. This builds a controlled vector-6 frame
 * with RPL=3 in CS so the C policy path can verify that user-origin exceptions
 * are separated from kernel-mode exception panic handling.
 */
void arch_test_user_invalid_opcode(void)
{
	static struct trap_frame frame;

	frame.vector = 6;
	frame.error_code = 0;
	frame.rip = (uint64_t)(uintptr_t)arch_test_user_invalid_opcode;
	frame.cs = KERNEL_CODE_SELECTOR | X86_RING3_RPL;
	frame.rflags = 1ull << 9;
	frame.rsp = 0x0000000000400000ull;
	frame.ss = USER_DATA_SELECTOR;

	trap_dispatch(&frame);
}
