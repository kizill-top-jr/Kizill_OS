# Kizill_OS

A 32-bit hobby operating system written from scratch in C and NASM.
No Buildroot, no Yocto, no LFS — everything from zero.

## Status

**Working:**
- Multiboot1 bootloader via GRUB
- Protected mode, GDT
- 32-bit paging with identity mapping (first 4 MB)
- Bitmap-based physical memory allocator (`kmalloc` / `kfree`)
- IDT + PIC remap
- Exception handling (#DE, #GP, #PF) with `KERNEL PANIC` screen
- PIT timer (100 Hz), PS/2 keyboard
- Cooperative round-robin scheduler (main, task1, task2)
- Interactive shell: `clear`, `help`, `version`, `echo`, `panic`, `reboot`

**Not implemented yet:**
- Preemptive multitasking (timer-driven)
- IPC
- Userspace / ring 3
- System calls
- File system
- Package manager / repository
- x86_64 long mode, ARM

## Build

Requirements:
- `i686-elf-gcc`, `i686-elf-ld` (or crossdev on Gentoo)
- `nasm`
- `grub-mkrescue`, `xorriso`
- `qemu-system-x86_64` for testing

```bash
make clean && make
rm -rf iso && mkdir -p iso/boot/grub
cp kernel.elf iso/boot/kernel.elf
cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0
menuentry "Kizill_OS" {
    multiboot /boot/kernel.elf
    boot
}
EOF
grub-mkrescue -o kizill.iso iso/
qemu-system-x86_64 -cdrom kizill.iso
