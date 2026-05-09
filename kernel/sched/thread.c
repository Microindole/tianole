#include <stddef.h>
#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/mm.h>
#include <tianole/sched.h>

#define KERNEL_STACK_SIZE (PAGE_SIZE * 4u)
#define STACK_ALIGNMENT 16u

static struct thread *run_queue_head;
static struct thread *run_queue_tail;
static uint64_t next_thread_id = 1;
static int scheduler_ready;

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
	thread->stack_size = KERNEL_STACK_SIZE;
	thread->next = 0;
	copy_thread_name(thread->name, sizeof(thread->name), name);

	enqueue_thread(thread);

	return thread;
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
