#include <tianole/sched.h>

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
