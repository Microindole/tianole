; boot.s - Final version with a hardcoded Multiboot header.

; --------------------------------------------------------------------
; Section 1: A foolproof, hardcoded Multiboot Header.
; We are not using variables (equ) to eliminate any assembler ambiguity.
; --------------------------------------------------------------------
section .multiboot
align 4
    ; Magic number: 0x1BADB002
    dd 0x1BADB002
    ; Flags: 0x00000003 (align modules on page boundaries, provide memory map)
    dd 0x00000003
    ; Checksum: -(MAGIC + FLAGS) which is -(0x1BADB005) = 0xE4524FFB
    dd 0xE4524FFB

; --------------------------------------------------------------------
; Section 2: Kernel entry point in .text section
; --------------------------------------------------------------------
section .text
global start
extern kernel_main

start:
    ; Set up the stack
    mov esp, stack_top
    ; Call our C kernel
    call kernel_main
    ; Halt the CPU if the kernel ever returns
    cli
.hang:
    hlt
    jmp .hang

; --------------------------------------------------------------------
; Section 3: Helper functions (unchanged)
; --------------------------------------------------------------------
global outb
outb:
    mov al, [esp + 8]
    mov dx, [esp + 4]
    out dx, al
    ret

global outw
outw:
    mov ax, [esp + 8]
    mov dx, [esp + 4]
    out dx, ax
    ret

global inb
inb:
    mov dx, [esp + 4]
    in al, dx
    ret

global inw
inw:
    mov dx, [esp + 4]
    in ax, dx
    ret

global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; --------------------------------------------------------------------
; Section 4: BSS section for the stack (unchanged)
; --------------------------------------------------------------------
section .bss
align 16
stack_bottom:
resb 16384 ; 16 KB stack
stack_top: