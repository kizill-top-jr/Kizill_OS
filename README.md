# Kizill_OS

Hobby OS from scratch. x86_64, no Linux/BSD base, no Buildroot, no Yocto.
Just C, NASM, and stubbornness.

## Status: v0.7

**Works:**
- Boot via Limine into long mode
- GDT64 + TSS (TSS.rsp0 wired for ring3->ring0)
- IDT64 + PIC + PIT (100 Hz)
- Framebuffer text output (8x8 font)
- Serial debug output (COM1)
- Preemptive round-robin scheduler (IRQ0 driven)
- Physical memory manager (bitmap, 4 KiB pages)
- Heap: kmalloc/kfree
- 4-level paging with map_page() helper
- Ring 3 + int 0x80 syscalls
  - write, read, yield, getpid, wait, exit, clear, exec
- Full PS/2 keyboard driver
  - Shift, Caps Lock, Ctrl, Alt
  - Arrow keys, Home/End, PgUp/PgDn, Ins/Del
  - Win/Menu keys
  - Multimedia keys (Mute, Vol, Play, Calc, ...)
- initramfs via Limine module (tar format)
- ELF64 loader (PT_LOAD segments, BSS, user paging)
- User programs written in C, built by tools/make_initramfs.sh
- Interactive shell (sh.elf) in ring 3
- hello.elf as external exec target

**Doesn't work (yet):**
- fork (only exec spawns a child)
- Signals, kill, waitpid with specific pid
- Real drivers (disk, mouse, network)
- FS beyond initramfs
- SMP, APIC

## Build

Need: `x86_64-elf-gcc`, `x86_64-elf-ld`, `nasm`, `xorriso`, `qemu-system-x86_64`,
plus Limine binaries (grab 'em per `limine/README.md`).

```bash
./tools/make_initramfs.sh

make -f Makefile.64 clean && make -f Makefile.64

rm -rf iso64 && mkdir -p iso64/boot
cp kernel.elf iso64/boot/
cp initramfs.tar iso64/boot/
cp limine.conf iso64/
cp limine/limine-bios.sys limine/limine-bios-cd.bin iso64/

xorriso -as mkisofs -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    iso64/ -o kizill64.iso

qemu-system-x86_64 -cdrom kizill64.iso -m 512M -serial stdio

Layout

Each dir has its own README. TL;DR:

    arch/x86_64/ — boot, gdt, idt, pic

    kernel/ — fb, serial, task, pmm, heap, paging, syscall, tar, elf, keyboard

    include/kernel/ — headers, mirrors kernel/

    userspace/ — user programs (sh.c, hello.c)

    tools/ — make_initramfs.sh

    limine/ — bootloader binaries (gitignored)

    docs/ — notes, wip

License

GPLv3. See LICENSE.
