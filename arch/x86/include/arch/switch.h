#ifndef ARCH_X86_SWITCH_H
#define ARCH_X86_SWITCH_H

#include <stdint.h>

/**
 * arch_context_switch() - Switch between two kernel thread stacks.
 * @prev_sp: Storage for the outgoing thread stack pointer.
 * @next_sp: Saved stack pointer of the incoming thread.
 *
 * The assembly backend preserves the callee-saved register set expected by the
 * scheduler ABI. Policy decisions stay in `kernel/sched/`; this function only
 * performs the x86 stack/register handoff.
 */
void arch_context_switch(uintptr_t *prev_sp, uintptr_t next_sp);

#endif
