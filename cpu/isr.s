; isr.s - 独立的中断服务例程入口

; C 语言的总处理器
extern isr_handler
extern irq_handler

; 通用处理部分
isr_common_stub:
    pusha       ; 保存通用寄存器
    mov ax, ds
    push eax    ; 保存数据段

    mov ax, 0x10 ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用 C 处理器
    mov eax, esp ; 将当前的栈指针 (指向 registers_t 结构体) 作为参数
    push eax
    
    ; 检查中断号，决定是调用 ISR 还是 IRQ 处理器
    mov eax, [esp + 36] ; 从栈上获取中断号
    cmp eax, 32
    jl is_exception
    
is_irq:
    call irq_handler
    jmp common_handler_exit ; 跳转到通用的退出逻辑

is_exception:
    call isr_handler
    ; 注意：异常处理后也应该发送 EOI，以防万一是伪装成异常的 IRQ
    ; 我们将这个逻辑统一放在 common_handler_exit 中

common_handler_exit:
    pop eax ; 清理 C 函数的参数
    
    ; --- 关键修复：在这里统一发送 EOI 信号 ---
    ; 检查中断号是否 >= 40 (来自从片)
    mov eax, [esp + 32] ; 重新从栈上获取中断号 (偏移量变了)
    cmp eax, 40
    jge send_eoi_slave

send_eoi_master:
    mov al, 0x20
    out 0x20, al ; 向主片发送 EOI
    jmp common_stub_exit

send_eoi_slave:
    mov al, 0x20
    out 0xA0, al ; 向从片发送 EOI
    out 0x20, al ; 也要向主片发送 EOI

common_stub_exit:
    pop eax     ; 恢复数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa        ; 恢复通用寄存器
    add esp, 8  ; 清理错误码和中断号
    iret        ; 从中断返回

; --- 宏定义 (保持不变) ---
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push %1
    jmp isr_common_stub
%endmacro

; --- 生成 ISRs (保持不变) ---
; ... (ISR_NOERRCODE 0 到 ISR_NOERRCODE 47 的所有宏调用) ...
ISR_NOERRCODE  0
ISR_NOERRCODE  1
ISR_NOERRCODE  2
ISR_NOERRCODE  3
ISR_NOERRCODE  4
ISR_NOERRCODE  5
ISR_NOERRCODE  6
ISR_NOERRCODE  7
ISR_ERRCODE    8
ISR_NOERRCODE  9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47


; --- 添加段声明以消除链接器警告 ---
section .note.GNU-stack,"",@progbits