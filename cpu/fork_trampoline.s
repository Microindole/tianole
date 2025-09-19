section .text
global fork_trampoline

fork_trampoline:
    ; 我们从 switch_task 的 'ret' 指令跳转到这里。
    ; 此时：
    ;   1. 子进程的通用寄存器 (eax, ecx...) 已经被 switch_task 的 'popa' 正确恢复。
    ;   2. eax 的值也已经是 0。
    ;   3. ESP 指向了我们当初在 syscall_fork 中构造的 `registers_t` 中断帧的 'ds' 字段。
    
    ; 我们要做的就是完成 `iret` 之前的最后几个步骤。
    
    ; 弹出 ds 并恢复它
    pop ds
    
    ; es, fs, gs 如果你也用了，也要在这里恢复
    
    ; 跳过栈上为 pusha 帧预留的 32 字节空间，因为那些寄存器已经被恢复了。
    add esp, 32
    
    ; 清理栈上的 error_code 和 int_no
    add esp, 8
    
    ; 执行中断返回。这将从栈上恢复 EIP, CS, EFLAGS 等，让子进程完美启动。
    iret