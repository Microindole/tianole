#include <tianole/early_log.h>
#include <tianole/errno.h>
#include <tianole/sched.h>
#include <tianole/workqueue.h>

/**
 * struct workqueue - Single early kernel deferred work queue.
 * @wait: Wait queue used as both event wait and list lock.
 * @head: Oldest pending work item.
 * @tail: Newest pending work item.
 * @started: Non-zero once the worker thread exists.
 * @initialized: Non-zero after workqueue_init().
 *
 * This is intentionally small: one global worker, FIFO ordering and caller
 * owned work items. The shape mirrors Linux workqueue boundaries without
 * bringing in per-CPU pools, attributes or cancellation yet.
 */
struct workqueue {
	struct wait_queue wait;
	struct work_struct *head;
	struct work_struct *tail;
	int started;
	int initialized;
};

static struct workqueue system_workqueue;
static struct work_struct workqueue_selftest_work;
static int workqueue_selftest_done;

static int workqueue_has_work(void *arg)
{
	struct workqueue *queue = arg;

	return queue->head != 0;
}

/**
 * workqueue_pop() - Remove one pending work item from the system queue.
 *
 * Return: Next work item, or NULL when the queue is empty.
 */
static struct work_struct *workqueue_pop(void)
{
	struct work_struct *work;
	uint64_t flags;

	wait_queue_lock_irqsave(&system_workqueue.wait, &flags);
	work = system_workqueue.head;
	if (work != 0) {
		system_workqueue.head = work->next;
		if (system_workqueue.head == 0) {
			system_workqueue.tail = 0;
		}
		work->next = 0;
		work->pending = 0;
	}
	wait_queue_unlock_irqrestore(&system_workqueue.wait, flags);

	return work;
}

/**
 * workqueue_thread() - Execute queued work in normal kernel thread context.
 * @arg: Unused worker argument.
 *
 * The worker sleeps on the queue condition when idle. Once woken, it drains all
 * currently pending work before checking the wait condition again.
 */
static void workqueue_thread(void *arg)
{
	(void)arg;

	for (;;) {
		struct work_struct *work;

		if (wait_queue_wait(&system_workqueue.wait,
			    workqueue_has_work,
			    &system_workqueue) != 0) {
			panic("workqueue wait failed");
		}

		for (;;) {
			work = workqueue_pop();
			if (work == 0) {
				break;
			}
			work->func(work);
		}
	}
}

static void workqueue_selftest_entry(struct work_struct *work)
{
	(void)work;

	workqueue_selftest_done = 1;
	early_log_puts("workqueue selftest ok\n");
}

void work_init(struct work_struct *work, work_func_t func, void *data)
{
	if (work == 0) {
		return;
	}

	work->func = func;
	work->data = data;
	work->next = 0;
	work->pending = 0;
}

void workqueue_init(void)
{
	if (system_workqueue.initialized != 0) {
		return;
	}

	wait_queue_init(&system_workqueue.wait);
	system_workqueue.head = 0;
	system_workqueue.tail = 0;
	system_workqueue.started = 0;
	system_workqueue.initialized = 1;
	workqueue_selftest_done = 0;
	work_init(&workqueue_selftest_work, workqueue_selftest_entry, 0);
	early_log_puts("workqueue initialized\n");
}

int workqueue_start(void)
{
	if (system_workqueue.initialized == 0) {
		return -EINVAL;
	}

	if (system_workqueue.started != 0) {
		return 0;
	}

	if (kernel_thread_create("workqueue", workqueue_thread, 0) == 0) {
		return -ENOMEM;
	}

	system_workqueue.started = 1;
	return queue_work(&workqueue_selftest_work);
}

int queue_work(struct work_struct *work)
{
	uint64_t flags;

	if (system_workqueue.initialized == 0 || work == 0 || work->func == 0) {
		return -EINVAL;
	}

	wait_queue_lock_irqsave(&system_workqueue.wait, &flags);
	if (work->pending != 0) {
		wait_queue_unlock_irqrestore(&system_workqueue.wait, flags);
		return -EBUSY;
	}

	work->next = 0;
	work->pending = 1;
	if (system_workqueue.tail != 0) {
		system_workqueue.tail->next = work;
	} else {
		system_workqueue.head = work;
	}
	system_workqueue.tail = work;
	wait_queue_wake_one_locked(&system_workqueue.wait);
	wait_queue_unlock_irqrestore(&system_workqueue.wait, flags);

	return 0;
}
