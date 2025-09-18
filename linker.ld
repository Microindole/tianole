/*
 * linker.ld -- The definitive linker script for our kernel.
 */

ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* Start at 1M */
    . = 1M;

    /* The first section is the multiboot header */
    .text :
    {
        *(.multiboot)
        *(.text*)
    } :text

    .rodata :
    {
        *(.rodata*)
    } :rodata

    .data :
    {
        *(.data)
    } :data

    .bss :
    {
        *(.bss)
    } :bss

    end = .; /* 'end' 标记了 BSS 段的结束，即内核的结束 */
}

/* Define program headers to ensure correct memory permissions */
PHDRS
{
    text PT_LOAD FLAGS(5); /* R-- E-X */
    rodata PT_LOAD FLAGS(4); /* R-- --- */
    data PT_LOAD FLAGS(6); /* RW- --- */
    bss PT_LOAD FLAGS(6); /* RW- --- */
}