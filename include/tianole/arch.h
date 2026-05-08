#ifndef TIANOLE_ARCH_H
#define TIANOLE_ARCH_H

void arch_early_log_init(void);
void arch_early_log_putc(char ch);
void arch_halt_forever(void) __attribute__((noreturn));

#endif
