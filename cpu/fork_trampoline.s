; File: cpu/fork_trampoline.s

section .text
global fork_trampoline

fork_trampoline:
    ; 恢复段寄存器
    pop ds
    
    ; 跳过栈上为 pusha 帧预留的 32 字节空间
    add esp, 32
    
    ; 清理栈上的 error_code 和 int_no
    add esp, 8
    
    ; 执行中断返回
    iret