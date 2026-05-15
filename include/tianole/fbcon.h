#ifndef TIANOLE_FBCON_H
#define TIANOLE_FBCON_H

#include <stddef.h>
#include <stdint.h>

/**
 * FBCON_PIXEL_FORMAT_RGB - 32-bit framebuffer pixels store red first.
 */
#define FBCON_PIXEL_FORMAT_RGB 0u

/**
 * FBCON_PIXEL_FORMAT_BGR - 32-bit framebuffer pixels store blue first.
 */
#define FBCON_PIXEL_FORMAT_BGR 1u

/**
 * struct fb_info - Linear framebuffer mode description.
 * @base: Mapped framebuffer base address.
 * @width: Visible width in pixels.
 * @height: Visible height in pixels.
 * @pixels_per_scan_line: Physical pixels between adjacent rows.
 * @pixel_format: One of the FBCON_PIXEL_FORMAT_* values.
 *
 * This is the small Tianole equivalent of Linux fbdev mode state. Architecture
 * code discovers the framebuffer, while fbcon consumes this description to
 * render text.
 */
struct fb_info {
	volatile uint32_t *base;
	uint32_t width;
	uint32_t height;
	uint32_t pixels_per_scan_line;
	uint32_t pixel_format;
};

/**
 * fbcon_init() - Bind the early framebuffer console to a framebuffer.
 * @info: Linear framebuffer mode to copy into the fbcon backend.
 *
 * Return: 0 on success, or a negative errno value.
 */
int fbcon_init(const struct fb_info *info);

/**
 * fbcon_putc() - Draw one byte on the framebuffer console.
 * @ch: Character byte to draw.
 */
void fbcon_putc(char ch);

/**
 * fbcon_write() - Draw a byte buffer on the framebuffer console.
 * @text: Bytes to draw.
 * @length: Number of bytes in @text.
 */
void fbcon_write(const char *text, size_t length);

#endif
