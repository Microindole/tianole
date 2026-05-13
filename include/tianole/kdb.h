#ifndef TIANOLE_KDB_H
#define TIANOLE_KDB_H

/**
 * kdb_init() - Start the early interactive kernel debugger shell.
 *
 * Creates a small command thread that consumes completed lines from the input
 * console. This is an early KDB-like debugging aid, not the future user-space
 * shell or tty implementation.
 */
void kdb_init(void);

#endif
