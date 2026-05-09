#include <stddef.h>
#include <stdint.h>

#include <arch/switch.h>

#include <tianole/early_log.h>
#include <tianole/mm.h>
#include <tianole/sched.h>

#define KERNEL_STACK_SIZE (PAGE_SIZE * 4u)
#define STACK_ALIGNMENT 16u

static struct thread *run_queue_head;
static struct thread *run_queue_tail;
static struct thread *current_thread;
static uintptr_t boot_stack_pointer;
static uint64_t next_thread_id = 1;
static int scheduler_ready;

static void thread_trampoline(void) __attribute__((noreturn));

static uintptr_t align_down_uintptr(uintptr_t value, uintptr_t alignment)
{
	return value & ~(alignment - 1);
}

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

static void enqueue_thread(struct thread *thread)
{
	thread->next = 0;

	if (run_queue_tail != 0) {
		run_queue_tail->next = thread;
	} else {
		run_queue_head = thread;
	}

	run_queue_tail = thread;
}

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

	thread->id = next_thread_id++;
	thread->state = THREAD_READY;
	thread->entry = entry;
	thread->arg = arg;
	thread->stack_top = align_down_uintptr(stack_top, STACK_ALIGNMENT);
	thread->stack_pointer = prepare_initial_stack(thread->stack_top);
	thread->stack_size = KERNEL_STACK_SIZE;
	thread->next = 0;
	copy_thread_name(thread->name, sizeof(thread->name), name);

	enqueue_thread(thread);

	return thread;
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
		if (thread->state == THREAD_READY) {
			return thread;
		}
		thread = thread->next;
	}

	for (thread = run_queue_head; thread != start; thread = thread->next) {
		if (thread->state == THREAD_READY) {
			return thread;
		}
	}

	return 0;
}

void sched_yield(void)
{
	struct thread *prev = current_thread;
	struct thread *next = next_runnable_thread();

	if (next == 0 || next == prev) {
		return;
	}

	if (prev != 0 && prev->state == THREAD_RUNNING) {
		prev->state = THREAD_READY;
	}

	next->state = THREAD_RUNNING;
	current_thread = next;

	if (prev == 0) {
		arch_context_switch(&boot_stack_pointer, next->stack_pointer);
		return;
	}

	arch_context_switch(&prev->stack_pointer, next->stack_pointer);
}

static void thread_trampoline(void)
{
	struct thread *thread = current_thread;

	if (thread == 0 || thread->entry == 0) {
		panic("kernel thread entered without entry");
	}

	thread->entry(thread->arg);
	thread->state = THREAD_DEAD;

	for (;;) {
		sched_yield();
	}
}

static void thread_selftest_entry(void *arg)
{
	(void)arg;
}

static void scheduler_selftest(void)
{
	struct thread *first =
		kernel_thread_create("worker-a", thread_selftest_entry, 0);
	struct thread *second =
		kernel_thread_create("worker-b", thread_selftest_entry, 0);

	if (first == 0 || second == 0 || first == second) {
		panic("kernel thread selftest allocation failed");
	}

	if (first->id == second->id || first->state != THREAD_READY ||
		second->state != THREAD_READY) {
		panic("kernel thread selftest state failed");
	}

	if ((first->stack_top & (STACK_ALIGNMENT - 1)) != 0 ||
		(second->stack_top & (STACK_ALIGNMENT - 1)) != 0) {
		panic("kernel thread selftest stack alignment failed");
	}

	if (run_queue_head != first || first->next != second ||
		run_queue_tail != second) {
		panic("kernel thread selftest run queue failed");
	}

	early_log_puts("kernel thread selftest ok\n");
}

static void scheduler_demo_entry(void *arg)
{
	uint64_t id = (uint64_t)(uintptr_t)arg;
	uint64_t step;

	for (step = 1; step <= 3; step++) {
		early_log_puts("thread ");
		early_log_u64_decimal(id);
		early_log_puts(" step=");
		early_log_u64_decimal(step);
		early_log_puts("\n");
		sched_yield();
	}
}

void sched_init(void)
{
	if (scheduler_ready != 0) {
		return;
	}

	run_queue_head = 0;
	run_queue_tail = 0;
	scheduler_ready = 1;

	early_log_puts("scheduler initialized\n");
	scheduler_selftest();
}

void sched_start(void)
{
	struct thread *first = kernel_thread_create(
		"round-robin-a", scheduler_demo_entry, (void *)(uintptr_t)1);
	struct thread *second = kernel_thread_create(
		"round-robin-b", scheduler_demo_entry, (void *)(uintptr_t)2);

	if (first == 0 || second == 0) {
		panic("scheduler demo thread creation failed");
	}

	early_log_puts("scheduler starting\n");
	sched_yield();

	panic("scheduler returned to boot context");
}
