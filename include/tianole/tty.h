#ifndef TIANOLE_TTY_H
#define TIANOLE_TTY_H

#include <stddef.h>

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
