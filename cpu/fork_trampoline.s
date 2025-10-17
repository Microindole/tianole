section .text
global fork_trampoline

fork_trampoline:
    ; 此时，通用寄存器已被 switch_task 中的 popa 恢复。
    ; esp 指向新栈上的 iret 帧，是有效的。
    ; 但 ebp 指向父进程的栈，是无效的。
    ; 我们将 ebp 设置为 0，这会安全地终止调用栈链。
    ; 任何在此之后被调用的 C 函数都能在此基础上建立自己的新栈帧。
    mov ebp, esp

    ; 现在可以安全地执行常规的中断返回流程
    pop ds
    add esp, 32  ; 跳过栈上的 GPRs
    add esp, 8   ; 跳过中断号和错误码
    iret         ; 安全返回

section .note.GNU-stack,"",@progbits