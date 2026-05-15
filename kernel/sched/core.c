#include <stdint.h>

#include <arch/switch.h>

#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/timer.h>

#include "sched.h"

struct thread *run_queue_head;
struct thread *run_queue_tail;
struct thread *current_thread;
uintptr_t boot_stack_pointer;
uint64_t next_thread_id = 1;
int scheduler_ready;
int schedule_locked;
int need_resched;
int irq_depth;
struct thread *idle_thread;
struct spinlock scheduler_lock = SPINLOCK_INITIALIZER;

void enqueue_thread(struct thread *thread)
{
	thread->next = 0;

	if (run_queue_tail != 0) {
		run_queue_tail->next = thread;
	} else {
		run_queue_head = thread;
	}

	run_queue_tail = thread;
}

static struct thread *next_runnable_thread(void)
{
	struct thread *start;
	struct thread *thread;

	if (current_thread == 0 || current_thread->next == 0) {
		start = run_queue_head;
	} else {
		start = current_thread->next;
	}

	thread = start;
	while (thread != 0) {
		if (thread_is_ready(thread) && thread != idle_thread) {
			return thread;
		}
		thread = thread->next;
	}

	for (thread = run_queue_head; thread != start; thread = thread->next) {
		if (thread_is_ready(thread) && thread != idle_thread) {
			return thread;
		}
	}

	if (thread_is_ready(idle_thread)) {
		return idle_thread;
	}

	return 0;
}

static void wake_sleeping_threads(uint64_t tick)
{
	struct thread *thread;

	for (thread = run_queue_head; thread != 0; thread = thread->next) {
		if (thread_is_sleeping(thread) && thread->wake_tick <= tick) {
			thread_set_ready(thread);
		}
	}
}

void sched_yield(void)
{
	struct thread *prev;
	struct thread *next;

	sched_assert_can_switch();

	sched_reap_dead_threads();

	prev = current_thread;
	next = next_runnable_thread();

	if (next == 0 || next == prev) {
		return;
	}

	schedule_locked = 1;

	if (thread_is_running(prev)) {
		thread_set_ready(prev);
	}

	thread_set_running(next);
	current_thread = next;
	schedule_locked = 0;

	if (prev == 0) {
		arch_context_switch(&boot_stack_pointer, next->stack_pointer);
		return;
	}

	arch_context_switch(&prev->stack_pointer, next->stack_pointer);
}

void sched_tick(uint64_t tick)
{
	wake_sleeping_threads(tick);

	if (thread_is_running(current_thread)) {
		need_resched = 1;
	}
}

/**
 * sched_irq_enter() - Enter external IRQ context.
 *
 * This is the scheduler side of the trap boundary. Handlers may request a
 * reschedule, but normal blocking paths remain forbidden until the matching
 * outermost sched_irq_exit() has left IRQ context.
 */
void sched_irq_enter(void)
{
	irq_depth++;
}

/**
 * sched_irq_exit() - Leave external IRQ context and run pending reschedule.
 *
 * Only the outermost IRQ exit may consume need_resched. The IRQ nesting count
 * is dropped before switching so the next thread runs in normal thread
 * context, while direct scheduling from inside an IRQ handler still trips the
 * scheduler context assertion.
 */
void sched_irq_exit(void)
{
	if (irq_depth <= 0) {
		panic("scheduler irq exit without irq entry");
	}

	irq_depth--;
	if (irq_depth != 0) {
		return;
	}

	if (need_resched == 0 || current_thread == 0 || schedule_locked != 0) {
		return;
	}

	need_resched = 0;
	sched_yield();
}

void sched_sleep(uint64_t ticks)
{
	uint64_t now;

	if (current_thread == 0 || ticks == 0) {
		return;
	}

	sched_assert_can_switch();

	now = timer_ticks();
	thread_set_sleeping(current_thread, now + ticks);
	sched_yield();
}

void sched_init(void)
{
	if (scheduler_ready != 0) {
		return;
	}

	run_queue_head = 0;
	run_queue_tail = 0;
	idle_thread = 0;
	scheduler_ready = 1;

	pr_info("scheduler initialized\n");
	sched_selftest();
}

void sched_start(void)
{
	if (sched_idle_create() != 0) {
		panic("idle thread creation failed");
	}

	sched_demo_start();
}
