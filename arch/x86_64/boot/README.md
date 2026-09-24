arch/x86_64/boot/

Entry point. Limine drops us here in long mode, paging on, HHDM set up.

    boot.asm — _start. Enables SSE (Limine doesn't), zeroes .bss,
    calls kmain. Never returns.

    linker.ld — puts kernel at 0xFFFFFFFF80000000, exports
    __bss_start, __bss_end, __kernel_end.

__kernel_end is what PMM uses to know where free memory starts.
