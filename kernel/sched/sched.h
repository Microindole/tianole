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

void enqueue_thread(struct thread *thread);
void sched_reap_dead_threads(void);
void sched_selftest(void);
int sched_idle_create(void);
void sched_demo_start(void) __attribute__((noreturn));

#endif
