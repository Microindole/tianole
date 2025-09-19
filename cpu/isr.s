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
    
    ; --- BEGIN FIX: 正确的中断分发逻辑 ---
    ; C 函数的第一个参数 (registers_t* regs) 位于 [esp+4]
    mov edx, [esp + 4]
    ; 从 regs 结构体中获取 int_no (偏移量为 36)
    mov eax, [edx + 36]

    cmp eax, 32          ; 中断号 < 32 是 CPU 异常
    jl is_exception

    ; 中断号 >= 32 是 IRQ 或我们自定义的中断 (如 syscall)
    call irq_handler
    jmp common_stub_exit

is_exception:
    call isr_handler
    ; isr_handler 之后也会跳转到 common_stub_exit，所以这里不需要 jmp

; --- BEGIN FINAL FIX ---
; 最终、健壮的中断退出公共存根
common_stub_exit:
   pop eax     ; 清理 C 函数的参数 (regs*)
   pop eax     ; 将 ds 的值从栈上弹到 ax
   mov ds, ax  ; 恢复 ds, es, fs, gs
   mov es, ax
   mov fs, ax
   mov gs, ax

   popa        ; 恢复所有通用寄存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)

   ; --- 正确的 EOI 发送逻辑 ---
   push eax            ; 保护 eax
   mov eax, [esp + 8]  ; 获取中断号 (在栈上，位于 saved eax 和 err_code 之下)
   cmp eax, 40
   jl .master_eoi_only

.send_slave_eoi:
   mov al, 0x20
   out 0xA0, al        ; 发送 EOI 到从片

.master_eoi_only:
   mov al, 0x20
   out 0x20, al        ; 发送 EOI 到主片

   pop eax             ; 恢复 eax

   add esp, 8  ; 清理栈上的 error_code 和 int_no
   iret        ; 从中断安全返回
; --- END FINAL FIX ---

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

ISR_NOERRCODE 128  ; 用于系统调用

; --- 段声明以消除链接器警告 ---
section .note.GNU-stack,"",@progbits