#ifndef KERNEL_INIT_H
#define KERNEL_INIT_H

#include <tianole/boot_info.h>

/**
 * kernel_report_boot_state() - Log the boot handoff state.
 * @boot_info: Firmware-independent boot data passed to the kernel.
 *
 * Prints early diagnostics used to verify the bootloader-to-kernel contract.
 */
void kernel_report_boot_state(const boot_info_t *boot_info);

#endif
