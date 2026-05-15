#include <stddef.h>
#include <stdint.h>

#include <tianole/errno.h>
#include <tianole/fbcon.h>

#include "font.h"

#define GLYPH_SCALE 1u
#define CELL_WIDTH 6u
#define CELL_HEIGHT 9u

#define ASCII_BS 8
#define ASCII_CR 13
#define ASCII_LF 10

static struct fb_info fbcon_info;
static uint32_t cursor_x;
static uint32_t cursor_y;
static uint32_t columns;
static uint32_t rows;

static uint32_t pixel_color(uint8_t red, uint8_t green, uint8_t blue)
{
	if (fbcon_info.pixel_format == FBCON_PIXEL_FORMAT_RGB) {
		return red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
	}

	return blue | ((uint32_t)green << 8) | ((uint32_t)red << 16);
}

static void draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x >= fbcon_info.width || y >= fbcon_info.height) {
		return;
	}

	fbcon_info.base[y * fbcon_info.pixels_per_scan_line + x] = color;
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
	const uint8_t *glyph = fbcon_font_glyph(ch);
	uint32_t foreground = pixel_color(220, 220, 220);
	uint32_t glyph_row;
	uint32_t glyph_col;
	uint32_t scale_x;
	uint32_t scale_y;
	uint32_t start_x = column * CELL_WIDTH;
	uint32_t start_y = row * CELL_HEIGHT + 1;

	clear_cell(column, row);

	for (glyph_row = 0; glyph_row < FBCON_FONT_HEIGHT; glyph_row++) {
		for (glyph_col = 0; glyph_col < FBCON_FONT_WIDTH; glyph_col++) {
			if ((glyph[glyph_row] &
				    (1u << (FBCON_FONT_WIDTH - 1 -
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

	for (y = 0; y < fbcon_info.height; y++) {
		for (x = 0; x < fbcon_info.width; x++) {
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
			fbcon_info.base[(y - CELL_HEIGHT) *
					fbcon_info.pixels_per_scan_line +
				x] = fbcon_info.base[y *
					fbcon_info.pixels_per_scan_line +
				x];
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

static void backspace(void)
{
	if (cursor_x == 0) {
		return;
	}

	cursor_x--;
	clear_cell(cursor_x, cursor_y);
}

int fbcon_init(const struct fb_info *info)
{
	if (info == 0 || info->base == 0 || info->width == 0 ||
		info->height == 0 || info->pixels_per_scan_line == 0 ||
		info->pixel_format > FBCON_PIXEL_FORMAT_BGR) {
		return -EINVAL;
	}

	fbcon_info = *info;
	columns = fbcon_info.width / CELL_WIDTH;
	rows = fbcon_info.height / CELL_HEIGHT;
	cursor_x = 0;
	cursor_y = 0;

	if (columns == 0 || rows == 0) {
		return -EINVAL;
	}

	clear_screen();
	return 0;
}

void fbcon_putc(char ch)
{
	if (fbcon_info.base == 0 || rows == 0 || columns == 0) {
		return;
	}

	if (ch == ASCII_CR) {
		return;
	}

	if (ch == ASCII_BS) {
		backspace();
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

void fbcon_write(const char *text, size_t length)
{
	size_t index;

	if (text == 0 || length == 0) {
		return;
	}

	for (index = 0; index < length; index++) {
		fbcon_putc(text[index]);
	}
}
