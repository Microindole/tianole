; boot.s - 设置 Multiboot Header 并调用 C 内核

; 定义 Multiboot Header 的常量
MBOOT_PAGE_ALIGN    equ 1<<0    ; 按页（4KB）对齐模块
MBOOT_MEM_INFO      equ 1<<1    ; 提供内存信息
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; GRUB 识别的魔数
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .text
align 4
    dd MBOOT_HEADER_MAGIC   ; GRUB Magic Number
    dd MBOOT_HEADER_FLAGS   ; Flags
    dd MBOOT_CHECKSUM       ; Checksum

; 定义一个小的栈空间
section .bss
align 16
stack_bottom:
resb 16384 ; 16 KB stack
stack_top:

section .text
global start
extern kernel_main ; 声明 C 函数 kernel_main

start:
    ; 设置栈指针
    mov esp, stack_top

    ; 调用 C 内核主函数
    call kernel_main

    ; 内核结束后，让 CPU 停机
    cli
.hang:
    hlt
    jmp .hang