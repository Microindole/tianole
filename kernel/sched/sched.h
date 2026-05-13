#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/sched.h>
#include <tianole/spinlock.h>

extern struct thread *run_queue_head;
extern struct thread *run_queue_tail;
extern struct thread *current_thread;
extern uintptr_t boot_stack_pointer;
extern uint64_t next_thread_id;
extern int scheduler_ready;
extern int schedule_locked;
extern int need_resched;
extern struct thread *idle_thread;
extern struct spinlock scheduler_lock;

static inline int thread_is_ready(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_READY;
}

static inline int thread_is_running(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_RUNNING;
}

static inline int thread_is_sleeping(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_SLEEPING;
}

static inline int thread_is_waiting(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_WAITING;
}

static inline int thread_is_zombie(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_ZOMBIE;
}

static inline int thread_is_dead(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_DEAD;
}

static inline int thread_state_transition_is_valid(
	enum thread_state from, enum thread_state to)
{
	if (from == to) {
		return 1;
	}

	switch (from) {
	case THREAD_READY:
		return to == THREAD_RUNNING;
	case THREAD_RUNNING:
		return to == THREAD_READY || to == THREAD_SLEEPING ||
			to == THREAD_WAITING || to == THREAD_ZOMBIE;
	case THREAD_SLEEPING:
	case THREAD_WAITING:
		return to == THREAD_READY || to == THREAD_ZOMBIE;
	case THREAD_ZOMBIE:
		return to == THREAD_DEAD;
	case THREAD_DEAD:
		return 0;
	default:
		return 0;
	}
}

static inline void thread_validate_state_transition(
	struct thread *thread, enum thread_state state)
{
	if (thread == 0) {
		panic("null thread state transition");
	}

	if (!thread_state_transition_is_valid(thread->state, state)) {
		panic("invalid thread state transition");
	}
}

static inline void thread_set_state(
	struct thread *thread, enum thread_state state)
{
	thread_validate_state_transition(thread, state);
	thread->state = state;
}

static inline void thread_init_ready(struct thread *thread)
{
	thread->wake_tick = 0;
	thread->state = THREAD_READY;
}

static inline void thread_set_ready(struct thread *thread)
{
	thread_set_state(thread, THREAD_READY);
	thread->wake_tick = 0;
}

static inline void thread_set_running(struct thread *thread)
{
	thread_set_state(thread, THREAD_RUNNING);
}

static inline void thread_set_sleeping(
	struct thread *thread, uint64_t wake_tick)
{
	thread_validate_state_transition(thread, THREAD_SLEEPING);
	thread->wake_tick = wake_tick;
	thread->state = THREAD_SLEEPING;
}

static inline void thread_set_waiting(struct thread *thread)
{
	thread_set_state(thread, THREAD_WAITING);
	thread->wake_tick = 0;
}

static inline void thread_set_zombie(struct thread *thread)
{
	thread_set_state(thread, THREAD_ZOMBIE);
	thread->wake_tick = 0;
}

static inline void thread_set_dead(struct thread *thread)
{
	thread_set_state(thread, THREAD_DEAD);
	thread->wake_tick = 0;
}

void enqueue_thread(struct thread *thread);
void sched_reap_dead_threads(void);
void sched_thread_exit(void) __attribute__((noreturn));
void sched_selftest(void);
int sched_idle_create(void);
void sched_demo_start(void) __attribute__((noreturn));

#endif
