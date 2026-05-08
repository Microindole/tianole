#ifndef ARCH_X86_KERNEL_CPU_H
#define ARCH_X86_KERNEL_CPU_H

#include <stdint.h>

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define TSS_SELECTOR 0x18

struct trap_frame {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t vector;
	uint64_t error_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
};

void gdt_init(void);
void idt_init(void);
void idt_set_gate(uint8_t vector, void (*handler)(void));
void trap_dispatch(struct trap_frame *frame);

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

#endif
