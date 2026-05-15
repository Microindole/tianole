#ifndef ARCH_X86_KERNEL_TRAP_POLICY_H
#define ARCH_X86_KERNEL_TRAP_POLICY_H

#include <stdint.h>

struct trap_frame;

enum x86_trap_origin {
	X86_TRAP_FROM_KERNEL,
	X86_TRAP_FROM_USER,
};

enum x86_trap_exit_reason {
	X86_TRAP_EXIT_IRQ,
	X86_TRAP_EXIT_SYSCALL,
	X86_TRAP_EXIT_USER_EXCEPTION,
};

/**
 * x86_trap_origin_from_cs() - Classify an interrupted x86 code selector.
 * @cs: Code selector saved in the trap frame.
 *
 * The low selector bits carry the requestor privilege level. This is the
 * boundary future user-mode exception delivery needs before full task and
 * signal machinery exists.
 */
enum x86_trap_origin x86_trap_origin_from_cs(uint64_t cs);

/**
 * x86_trap_origin_name() - Return a diagnostic name for a trap origin.
 * @origin: Origin classification from x86_trap_origin_from_cs().
 */
const char *x86_trap_origin_name(enum x86_trap_origin origin);

/**
 * x86_trap_exit() - Process pending work before returning from a trap.
 * @frame: Trap frame that would be restored on exit.
 * @origin: Interrupted context privilege.
 * @reason: Exit path being prepared.
 *
 * IRQ, syscall and user-exception return paths should meet here before the
 * assembly entry code restores registers. Today only IRQ exit consumes pending
 * reschedule work; syscall/user-exception reasons reserve the shared boundary
 * for future user-mode return handling.
 */
void x86_trap_exit(struct trap_frame *frame,
	enum x86_trap_origin origin,
	enum x86_trap_exit_reason reason);

/**
 * x86_unhandled_user_exception() - Stop on a future user-mode exception.
 * @frame: Register state from the trap entry.
 * @origin: Trap origin, expected to be X86_TRAP_FROM_USER.
 * @name: Exception name from the vector descriptor, or NULL.
 *
 * This is a temporary fatal policy. Later user-mode work should replace this
 * with process fault delivery without changing kernel exception handling.
 */
void x86_unhandled_user_exception(struct trap_frame *frame,
	enum x86_trap_origin origin,
	const char *name);

/**
 * x86_unhandled_kernel_exception() - Stop on an unhandled kernel exception.
 * @frame: Register state from the trap entry.
 * @origin: Trap origin, expected to be X86_TRAP_FROM_KERNEL.
 *
 * Until fixup/oops support exists, kernel-mode exceptions are fatal.
 */
void x86_unhandled_kernel_exception(
	struct trap_frame *frame, enum x86_trap_origin origin);

#endif
