#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/spinlock.h>

static int spinlock_depth;

/**
 * spinlock_held_count() - Return current CPU spinlock nesting depth.
 *
 * The early kernel is single-CPU, so a global counter is enough to catch
 * accidental calls into blocking scheduler paths while an irq-safe spinlock
 * is held. SMP support will need to move this state to per-CPU storage.
 */
int spinlock_held_count(void)
{
	return spinlock_depth;
}

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
	spinlock_depth++;
	*flags = saved_flags;
}

void spin_unlock_irqrestore(struct spinlock *lock, uint64_t flags)
{
	if (lock == 0 || lock->locked == 0) {
		panic("invalid spinlock release");
	}

	if (spinlock_depth <= 0) {
		panic("spinlock depth underflow");
	}

	spinlock_depth--;
	lock->locked = 0;
	arch_irq_restore(flags);
}
