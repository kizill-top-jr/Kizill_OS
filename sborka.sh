#!/bin/bash
set -e

./tools/make_initramfs.sh

make -f Makefile.64 clean
make -f Makefile.64

rm -rf iso64
mkdir -p iso64/boot

cp kernel.elf iso64/boot/
cp initramfs.tar iso64/boot/
cp limine.conf iso64/
cp limine/limine-bios.sys iso64/
cp limine/limine-bios-cd.bin iso64/

xorriso -as mkisofs -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    iso64/ -o kizill64.iso

# Create disk image if it doesn't exist
if [ ! -f disk.img ]; then
    echo "creating empty disk.img..."
    dd if=/dev/zero of=disk.img bs=1M count=10
fi

qemu-system-x86_64 -cdrom kizill64.iso -m 512M -serial stdio -drive file=disk.img,format=raw,if=ide
