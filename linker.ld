/*
 * linker.ld -- The definitive, corrected linker script for our kernel.
 */

ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* 内核从 1MB 物理地址开始 */
    . = 1M;

    /* .text 段：包含 multiboot 头、代码和只读数据 */
    .text :
    {
        *(.multiboot)   /* 必须是第一项！ */
        *(.text*)
        *(.rodata*)
    }

    /* .data 段：包含已初始化的数据 */
    .data :
    {
        *(.data)
    }

    /* .bss 段：包含未初始化的数据 (内核加载后会清零这片区域) */
    .bss :
    {
        *(COMMON)
        *(.bss*)
    }

    /* 定义一个符号 'end'，它指向内核文件的末尾 */
    end = .;
}