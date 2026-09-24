kernel/

Arch-independent core.

    main.c — boot sequence. fb, Limine requests, GDT, IDT, PMM, heap,
    scheduler, tasks, sti.

    fb.c — framebuffer console. 8x8 bitmap font, printk,
    printk_hex, printk_dec, scroll.

    serial.c — COM1 (0x3F8) debug output. Lives before fb, works forever.

    task.c — round-robin scheduler. task_create builds an initial
    stack frame for iretq.

    pmm.c — physical memory manager. Bitmap, 1 bit = 1 page (4 KiB).

    heap.c — kmalloc/kfree. Free-list allocator, one page from PMM at
    init, grows on demand.

Boot order matters: fb -> gdt -> idt -> pmm -> heap -> scheduler.
