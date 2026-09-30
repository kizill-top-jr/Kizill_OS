# Kizill_OS

Hobby OS from scratch. x86_64, no Linux/BSD base, no Buildroot, no Yocto.
Just C, NASM, and stubbornness.

## Status: v0.8.1

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

Doesn't work yet:
- fork (only exec spawns a child)
- signals, kill
- real drivers (disk, mouse, network)
- FS beyond initramfs
- SMP, APIC

## Build

Requirements: x86_64-elf-gcc, x86_64-elf-ld, nasm, xorriso, qemu-system-x86_64,
plus Limine binaries (see limine/README.md).

Steps:

./tools/make_initramfs.sh

make -f Makefile.64 clean && make -f Makefile.64

rm -rf iso64 && mkdir -p iso64/boot
cp kernel.elf iso64/boot/
cp initramfs.tar iso64/boot/
cp limine.conf iso64/
cp limine/limine-bios.sys limine/limine-bios-cd.bin iso64/

xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table iso64/ -o kizill64.iso

qemu-system-x86_64 -cdrom kizill64.iso -m 512M -serial stdio

## Layout

Each dir has its own README:

- arch/x86_64/ — boot, gdt, idt, pic
- kernel/ — fb, serial, task, pmm, heap, paging, syscall, tar, elf, keyboard
- include/kernel/ — headers, mirrors kernel/
- userspace/ — user programs (sh.c, hello.c)
- tools/ — make_initramfs.sh
- limine/ — bootloader binaries (gitignored)
- docs/ — notes, wip

## Versioning

Semver: MAJOR.MINOR.PATCH.

- v0.8.0 — per-process PML4, CR3 switch
- v0.8.1 — zombie reap, slot reuse, non-blocking wait

## License

GPLv3. See LICENSE.
