arch/x86_64/idt/

Interrupts: table, handlers, drivers.

    idt.c — 256 entries, 16 bytes each. Default = isr_stub_default,
    specific for 0/13/14/32/33/0x80. Syscall gate has DPL=3 so ring 3
    code can int 0x80.

    isr.asm — stubs. Each pushes err code (or dummy 0) + vector.
    isr_stub_timer does the task switch: save all GPRs, call
    scheduler_tick(rsp), get next task's rsp back, restore, iretq.
    isr_stub_syscall saves GPRs, calls syscall_dispatch(rsp), restores,
    iretq.

    exception.c — panic screen (red, KERNEL PANIC:).

    timer.c — PIT init, 100 Hz. timer_handler is a stub; the switch
    happens in asm.

    keyboard.c — PS/2 read from 0x60. Still a stub, no ASCII yet.

Watch out: isr_common must NOT handle vector 32 (timer), it's done
separately in isr_stub_timer.
