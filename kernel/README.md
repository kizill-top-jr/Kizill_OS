kernel/

Arch-independent core.

    main.c — boot sequence. fb, Limine requests, GDT, IDT, PMM, heap,
    paging setup, scheduler, tasks (kernel + one ring3 user), sti.

    fb.c — framebuffer console. 8x8 bitmap font, printk,
    printk_hex, printk_dec, scroll.

    serial.c — COM1 (0x3F8) debug output. Lives before fb, works forever.

    task.c — round-robin scheduler. task_create for kernel tasks,
    task_create_user for ring 3. scheduler_tick also updates TSS.rsp0
    so the next task gets the right kernel stack on ring3->ring0.

    pmm.c — physical memory manager. Bitmap, 1 bit = 1 page (4 KiB).

    heap.c — kmalloc/kfree. Free-list allocator, grows via PMM.

    paging.c — map_page/virt_to_phys. Walks PML4->PDPT->PD->PT,
    allocates intermediate tables on demand, invlpg's after map.

    syscall.c — int 0x80 dispatcher. Read frame on stack, switch on
    rax. Currently: SYS_WRITE (1), SYS_EXIT (60). Return in rax.

Boot order: fb -> gdt -> idt -> pmm -> heap -> paging setup -> scheduler.
