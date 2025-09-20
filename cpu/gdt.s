; cpu/gdt.s - Loads the new GDT and TSS

global gdt_flush
global tss_flush

gdt_flush:
    mov eax, [esp+4]  ; 获取 gdt_ptr 的地址
    lgdt [eax]        ; 加载 GDT

    ; 重新加载所有数据段寄存器
    mov ax, 0x10      ; 0x10 是内核数据段的选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; 执行一个远跳转来刷新 CS 寄存器
    jmp 0x08:.flush   ; 0x08 是内核代码段的选择子
.flush:
    ret

tss_flush:
    mov ax, 0x28 | 0  ; TSS段选择子 (5*8=40=0x28, RPL=0)
    ltr ax
    ret

section .note.GNU-stack,"",@progbits