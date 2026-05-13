#ifndef X86_KERNEL_SCREEN_H
#define X86_KERNEL_SCREEN_H

#include <tianole/boot_info.h>

/**
 * screen_console_init() - Bind early log output to a boot framebuffer.
 * @boot_info: Boot handoff data with framebuffer mode information.
 */
void screen_console_init(const boot_info_t *boot_info);

/**
 * screen_console_putc() - Draw one early log byte on the boot framebuffer.
 * @ch: Character emitted by early_log.
 */
void screen_console_putc(char ch);

#endif
