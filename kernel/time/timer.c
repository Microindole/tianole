#include <stdint.h>

#include <tianole/early_log.h>
#include <tianole/sched.h>
#include <tianole/timer.h>

static uint64_t tick_count;

void timer_tick(void)
{
	tick_count++;

	if (tick_count <= 3) {
		early_log_puts("timer tick=");
		early_log_u64_decimal(tick_count);
		early_log_puts("\n");
	}

	sched_tick(tick_count);
}

uint64_t timer_ticks(void)
{
	return tick_count;
}
