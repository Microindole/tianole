#ifndef TIANOLE_EARLY_LOG_H
#define TIANOLE_EARLY_LOG_H

#include <stdint.h>

/**
 * early_log_init() - Initialize early logging backends.
 *
 * Sets up the architecture-provided early output path. This must be usable
 * before heap, scheduler, VFS or normal drivers exist.
 */
void early_log_init(void);

/**
 * early_log_putc() - Write one character to every early log backend.
 * @ch: Character to emit.
 *
 * This is the byte-level output path used by the string and number helpers.
 */
void early_log_putc(char ch);

/**
 * early_log_puts() - Write a NUL-terminated string to early logs.
 * @text: String to emit. A NULL pointer is ignored by callers only if they
 *        explicitly check before calling.
 *
 * The function does not allocate memory and is safe for early boot diagnostics.
 */
void early_log_puts(const char *text);

/**
 * early_log_u64_hex() - Write a 64-bit value in hexadecimal form.
 * @value: Value to print.
 *
 * Used by trap, memory and boot diagnostics where libc formatting is absent.
 */
void early_log_u64_hex(uint64_t value);

/**
 * early_log_u64_decimal() - Write a 64-bit value in decimal form.
 * @value: Value to print.
 *
 * Used for counters and boot statistics without depending on printf support.
 */
void early_log_u64_decimal(uint64_t value);

/**
 * panic() - Print a fatal error and stop the machine.
 * @message: Panic reason to include in the early log.
 *
 * This path must work even when normal kernel services are not initialized.
 */
void panic(const char *message) __attribute__((noreturn));

#endif
