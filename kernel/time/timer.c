#include <stdint.h>

#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/timer.h>

static uint64_t tick_count;

void timer_tick(void)
{
	tick_count++;

	if (tick_count <= 3) {
		pr_info("timer tick=%llu\n", (unsigned long long)tick_count);
	}

	sched_tick(tick_count);
}

uint64_t timer_ticks(void)
{
	return tick_count;
}
