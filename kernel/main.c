#include <tianole/arch.h>
#include <tianole/early_log.h>
#include <tianole/kernel_init.h>
#include <tianole/mm.h>

void kernel_main(const boot_info_t *boot_info)
{
	early_log_init();
	early_log_puts("kernel_main entered\n");
	arch_traps_init();
	kernel_report_boot_state(boot_info);
	mm_init(boot_info);

#if KERNEL_TEST_TRAP
	__asm__ volatile("ud2");
#endif

	for (;;) {
		__asm__ volatile("hlt");
	}
}
