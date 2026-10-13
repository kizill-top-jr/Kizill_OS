# include/kernel/

- types.h - u8..u64, size_t, NULL.
- limine.h - Limine boot protocol structs + request IDs.
- gdt.h - gdt_init, tss_set_rsp0.
- idt.h - idt_init.
- pic.h - pic_remap, pic_unmask_timer.
- fb.h - framebuffer console + color constants + printk_*.
- serial.h - serial_init/puts/putc/hex/dec.
- exception.h - struct exception_frame + exception_dispatch.
- task.h - task_t with cr3, state, waiting_for_pid, exit_code.
  API: scheduler_init, task_create, task_create_user, task_fork,
  task_try_reap, task_yield, task_block_current, task_current_pid,
  task_current_cr3.
- pmm.h - page alloc/free, refcount (pmm_ref_inc/pmm_ref_dec),
  PAGE_SIZE, PMM_MAX_PAGES.
- heap.h - kmalloc/kfree + stats.
- paging.h - map_page, map_page_in, virt_to_phys, virt_to_phys_in,
  pml4_create, pml4_destroy, pml4_copy_for_fork, pte_for_va,
  validate_user_ptr, PTE flags (PRESENT, WRITE, USER, HUGE, COW, NX).
- tar.h - tar_entry_t, tar_next, tar_find.
- elf.h - elf64 structs, elf_check, elf_dump, elf_load, elf_load_in.
- keyboard.h - keyboard_init/handler/getchar + KEY_* constants.
