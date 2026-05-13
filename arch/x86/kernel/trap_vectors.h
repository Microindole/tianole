#ifndef ARCH_X86_KERNEL_TRAP_VECTORS_H
#define ARCH_X86_KERNEL_TRAP_VECTORS_H

/*
 * X86_EXCEPTION_VECTOR(vector, entry, has_error, name, handler)
 *
 * The table is intentionally shared by the assembly-entry generation, C stub
 * declarations, IDT installation and C trap dispatch. Adding a CPU exception
 * should update this single list instead of touching exception_entry.S, cpu.h,
 * idt.c and traps.c separately.
 */
#define X86_EXCEPTION_VECTORS(X)                                               \
	X(0, exception_0, 0, "divide error", x86_handle_default_exception)     \
	X(1, exception_1, 0, "debug", x86_handle_default_exception)            \
	X(2,                                                                   \
		exception_2,                                                   \
		0,                                                             \
		"non-maskable interrupt",                                      \
		x86_handle_default_exception)                                  \
	X(3, exception_3, 0, "breakpoint", x86_handle_default_exception)       \
	X(4, exception_4, 0, "overflow", x86_handle_default_exception)         \
	X(5,                                                                   \
		exception_5,                                                   \
		0,                                                             \
		"bound range exceeded",                                        \
		x86_handle_default_exception)                                  \
	X(6, exception_6, 0, "invalid opcode", x86_handle_default_exception)   \
	X(7,                                                                   \
		exception_7,                                                   \
		0,                                                             \
		"device not available",                                        \
		x86_handle_default_exception)                                  \
	X(8, exception_8, 1, "double fault", x86_handle_default_exception)     \
	X(9,                                                                   \
		exception_9,                                                   \
		0,                                                             \
		"coprocessor segment overrun",                                 \
		x86_handle_default_exception)                                  \
	X(10, exception_10, 1, "invalid tss", x86_handle_default_exception)    \
	X(11,                                                                  \
		exception_11,                                                  \
		1,                                                             \
		"segment not present",                                         \
		x86_handle_default_exception)                                  \
	X(12,                                                                  \
		exception_12,                                                  \
		1,                                                             \
		"stack segment fault",                                         \
		x86_handle_default_exception)                                  \
	X(13,                                                                  \
		exception_13,                                                  \
		1,                                                             \
		"general protection fault",                                    \
		x86_handle_default_exception)                                  \
	X(14, exception_14, 1, "page fault", x86_handle_page_fault)            \
	X(15, exception_15, 0, "reserved", x86_handle_default_exception)       \
	X(16,                                                                  \
		exception_16,                                                  \
		0,                                                             \
		"x87 floating-point exception",                                \
		x86_handle_default_exception)                                  \
	X(17,                                                                  \
		exception_17,                                                  \
		1,                                                             \
		"alignment check",                                             \
		x86_handle_default_exception)                                  \
	X(18, exception_18, 0, "machine check", x86_handle_default_exception)  \
	X(19,                                                                  \
		exception_19,                                                  \
		0,                                                             \
		"simd floating-point exception",                               \
		x86_handle_default_exception)                                  \
	X(20,                                                                  \
		exception_20,                                                  \
		0,                                                             \
		"virtualization exception",                                    \
		x86_handle_default_exception)                                  \
	X(21,                                                                  \
		exception_21,                                                  \
		1,                                                             \
		"control protection exception",                                \
		x86_handle_default_exception)                                  \
	X(22, exception_22, 0, "reserved", x86_handle_default_exception)       \
	X(23, exception_23, 0, "reserved", x86_handle_default_exception)       \
	X(24, exception_24, 0, "reserved", x86_handle_default_exception)       \
	X(25, exception_25, 0, "reserved", x86_handle_default_exception)       \
	X(26, exception_26, 0, "reserved", x86_handle_default_exception)       \
	X(27, exception_27, 0, "reserved", x86_handle_default_exception)       \
	X(28,                                                                  \
		exception_28,                                                  \
		0,                                                             \
		"hypervisor injection exception",                              \
		x86_handle_default_exception)                                  \
	X(29,                                                                  \
		exception_29,                                                  \
		1,                                                             \
		"vmm communication exception",                                 \
		x86_handle_default_exception)                                  \
	X(30,                                                                  \
		exception_30,                                                  \
		1,                                                             \
		"security exception",                                          \
		x86_handle_default_exception)                                  \
	X(31, exception_31, 0, "reserved", x86_handle_default_exception)

/*
 * X86_IRQ_VECTOR(vector, entry, irq)
 *
 * Legacy PIC vectors start at 32. The current kernel only installs entries it
 * can route through handle_irq(); adding another external IRQ requires one
 * table entry here and one registered IRQ action.
 */
#define X86_IRQ_VECTORS(X)                                                     \
	X(32, irq_32, 0)                                                       \
	X(33, irq_33, 1)

#endif
