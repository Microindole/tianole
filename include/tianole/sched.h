#ifndef TIANOLE_SCHED_H
#define TIANOLE_SCHED_H

#include <stddef.h>
#include <stdint.h>

#include <tianole/spinlock.h>

/**
 * typedef kernel_thread_entry_t - Kernel thread entry function.
 * @arg: Opaque argument supplied at thread creation.
 *
 * The function runs in kernel thread context. Returning from it is equivalent
 * to calling kernel_thread_exit().
 */
typedef void (*kernel_thread_entry_t)(void *arg);

/**
 * typedef wait_condition_t - Wait queue condition predicate.
 * @arg: Opaque predicate argument supplied by the caller.
 *
 * Return: Non-zero when the condition is satisfied, zero to keep waiting.
 */
typedef int (*wait_condition_t)(void *arg);

/**
 * enum thread_state - Scheduler-visible thread lifecycle state.
 * @THREAD_READY: Thread is runnable and may be selected by the scheduler.
 * @THREAD_RUNNING: Thread is currently executing on the CPU.
 * @THREAD_SLEEPING: Thread is blocked until a timer deadline.
 * @THREAD_WAITING: Thread is blocked on a wait queue.
 * @THREAD_ZOMBIE: Thread exited; resources are kept until safe reclamation.
 * @THREAD_DEAD: Thread is detached from scheduler queues and being released.
 */
enum thread_state {
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_SLEEPING,
	THREAD_WAITING,
	THREAD_ZOMBIE,
	THREAD_DEAD,
};

/**
 * struct wait_queue - Queue of threads blocked on an event or condition.
 * @lock: Interrupt-safe lock protecting queue links and wakeup state.
 * @head: Oldest waiting thread.
 * @tail: Newest waiting thread.
 *
 * The queue owns only wait links. A condition protected by this queue must be
 * updated under wait_queue_lock_irqsave(), followed by a locked wakeup before
 * releasing the lock; otherwise the caller must document its own lock rule.
 */
struct wait_queue {
	struct spinlock lock;
	struct thread *head;
	struct thread *tail;
};

/**
 * struct thread - Kernel scheduler thread object.
 * @id: Scheduler-assigned thread identifier.
 * @state: Current scheduler state.
 * @stack_pointer: Saved context stack pointer for context switching.
 * @entry: Thread entry function.
 * @arg: Entry function argument.
 * @stack_base: Allocated kernel stack base.
 * @stack_top: Aligned initial stack top.
 * @stack_size: Kernel stack size in bytes.
 * @wake_tick: Timer tick deadline for sleeping threads.
 * @next: Run queue link owned by the scheduler.
 * @wait_next: Wait queue link owned by wait queue code.
 * @wait_queue: Wait queue currently owning @wait_next, or NULL.
 * @name: Diagnostic thread name.
 *
 * This structure is public only as an early-stage compromise. Long term, most
 * fields should move behind scheduler-private helpers or an opaque handle.
 */
struct thread {
	uint64_t id;
	enum thread_state state;
	uintptr_t stack_pointer;
	kernel_thread_entry_t entry;
	void *arg;
	void *stack_base;
	uintptr_t stack_top;
	size_t stack_size;
	uint64_t wake_tick;
	struct thread *next;
	struct thread *wait_next;
	struct wait_queue *wait_queue;
	char name[32];
};

/**
 * sched_init() - Initialize scheduler core state.
 *
 * Creates scheduler data structures and runs scheduler selftests before
 * threads are started.
 */
void sched_init(void);

/**
 * kernel_thread_create() - Create a kernel thread.
 * @name: Human-readable thread name used by diagnostics.
 * @entry: Thread entry point.
 * @arg: Opaque argument passed to @entry.
 *
 * Return: Thread object on success, or NULL when allocation fails.
 */
struct thread *kernel_thread_create(
	const char *name, kernel_thread_entry_t entry, void *arg);

/**
 * kernel_thread_exit() - Terminate the current kernel thread.
 *
 * Marks the current thread as zombie and yields forever until the scheduler
 * switches away. Reclamation happens only after the thread is no longer
 * current.
 */
void kernel_thread_exit(void) __attribute__((noreturn));

/**
 * sched_start() - Start scheduler demo threads and enter scheduling.
 *
 * Creates the idle thread and current boot-stage scheduler demonstration.
 */
void sched_start(void) __attribute__((noreturn));

/**
 * sched_tick() - Notify the scheduler about a timer tick.
 * @tick: Current generic timer tick.
 *
 * Wakes sleeping threads whose deadlines expired and requests rescheduling.
 */
void sched_tick(uint64_t tick);

/**
 * sched_irq_exit() - Consume pending reschedule work after IRQ handling.
 *
 * Keeps timer IRQ handling short by moving the actual scheduling decision to
 * a common interrupt-exit boundary.
 */
void sched_irq_exit(void);

/**
 * sched_yield() - Yield the CPU to another runnable kernel thread.
 *
 * Performs cooperative scheduling and context switching when another runnable
 * thread is available.
 */
void sched_yield(void);

/**
 * sched_sleep() - Sleep the current thread for a number of ticks.
 * @ticks: Number of timer ticks to sleep.
 *
 * Marks the current thread sleeping and yields until the timer path wakes it.
 */
void sched_sleep(uint64_t ticks);

/**
 * wait_queue_init() - Initialize a wait queue.
 * @queue: Wait queue storage owned by the caller.
 *
 * Resets queue links and the internal interrupt-safe lock.
 */
void wait_queue_init(struct wait_queue *queue);

/**
 * wait_queue_lock_irqsave() - Lock a wait queue and save interrupt state.
 * @queue: Wait queue whose condition or waiter links will be updated.
 * @flags: Storage for the previous interrupt state.
 *
 * Callers may use this lock to protect the condition paired with
 * wait_queue_wait(). In that model, update the condition and call a locked
 * wakeup helper before unlocking to avoid lost wakeups.
 */
void wait_queue_lock_irqsave(struct wait_queue *queue, uint64_t *flags);

/**
 * wait_queue_unlock_irqrestore() - Unlock a wait queue and restore interrupts.
 * @queue: Wait queue previously locked by wait_queue_lock_irqsave().
 * @flags: Interrupt state returned by wait_queue_lock_irqsave().
 */
void wait_queue_unlock_irqrestore(struct wait_queue *queue, uint64_t flags);

/**
 * wait_queue_wake_one_locked() - Wake one waiter while holding queue lock.
 * @queue: Locked wait queue containing waiting threads.
 *
 * Use when the wakeup condition is modified under the wait queue lock.
 */
void wait_queue_wake_one_locked(struct wait_queue *queue);

/**
 * wait_queue_wake_all_locked() - Wake all waiters while holding queue lock.
 * @queue: Locked wait queue containing waiting threads.
 *
 * Use when the wakeup condition is modified under the wait queue lock.
 */
void wait_queue_wake_all_locked(struct wait_queue *queue);

/**
 * wait_queue_sleep() - Sleep until another thread wakes the queue.
 * @queue: Queue to sleep on.
 *
 * Enqueues the current thread, marks it waiting and yields until wakeup.
 */
void wait_queue_sleep(struct wait_queue *queue);

/**
 * wait_queue_wait() - Sleep until a condition becomes true.
 * @queue: Queue used for wakeups.
 * @condition: Predicate checked before and after sleeping.
 * @arg: Opaque predicate argument.
 *
 * The predicate is evaluated while the queue lock is held. If the waited-on
 * condition uses this queue for synchronization, writers must hold the same
 * lock, update the condition, and then call wait_queue_wake_*_locked().
 *
 * Return: 0 when the condition is true, -EINVAL on invalid input.
 */
int wait_queue_wait(
	struct wait_queue *queue, wait_condition_t condition, void *arg);

/**
 * wait_queue_wait_timeout() - Sleep until a condition or timeout.
 * @queue: Queue used for wakeups.
 * @condition: Predicate checked before and after sleeping.
 * @arg: Opaque predicate argument.
 * @ticks: Maximum number of timer ticks to wait.
 *
 * The predicate is evaluated while the queue lock is held. The same condition
 * locking rule as wait_queue_wait() applies.
 *
 * Return: 0 when the condition is true, -EINVAL or -ETIMEDOUT otherwise.
 */
int wait_queue_wait_timeout(struct wait_queue *queue,
	wait_condition_t condition,
	void *arg,
	uint64_t ticks);

/**
 * wait_queue_wake_one() - Wake one thread from a wait queue.
 * @queue: Queue containing waiting threads.
 *
 * Removes the oldest waiter and marks it ready if it is still waitable.
 */
void wait_queue_wake_one(struct wait_queue *queue);

/**
 * wait_queue_wake_all() - Wake every thread from a wait queue.
 * @queue: Queue containing waiting threads.
 *
 * Drains the queue and marks every waitable thread ready.
 */
void wait_queue_wake_all(struct wait_queue *queue);

#endif
