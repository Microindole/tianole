#ifndef TIANOLE_PANIC_H
#define TIANOLE_PANIC_H

/**
 * panic() - Print a fatal error and stop the machine.
 * @message: Panic reason to include in emergency diagnostics.
 *
 * This path must work even when normal kernel services are not initialized.
 */
void panic(const char *message) __attribute__((noreturn));

#endif
