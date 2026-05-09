#ifndef TIANOLE_SCHED_H
#define TIANOLE_SCHED_H

#include <stddef.h>
#include <stdint.h>

typedef void (*kernel_thread_entry_t)(void *arg);

enum thread_state {
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_SLEEPING,
	THREAD_DEAD,
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
	struct thread *next;
	char name[32];
};

void sched_init(void);
struct thread *kernel_thread_create(
	const char *name, kernel_thread_entry_t entry, void *arg);
void sched_start(void) __attribute__((noreturn));
void sched_yield(void);

#endif
