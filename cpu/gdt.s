; cpu/gdt.s - Loads the GDT and refreshes segment registers

global gdt_flush

gdt_flush:
    mov eax, [esp+4] ; 获取 C 函数传来的 gdt_ptr 地址
    lgdt [eax]       ; 加载 GDT

    ; 加载段选择子到段寄存器。
    ; 0x10 是内核数据段的选择子。
    ; 我们将 CS (代码段) 的加载延迟到 jmp 指令中完成。
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; 远跳转，强制刷新 CS 寄存器。
    ; 0x08 是内核代码段的选择子。
    jmp 0x08:.flush
.flush:
    ret

global tss_flush
tss_flush:
    mov ax, 0x2B ; TSS 段选择子 (第5个条目, RPL=3)
    ltr ax
    ret

section .note.GNU-stack,"",@progbits