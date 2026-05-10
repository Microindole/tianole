#ifndef TIANOLE_ARCH_H
#define TIANOLE_ARCH_H

#include <stdint.h>

void arch_early_log_init(void);
void arch_early_log_putc(char ch);
void arch_halt_forever(void) __attribute__((noreturn));
uint64_t arch_irq_save(void);
void arch_irq_restore(uint64_t flags);
int arch_page_table_uses_page(uint64_t page);
void arch_traps_init(void);
void arch_timer_init(void);

#endif
