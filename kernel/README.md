# kernel/

Arch-independent core.

- main.c — boot sequence. fb, Limine requests, GDT, IDT, PMM, heap,
  paging, initramfs, ELF load, shell spawn, sti.
- fb.c — framebuffer console. 8x8 bitmap font, printk, printk_hex,
  printk_dec, scroll, backspace handling.
- serial.c — COM1 (0x3F8) debug output.
- task.c — round-robin scheduler. States: READY, RUNNING, BLOCKED,
  ZOMBIE, DEAD. alloc_slot reuses DEAD slots. task_try_reap frees
  child PML4 and marks slot DEAD. switch_cr3_to handles per-task
  address spaces.
- pmm.c — physical memory manager. Bitmap, 1 bit = 1 page (4 KiB).
- heap.c — kmalloc/kfree. Free-list allocator, grows via PMM.
- paging.c — map_page / map_page_in. Walks PML4->PDPT->PD->PT,
  allocates intermediate tables on demand, invlpg after map.
  pml4_create copies kernel entries 256..511. pml4_destroy walks
  user entries 0..255 only (kernel half is shared).
- syscall.c — int 0x80 dispatcher. Frame on stack, switch on rax.
  SYS_WRITE(1), SYS_READ(0), SYS_YIELD(24), SYS_GETPID(39),
  SYS_EXIT(60), SYS_WAIT(61, non-blocking), SYS_CLEAR(99),
  SYS_EXEC(100).
- tar.c — minimal tar (ustar) parser.
- elf.c — ELF64 parser + loader. elf_load_in(pml4, ...) writes
  PT_LOAD segments into a specific PML4.
- keyboard.c — full PS/2 driver. Ring buffer, scancode set 1,
  E0 prefix for arrows/Home/End/PgUp/PgDn/Ins/Del/Win/Menu,
  multimedia keys. Shift/Caps/Ctrl/Alt.

Boot order: fb -> gdt -> idt -> keyboard -> pmm -> heap -> paging
-> initramfs -> ELF load -> shell.
