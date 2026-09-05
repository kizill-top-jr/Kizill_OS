# Makefile - build Kizill_OS kernel.elf
CC = i686-elf-gcc
LD = i686-elf-ld
ASM = nasm
CFLAGS = -ffreestanding -O2 -Wall -Wextra -nostdlib -Iinclude -m32
LDFLAGS = -T kernel/linker.ld -nostdlib

OBJS = boot/start.o kernel/kernel.o kernel/idt.o kernel/timer.o \
       kernel/keyboard.o boot/isr.o kernel/pic.o kernel/shell.o \
       kernel/memory.o

all: kernel.elf

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

boot/start.o: boot/start.asm
	$(ASM) -f elf32 boot/start.asm -o boot/start.o

boot/isr.o: boot/isr.asm
	$(ASM) -f elf32 boot/isr.asm -o boot/isr.o

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f boot/*.o kernel/*.o kernel.elf

run: kernel.elf
	qemu-system-x86_64 -kernel kernel.elf
