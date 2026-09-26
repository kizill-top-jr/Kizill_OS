arch/x86_64/gdt/

GDT64 + TSS.

    gdt.c — 7 entries: null, kcode, kdata, ucode, udata, TSS (two slots).
    Loads via lgdt, then ltr. Exports tss_set_rsp0() so scheduler
    can point TSS.rsp0 at the next task's kernel stack.

    gdt_load.asm — lgdt + far jump to reload CS. ltr for TSS.

TSS.rsp0 is the stack CPU jumps to on ring3->ring0. Scheduler updates it
on every switch so each user task gets its own kernel stack on interrupt.

Gotcha: tables live in .data b/c Limine maps .text RX only.
