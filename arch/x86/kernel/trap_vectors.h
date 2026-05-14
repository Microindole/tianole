#ifndef ARCH_X86_KERNEL_TRAP_VECTORS_H
#define ARCH_X86_KERNEL_TRAP_VECTORS_H

#define X86_IDT_INTERRUPT_GATE 0x0e
#define X86_IDT_TRAP_GATE 0x0f
#define X86_IDT_DPL0 0u
#define X86_IDT_DPL3 3u
#define X86_IST_NONE 0u
#define X86_IST_DOUBLE_FAULT 1u

/*
 * Keep IDT vector ranges explicit. Linux keeps the same kind of separation in
 * arch/x86/include/asm/irq_vectors.h: architectural exceptions live at 0-31,
 * external interrupts start at 0x20, int 0x80 is reserved for the legacy
 * syscall ABI, and system vectors are reserved from the high end. Tianole
 * currently installs only legacy PIC IRQ0/IRQ1, but the ranges below make
 * later APIC and syscall work avoid the exception path.
 */
#define X86_VECTOR_COUNT 256u
#define X86_EXCEPTION_VECTOR_BASE 0x00u
#define X86_EXCEPTION_VECTOR_COUNT 32u
#define X86_FIRST_EXTERNAL_VECTOR 0x20u
#define X86_LEGACY_IRQ_VECTOR_BASE X86_FIRST_EXTERNAL_VECTOR
#define X86_LEGACY_IRQ_VECTOR_COUNT 16u
#define X86_LEGACY_SYSCALL_VECTOR 0x80u
#define X86_FIRST_SYSTEM_VECTOR 0xecu

#ifndef __ASSEMBLER__
enum x86_vector_class {
	X86_VECTOR_EXCEPTION,
	X86_VECTOR_LEGACY_IRQ,
	X86_VECTOR_SYSCALL,
	X86_VECTOR_EXTERNAL_IRQ,
	X86_VECTOR_SYSTEM,
	X86_VECTOR_RESERVED,
};

static inline int x86_vector_in_range(
	uint64_t vector, uint64_t first, uint64_t count)
{
	return vector >= first && vector < first + count;
}

static inline enum x86_vector_class x86_vector_class(uint64_t vector)
{
	if (x86_vector_in_range(vector,
		    X86_EXCEPTION_VECTOR_BASE,
		    X86_EXCEPTION_VECTOR_COUNT)) {
		return X86_VECTOR_EXCEPTION;
	}

	if (x86_vector_in_range(vector,
		    X86_LEGACY_IRQ_VECTOR_BASE,
		    X86_LEGACY_IRQ_VECTOR_COUNT)) {
		return X86_VECTOR_LEGACY_IRQ;
	}

	if (vector == X86_LEGACY_SYSCALL_VECTOR) {
		return X86_VECTOR_SYSCALL;
	}

	if (vector >= X86_FIRST_SYSTEM_VECTOR && vector < X86_VECTOR_COUNT) {
		return X86_VECTOR_SYSTEM;
	}

	if (vector >= X86_FIRST_EXTERNAL_VECTOR &&
		vector < X86_FIRST_SYSTEM_VECTOR) {
		return X86_VECTOR_EXTERNAL_IRQ;
	}

	return X86_VECTOR_RESERVED;
}
#endif

/*
 * X86_EXCEPTION_VECTOR(vector, entry, has_error, gate_type, dpl, ist, name,
 *                      handler)
 *
 * The table is shared by assembly-entry generation, C stub declarations, IDT
 * installation and C trap dispatch. Adding or changing a CPU exception should
 * update this list instead of editing exception_entry.S, cpu.h, idt.c and
 * traps.c separately.
 */
#define X86_EXCEPTION_VECTORS(X)                                               \
	X(0,                                                                   \
		exception_0,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"divide error",                                                \
		x86_handle_default_exception)                                  \
	X(1,                                                                   \
		exception_1,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"debug",                                                       \
		x86_handle_default_exception)                                  \
	X(2,                                                                   \
		exception_2,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"non-maskable interrupt",                                      \
		x86_handle_default_exception)                                  \
	X(3,                                                                   \
		exception_3,                                                   \
		0,                                                             \
		X86_IDT_TRAP_GATE,                                             \
		X86_IDT_DPL3,                                                  \
		X86_IST_NONE,                                                  \
		"breakpoint",                                                  \
		x86_handle_default_exception)                                  \
	X(4,                                                                   \
		exception_4,                                                   \
		0,                                                             \
		X86_IDT_TRAP_GATE,                                             \
		X86_IDT_DPL3,                                                  \
		X86_IST_NONE,                                                  \
		"overflow",                                                    \
		x86_handle_default_exception)                                  \
	X(5,                                                                   \
		exception_5,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"bound range exceeded",                                        \
		x86_handle_default_exception)                                  \
	X(6,                                                                   \
		exception_6,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"invalid opcode",                                              \
		x86_handle_invalid_opcode)                                     \
	X(7,                                                                   \
		exception_7,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"device not available",                                        \
		x86_handle_default_exception)                                  \
	X(8,                                                                   \
		exception_8,                                                   \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_DOUBLE_FAULT,                                          \
		"double fault",                                                \
		x86_handle_double_fault)                                       \
	X(9,                                                                   \
		exception_9,                                                   \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"coprocessor segment overrun",                                 \
		x86_handle_default_exception)                                  \
	X(10,                                                                  \
		exception_10,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"invalid tss",                                                 \
		x86_handle_default_exception)                                  \
	X(11,                                                                  \
		exception_11,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"segment not present",                                         \
		x86_handle_default_exception)                                  \
	X(12,                                                                  \
		exception_12,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"stack segment fault",                                         \
		x86_handle_default_exception)                                  \
	X(13,                                                                  \
		exception_13,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"general protection fault",                                    \
		x86_handle_general_protection)                                 \
	X(14,                                                                  \
		exception_14,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"page fault",                                                  \
		x86_handle_page_fault)                                         \
	X(15,                                                                  \
		exception_15,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(16,                                                                  \
		exception_16,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"x87 floating-point exception",                                \
		x86_handle_default_exception)                                  \
	X(17,                                                                  \
		exception_17,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"alignment check",                                             \
		x86_handle_default_exception)                                  \
	X(18,                                                                  \
		exception_18,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"machine check",                                               \
		x86_handle_default_exception)                                  \
	X(19,                                                                  \
		exception_19,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"simd floating-point exception",                               \
		x86_handle_default_exception)                                  \
	X(20,                                                                  \
		exception_20,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"virtualization exception",                                    \
		x86_handle_default_exception)                                  \
	X(21,                                                                  \
		exception_21,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"control protection exception",                                \
		x86_handle_default_exception)                                  \
	X(22,                                                                  \
		exception_22,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(23,                                                                  \
		exception_23,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(24,                                                                  \
		exception_24,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(25,                                                                  \
		exception_25,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(26,                                                                  \
		exception_26,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(27,                                                                  \
		exception_27,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)                                  \
	X(28,                                                                  \
		exception_28,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"hypervisor injection exception",                              \
		x86_handle_default_exception)                                  \
	X(29,                                                                  \
		exception_29,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"vmm communication exception",                                 \
		x86_handle_default_exception)                                  \
	X(30,                                                                  \
		exception_30,                                                  \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"security exception",                                          \
		x86_handle_default_exception)                                  \
	X(31,                                                                  \
		exception_31,                                                  \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE,                                                  \
		"reserved",                                                    \
		x86_handle_default_exception)

/*
 * X86_IRQ_VECTOR(vector, entry, irq, gate_type, dpl, ist)
 *
 * Legacy PIC vectors start at X86_LEGACY_IRQ_VECTOR_BASE. External IRQs use
 * interrupt gates so IF is cleared while the common trap path dispatches the
 * device handler.
 */
#define X86_IRQ_VECTORS(X)                                                     \
	X(X86_LEGACY_IRQ_VECTOR_BASE + 0,                                      \
		irq_32,                                                        \
		0,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE)                                                  \
	X(X86_LEGACY_IRQ_VECTOR_BASE + 1,                                      \
		irq_33,                                                        \
		1,                                                             \
		X86_IDT_INTERRUPT_GATE,                                        \
		X86_IDT_DPL0,                                                  \
		X86_IST_NONE)

#endif
