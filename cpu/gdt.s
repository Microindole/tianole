; cpu/gdt.s (最终修正版)
global gdt_flush
global tss_flush

gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    ; 加载段寄存器
    mov ax, 0x10      ; 0x10 is the offset in the GDT for the data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 is the code segment offset
.flush:
    ret

tss_flush:
    mov ax, 0x28      ; <-- 最终修正：使用 Ring 0 权限的选择子 0x28
    ltr ax
    ret

section .note.GNU-stack,"",@progbits