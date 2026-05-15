#include <stdint.h>

#include <tianole/boot_info.h>
#include <tianole/fbcon.h>

#include "screen.h"

/**
 * screen_console_init() - Attach early logging to the boot framebuffer.
 * @boot_info: Boot handoff data captured before ExitBootServices().
 *
 * x86 owns discovery of the firmware-provided GOP mode. Text rendering is
 * delegated to the generic fbcon backend, following the same architecture split
 * as Linux's firmware/framebuffer discovery and fbcon display driver.
 */
void screen_console_init(const boot_info_t *boot_info)
{
	struct fb_info info;

	if (boot_info == 0 || boot_info->framebuffer_base == 0 ||
		boot_info->framebuffer_width == 0 ||
		boot_info->framebuffer_height == 0 ||
		boot_info->framebuffer_pixels_per_scan_line == 0 ||
		boot_info->framebuffer_pixel_format > FBCON_PIXEL_FORMAT_BGR) {
		return;
	}

	info.base = (volatile uint32_t *)(uintptr_t)boot_info->framebuffer_base;
	info.width = boot_info->framebuffer_width;
	info.height = boot_info->framebuffer_height;
	info.pixels_per_scan_line = boot_info->framebuffer_pixels_per_scan_line;
	info.pixel_format = boot_info->framebuffer_pixel_format;

	(void)fbcon_init(&info);
}

/**
 * screen_console_putc() - Mirror a log byte to the early framebuffer console.
 * @ch: Byte emitted by early_log.
 *
 * Characters are drawn into fixed-size cells. Newlines advance the cursor; when
 * the last row is exhausted the framebuffer text area scrolls up one row so
 * real boot smoke tests keep useful context on screen.
 */
void screen_console_putc(char ch)
{
	fbcon_putc(ch);
}
