#include "trap_vectors.h"

.macro EXCEPTION_NO_ERROR vector entry
.global \entry
.type \entry, @function
\entry:
	pushq $0
	pushq $\vector
	jmp exception_common
.endm

.macro EXCEPTION_ERROR vector entry
.global \entry
.type \entry, @function
\entry:
	pushq $\vector
	jmp exception_common
.endm

.macro IRQ vector entry;
.global \entry
.type \entry, @function
\entry:
	pushq $0
	pushq $\vector
	jmp exception_common
.endm

#define EMIT_EXCEPTION_STUB(vector, entry, has_error, name, handler) \
	EMIT_EXCEPTION_STUB_##has_error(vector, entry)
#define EMIT_EXCEPTION_STUB_0(vector, entry) \
	EXCEPTION_NO_ERROR vector entry;
#define EMIT_EXCEPTION_STUB_1(vector, entry) \
	EXCEPTION_ERROR vector entry;
#define EMIT_IRQ_STUB(vector, entry, irq) \
	IRQ vector entry;

.section .text

X86_EXCEPTION_VECTORS(EMIT_EXCEPTION_STUB)
X86_IRQ_VECTORS(EMIT_IRQ_STUB)

#undef EMIT_IRQ_STUB
#undef EMIT_EXCEPTION_STUB_1
#undef EMIT_EXCEPTION_STUB_0
#undef EMIT_EXCEPTION_STUB

.type exception_common, @function
exception_common:
	cld
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

	movq %rsp, %rdi
	call trap_dispatch

	addq $120, %rsp
	addq $16, %rsp
	iretq
