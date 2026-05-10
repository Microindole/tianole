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

void spin_lock_irqsave(struct spinlock *lock, uint64_t *flags);
void spin_unlock_irqrestore(struct spinlock *lock, uint64_t flags);

#endif
