# Kizill_OS

A hobby OS written from scratch. Currently x86_64, boots via Limine.

## Status

`main` — x86_64, work in progress.
`legacy-32bit` — archived 32-bit version (GRUB + Multiboot1). Kept for reference.

## What works

- Boot via Limine (x86_64 long mode)
- Framebuffer fill

## What's next

- Framebuffer text output
- GDT64 + TSS
- IDT64 + ISRs
- Preemptive scheduler
- Memory management
- Userspace (ring 3)

## Build

Requires `x86_64-elf-gcc`, `nasm`, `xorriso`, and Limine binaries.

```bash
make -f Makefile.64
mkdir -p iso64/boot
cp kernel.elf iso64/boot/
cp limine.conf iso64/
cp limine/limine-bios.sys limine/limine-bios-cd.bin iso64/
xorriso -as mkisofs -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    iso64/ -o kizill64.iso
qemu-system-x86_64 -cdrom kizill64.iso -m 512M
```

```privet
privet me Y0tal1nk
