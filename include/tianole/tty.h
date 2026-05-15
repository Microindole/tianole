#ifndef TIANOLE_TTY_H
#define TIANOLE_TTY_H

#include <stddef.h>
#include <stdint.h>

/**
 * tty_init() - Initialize the early terminal line discipline.
 *
 * This is the first tty-shaped boundary between generic input events and
 * command consumers. It owns line editing, echo and completed line queues.
 */
void tty_init(void);

/**
 * tty_receive_char() - Feed one decoded character into the early tty.
 * @ch: ASCII-like character produced by an input consumer.
 *
 * Handles printable input, newline submission and backspace editing.
 */
void tty_receive_char(char ch);

/**
 * tty_key_event_to_char() - Convert a key event into the active tty keymap.
 * @code: Hardware-independent input key code.
 * @modifiers: Modifier snapshot from struct input_event.
 * @ch: Destination character.
 *
 * Return: 1 when @ch was filled, 0 when the key has no character mapping.
 */
int tty_key_event_to_char(uint16_t code, uint32_t modifiers, char *ch);

/**
 * tty_write() - Write bytes to the early tty output path.
 * @buffer: Bytes to write.
 * @size: Number of bytes in @buffer.
 *
 * This is raw terminal output for interactive consumers such as kdb. It is not
 * a kernel log record.
 *
 */
void tty_write(const char *buffer, size_t size);

/**
 * tty_write_string() - Write a NUL-terminated string to the early tty.
 * @text: String to write.
 *
 */
void tty_write_string(const char *text);

/**
 * tty_read_line() - Read one completed tty input line.
 * @buffer: Destination byte buffer.
 * @size: Size of @buffer in bytes.
 *
 * Sleeps until a complete line has been submitted.
 *
 * Return: Number of bytes copied excluding the NUL terminator, or negative
 * errno for invalid input.
 */
int tty_read_line(char *buffer, size_t size);

/**
 * tty_try_read_line() - Try to read one completed line without sleeping.
 * @buffer: Destination byte buffer.
 * @size: Size of @buffer in bytes.
 *
 * Return: Number of bytes copied excluding the NUL terminator, -EAGAIN when no
 * line is ready, or negative errno for invalid input.
 */
int tty_try_read_line(char *buffer, size_t size);

/**
 * tty_dropped_lines() - Return line drops caused by full queues.
 *
 * Return: Number of completed lines dropped by the early tty line queue.
 */
unsigned long tty_dropped_lines(void);

#endif
