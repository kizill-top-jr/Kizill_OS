include/kernel/

    types.h — u8..u64, size_t, NULL.

    limine.h — Limine boot protocol structs + request IDs.

    task.h — task_t with state/exit_code/is_user, scheduler API.

    pmm.h — page alloc/free, PAGE_SIZE.

    heap.h — kmalloc/kfree + stats.

    paging.h — map_page, virt_to_phys, PTE flags.

    tar.h — tar_entry_t, tar_next, tar_find.

    elf.h — elf64 structs, elf_check/elf_dump/elf_load.

    keyboard.h — keyboard_init/handler/getchar + KEY_* constants.
    EOF

cat > arch/x86_64/idt/README.md << 'EOF'
arch/x86_64/idt/

    idt.c — 256 entries, 16 bytes each. Vectors:

        0 (#DE), 8 (#DF), 13 (#GP), 14 (#PF) — exceptions

        32 (IRQ0 timer), 33 (IRQ1 keyboard)

        0x80 — syscall gate, DPL=3, int 0x80 from ring 3

    isr.asm — stubs. Each pushes err code (or dummy 0) + vector.

        isr_stub_timer saves GPRs, calls scheduler_tick(rsp), restores
        next task's rsp, iretq.

        isr_stub_syscall saves GPRs, calls syscall_dispatch(rsp),
        may return different rsp for task switch, iretq.

    exception.c — panic screen (red, KERNEL PANIC:).

    timer.c — PIT init, 100 Hz.
    EOF

cat > userspace/README.md << 'EOF'
userspace/

User programs, built to static ELF64 by tools/make_initramfs.sh,
packed into initramfs.tar, loaded by kernel's ELF loader.

    sh.c — Kizill_OS shell. read/write/exec/yield/clear/exit.
    Built-in: help, clear, echo X, hello, exit.

    hello.c — prints "hello from ELF!" and exits with code 33.

No libc. Raw int 0x80 syscalls.
Syscall ABI

rax = number, rdi/rsi/rdx = args, result in rax.

    0 READ(fd, buf, len)

    1 WRITE(fd, buf, len)

    24 YIELD()

    39 GETPID()

    60 EXIT(code)

    61 WAIT() → exit code or -1

    99 CLEAR()

    100 EXEC(name) → 0 on success

Build
bash

../tools/make_initramfs.sh

