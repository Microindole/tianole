; kernel/switch.s - The magic of context switching

global switch_task

; switch_task(registers_t* old, registers_t* new)
; C 函数调用时，参数从右到左压栈，所以：
; [esp + 4] -> old (pointer to old task's registers)
; [esp + 8] -> new (pointer to new task's registers)

switch_task:
    ; --- 关键修复 1: 先保存参数 ---
    ; 在 pusha 修改 esp 之前，先把参数从栈上取到寄存器里
    mov edi, [esp + 4]  ; edi = old
    mov esi, [esp + 8]  ; esi = new

    ; --- Step 1: 保存旧任务的状态 ---
    
    ; 保存通用寄存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    pusha

    ; pusha 保存完后，esp 指向了栈顶。这个 esp 就是旧任务被中断时的内核栈顶。
    ; 我们需要把这个栈顶地址保存到 old->registers.esp 中。
    ; --- 关键修复 2: 使用正确的偏移量 ---
    ; 在 isr.h 的 registers_t 结构体中, esp 字段的偏移量是 16.
    ; (ds(4) + edi(4) + esi(4) + ebp(4) = 16)
    mov [edi + 16], esp
    
    ; --- Step 2: 加载新任务的状态 ---

    ; 从 new->registers.esp 加载新任务的栈顶指针
    mov esp, [esi + 16]

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