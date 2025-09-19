; cpu/paging.s

global load_page_directory
global enable_paging

; 加载页目录的物理地址到 CR3 寄存器
; C 调用: void load_page_directory(page_directory_t* dir);
load_page_directory:
    mov eax, [esp + 4]  ; 获取参数 (页目录的地址)
    mov cr3, eax        ; 将地址加载到 CR3
    ret

; 开启分页
; 通过设置 CR0 寄存器的第31位 (PG)
enable_paging:
    mov eax, cr0
    or  eax, 0x80000000 ; Set bit 31
    mov cr0, eax
    ret

; --- 修复链接器警告：添加段声明 ---
section .note.GNU-stack,"",@progbits
