# Limine binaries

Limine is our bootloader for the x86_64 branch. It puts the CPU into long
mode, sets up paging, and hands us a framebuffer plus a memory map before
jumping to `_start`.

Binaries are **not** tracked in this repository. Download them once:

```bash
wget https://github.com/limine-bootloader/limine/releases/latest/download/limine-binary.tar.gz
tar -xzf limine-binary.tar.gz
cp limine-binary/limine-bios-cd.bin  .
cp limine-binary/limine-bios.sys     .
```

You need exactly two files for BIOS boot:

    limine-bios-cd.bin — boot sector for the ISO

    limine-bios.sys — the actual bootloader

For UEFI boot, add limine-uefi-cd.bin. Not used yet.

The .gitignore in the project root excludes *.bin, *.sys, and
*.EFI under limine/, so these won't end up in commits.
