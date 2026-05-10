#include <stdint.h>

#include <arch/switch.h>

#include <tianole/early_log.h>
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
		if (thread->state == THREAD_READY && thread != idle_thread) {
			return thread;
		}
		thread = thread->next;
	}

	for (thread = run_queue_head; thread != start; thread = thread->next) {
		if (thread->state == THREAD_READY && thread != idle_thread) {
			return thread;
		}
	}

	if (idle_thread != 0 && idle_thread->state == THREAD_READY) {
		return idle_thread;
	}

	return 0;
}

static void wake_sleeping_threads(uint64_t tick)
{
	struct thread *thread;

	for (thread = run_queue_head; thread != 0; thread = thread->next) {
		if (thread->state == THREAD_SLEEPING &&
			thread->wake_tick <= tick) {
			thread->wake_tick = 0;
			thread->state = THREAD_READY;
		}
	}
}

void sched_yield(void)
{
	struct thread *prev;
	struct thread *next;

	if (schedule_locked != 0) {
		return;
	}

	sched_reap_dead_threads();

	prev = current_thread;
	next = next_runnable_thread();

	if (next == 0 || next == prev) {
		return;
	}

	schedule_locked = 1;

	if (prev != 0 && prev->state == THREAD_RUNNING) {
		prev->state = THREAD_READY;
	}

	next->state = THREAD_RUNNING;
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

	if (current_thread != 0 && current_thread->state == THREAD_RUNNING) {
		need_resched = 1;
	}
}

void sched_irq_exit(void)
{
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

	now = timer_ticks();
	current_thread->wake_tick = now + ticks;
	current_thread->state = THREAD_SLEEPING;
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

	early_log_puts("scheduler initialized\n");
	sched_selftest();
}

void sched_start(void)
{
	if (sched_idle_create() != 0) {
		panic("idle thread creation failed");
	}

	sched_demo_start();
}
