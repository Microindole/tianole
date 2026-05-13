#ifndef TIANOLE_WORKQUEUE_H
#define TIANOLE_WORKQUEUE_H

struct work_struct;

typedef void (*work_func_t)(struct work_struct *work);

/**
 * struct work_struct - Deferred work item executed in thread context.
 * @func: Callback run by the system workqueue worker.
 * @data: Opaque caller data consumed by @func.
 * @next: Queue link owned by the workqueue core.
 * @pending: Non-zero while the item is queued but not yet running.
 *
 * A work item is caller-owned storage. It may be queued from IRQ context, but
 * its callback runs later in a kernel thread and may use normal sleeping
 * interfaces according to their own rules.
 */
struct work_struct {
	work_func_t func;
	void *data;
	struct work_struct *next;
	int pending;
};

/**
 * work_init() - Initialize a caller-owned deferred work item.
 * @work: Work item storage.
 * @func: Callback to run in workqueue thread context.
 * @data: Opaque callback data.
 */
void work_init(struct work_struct *work, work_func_t func, void *data);

/**
 * workqueue_init() - Initialize the global system workqueue state.
 *
 * Must run after the scheduler core exists and before workqueue_start().
 */
void workqueue_init(void);

/**
 * workqueue_start() - Start the global system workqueue worker thread.
 *
 * Return: 0 on success, or a negative errno value.
 */
int workqueue_start(void);

/**
 * queue_work() - Queue work for deferred execution.
 * @work: Initialized work item.
 *
 * May be called from IRQ context. The callback itself runs later in process
 * context on the workqueue worker thread.
 *
 * Return: 0 on success, -EINVAL for invalid input, or -EBUSY if already
 * queued.
 */
int queue_work(struct work_struct *work);

#endif
