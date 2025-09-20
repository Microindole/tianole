/*
 * user/link.ld -- 用户程序的链接脚本
 */

ENTRY(main) /* 入口点是 main 函数 */
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* 用户程序从虚拟地址 0x40000000 开始 */
    . = 0x40000000;

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data)
        
        /* 嵌入用户程序二进制文件 */
        . = ALIGN(4096);
        _user_program_start = .;
        INCBIN("build/program.bin")
        _user_program_end = .;

    } :data

    .bss :
    {
        *(.bss)
    }
}