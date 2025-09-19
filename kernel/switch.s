; kernel/switch.s - The magic of context switching

global switch_task

; switch_task(registers_t* old, registers_t* new)
; C 函数调用时，参数从右到左压栈，所以：
; [esp + 4] -> old (pointer to old task's registers)
; [esp + 8] -> new (pointer to new task's registers)

switch_task:
    ; --- Step 1: 保存旧任务的状态 ---
    
    ; 获取指向 old->registers 的指针
    mov edi, [esp + 4]

    ; 保存通用寄存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    ; 注意：这里的 edi 已经是 old 的地址，但 pusha 会保存切换前的原始值
    pusha

    ; pusha 保存完后，esp 指向了栈顶。这个 esp 就是旧任务被中断时的内核栈顶。
    ; 我们需要把这个栈顶地址保存到 old->registers.esp 中。
    ; pusha 压入了 8 个 4 字节的寄存器，所以 old->registers 的地址 edi 
    ; 现在相对于 esp 的偏移是 32。
    mov [edi + 4*7], esp  ; esp 对应 registers_t 中的 esp 字段 (第 8 个，索引 7)
    
    ; --- Step 2: 加载新任务的状态 ---

    ; 获取指向 new->registers 的指针
    mov esi, [esp + 8]

    ; 从 new->registers.esp 加载新任务的栈顶指针
    mov esp, [esi + 4*7]

    ; 从新任务的栈中恢复所有通用寄存器
    popa

    ; --- Step 3: 返回 ---
    ; ret 指令会从当前栈顶弹出一个地址，并跳转到那里。
    ; 由于我们已经切换到了新任务的栈，栈顶存放的正是新任务上次执行到的
    ; EIP (或者，对于新创建的任务，是我们手动设置的函数入口地址)。
    ; 这一步就完成了任务的切换！
    ret

; --- 添加段声明以消除链接器警告 ---
section .note.GNU-stack,"",@progbits