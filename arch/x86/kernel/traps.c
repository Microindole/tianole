#include <stdint.h>

#include <tianole/arch.h>
#include <tianole/early_log.h>

#include "cpu.h"

static const char *const exception_names[32] = {
	"divide error",
	"debug",
	"non-maskable interrupt",
	"breakpoint",
	"overflow",
	"bound range exceeded",
	"invalid opcode",
	"device not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid tss",
	"segment not present",
	"stack segment fault",
	"general protection fault",
	"page fault",
	"reserved",
	"x87 floating-point exception",
	"alignment check",
	"machine check",
	"simd floating-point exception",
	"virtualization exception",
	"control protection exception",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"hypervisor injection exception",
	"vmm communication exception",
	"security exception",
	"reserved",
};

static void log_hex_digit(uint8_t digit)
{
	if (digit < 10) {
		early_log_putc((char)('0' + digit));
		return;
	}

	early_log_putc((char)('a' + digit - 10));
}

static void log_u64_hex(uint64_t value)
{
	int shift;

	early_log_puts("0x");
	for (shift = 60; shift >= 0; shift -= 4) {
		log_hex_digit((uint8_t)((value >> shift) & 0x0f));
	}
}

static uint64_t interrupted_rsp(const struct trap_frame *frame)
{
	return (uint64_t)(uintptr_t)&frame->rflags + sizeof(frame->rflags);
}

static uint64_t read_cr2(void)
{
	uint64_t cr2;

	__asm__ volatile("movq %%cr2, %0" : "=r"(cr2));
	return cr2;
}

void arch_traps_init(void)
{
	gdt_init();
	idt_init();
	early_log_puts("traps initialized\n");
}

void trap_dispatch(struct trap_frame *frame)
{
	uint64_t vector = frame->vector;
	const char *name = "unknown exception";

	if (vector < 32) {
		name = exception_names[vector];
	}

	early_log_puts("exception: ");
	early_log_puts(name);
	early_log_puts("\n");
	early_log_puts("vector=");
	early_log_u64_decimal(vector);
	early_log_puts(" error=");
	log_u64_hex(frame->error_code);
	early_log_puts("\n");
	early_log_puts("rip=");
	log_u64_hex(frame->rip);
	early_log_puts(" rsp=");
	log_u64_hex(interrupted_rsp(frame));
	early_log_puts(" rflags=");
	log_u64_hex(frame->rflags);
	early_log_puts("\n");

	if (vector == 14) {
		early_log_puts("fault_address=");
		log_u64_hex(read_cr2());
		early_log_puts("\n");
	}

	panic("unhandled CPU exception");
}
