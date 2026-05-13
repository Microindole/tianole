#include <stdint.h>

#include "cpu.h"

#define IDT_PRESENT 0x80
#define IDT_INTERRUPT_GATE 0x0e

/**
 * struct idt_entry - Packed x86_64 interrupt gate descriptor.
 *
 * The descriptor stores a 64-bit assembly entry address split across hardware
 * fields. All current gates use IST 0 and the kernel code selector.
 */
struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attr;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute__((packed));

struct idtr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

/**
 * load_idt() - Load the CPU interrupt descriptor table register.
 * @idtr: Pointer consumed by the LIDT instruction.
 */
static void load_idt(const struct idtr *idtr)
{
	__asm__ volatile("lidt (%0)" : : "r"(idtr) : "memory");
}

/**
 * idt_set_gate() - Install one present ring-0 interrupt gate.
 * @vector: IDT vector number.
 * @handler: Assembly entry point that builds a trap_frame.
 */
void idt_set_gate(uint8_t vector, void (*handler)(void))
{
	uint64_t offset = (uint64_t)(uintptr_t)handler;
	struct idt_entry *entry = &idt[vector];

	entry->offset_low = (uint16_t)(offset & 0xffff);
	entry->selector = KERNEL_CODE_SELECTOR;
	entry->ist = 0;
	entry->type_attr = IDT_PRESENT | IDT_INTERRUPT_GATE;
	entry->offset_mid = (uint16_t)((offset >> 16) & 0xffff);
	entry->offset_high = (uint32_t)(offset >> 32);
	entry->reserved = 0;
}

/**
 * idt_init() - Install early CPU exception and timer IRQ gates.
 *
 * Vectors are installed from trap_vectors.h so vector metadata has one
 * owner. Additional device IRQs should add one table entry and one assembly
 * stub instead of growing an ad hoc setup list here.
 */
void idt_init(void)
{
	struct idtr idtr = {
		.limit = sizeof(idt) - 1,
		.base = (uint64_t)(uintptr_t)idt,
	};

#define INSTALL_EXCEPTION_GATE(vector, entry, has_error, name, handler)        \
	idt_set_gate(vector, entry);
#define INSTALL_IRQ_GATE(vector, entry, irq) idt_set_gate(vector, entry);

	X86_EXCEPTION_VECTORS(INSTALL_EXCEPTION_GATE)
	X86_IRQ_VECTORS(INSTALL_IRQ_GATE)

#undef INSTALL_IRQ_GATE
#undef INSTALL_EXCEPTION_GATE

	load_idt(&idtr);
}
