arch/x86_64/

x86_64 long-mode code. Four subdirs, each with one job:

    boot/ — entry point, linker script

    gdt/ — GDT64 + TSS

    idt/ — IDT64, ISR stubs, exceptions, PIT, PS/2

    pic/ — 8259 PIC remap

Each has its own README.
