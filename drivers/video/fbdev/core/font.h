#ifndef DRIVERS_VIDEO_FBDEV_CORE_FONT_H
#define DRIVERS_VIDEO_FBDEV_CORE_FONT_H

#include <stdint.h>

/**
 * FBCON_FONT_WIDTH - Width of one fbcon bitmap glyph in pixels.
 */
#define FBCON_FONT_WIDTH 5u

/**
 * FBCON_FONT_HEIGHT - Height of one fbcon bitmap glyph in pixels.
 */
#define FBCON_FONT_HEIGHT 7u

/**
 * fbcon_font_glyph() - Return a tiny bitmap glyph for console text.
 * @ch: ASCII character requested by the framebuffer console.
 *
 * Return: Pointer to FBCON_FONT_HEIGHT rows of bitmap data.
 */
const uint8_t *fbcon_font_glyph(char ch);

#endif
