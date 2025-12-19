[BITS 32]

; 加载GDT
global gdt_flush
gdt_flush:
    mov eax, [esp+4]    ; 获取GDT指针参数
    lgdt [eax]          ; 加载GDT

    ; 重新加载段寄存器
    mov ax, 0x10        ; 内核数据段选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 远跳转重新加载CS
    jmp 0x08:.flush     ; 0x08是内核代码段选择子
.flush:
    ret

; 加载TSS
global tss_flush
tss_flush:
    mov ax, 0x28 | 3    ; TSS段选择子，RPL=3
    ltr ax              ; 加载任务寄存器
    ret
