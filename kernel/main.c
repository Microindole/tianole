#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/console.h>
#include <tianole/early_log.h>
#include <tianole/input.h>
#include <tianole/kdb.h>
#include <tianole/kernel_init.h>
#include <tianole/keyboard.h>
#include <tianole/mm.h>
#include <tianole/panic.h>
#include <tianole/printk.h>
#include <tianole/sched.h>
#include <tianole/workqueue.h>

void kernel_main(const boot_info_t *boot_info)
{
	early_log_init(boot_info);
	printk_init();
	pr_info("kernel_main entered\n");
	arch_traps_init();
	kernel_report_boot_state(boot_info);
	mm_init(boot_info);
	sched_init();
	workqueue_init();
	input_init();
	ps2_keyboard_init();
	arch_timer_init();

#if KERNEL_TEST_TRAP
	__asm__ volatile("ud2");
#endif

#if KERNEL_TEST_DOUBLE_FAULT
	arch_test_double_fault();
#endif

#if KERNEL_TEST_GENERAL_PROTECTION
	arch_test_general_protection();
#endif

#if KERNEL_TEST_USER_EXCEPTION
	arch_test_user_invalid_opcode();
#endif

#if KERNEL_TEST_PAGE_FAULT
	*(volatile uint64_t *)(uintptr_t)0xffffff1000000000ull = 1;
#endif

	if (workqueue_start() != 0) {
		panic("workqueue start failed");
	}
	input_console_init();
	kdb_init();

	sched_start();
}
