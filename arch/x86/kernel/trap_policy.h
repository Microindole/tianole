#ifndef ARCH_X86_KERNEL_TRAP_POLICY_H
#define ARCH_X86_KERNEL_TRAP_POLICY_H

#include <stdint.h>

enum x86_trap_origin {
	X86_TRAP_FROM_KERNEL,
	X86_TRAP_FROM_USER,
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
 * x86_unhandled_user_exception() - Stop on a future user-mode exception.
 * @name: Exception name from the vector descriptor, or NULL.
 *
 * This is a temporary fatal policy. Later user-mode work should replace this
 * with process fault delivery without changing kernel exception handling.
 */
void x86_unhandled_user_exception(const char *name);

/**
 * x86_unhandled_kernel_exception() - Stop on an unhandled kernel exception.
 *
 * Until fixup/oops support exists, kernel-mode exceptions are fatal.
 */
void x86_unhandled_kernel_exception(void);

#endif
