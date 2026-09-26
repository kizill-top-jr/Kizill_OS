#!/bin/bash
# build userspace programs, pack into initramfs.tar

set -e

CC=x86_64-elf-gcc
USER_DIR=userspace
BUILD_DIR=build/userspace
OUT=initramfs.tar

mkdir -p $BUILD_DIR

# compile each .c in userspace to ELF
for src in $USER_DIR/*.c; do
    name=$(basename "$src" .c)
    echo "compiling $name"
    $CC -ffreestanding -nostdlib -static -O2 \
        -fno-stack-protector -mno-red-zone \
        -o $BUILD_DIR/$name.elf $src
done

# pack into tar
cd $BUILD_DIR
tar -cf ../../$OUT *.elf
cd ../..

echo "initramfs: $OUT ($(stat -c%s $OUT) bytes)"
