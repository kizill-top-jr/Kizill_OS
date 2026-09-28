kernel/

Arch-independent core.

    main.c — boot sequence. fb, Limine requests, GDT, IDT, PMM, heap,
    paging, initramfs, ELF load, shell spawn, sti.

    fb.c — framebuffer console. 8x8 bitmap font, printk,
    printk_hex, printk_dec, scroll.

    serial.c — COM1 (0x3F8) debug output.

    task.c — round-robin scheduler. Kernel tasks via task_create,
    ring3 via task_create_user. scheduler_tick also updates TSS.rsp0.
    States: READY, RUNNING, BLOCKED, DEAD.

    pmm.c — physical memory manager. Bitmap, 1 bit = 1 page (4 KiB).

    heap.c — kmalloc/kfree. Free-list allocator, grows via PMM.

    paging.c — map_page/virt_to_phys. Walks PML4->PDPT->PD->PT,
    allocates intermediate tables on demand, invlpg's after map.

    syscall.c — int 0x80 dispatcher. Frame on stack, switch on rax.
    SYS_WRITE(1), SYS_READ(0), SYS_YIELD(24), SYS_GETPID(39),
    SYS_EXIT(60), SYS_WAIT(61), SYS_CLEAR(99), SYS_EXEC(100).

    tar.c — minimal tar (ustar) parser. tar_next iterates,
    tar_find searches by name.

    elf.c — ELF64 parser + loader. Validates magic/class/machine,
    dumps header info, loads PT_LOAD segments via map_page, returns
    entry point.

    keyboard.c — full PS/2 driver. Ring buffer, scancode set 1,
    E0 prefix for arrows/Home/End/PgUp/PgDn/Ins/Del/Win/Menu,
    multimedia keys (Mute, Vol, Play, Calc). Shift/Caps/Ctrl/Alt.
    ASCII or KEY_* constants out.

Boot order: fb -> gdt -> idt -> keyboard -> pmm -> heap -> paging
-> initramfs -> ELF load -> shell.
