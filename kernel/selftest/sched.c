#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/sched.h>
#include <tianole/spinlock.h>

#include "sched/sched.h"

#define STACK_ALIGNMENT 16u

static void thread_selftest_entry(void *arg)
{
	(void)arg;
}

static int selftest_condition_false(void *arg)
{
	(void)arg;
	return 0;
}

static void assert_thread_transition(
	enum thread_state from, enum thread_state to, int expected)
{
	if (thread_state_transition_is_valid(from, to) != expected) {
		panic("thread state transition selftest failed");
	}
}

static void sched_state_machine_selftest(void)
{
	assert_thread_transition(THREAD_READY, THREAD_RUNNING, 1);
	assert_thread_transition(THREAD_RUNNING, THREAD_READY, 1);
	assert_thread_transition(THREAD_RUNNING, THREAD_SLEEPING, 1);
	assert_thread_transition(THREAD_RUNNING, THREAD_WAITING, 1);
	assert_thread_transition(THREAD_RUNNING, THREAD_DEAD, 1);
	assert_thread_transition(THREAD_SLEEPING, THREAD_READY, 1);
	assert_thread_transition(THREAD_WAITING, THREAD_READY, 1);
	assert_thread_transition(THREAD_SLEEPING, THREAD_DEAD, 1);
	assert_thread_transition(THREAD_WAITING, THREAD_DEAD, 1);

	assert_thread_transition(THREAD_READY, THREAD_SLEEPING, 0);
	assert_thread_transition(THREAD_READY, THREAD_WAITING, 0);
	assert_thread_transition(THREAD_READY, THREAD_DEAD, 0);
	assert_thread_transition(THREAD_SLEEPING, THREAD_RUNNING, 0);
	assert_thread_transition(THREAD_WAITING, THREAD_RUNNING, 0);
	assert_thread_transition(THREAD_DEAD, THREAD_READY, 0);
	assert_thread_transition(THREAD_DEAD, THREAD_RUNNING, 0);
}

void sched_selftest(void)
{
	struct spinlock test_lock;
	struct wait_queue test_wait_queue;
	struct thread *first =
		kernel_thread_create("worker-a", thread_selftest_entry, 0);
	struct thread *second =
		kernel_thread_create("worker-b", thread_selftest_entry, 0);
	uint64_t flags;

	sched_state_machine_selftest();

	test_lock.locked = 0;

	if (first == 0 || second == 0 || first == second) {
		panic("kernel thread selftest allocation failed");
	}

	if (first->id == second->id || first->state != THREAD_READY ||
		second->state != THREAD_READY) {
		panic("kernel thread selftest state failed");
	}

	if (first->wait_queue != 0 || second->wait_queue != 0) {
		panic("kernel thread selftest wait queue ownership failed");
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

	wait_queue_init(&test_wait_queue);
	if (wait_queue_wait(0, selftest_condition_false, 0) != -EINVAL) {
		panic("wait queue selftest null queue errno failed");
	}

	if (wait_queue_wait(&test_wait_queue, 0, 0) != -EINVAL) {
		panic("wait queue selftest null condition errno failed");
	}

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
static struct wait_queue condition_wait_queue;
static struct wait_queue timeout_wait_queue;
static int condition_ready;

static int condition_is_ready(void *arg)
{
	int *ready = arg;

	return *ready != 0;
}

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

static void condition_wait_demo_waiter(void *arg)
{
	(void)arg;

	early_log_puts("condition waiter sleeping\n");
	if (wait_queue_wait(&condition_wait_queue,
		    condition_is_ready,
		    &condition_ready) != 0) {
		panic("condition wait failed");
	}
	early_log_puts("condition waiter woke\n");
}

static void condition_wait_demo_waker(void *arg)
{
	uint64_t flags;

	(void)arg;

	early_log_puts("condition waker sleeping\n");
	sched_sleep(5);
	wait_queue_lock_irqsave(&condition_wait_queue, &flags);
	condition_ready = 1;
	early_log_puts("condition waker wake_all\n");
	wait_queue_wake_all_locked(&condition_wait_queue);
	wait_queue_unlock_irqrestore(&condition_wait_queue, flags);
}

static void timeout_wait_demo_waiter(void *arg)
{
	int never_ready = 0;

	(void)arg;

	early_log_puts("timeout waiter sleeping\n");
	if (wait_queue_wait_timeout(
		    &timeout_wait_queue, condition_is_ready, &never_ready, 0) !=
		-ETIMEDOUT) {
		panic("timeout wait zero tick errno failed");
	}

	if (wait_queue_wait_timeout(
		    &timeout_wait_queue, condition_is_ready, &never_ready, 3) !=
		-ETIMEDOUT) {
		panic("timeout wait did not time out");
	}
	early_log_puts("timeout waiter timed out\n");
}

static void return_exit_demo_thread(void *arg)
{
	(void)arg;

	early_log_puts("return exit thread returning\n");
}

static void explicit_exit_demo_thread(void *arg)
{
	(void)arg;

	early_log_puts("explicit exit thread exiting\n");
	kernel_thread_exit();
}

void sched_demo_start(void)
{
	struct thread *first;
	struct thread *second;
	struct thread *waiter;
	struct thread *waker;
	struct thread *condition_waiter;
	struct thread *condition_waker;
	struct thread *timeout_waiter;
	struct thread *return_exit;
	struct thread *explicit_exit;

	wait_queue_init(&demo_wait_queue);
	wait_queue_init(&condition_wait_queue);
	wait_queue_init(&timeout_wait_queue);
	condition_ready = 0;

	first = kernel_thread_create(
		"round-robin-a", scheduler_demo_entry, (void *)(uintptr_t)1);
	second = kernel_thread_create(
		"round-robin-b", scheduler_demo_entry, (void *)(uintptr_t)2);
	waiter = kernel_thread_create("waiter", wait_queue_demo_waiter, 0);
	waker = kernel_thread_create("waker", wait_queue_demo_waker, 0);
	condition_waiter = kernel_thread_create(
		"condition-waiter", condition_wait_demo_waiter, 0);
	condition_waker = kernel_thread_create(
		"condition-waker", condition_wait_demo_waker, 0);
	timeout_waiter = kernel_thread_create(
		"timeout-waiter", timeout_wait_demo_waiter, 0);
	return_exit =
		kernel_thread_create("return-exit", return_exit_demo_thread, 0);
	explicit_exit = kernel_thread_create(
		"explicit-exit", explicit_exit_demo_thread, 0);

	if (first == 0 || second == 0 || waiter == 0 || waker == 0 ||
		condition_waiter == 0 || condition_waker == 0 ||
		timeout_waiter == 0 || return_exit == 0 || explicit_exit == 0) {
		panic("scheduler demo thread creation failed");
	}

	early_log_puts("scheduler starting\n");
	sched_yield();

	panic("scheduler returned to boot context");
}
