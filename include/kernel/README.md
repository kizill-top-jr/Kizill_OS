# include/kernel/

- types.h — u8..u64, size_t, NULL.
- limine.h — Limine boot protocol structs + request IDs.
- task.h — task_t with cr3, state, waiting_for_pid, exit_code.
  States and API: scheduler_init, task_create, task_create_user,
  task_try_reap, task_yield, task_block_current.
- pmm.h — page alloc/free, PAGE_SIZE.
- heap.h — kmalloc/kfree + stats.
- paging.h — map_page, map_page_in, virt_to_phys, virt_to_phys_in,
  pml4_create, pml4_destroy, PTE flags.
- tar.h — tar_entry_t, tar_next, tar_find.
- elf.h — elf64 structs, elf_check, elf_dump, elf_load, elf_load_in.
- keyboard.h — keyboard_init/handler/getchar + KEY_* constants.
