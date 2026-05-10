#include <tianole/sched.h>
#include <tianole/timer.h>

#include "sched.h"

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

static void wait_queue_remove(struct wait_queue *queue, struct thread *target)
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

void wait_queue_sleep(struct wait_queue *queue)
{
	if (queue == 0 || current_thread == 0) {
		return;
	}

	wait_queue_enqueue(queue, current_thread);
	current_thread->state = THREAD_WAITING;
	sched_yield();
}

int wait_queue_wait(
	struct wait_queue *queue, wait_condition_t condition, void *arg)
{
	if (queue == 0 || condition == 0 || current_thread == 0) {
		return -1;
	}

	while (condition(arg) == 0) {
		wait_queue_sleep(queue);
	}

	return 0;
}

int wait_queue_wait_timeout(struct wait_queue *queue,
	wait_condition_t condition,
	void *arg,
	uint64_t ticks)
{
	uint64_t deadline;

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
	while (condition(arg) == 0) {
		uint64_t now = timer_ticks();

		if (now >= deadline) {
			return -1;
		}

		current_thread->wake_tick = deadline;
		current_thread->state = THREAD_SLEEPING;
		wait_queue_enqueue(queue, current_thread);
		sched_yield();
		wait_queue_remove(queue, current_thread);
	}

	current_thread->wake_tick = 0;
	return 0;
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
	if (thread->state == THREAD_WAITING ||
		thread->state == THREAD_SLEEPING) {
		thread->wake_tick = 0;
		thread->state = THREAD_READY;
	}
}

void wait_queue_wake_all(struct wait_queue *queue)
{
	while (queue != 0 && queue->head != 0) {
		wait_queue_wake_one(queue);
	}
}
