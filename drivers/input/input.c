#include <tianole/errno.h>
#include <tianole/input.h>
#include <tianole/printk.h>
#include <tianole/sched.h>

#define INPUT_QUEUE_CAPACITY 64u

/**
 * struct input_queue - Global fixed-size input event ring.
 * @wait: Wait queue used for blocking reads and queue locking.
 * @events: Ring storage.
 * @head: Next event to read.
 * @tail: Next slot to write.
 * @count: Number of queued events.
 * @dropped: Events dropped because the ring was full.
 * @last: Most recently reported event for debug observability.
 * @has_last: Non-zero after @last has been filled.
 * @initialized: Non-zero once input_init() has run.
 *
 * The first implementation uses one global queue, mirroring the long-term
 * Linux boundary that drivers report events into input core instead of calling
 * terminal or shell code directly.
 */
struct input_queue {
	struct wait_queue wait;
	struct input_event events[INPUT_QUEUE_CAPACITY];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
	uint64_t dropped;
	struct input_event last;
	int has_last;
	int initialized;
};

static struct input_queue input_queue;

static int input_has_event(void *arg)
{
	struct input_queue *queue = arg;

	return queue->count != 0;
}

static int input_pop_locked(struct input_event *event)
{
	if (input_queue.count == 0) {
		return -EAGAIN;
	}

	*event = input_queue.events[input_queue.head];
	input_queue.head = (input_queue.head + 1) % INPUT_QUEUE_CAPACITY;
	input_queue.count--;
	return 0;
}

void input_init(void)
{
	if (input_queue.initialized != 0) {
		return;
	}

	wait_queue_init(&input_queue.wait);
	input_queue.head = 0;
	input_queue.tail = 0;
	input_queue.count = 0;
	input_queue.dropped = 0;
	input_queue.has_last = 0;
	input_queue.initialized = 1;
	pr_info("input initialized\n");
}

int input_report_event(const struct input_event *event)
{
	uint64_t flags;

	if (input_queue.initialized == 0 || event == 0) {
		return -EINVAL;
	}

	wait_queue_lock_irqsave(&input_queue.wait, &flags);
	if (input_queue.count == INPUT_QUEUE_CAPACITY) {
		input_queue.dropped++;
		wait_queue_unlock_irqrestore(&input_queue.wait, flags);
		return -ENOSPC;
	}

	input_queue.events[input_queue.tail] = *event;
	input_queue.last = *event;
	input_queue.has_last = 1;
	input_queue.tail = (input_queue.tail + 1) % INPUT_QUEUE_CAPACITY;
	input_queue.count++;
	wait_queue_wake_one_locked(&input_queue.wait);
	wait_queue_unlock_irqrestore(&input_queue.wait, flags);
	return 0;
}

int input_read_event(struct input_event *event)
{
	uint64_t flags;
	int ret;

	if (input_queue.initialized == 0 || event == 0) {
		return -EINVAL;
	}

	ret = wait_queue_wait(&input_queue.wait, input_has_event, &input_queue);
	if (ret != 0) {
		return ret;
	}

	wait_queue_lock_irqsave(&input_queue.wait, &flags);
	ret = input_pop_locked(event);
	wait_queue_unlock_irqrestore(&input_queue.wait, flags);
	return ret;
}

int input_try_read_event(struct input_event *event)
{
	uint64_t flags;
	int ret;

	if (input_queue.initialized == 0 || event == 0) {
		return -EINVAL;
	}

	wait_queue_lock_irqsave(&input_queue.wait, &flags);
	ret = input_pop_locked(event);
	wait_queue_unlock_irqrestore(&input_queue.wait, flags);
	return ret;
}

uint64_t input_dropped_events(void)
{
	uint64_t dropped;
	uint64_t flags;

	if (input_queue.initialized == 0) {
		return 0;
	}

	wait_queue_lock_irqsave(&input_queue.wait, &flags);
	dropped = input_queue.dropped;
	wait_queue_unlock_irqrestore(&input_queue.wait, flags);
	return dropped;
}

int input_last_event(struct input_event *event)
{
	uint64_t flags;

	if (input_queue.initialized == 0 || event == 0) {
		return -EINVAL;
	}

	wait_queue_lock_irqsave(&input_queue.wait, &flags);
	if (input_queue.has_last == 0) {
		wait_queue_unlock_irqrestore(&input_queue.wait, flags);
		return -EAGAIN;
	}

	*event = input_queue.last;
	wait_queue_unlock_irqrestore(&input_queue.wait, flags);
	return 0;
}
