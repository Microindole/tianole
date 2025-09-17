ENTRY(start) /* 设置内核的入口点是 kernel_main 函数 */
SECTIONS
{
    /* 内核加载到物理内存1MB的位置 */
    . = 1M; 
    
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}