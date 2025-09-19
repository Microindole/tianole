; kernel/switch.s - The magic of context switching (Final Version)

global switch_task

; switch_task(volatile task_t* old, volatile task_t* new)
; [esp + 4] -> old_task (pointer to old TCB)
; [esp + 8] -> new_task (pointer to new TCB)

switch_task:
    ; --- Step 1: 保存参数 ---
    mov edi, [esp + 4]  ; edi = old_task
    mov esi, [esp + 8]  ; esi = new_task

    ; --- Step 2: 保存旧任务的状态 ---
    pusha
    
    ; --- 关键修正：使用正确的偏移量 8 (id:4 + state:4) ---
    ; 将当前栈顶 (esp) 保存到 old_task->kernel_stack_ptr
    mov [edi + 8], esp
    
    ; --- Step 3: 加载新任务的状态 ---
    ; 从 new_task->kernel_stack_ptr 加载新任务的栈顶指针
    mov esp, [esi + 8]

    popa

    ; --- Step 4: 返回 ---
    ; ret 会跳转到新任务栈顶的地址 (fork_trampoline 或其他)
    ret

section .note.GNU-stack,"",@progbits
