#ifndef X86_KERNEL_SCREEN_FONT_H
#define X86_KERNEL_SCREEN_FONT_H

#include <stdint.h>

#define SCREEN_FONT_WIDTH 5u
#define SCREEN_FONT_HEIGHT 7u

const uint8_t *screen_font_glyph(char ch);

#endif
