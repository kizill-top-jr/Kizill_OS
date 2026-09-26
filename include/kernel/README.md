include/kernel/

    types.h — u8..u64, size_t, NULL.

    limine.h — Limine boot protocol structs + request IDs. Shared by
    main.c and pmm.c.

    task.h — task_t with is_user flag, scheduler API.

    pmm.h — page alloc/free, page size const.

    heap.h — kmalloc/kfree + stats.

    paging.h — map_page, virt_to_phys, PTE flag macros.

    Rest is 1:1 with kernel/*.c.
    EOF

cat > arch/x86_64/README.md << 'EOF'
arch/x86_64/

x86_64 long-mode code. Four subdirs:

    boot/ — entry point, linker script

    gdt/ — GDT64 + TSS (rsp0 for ring3->ring0)

    idt/ — IDT64, ISR stubs (incl. syscall at 0x80), exceptions, PIT, PS/2

    pic/ — 8259 PIC remap

Each has its own README.
