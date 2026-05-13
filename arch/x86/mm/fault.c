#include <arch/traps.h>

#include <tianole/early_log.h>

#define PF_PRESENT (1ull << 0)
#define PF_WRITE (1ull << 1)
#define PF_USER (1ull << 2)
#define PF_RESERVED (1ull << 3)
#define PF_INSTRUCTION_FETCH (1ull << 4)

/**
 * read_cr2() - Read the faulting linear address for a page fault.
 *
 * Return: CR2 value captured by the CPU for the current page-fault handler.
 */
static uint64_t read_cr2(void)
{
	uint64_t cr2;

	__asm__ volatile("movq %%cr2, %0" : "=r"(cr2));
	return cr2;
}

/**
 * log_fault_access() - Decode and print page-fault access bits.
 * @error_code: x86 page-fault error code from the trap frame.
 *
 * This is diagnostic-only today. Demand paging, copy-on-write and user-mode
 * recovery will later replace the unconditional panic path in trap_dispatch().
 */
static void log_fault_access(uint64_t error_code)
{
	early_log_puts("access=");

	if ((error_code & PF_INSTRUCTION_FETCH) != 0) {
		early_log_puts("execute");
	} else if ((error_code & PF_WRITE) != 0) {
		early_log_puts("write");
	} else {
		early_log_puts("read");
	}

	early_log_puts(" mode=");
	early_log_puts((error_code & PF_USER) != 0 ? "user" : "kernel");

	early_log_puts(" reason=");
	early_log_puts(
		(error_code & PF_PRESENT) != 0 ? "protection" : "not-present");

	if ((error_code & PF_RESERVED) != 0) {
		early_log_puts(" reserved-bit");
	}

	early_log_puts("\n");
}

/**
 * handle_page_fault() - Print x86 page-fault diagnostics.
 * @frame: Trap frame for vector 14.
 *
 * The handler currently records fault address, access type and reason, then
 * returns to trap_dispatch(), which panics because recovery is not implemented.
 */
void handle_page_fault(struct trap_frame *frame)
{
	early_log_puts("page fault: address=");
	early_log_u64_hex(read_cr2());
	early_log_puts(" error=");
	early_log_u64_hex(frame->error_code);
	early_log_puts("\n");
	log_fault_access(frame->error_code);
}
