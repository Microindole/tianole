#ifndef TIANOLE_SCHED_H
#define TIANOLE_SCHED_H

#include <stddef.h>
#include <stdint.h>

typedef void (*kernel_thread_entry_t)(void *arg);

enum thread_state {
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_SLEEPING,
	THREAD_WAITING,
	THREAD_DEAD,
};

struct wait_queue {
	struct thread *head;
	struct thread *tail;
};

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
	char name[32];
};

void sched_init(void);
struct thread *kernel_thread_create(
	const char *name, kernel_thread_entry_t entry, void *arg);
void sched_start(void) __attribute__((noreturn));
void sched_tick(uint64_t tick);
void sched_irq_exit(void);
void sched_yield(void);
void sched_sleep(uint64_t ticks);
void wait_queue_init(struct wait_queue *queue);
void wait_queue_sleep(struct wait_queue *queue);
void wait_queue_wake_one(struct wait_queue *queue);
void wait_queue_wake_all(struct wait_queue *queue);

#endif
