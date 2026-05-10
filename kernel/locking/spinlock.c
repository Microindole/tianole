#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/spinlock.h>

void spin_lock_irqsave(struct spinlock *lock, uint64_t *flags)
{
	uint64_t saved_flags;

	if (lock == 0 || flags == 0) {
		panic("invalid spinlock acquire");
	}

	saved_flags = arch_irq_save();
	if (lock->locked != 0) {
		panic("spinlock recursion or contention");
	}

	lock->locked = 1;
	*flags = saved_flags;
}

void spin_unlock_irqrestore(struct spinlock *lock, uint64_t flags)
{
	if (lock == 0 || lock->locked == 0) {
		panic("invalid spinlock release");
	}

	lock->locked = 0;
	arch_irq_restore(flags);
}
