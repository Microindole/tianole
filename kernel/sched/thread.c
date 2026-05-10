#include <stddef.h>
#include <stdint.h>

#include <arch/switch.h>

#include <tianole/early_log.h>
#include <tianole/mm.h>
#include <tianole/sched.h>
#include <tianole/spinlock.h>
#include <tianole/timer.h>

#define KERNEL_STACK_SIZE (PAGE_SIZE * 4u)
#define STACK_ALIGNMENT 16u

static struct thread *run_queue_head;
static struct thread *run_queue_tail;
static struct thread *current_thread;
static uintptr_t boot_stack_pointer;
static uint64_t next_thread_id = 1;
static int scheduler_ready;
static int schedule_locked;
static int need_resched;
static struct thread *idle_thread;
static struct spinlock scheduler_lock = SPINLOCK_INITIALIZER;

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

static void release_thread(struct thread *thread)
{
	kfree(thread->stack_base);
	kfree(thread);
}

static void reap_dead_threads(void)
{
	struct thread *prev = 0;
	struct thread *thread = run_queue_head;

	while (thread != 0) {
		struct thread *next = thread->next;

		if (thread->state == THREAD_DEAD && thread != current_thread) {
			if (prev != 0) {
				prev->next = next;
			} else {
				run_queue_head = next;
			}

			if (run_queue_tail == thread) {
				run_queue_tail = prev;
			}

			release_thread(thread);
		} else {
			prev = thread;
		}

		thread = next;
	}
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

	thread->state = THREAD_READY;
	thread->entry = entry;
	thread->arg = arg;
	thread->stack_top = align_down_uintptr(stack_top, STACK_ALIGNMENT);
	thread->stack_pointer = prepare_initial_stack(thread->stack_top);
	thread->stack_size = KERNEL_STACK_SIZE;
	thread->wake_tick = 0;
	thread->next = 0;
	thread->wait_next = 0;
	copy_thread_name(thread->name, sizeof(thread->name), name);

	spin_lock_irqsave(&scheduler_lock, &flags);
	thread->id = next_thread_id++;
	enqueue_thread(thread);
	spin_unlock_irqrestore(&scheduler_lock, flags);

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

	reap_dead_threads();

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

void wait_queue_init(struct wait_queue *queue)
{
	if (queue == 0) {
		return;
	}

	queue->head = 0;
	queue->tail = 0;
}

static void wait_queue_enqueue(struct wait_queue *queue, struct thread *thread)
{
	thread->wait_next = 0;

	if (queue->tail != 0) {
		queue->tail->wait_next = thread;
	} else {
		queue->head = thread;
	}

	queue->tail = thread;
}

void wait_queue_sleep(struct wait_queue *queue)
{
	if (queue == 0 || current_thread == 0) {
		return;
	}

	wait_queue_enqueue(queue, current_thread);
	current_thread->state = THREAD_WAITING;
	sched_yield();
}

void wait_queue_wake_one(struct wait_queue *queue)
{
	struct thread *thread;

	if (queue == 0 || queue->head == 0) {
		return;
	}

	thread = queue->head;
	queue->head = thread->wait_next;
	if (queue->head == 0) {
		queue->tail = 0;
	}

	thread->wait_next = 0;
	if (thread->state == THREAD_WAITING) {
		thread->state = THREAD_READY;
	}
}

void wait_queue_wake_all(struct wait_queue *queue)
{
	while (queue != 0 && queue->head != 0) {
		wait_queue_wake_one(queue);
	}
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
	struct spinlock test_lock;
	struct thread *first =
		kernel_thread_create("worker-a", thread_selftest_entry, 0);
	struct thread *second =
		kernel_thread_create("worker-b", thread_selftest_entry, 0);
	uint64_t flags;

	test_lock.locked = 0;

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

	spin_lock_irqsave(&test_lock, &flags);
	spin_unlock_irqrestore(&test_lock, flags);

	early_log_puts("kernel thread selftest ok\n");
}

static void scheduler_demo_entry(void *arg)
{
	uint64_t id = (uint64_t)(uintptr_t)arg;
	uint64_t step;

	for (step = 1; step <= 3; step++) {
		early_log_puts("preempt thread ");
		early_log_u64_decimal(id);
		early_log_puts(" step=");
		early_log_u64_decimal(step);
		early_log_puts("\n");
		sched_sleep(2);
	}
}

static struct wait_queue demo_wait_queue;

static void wait_queue_demo_waiter(void *arg)
{
	(void)arg;

	early_log_puts("waiter sleeping\n");
	wait_queue_sleep(&demo_wait_queue);
	early_log_puts("waiter woke\n");
}

static void wait_queue_demo_waker(void *arg)
{
	(void)arg;

	early_log_puts("waker sleeping\n");
	sched_sleep(4);
	early_log_puts("waker wake_one\n");
	wait_queue_wake_one(&demo_wait_queue);
}

static void idle_thread_entry(void *arg)
{
	(void)arg;

	for (;;) {
		__asm__ volatile("hlt");
	}
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
	scheduler_selftest();
}

void sched_start(void)
{
	struct thread *first = kernel_thread_create(
		"round-robin-a", scheduler_demo_entry, (void *)(uintptr_t)1);
	struct thread *second = kernel_thread_create(
		"round-robin-b", scheduler_demo_entry, (void *)(uintptr_t)2);
	struct thread *waiter =
		kernel_thread_create("waiter", wait_queue_demo_waiter, 0);
	struct thread *waker =
		kernel_thread_create("waker", wait_queue_demo_waker, 0);
	idle_thread = kernel_thread_create("idle", idle_thread_entry, 0);

	if (first == 0 || second == 0 || waiter == 0 || waker == 0 ||
		idle_thread == 0) {
		panic("scheduler demo thread creation failed");
	}

	wait_queue_init(&demo_wait_queue);
	early_log_puts("scheduler starting\n");
	sched_yield();

	panic("scheduler returned to boot context");
}
