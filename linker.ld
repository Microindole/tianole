OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

/* 定义程序头表 (PHDRS) - Limine 需要这个 */
PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    /* 高半核：内核虚拟地址从 -2GB 开始 (0xFFFFFFFF80000000 + 2MB) */
    . = 0xFFFFFFFF80200000;

    /* 每个段都对齐到 4KB 页面边界，防止不同权限的段共享页面 */
    
    . = ALIGN(0x1000); /* 4KB 对齐 */
    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(0x1000); /* 4KB 对齐 */
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(0x1000); /* 4KB 对齐 */
    .data : {
        *(.data .data.*)
    } :data

    . = ALIGN(0x1000); /* 4KB 对齐 */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}