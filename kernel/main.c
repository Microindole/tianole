#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/kernel_init.h>
#include <tianole/mm.h>
#include <tianole/sched.h>
#include <tianole/workqueue.h>

void kernel_main(const boot_info_t *boot_info)
{
	early_log_init(boot_info);
	early_log_puts("kernel_main entered\n");
	arch_traps_init();
	kernel_report_boot_state(boot_info);
	mm_init(boot_info);
	sched_init();
	workqueue_init();
	arch_timer_init();

#if KERNEL_TEST_TRAP
	__asm__ volatile("ud2");
#endif

#if KERNEL_TEST_PAGE_FAULT
	*(volatile uint64_t *)(uintptr_t)0xffffff1000000000ull = 1;
#endif

	if (workqueue_start() != 0) {
		panic("workqueue start failed");
	}

	sched_start();
}
