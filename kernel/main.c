#include <tianole/early_log.h>
#include <tianole/kernel_init.h>

void kernel_main(const boot_info_t *boot_info)
{
	early_log_init();
	early_log_puts("kernel_main entered\n");
	kernel_report_boot_state(boot_info);

	for (;;) {
		__asm__ volatile("hlt");
	}
}
