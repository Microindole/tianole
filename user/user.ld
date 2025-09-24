/* user/user.ld - Linker script for user programs */
ENTRY(main)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x400000; /* Start at 4MB virtual address */
    .text :
    {
        *(.text)
    }
    .data :
    {
        *(.data)
    }
    .bss :
    {
        *(.bss)
    }
}