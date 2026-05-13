#ifndef TIANOLE_MONITOR_H
#define TIANOLE_MONITOR_H

/**
 * monitor_init() - Start the early interactive kernel monitor.
 *
 * Creates a small command thread that consumes completed lines from the input
 * console. The monitor is intentionally tiny: it proves that keyboard input can
 * flow through IRQ, input, console line editing and a kernel consumer before a
 * real tty, VFS or userspace exists.
 */
void monitor_init(void);

#endif
