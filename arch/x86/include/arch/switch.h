#ifndef ARCH_X86_SWITCH_H
#define ARCH_X86_SWITCH_H

#include <stdint.h>

void arch_context_switch(uintptr_t *prev_sp, uintptr_t next_sp);

#endif
