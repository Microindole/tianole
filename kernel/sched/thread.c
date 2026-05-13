#include <stddef.h>
#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/mm.h>
#include <tianole/sched.h>
#include <tianole/spinlock.h>

#include "sched.h"

#define KERNEL_STACK_SIZE (PAGE_SIZE * 4u)
#define STACK_ALIGNMENT 16u

static void thread_trampoline(void) __attribute__((noreturn));

/**
 * align_down_uintptr() - Round a pointer value down to an alignment.
 * @value: Pointer-sized value to align.
 * @alignment: Power-of-two alignment boundary.
 *
 * Return: @value rounded down to the nearest @alignment boundary.
 */
static uintptr_t align_down_uintptr(uintptr_t value, uintptr_t alignment)
{
	return value & ~(alignment - 1);
}

/**
 * copy_thread_name() - Copy a bounded diagnostic thread name.
 * @dest: Destination name buffer.
 * @dest_size: Size of @dest in bytes.
 * @src: Caller supplied name, or NULL for the default name.
 *
 * The scheduler keeps names only for early logging. This helper mirrors the
 * kernel pattern of avoiding unbounded string copies in low-level code.
 */
static void copy_thread_name(char *dest, size_t dest_size, const char *src)
{
	size_t index;

	if (dest_size == 0) {
		return;
	}

	if (src == 0) {
		src = "thread";
	}

	for (index = 0; index + 1 < dest_size && src[index] != '\0'; index++) {
		dest[index] = src[index];
	}

	dest[index] = '\0';
}

/**
 * prepare_initial_stack() - Build the first saved context for a new thread.
 * @stack_top: Aligned top of the allocated kernel stack.
 *
 * The architecture switch code restores callee-saved registers and returns
 * through the saved return address, so the initial frame points at
 * thread_trampoline().
 *
 * Return: Stack pointer to store in struct thread::stack_pointer.
 */
static uintptr_t prepare_initial_stack(uintptr_t stack_top)
{
	uintptr_t *stack = (uintptr_t *)stack_top;

	*--stack = 0;
	*--stack = (uintptr_t)thread_trampoline;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;

	return (uintptr_t)stack;
}

struct thread *kernel_thread_create(
	const char *name, kernel_thread_entry_t entry, void *arg)
{
	struct thread *thread;
	uintptr_t stack_top;
	uint64_t flags;

	if (entry == 0) {
		return 0;
	}

	thread = kmalloc(sizeof(*thread));
	if (thread == 0) {
		return 0;
	}

	thread->stack_base = kmalloc(KERNEL_STACK_SIZE);
	if (thread->stack_base == 0) {
		kfree(thread);
		return 0;
	}

	stack_top = (uintptr_t)thread->stack_base + KERNEL_STACK_SIZE;

	thread_init_ready(thread);
	thread->entry = entry;
	thread->arg = arg;
	thread->stack_top = align_down_uintptr(stack_top, STACK_ALIGNMENT);
	thread->stack_pointer = prepare_initial_stack(thread->stack_top);
	thread->stack_size = KERNEL_STACK_SIZE;
	thread->wake_tick = 0;
	thread->next = 0;
	thread->wait_next = 0;
	thread->wait_queue = 0;
	copy_thread_name(thread->name, sizeof(thread->name), name);

	spin_lock_irqsave(&scheduler_lock, &flags);
	thread->id = next_thread_id++;
	enqueue_thread(thread);
	spin_unlock_irqrestore(&scheduler_lock, flags);

	return thread;
}

/**
 * release_thread() - Free a detached thread object and its kernel stack.
 * @thread: Thread that has already moved through THREAD_DEAD.
 *
 * A running thread cannot free its own stack. The scheduler first marks an
 * exited task ZOMBIE, switches away, then reaps it here after it is no longer
 * current and has been unlinked from scheduler queues.
 */
static void release_thread(struct thread *thread)
{
	if (thread == 0 || thread == current_thread) {
		panic("invalid thread release target");
	}

	if (!thread_is_dead(thread)) {
		panic("reaping thread before DEAD state");
	}

	if (thread->wait_queue != 0 || thread->wait_next != 0) {
		panic("reaping thread still on wait queue");
	}

	if (thread->wake_tick != 0) {
		panic("reaping thread still has wake deadline");
	}

	if (thread->stack_base == 0 || thread->stack_size == 0) {
		panic("reaping thread without kernel stack");
	}

	early_log_puts("thread reaped ");
	early_log_puts(thread->name);
	early_log_puts("\n");
	kfree(thread->stack_base);
	kfree(thread);
}

/**
 * sched_reap_dead_threads() - Reclaim exited threads at a safe boundary.
 *
 * Walks the run queue under the scheduler lock, detaches non-current zombie
 * threads, transitions them to DEAD, then frees memory after dropping the
 * lock. This keeps queue mutation serialized while avoiding allocator work
 * inside the scheduler critical section.
 */
void sched_reap_dead_threads(void)
{
	struct thread *prev = 0;
	struct thread *thread = run_queue_head;
	struct thread *reap_list = 0;
	uint64_t flags;

	spin_lock_irqsave(&scheduler_lock, &flags);
	while (thread != 0) {
		struct thread *next = thread->next;

		if (thread_is_zombie(thread) && thread != current_thread) {
			thread_set_dead(thread);
			if (prev != 0) {
				prev->next = next;
			} else {
				run_queue_head = next;
			}

			if (run_queue_tail == thread) {
				run_queue_tail = prev;
			}

			thread->next = reap_list;
			reap_list = thread;
		} else {
			prev = thread;
		}

		thread = next;
	}
	spin_unlock_irqrestore(&scheduler_lock, flags);

	while (reap_list != 0) {
		struct thread *next = reap_list->next;

		reap_list->next = 0;
		release_thread(reap_list);
		reap_list = next;
	}
}

/**
 * sched_thread_exit() - Terminate the current thread without freeing its stack.
 *
 * The current thread becomes ZOMBIE and yields forever. A later scheduler pass
 * observes that it is no longer current, turns it DEAD, and releases storage.
 */
void sched_thread_exit(void)
{
	if (current_thread == 0) {
		panic("thread exit without current thread");
	}

	if (thread_is_zombie(current_thread) ||
		thread_is_dead(current_thread)) {
		panic("thread exit entered twice");
	}

	thread_set_zombie(current_thread);

	for (;;) {
		sched_yield();
	}
}

void kernel_thread_exit(void)
{
	sched_thread_exit();
}

/**
 * thread_trampoline() - Enter a kernel thread and normalize return-to-exit.
 *
 * New contexts start here after the first architecture switch. If the entry
 * function returns, the trampoline routes it through kernel_thread_exit() so
 * both explicit and implicit exits use the same lifecycle.
 */
static void thread_trampoline(void)
{
	struct thread *thread = current_thread;

	if (thread == 0 || thread->entry == 0) {
		panic("kernel thread entered without entry");
	}

	thread->entry(thread->arg);
	kernel_thread_exit();
}
