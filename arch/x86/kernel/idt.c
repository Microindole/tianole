#include <stdint.h>

#include "cpu.h"

#define IDT_PRESENT 0x80
#define IDT_INTERRUPT_GATE 0x0e

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

static void load_idt(const struct idtr *idtr)
{
	__asm__ volatile("lidt (%0)" : : "r"(idtr) : "memory");
}

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

void idt_init(void)
{
	struct idtr idtr = {
		.limit = sizeof(idt) - 1,
		.base = (uint64_t)(uintptr_t)idt,
	};

	idt_set_gate(0, exception_0);
	idt_set_gate(1, exception_1);
	idt_set_gate(2, exception_2);
	idt_set_gate(3, exception_3);
	idt_set_gate(4, exception_4);
	idt_set_gate(5, exception_5);
	idt_set_gate(6, exception_6);
	idt_set_gate(7, exception_7);
	idt_set_gate(8, exception_8);
	idt_set_gate(9, exception_9);
	idt_set_gate(10, exception_10);
	idt_set_gate(11, exception_11);
	idt_set_gate(12, exception_12);
	idt_set_gate(13, exception_13);
	idt_set_gate(14, exception_14);
	idt_set_gate(15, exception_15);
	idt_set_gate(16, exception_16);
	idt_set_gate(17, exception_17);
	idt_set_gate(18, exception_18);
	idt_set_gate(19, exception_19);
	idt_set_gate(20, exception_20);
	idt_set_gate(21, exception_21);
	idt_set_gate(22, exception_22);
	idt_set_gate(23, exception_23);
	idt_set_gate(24, exception_24);
	idt_set_gate(25, exception_25);
	idt_set_gate(26, exception_26);
	idt_set_gate(27, exception_27);
	idt_set_gate(28, exception_28);
	idt_set_gate(29, exception_29);
	idt_set_gate(30, exception_30);
	idt_set_gate(31, exception_31);
	idt_set_gate(32, irq_32);

	load_idt(&idtr);
}
