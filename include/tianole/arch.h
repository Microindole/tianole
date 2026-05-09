#ifndef TIANOLE_ARCH_H
#define TIANOLE_ARCH_H

#include <stdint.h>

void arch_early_log_init(void);
void arch_early_log_putc(char ch);
void arch_halt_forever(void) __attribute__((noreturn));
int arch_page_table_uses_page(uint64_t page);
void arch_traps_init(void);

#endif
