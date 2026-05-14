#ifndef ARCH_X86_KERNEL_CPU_H
#define ARCH_X86_KERNEL_CPU_H

#include <stdint.h>

/**
 * KERNEL_CODE_SELECTOR - Ring-0 64-bit code segment selector.
 */
#define KERNEL_CODE_SELECTOR 0x08

/**
 * X86_SELECTOR_RPL_MASK - Mask for a segment selector requestor privilege.
 */
#define X86_SELECTOR_RPL_MASK 0x03

/**
 * X86_RING3_RPL - Requestor privilege level used by future user mode frames.
 */
#define X86_RING3_RPL 0x03

/**
 * KERNEL_DATA_SELECTOR - Ring-0 data segment selector.
 */
#define KERNEL_DATA_SELECTOR 0x10

/**
 * USER_DATA_SELECTOR - Placeholder ring-3 data selector for test frames.
 *
 * The GDT does not install user descriptors yet. This value is only used to
 * make controlled trap-frame tests carry the eventual RPL=3 stack selector
 * shape expected by the entry path.
 */
#define USER_DATA_SELECTOR 0x23

/**
 * TSS_SELECTOR - Task-state segment selector used for kernel stack metadata.
 */
#define TSS_SELECTOR 0x18

#include <arch/traps.h>

/**
 * gdt_init() - Install the x86 GDT and load the task-state segment.
 */
void gdt_init(void);

/**
 * idt_init() - Install exception and IRQ gates into the x86 IDT.
 */
void idt_init(void);

/**
 * idt_set_gate() - Install one interrupt gate in the x86 IDT.
 * @vector: IDT vector number.
 * @handler: Assembly entry point for the vector.
 */
void idt_set_gate(uint8_t vector,
	void (*handler)(void),
	uint8_t gate_type,
	uint8_t dpl,
	uint8_t ist);

/*
 * CPU exception and IRQ entry stubs implemented in assembly. Each stub builds
 * a trap_frame and then enters the common C trap dispatcher.
 */
#include "trap_vectors.h"

#define DECLARE_EXCEPTION_STUB(                                                \
	vector, entry, has_error, gate_type, dpl, ist, name, handler)          \
	void entry(void);
#define DECLARE_IRQ_STUB(vector, entry, irq, gate_type, dpl, ist)              \
	void entry(void);

X86_EXCEPTION_VECTORS(DECLARE_EXCEPTION_STUB)
X86_IRQ_VECTORS(DECLARE_IRQ_STUB)

#undef DECLARE_IRQ_STUB
#undef DECLARE_EXCEPTION_STUB

#endif
