#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include <stdint.h>

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

static inline int thread_is_dead(const struct thread *thread)
{
	return thread != 0 && thread->state == THREAD_DEAD;
}

static inline void thread_set_ready(struct thread *thread)
{
	thread->wake_tick = 0;
	thread->state = THREAD_READY;
}

static inline void thread_set_running(struct thread *thread)
{
	thread->state = THREAD_RUNNING;
}

static inline void thread_set_sleeping(
	struct thread *thread, uint64_t wake_tick)
{
	thread->wake_tick = wake_tick;
	thread->state = THREAD_SLEEPING;
}

static inline void thread_set_waiting(struct thread *thread)
{
	thread->wake_tick = 0;
	thread->state = THREAD_WAITING;
}

static inline void thread_set_dead(struct thread *thread)
{
	thread->wake_tick = 0;
	thread->state = THREAD_DEAD;
}

void enqueue_thread(struct thread *thread);
void sched_reap_dead_threads(void);
void sched_selftest(void);
int sched_idle_create(void);
void sched_demo_start(void) __attribute__((noreturn));

#endif
