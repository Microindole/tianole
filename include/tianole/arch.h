#ifndef TIANOLE_ARCH_H
#define TIANOLE_ARCH_H

#include <stdint.h>

#include <tianole/boot_info.h>

/**
 * arch_early_log_init() - Initialize architecture early log devices.
 * @boot_info: Bootloader handoff data, or NULL for backend-only setup.
 *
 * Provides the backend setup used by the generic early log front end.
 */
void arch_early_log_init(const boot_info_t *boot_info);

/**
 * arch_early_log_putc() - Emit one character through arch early log devices.
 * @ch: Character to write.
 *
 * This keeps port I/O and other architecture details out of generic logging.
 */
void arch_early_log_putc(char ch);

/**
 * arch_halt_forever() - Stop execution permanently.
 *
 * Used by panic and unrecoverable paths after diagnostics have been emitted.
 */
void arch_halt_forever(void) __attribute__((noreturn));

/**
 * arch_irq_save() - Save interrupt state and disable maskable interrupts.
 *
 * Return: Architecture flags needed by arch_irq_restore().
 */
uint64_t arch_irq_save(void);

/**
 * arch_irq_restore() - Restore a previously saved interrupt state.
 * @flags: Value returned by arch_irq_save().
 *
 * This is the backend for interrupt-safe spinlocks on the current CPU.
 */
void arch_irq_restore(uint64_t flags);

/**
 * arch_page_table_uses_page() - Check whether a page backs page tables.
 * @page: Physical page base address.
 *
 * Return: Non-zero if the architecture page-table layer owns the page.
 */
int arch_page_table_uses_page(uint64_t page);

/**
 * arch_traps_init() - Initialize architecture trap and IRQ entry tables.
 *
 * Sets up descriptor tables and entry points before devices enable IRQs.
 */
void arch_traps_init(void);

/**
 * arch_test_double_fault() - Run the architecture double fault test path.
 *
 * This is a controlled test hook used only by KERNEL_TEST_DOUBLE_FAULT. It
 * exercises the vector-8 C dispatch policy without trying to corrupt the
 * active stack to force a real hardware double fault.
 */
void arch_test_double_fault(void);

/**
 * arch_test_general_protection() - Run the #GP policy test path.
 *
 * This controlled hook is used only by KERNEL_TEST_GENERAL_PROTECTION. It
 * routes a vector-13 frame through trap dispatch so checks can verify the
 * dedicated general-protection policy before user-mode faults exist.
 */
void arch_test_general_protection(void);

/**
 * arch_timer_init() - Initialize the architecture timer backend.
 *
 * Connects the hardware timer to the generic timer and scheduler path.
 */
void arch_timer_init(void);

#endif
