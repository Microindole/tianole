; cpu/user_mode.s (修正版)

global switch_to_user_mode

; void switch_to_user_mode(uint32_t entry, uint32_t stack);
switch_to_user_mode:
    mov edi, [esp + 4]  ; edi = entry point
    mov esi, [esp + 8]  ; esi = user stack

    ; 设置用户模式的数据段寄存器
    ; 0x23 是用户数据段的选择子 (GDT 第4个条目, 索引4*8=32=0x20, RPL=3 -> 0x23)
    mov ax, 0x23        
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 构造一个完美的 iret 栈帧，从栈底向栈顶
    push 0x23           ; SS  (用户数据段)
    push esi            ; ESP (用户栈指针)
    
    ; --- 核心修正 ---
    ; 直接压入一个已知有效的 EFLAGS 值 (0x202)
    ; Bit 9 (IF) = 1 -> 中断开启
    ; Bit 1 (Reserved) = 1 -> 必须为 1
    push 0x202          ; EFLAGS

    ; 0x1B 是用户代码段的选择子 (GDT 第3个条目, 索引3*8=24=0x18, RPL=3 -> 0x1B)
    push 0x1B           ; CS  (用户代码段)
    push edi            ; EIP (入口点)

    iret                ; 执行跳转！

section .note.GNU-stack,"",@progbits