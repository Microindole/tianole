#include <stdint.h>

#include "cpu.h"

#define IDT_PRESENT 0x80

/**
 * struct idt_entry - Packed x86_64 interrupt gate descriptor.
 *
 * The descriptor stores a 64-bit assembly entry address split across hardware
 * fields. Gate type, DPL and IST come from the shared vector table.
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
 * @gate_type: x86 gate type, normally interrupt or trap gate.
 * @dpl: Descriptor privilege level accepted by software INT instructions.
 * @ist: Interrupt stack table index, or X86_IST_NONE.
 */
void idt_set_gate(uint8_t vector,
	void (*handler)(void),
	uint8_t gate_type,
	uint8_t dpl,
	uint8_t ist)
{
	uint64_t offset = (uint64_t)(uintptr_t)handler;
	struct idt_entry *entry = &idt[vector];

	entry->offset_low = (uint16_t)(offset & 0xffff);
	entry->selector = KERNEL_CODE_SELECTOR;
	entry->ist = ist & 0x07;
	entry->type_attr =
		(uint8_t)(IDT_PRESENT | ((dpl & 0x03) << 5) | gate_type);
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

#define INSTALL_EXCEPTION_GATE(                                                \
	vector, entry, has_error, gate_type, dpl, ist, name, handler)          \
	idt_set_gate(vector, entry, gate_type, dpl, ist);
#define INSTALL_IRQ_GATE(vector, entry, irq, gate_type, dpl, ist)              \
	idt_set_gate(vector, entry, gate_type, dpl, ist);

	X86_EXCEPTION_VECTORS(INSTALL_EXCEPTION_GATE)
	X86_IRQ_VECTORS(INSTALL_IRQ_GATE)

#undef INSTALL_IRQ_GATE
#undef INSTALL_EXCEPTION_GATE

	load_idt(&idtr);
}
