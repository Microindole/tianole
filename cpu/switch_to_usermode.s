[BITS 32]

; 切换到用户态
; void switch_to_usermode(uint32_t entry, uint32_t user_stack)
global switch_to_usermode
switch_to_usermode:
    cli                      ; 关闭中断

    mov ebp, esp             ; 保存栈指针
    mov eax, [ebp + 4]       ; 参数1: entry (用户程序入口地址)
    mov ebx, [ebp + 8]       ; 参数2: user_stack (用户栈地址)

    ; 设置所有数据段寄存器为用户数据段
    mov cx, 0x23             ; 用户数据段选择子 (0x20 | 3)
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    ; 构造iret需要的栈帧
    ; iret会依次弹出：EIP, CS, EFLAGS, ESP, SS
    push 0x23                ; SS (用户数据段)
    push ebx                 ; ESP (用户栈指针)

    pushf                    ; 保存当前EFLAGS
    pop ecx                  ; 弹出到ECX
    or ecx, 0x200            ; 设置IF位（开启中断）
    push ecx                 ; 压入修改后的EFLAGS

    push 0x1B                ; CS (用户代码段选择子: 0x18 | 3)
    push eax                 ; EIP (用户程序入口地址)

    iret                     ; 中断返回，切换到Ring 3
