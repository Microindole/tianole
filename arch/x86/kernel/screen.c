#include <stdint.h>

#include <tianole/boot_info.h>

#include "screen.h"
#include "screen_font.h"

#define GLYPH_SCALE 1u
#define CELL_WIDTH 6u
#define CELL_HEIGHT 9u

#define PIXEL_FORMAT_RGB 0u
#define PIXEL_FORMAT_BGR 1u

#define ASCII_CR 13
#define ASCII_LF 10

static volatile uint32_t *framebuffer;
static uint32_t screen_width;
static uint32_t screen_height;
static uint32_t pixels_per_scan_line;
static uint32_t pixel_format;
static uint32_t cursor_x;
static uint32_t cursor_y;
static uint32_t columns;
static uint32_t rows;

static uint32_t pixel_color(uint8_t red, uint8_t green, uint8_t blue)
{
	if (pixel_format == PIXEL_FORMAT_RGB) {
		return red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
	}

	return blue | ((uint32_t)green << 8) | ((uint32_t)red << 16);
}

static void draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x >= screen_width || y >= screen_height) {
		return;
	}

	framebuffer[y * pixels_per_scan_line + x] = color;
}

static void clear_cell(uint32_t column, uint32_t row)
{
	uint32_t x;
	uint32_t y;
	uint32_t start_x = column * CELL_WIDTH;
	uint32_t start_y = row * CELL_HEIGHT;
	uint32_t background = pixel_color(0, 0, 0);

	for (y = 0; y < CELL_HEIGHT; y++) {
		for (x = 0; x < CELL_WIDTH; x++) {
			draw_pixel(start_x + x, start_y + y, background);
		}
	}
}

static void draw_char(char ch, uint32_t column, uint32_t row)
{
	const uint8_t *glyph = screen_font_glyph(ch);
	uint32_t foreground = pixel_color(220, 220, 220);
	uint32_t glyph_row;
	uint32_t glyph_col;
	uint32_t scale_x;
	uint32_t scale_y;
	uint32_t start_x = column * CELL_WIDTH;
	uint32_t start_y = row * CELL_HEIGHT + 1;

	clear_cell(column, row);

	for (glyph_row = 0; glyph_row < SCREEN_FONT_HEIGHT; glyph_row++) {
		for (glyph_col = 0; glyph_col < SCREEN_FONT_WIDTH;
			glyph_col++) {
			if ((glyph[glyph_row] &
				    (1u << (SCREEN_FONT_WIDTH - 1 -
					     glyph_col))) == 0) {
				continue;
			}

			for (scale_y = 0; scale_y < GLYPH_SCALE; scale_y++) {
				for (scale_x = 0; scale_x < GLYPH_SCALE;
					scale_x++) {
					draw_pixel(start_x +
							glyph_col *
								GLYPH_SCALE +
							scale_x,
						start_y +
							glyph_row *
								GLYPH_SCALE +
							scale_y,
						foreground);
				}
			}
		}
	}
}

static void clear_screen(void)
{
	uint32_t x;
	uint32_t y;
	uint32_t background = pixel_color(0, 0, 0);

	for (y = 0; y < screen_height; y++) {
		for (x = 0; x < screen_width; x++) {
			draw_pixel(x, y, background);
		}
	}
}

static void clear_text_row(uint32_t row)
{
	uint32_t column;

	for (column = 0; column < columns; column++) {
		clear_cell(column, row);
	}
}

static void scroll_up(void)
{
	uint32_t x;
	uint32_t y;

	for (y = CELL_HEIGHT; y < rows * CELL_HEIGHT; y++) {
		for (x = 0; x < columns * CELL_WIDTH; x++) {
			framebuffer[(y - CELL_HEIGHT) * pixels_per_scan_line +
				x] = framebuffer[y * pixels_per_scan_line + x];
		}
	}

	clear_text_row(rows - 1);
}

static void newline(void)
{
	cursor_x = 0;
	if (cursor_y + 1 < rows) {
		cursor_y++;
		return;
	}

	scroll_up();
	cursor_y = rows - 1;
}

/**
 * screen_console_init() - Attach early logging to the GOP framebuffer.
 * @boot_info: Boot handoff data captured before ExitBootServices().
 *
 * The console is intentionally minimal. It validates that the bootloader found
 * a linear 32-bit RGB/BGR framebuffer, records the current mode, and clears the
 * visible surface so subsequent early_log output is readable in the QEMU
 * window.
 */
void screen_console_init(const boot_info_t *boot_info)
{
	if (boot_info == 0 || boot_info->framebuffer_base == 0 ||
		boot_info->framebuffer_width == 0 ||
		boot_info->framebuffer_height == 0 ||
		boot_info->framebuffer_pixels_per_scan_line == 0 ||
		boot_info->framebuffer_pixel_format > PIXEL_FORMAT_BGR) {
		return;
	}

	framebuffer =
		(volatile uint32_t *)(uintptr_t)boot_info->framebuffer_base;
	screen_width = boot_info->framebuffer_width;
	screen_height = boot_info->framebuffer_height;
	pixels_per_scan_line = boot_info->framebuffer_pixels_per_scan_line;
	pixel_format = boot_info->framebuffer_pixel_format;
	columns = screen_width / CELL_WIDTH;
	rows = screen_height / CELL_HEIGHT;
	cursor_x = 0;
	cursor_y = 0;
	clear_screen();
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
	if (framebuffer == 0 || rows == 0 || columns == 0) {
		return;
	}

	if (ch == ASCII_CR) {
		return;
	}

	if (ch == ASCII_LF) {
		newline();
		return;
	}

	draw_char(ch, cursor_x, cursor_y);
	cursor_x++;
	if (cursor_x >= columns) {
		newline();
	}
}
