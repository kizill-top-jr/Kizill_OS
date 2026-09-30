# arch/x86_64/idt/

Interrupts: table, handlers, drivers.

- idt.c — 256 entries, 16 bytes each. Vectors:
  - 0 (#DE), 8 (#DF), 13 (#GP), 14 (#PF) — exceptions
  - 32 (IRQ0 timer), 33 (IRQ1 keyboard)
  - 0x80 — syscall gate, DPL=3, int 0x80 from ring 3
- isr.asm — stubs. Each pushes err code (or dummy 0) + vector.
  - isr_stub_timer saves GPRs, calls scheduler_tick(rsp),
    restores next task's rsp, iretq.
  - isr_stub_syscall saves GPRs, calls syscall_dispatch(rsp),
    mov rsp, rax (may be a different task), iretq.
- exception.c — panic screen (red, KERNEL PANIC:), CR2 for #PF.
- timer.c — PIT init, 100 Hz, g_ticks counter.
