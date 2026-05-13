#ifndef ARCH_X86_KERNEL_CPU_H
#define ARCH_X86_KERNEL_CPU_H

#include <stdint.h>

/**
 * KERNEL_CODE_SELECTOR - Ring-0 64-bit code segment selector.
 */
#define KERNEL_CODE_SELECTOR 0x08

/**
 * KERNEL_DATA_SELECTOR - Ring-0 data segment selector.
 */
#define KERNEL_DATA_SELECTOR 0x10

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
void idt_set_gate(uint8_t vector, void (*handler)(void));

/*
 * CPU exception and IRQ entry stubs implemented in assembly. Each stub builds
 * a trap_frame and then enters the common C trap dispatcher.
 */
void exception_0(void);
void exception_1(void);
void exception_2(void);
void exception_3(void);
void exception_4(void);
void exception_5(void);
void exception_6(void);
void exception_7(void);
void exception_8(void);
void exception_9(void);
void exception_10(void);
void exception_11(void);
void exception_12(void);
void exception_13(void);
void exception_14(void);
void exception_15(void);
void exception_16(void);
void exception_17(void);
void exception_18(void);
void exception_19(void);
void exception_20(void);
void exception_21(void);
void exception_22(void);
void exception_23(void);
void exception_24(void);
void exception_25(void);
void exception_26(void);
void exception_27(void);
void exception_28(void);
void exception_29(void);
void exception_30(void);
void exception_31(void);
void irq_32(void);

#endif
