arch/x86_64/gdt/

GDT64 + TSS.

    gdt.c — sets up 7 entries: null, kcode, kdata, ucode (unused yet),
    udata (unused), TSS (two slots). Loads via lgdt, then ltr.

    gdt_load.asm — lgdt + far jump to reload CS. ltr for TSS.

TSS needed for ring3->ring0 stack switch later. Right now unused, but loaded.

Gotcha: tables live in .data b/c Limine maps .text RX only.
