#ifndef TIANOLE_EARLY_LOG_H
#define TIANOLE_EARLY_LOG_H

#include <stdint.h>

void early_log_init(void);
void early_log_putc(char ch);
void early_log_puts(const char *text);
void early_log_u64_decimal(uint64_t value);
void panic(const char *message) __attribute__((noreturn));

#endif
