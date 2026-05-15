#ifndef TIANOLE_PRINTK_H
#define TIANOLE_PRINTK_H

/**
 * enum printk_loglevel - Kernel log message severity.
 * @LOGLEVEL_EMERG: System is unusable.
 * @LOGLEVEL_ERR: Error condition.
 * @LOGLEVEL_WARN: Warning condition.
 * @LOGLEVEL_INFO: Informational message.
 * @LOGLEVEL_DEBUG: Debug message.
 *
 * The numeric order follows the usual kernel convention where lower values
 * are more severe. Tianole keeps the set small until log filtering and
 * userspace log access exist.
 */
enum printk_loglevel {
	LOGLEVEL_EMERG = 0,
	LOGLEVEL_ERR = 3,
	LOGLEVEL_WARN = 4,
	LOGLEVEL_INFO = 6,
	LOGLEVEL_DEBUG = 7,
};

/**
 * printk_init() - Initialize the generic kernel log frontend.
 *
 * The early output backend must already be available. The log buffer itself is
 * static storage so printk can be initialized before the heap and scheduler.
 */
void printk_init(void);

/**
 * printk() - Write a kernel log record at the default log level.
 * @fmt: printf-style format string.
 *
 * Supports the small format subset Tianole currently needs: %%, %c, %s, %d,
 * %u, %x, %p, and l/ll length variants for integer formats.
 *
 * Return: Number of characters formatted.
 */
int printk(const char *fmt, ...);

/**
 * printk_level() - Write a kernel log record at an explicit severity.
 * @level: Severity from enum printk_loglevel.
 * @fmt: printf-style format string.
 *
 * Return: Number of characters formatted.
 */
int printk_level(enum printk_loglevel level, const char *fmt, ...);

/**
 * pr_emerg - Emit an emergency kernel log message.
 */
#define pr_emerg(fmt, ...) printk_level(LOGLEVEL_EMERG, fmt, ##__VA_ARGS__)

/**
 * pr_err - Emit an error kernel log message.
 */
#define pr_err(fmt, ...) printk_level(LOGLEVEL_ERR, fmt, ##__VA_ARGS__)

/**
 * pr_warn - Emit a warning kernel log message.
 */
#define pr_warn(fmt, ...) printk_level(LOGLEVEL_WARN, fmt, ##__VA_ARGS__)

/**
 * pr_info - Emit an informational kernel log message.
 */
#define pr_info(fmt, ...) printk_level(LOGLEVEL_INFO, fmt, ##__VA_ARGS__)

/**
 * pr_debug - Emit a debug kernel log message.
 */
#define pr_debug(fmt, ...) printk_level(LOGLEVEL_DEBUG, fmt, ##__VA_ARGS__)

#endif
