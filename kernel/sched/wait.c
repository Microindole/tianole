#include <tianole/sched.h>
#include <tianole/timer.h>

#include "sched.h"

void wait_queue_init(struct wait_queue *queue)
{
	if (queue == 0) {
		return;
	}

	queue->lock.locked = 0;
	queue->head = 0;
	queue->tail = 0;
}

static void wait_queue_enqueue_locked(
	struct wait_queue *queue, struct thread *thread)
{
	thread->wait_next = 0;

	if (queue->tail != 0) {
		queue->tail->wait_next = thread;
	} else {
		queue->head = thread;
	}

	queue->tail = thread;
}

static void wait_queue_remove_locked(
	struct wait_queue *queue, struct thread *target)
{
	struct thread *prev = 0;
	struct thread *thread;

	if (queue == 0 || target == 0) {
		return;
	}

	for (thread = queue->head; thread != 0; thread = thread->wait_next) {
		if (thread == target) {
			if (prev != 0) {
				prev->wait_next = thread->wait_next;
			} else {
				queue->head = thread->wait_next;
			}

			if (queue->tail == thread) {
				queue->tail = prev;
			}

			thread->wait_next = 0;
			return;
		}

		prev = thread;
	}
}

static void wait_queue_mark_ready_locked(struct thread *thread)
{
	if (thread_is_waiting(thread) || thread_is_sleeping(thread)) {
		thread_set_ready(thread);
	}
}

void wait_queue_sleep(struct wait_queue *queue)
{
	uint64_t flags;

	if (queue == 0 || current_thread == 0) {
		return;
	}

	spin_lock_irqsave(&queue->lock, &flags);
	wait_queue_enqueue_locked(queue, current_thread);
	thread_set_waiting(current_thread);
	spin_unlock_irqrestore(&queue->lock, flags);

	for (;;) {
		sched_yield();

		spin_lock_irqsave(&queue->lock, &flags);
		if (!thread_is_waiting(current_thread)) {
			spin_unlock_irqrestore(&queue->lock, flags);
			return;
		}
		spin_unlock_irqrestore(&queue->lock, flags);
	}
}

int wait_queue_wait(
	struct wait_queue *queue, wait_condition_t condition, void *arg)
{
	uint64_t flags;

	if (queue == 0 || condition == 0 || current_thread == 0) {
		return -1;
	}

	for (;;) {
		spin_lock_irqsave(&queue->lock, &flags);
		if (condition(arg) != 0) {
			spin_unlock_irqrestore(&queue->lock, flags);
			return 0;
		}

		wait_queue_enqueue_locked(queue, current_thread);
		thread_set_waiting(current_thread);
		spin_unlock_irqrestore(&queue->lock, flags);

		sched_yield();

		spin_lock_irqsave(&queue->lock, &flags);
		wait_queue_remove_locked(queue, current_thread);
		spin_unlock_irqrestore(&queue->lock, flags);
	}
}

int wait_queue_wait_timeout(struct wait_queue *queue,
	wait_condition_t condition,
	void *arg,
	uint64_t ticks)
{
	uint64_t deadline;
	uint64_t flags;

	if (queue == 0 || condition == 0 || current_thread == 0) {
		return -1;
	}

	if (condition(arg) != 0) {
		return 0;
	}

	if (ticks == 0) {
		return -1;
	}

	deadline = timer_ticks() + ticks;
	for (;;) {
		uint64_t now = timer_ticks();

		spin_lock_irqsave(&queue->lock, &flags);
		if (condition(arg) != 0) {
			current_thread->wake_tick = 0;
			spin_unlock_irqrestore(&queue->lock, flags);
			return 0;
		}

		if (now >= deadline) {
			current_thread->wake_tick = 0;
			spin_unlock_irqrestore(&queue->lock, flags);
			return -1;
		}

		thread_set_sleeping(current_thread, deadline);
		wait_queue_enqueue_locked(queue, current_thread);
		spin_unlock_irqrestore(&queue->lock, flags);

		sched_yield();

		spin_lock_irqsave(&queue->lock, &flags);
		wait_queue_remove_locked(queue, current_thread);
		spin_unlock_irqrestore(&queue->lock, flags);
	}
}

void wait_queue_wake_one(struct wait_queue *queue)
{
	struct thread *thread;
	uint64_t flags;

	if (queue == 0) {
		return;
	}

	spin_lock_irqsave(&queue->lock, &flags);
	if (queue->head == 0) {
		spin_unlock_irqrestore(&queue->lock, flags);
		return;
	}

	thread = queue->head;
	queue->head = thread->wait_next;
	if (queue->head == 0) {
		queue->tail = 0;
	}

	thread->wait_next = 0;
	wait_queue_mark_ready_locked(thread);
	spin_unlock_irqrestore(&queue->lock, flags);
}

void wait_queue_wake_all(struct wait_queue *queue)
{
	struct thread *thread;
	uint64_t flags;

	if (queue == 0) {
		return;
	}

	spin_lock_irqsave(&queue->lock, &flags);
	while (queue->head != 0) {
		thread = queue->head;
		queue->head = thread->wait_next;
		if (queue->head == 0) {
			queue->tail = 0;
		}

		thread->wait_next = 0;
		wait_queue_mark_ready_locked(thread);
	}
	spin_unlock_irqrestore(&queue->lock, flags);
}
