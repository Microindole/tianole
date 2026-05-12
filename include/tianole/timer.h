#ifndef TIANOLE_TIMER_H
#define TIANOLE_TIMER_H

#include <stdint.h>

/**
 * timer_tick() - Advance generic timer state by one hardware tick.
 *
 * Called by the architecture timer IRQ handler before scheduler tick handling.
 */
void timer_tick(void);

/**
 * timer_ticks() - Read the generic monotonic tick counter.
 *
 * Return: Number of timer ticks observed since timer initialization.
 */
uint64_t timer_ticks(void);

#endif
