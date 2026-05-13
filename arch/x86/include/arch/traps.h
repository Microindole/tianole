#ifndef ARCH_X86_TRAPS_H
#define ARCH_X86_TRAPS_H

#include <stdint.h>

/**
 * struct trap_frame - Register snapshot built by x86 exception/IRQ entry.
 * @rax: Saved general-purpose register.
 * @rbx: Saved general-purpose register.
 * @rcx: Saved general-purpose register.
 * @rdx: Saved general-purpose register.
 * @rsi: Saved general-purpose register.
 * @rdi: Saved general-purpose register.
 * @rbp: Saved frame pointer.
 * @r8: Saved general-purpose register.
 * @r9: Saved general-purpose register.
 * @r10: Saved general-purpose register.
 * @r11: Saved general-purpose register.
 * @r12: Saved general-purpose register.
 * @r13: Saved general-purpose register.
 * @r14: Saved general-purpose register.
 * @r15: Saved general-purpose register.
 * @vector: CPU exception vector or remapped external IRQ vector.
 * @error_code: Hardware or synthetic exception error code.
 * @rip: Interrupted instruction pointer.
 * @cs: Interrupted code segment selector.
 * @rflags: Interrupted RFLAGS value.
 *
 * The assembly entry code owns the exact push order. C trap handlers may read
 * this frame for diagnostics and dispatch, but must not assume it is a stable
 * user-visible ABI.
 */
struct trap_frame {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t vector;
	uint64_t error_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
};

/**
 * trap_dispatch() - Dispatch an x86 trap frame to the proper handler.
 * @frame: Register snapshot from the assembly entry path.
 */
void trap_dispatch(struct trap_frame *frame);

/**
 * handle_irq() - Dispatch a remapped external IRQ vector.
 * @frame: Trap frame whose vector is in the external IRQ range.
 */
void handle_irq(struct trap_frame *frame);

/**
 * handle_page_fault() - Diagnose and handle an x86 page fault.
 * @frame: Trap frame for vector 14.
 */
void handle_page_fault(struct trap_frame *frame);

#endif
