#include <tianole/errno.h>
#include <tianole/sched.h>

#include "sched.h"

static void idle_thread_entry(void *arg)
{
	(void)arg;

	for (;;) {
		__asm__ volatile("hlt");
	}
}

int sched_idle_create(void)
{
	idle_thread = kernel_thread_create("idle", idle_thread_entry, 0);
	if (idle_thread == 0) {
		return -ENOMEM;
	}

	return 0;
}
