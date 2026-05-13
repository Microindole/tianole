#ifndef X86_KERNEL_SCREEN_FONT_H
#define X86_KERNEL_SCREEN_FONT_H

#include <stdint.h>

/**
 * SCREEN_FONT_WIDTH - Width of one early-console bitmap glyph in pixels.
 */
#define SCREEN_FONT_WIDTH 5u

/**
 * SCREEN_FONT_HEIGHT - Height of one early-console bitmap glyph in pixels.
 */
#define SCREEN_FONT_HEIGHT 7u

/**
 * screen_font_glyph() - Return a tiny bitmap glyph for early console text.
 * @ch: ASCII character requested by the framebuffer console.
 *
 * Return: Pointer to SCREEN_FONT_HEIGHT rows of bitmap data.
 */
const uint8_t *screen_font_glyph(char ch);

#endif
