#ifndef TIANOLE_SPINLOCK_H
#define TIANOLE_SPINLOCK_H

#include <stdint.h>

struct spinlock {
	int locked;
};

#define SPINLOCK_INITIALIZER                                                   \
	{                                                                      \
		0                                                              \
	}

/**
 * spin_lock_irqsave() - Acquire a single-CPU interrupt-safe spinlock.
 * @lock: Lock to acquire.
 * @flags: Storage for the previous interrupt state.
 *
 * Disables interrupts before taking the lock so IRQ and thread context can
 * share early kernel data structures.
 */
void spin_lock_irqsave(struct spinlock *lock, uint64_t *flags);

/**
 * spin_unlock_irqrestore() - Release an interrupt-safe spinlock.
 * @lock: Lock to release.
 * @flags: Interrupt state returned by spin_lock_irqsave().
 *
 * Restores the caller's original interrupt state after releasing the lock.
 */
void spin_unlock_irqrestore(struct spinlock *lock, uint64_t flags);

#endif
