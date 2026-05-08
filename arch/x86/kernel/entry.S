.global _start
.global kernel_stack_top
.type _start, @function

.section .text
_start:
	lea kernel_stack_top(%rip), %rsp
	call kernel_main

1:
	hlt
	jmp 1b

.section .bss
.balign 16
kernel_stack:
	.skip 16384
kernel_stack_top:
