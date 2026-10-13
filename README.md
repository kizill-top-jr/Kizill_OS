# Kizill_OS

Hobby OS from scratch. x86_64, no Linux/BSD base, no Buildroot, no Yocto.
Just C, NASM, and stubbornness.

## Status: v0.9.0

Works:
- Boot via Limine into long mode
- GDT64 + TSS (rsp0 wired for ring3->ring0)
- IDT64 + PIC + PIT (100 Hz)
- Framebuffer text output (8x8 font, backspace handled)
- Serial debug output (COM1)
- Preemptive round-robin scheduler (IRQ0 driven)
- Physical memory manager (bitmap, 4 KiB pages)
- Heap: kmalloc/kfree
- 4-level paging with map_page / map_page_in
- Per-process PML4 (each user task has its own address space)
- CR3 switch on task switch
- Ring 3 + int 0x80 syscalls (write, read, yield, getpid, wait, exit, clear, exec)
- Full PS/2 keyboard driver (Shift, Caps, arrows, multimedia)
- initramfs via Limine module (tar format)
- ELF64 loader (PT_LOAD segments, BSS, per-PML4 mapping)
- User programs written in C, built by tools/make_initramfs.sh
- Interactive shell (sh.elf) in ring 3
- Task lifecycle: zombie, reap, slot reuse (PID cycling)
- Non-blocking wait, shell polls with yield
- fork (naive page copy, no CoW yet)

Doesn't work yet:
- signals, kill
- real drivers (disk, mouse, network)
- FS beyond initramfs
- SMP, APIC

## Build

Requirements: x86_64-elf-gcc, x86_64-elf-ld, nasm, xorriso, qemu-system-x86_64,
plus Limine binaries (see limine/README.md).

Steps:

./sborka.sh

## Layout

Each dir has its own README:

- arch/x86_64/ boot, gdt, idt, pic
- kernel/ fb, serial, task, pmm, heap, paging, syscall, tar, elf, keyboard
- include/kernel/ headers, mirrors kernel/
- userspace/ user programs (sh.c, hello.c)
- tools/ make_initramfs.sh
- limine/ bootloader binaries (gitignored)
- docs/ notes, wip

## Versioning

Semver: MAJOR.MINOR.PATCH.

- v0.8.0 - per-process PML4, CR3 switch
- v0.8.1 - zombie reap, slot reuse, non-blocking wait
- v0.9.0 - fork, about/uname commands, build script

## License

GPLv3
