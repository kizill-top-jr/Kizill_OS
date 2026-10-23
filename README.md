<div align="center">
  <img src="logo.png" alt="Kizill_OS Logo" width="300">

  # Kizill_OS

  **Grows where nothing else can.**

  [![Version](https://img.shields.io/badge/version-v0.9.2-red.svg)](#)
  [![License](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE)
  [![Architecture](https://img.shields.io/badge/arch-x86__64-lightgrey.svg)](#)
 
  A hobby operating system written from scratch. No Linux/BSD base, no Buildroot, no Yocto. Just C, NASM, and stubbornness.
</div>

---

## 📖 About

Kizill_OS is an independent x86_64 operating system built completely from scratch. The goal is to understand how computers work under the hood, without relying on decades of legacy code. It features its own boot process, memory manager, scheduler, and userspace.

## ⚙️hat Works (v0.9.2)

* **Boot:** Limine bootloader, Long mode, HHDM.
* **CPU:** GDT64, TSS, IDT64, IST for Double Fault.
* **Interrupts:** PIC (remapped), PIT (100 Hz).
* **Memory:** Physical Memory Manager (bitmap + refcount), Heap (kmalloc/kfree), 4-level Paging.
* **Processes:** Preemptive multitasking (Round-Robin), Ring 3 isolation, Syscalls (`int 0x80`).
* **Userspace:** ELF64 loader, `initramfs` (tar), `fork` (naive copy), `exec`/`spawn`, `wait`.
* **Drivers:** Framebuffer (32bpp), PS/2 Keyboard (E0 keys, multimedia).
* **Security:** SMEP enabled, `copy_from_user`/`copy_to_user` validations.
* **Tools:** Interactive shell in Ring 3 (`sh.elf`).

## What Doesn't Work Yet

* CoW (Copy-on-Write) for `fork`.
* Real File System (VFS, disk drivers).
* Signals (`kill`, `SIGKILL`).
* SMP (Multi-core), APIC/IOAPIC.
* Network stack.

## 🛠 Build Instructions

**Requirements:** `x86_64-elf-gcc`, `x86_64-elf-ld`, `nasm`, `xorriso`, `qemu-system-x86_64`, and Limine binaries (see `limine/README.md`).

To build and run the OS, simply execute the build script:

```bash
./sborka.sh
```

This script will compile the kernel, build the userspace programs, create the initramfs.tar, generate the ISO, and launch it in QEMU.
Project Structure
text

kizill-os/
├── arch/          # x86_64 specific code (boot, gdt, idt, pic)
├── kernel/        # Architecture-independent core (mm, sched, fs, syscall)
├── include/       # Headers (kernel/ and uapi/)
├── userspace/     # Ring 3 programs (shell, hello, tests)
├── tools/         # Build scripts (make_initramfs.sh)
└── docs/          # Design decisions and documentation

🧪 Testing

The kernel includes an internal testing suite (ktest.c) and a userspace test program (test.elf).
To run kernel tests, uncomment the debug pause in kernel/main.c.
To run userspace tests, type test in the shell.
📜 License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](https://license/) file for details.

By Kizill_TOP.
