ENTRY(_start)

SECTIONS {
    . = 0xffffffff80000000;

    .limine_requests_start : ALIGN(4096) { KEEP(*(.limine_requests_start)) }
    .limine_requests       : ALIGN(4096) { KEEP(*(.limine_requests)) }
    .limine_requests_end   : ALIGN(4096) { KEEP(*(.limine_requests_end)) }

    .text   : ALIGN(4096) { *(.text) }
    .rodata : ALIGN(4096) { *(.rodata) }
    .data   : ALIGN(4096) { *(.data) }

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    . = ALIGN(4096);
    __kernel_end = .;
    }
}
