#include <kernel/elf.h>
#include <kernel/fb.h>

int elf_check(const u8 *data, u64 size) {
    if (size < sizeof(struct elf64_ehdr)) return -1;

    // magic: 0x7F 'E' 'L' 'F'
    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F')
        return -1;

    // class: 2 = 64-bit
    if (data[4] != 2) return -1;

    // endianness: 1 = little-endian
    if (data[5] != 1) return -1;

    struct elf64_ehdr *h = (struct elf64_ehdr *)data;

    // machine: 0x3E = x86_64
    if (h->e_machine != 0x3E) return -1;

    // must have program headers
    if (h->e_phnum == 0) return -1;
    if (h->e_phoff + h->e_phnum * h->e_phentsize > size) return -1;

    return 0;
}

u64 elf_entry(const u8 *data, u64 size) {
    if (elf_check(data, size) != 0) return 0;
    struct elf64_ehdr *h = (struct elf64_ehdr *)data;
    return h->e_entry;
}

void elf_dump(const u8 *data, u64 size) {
    if (elf_check(data, size) != 0) {
        printk_color("elf: not a valid ELF64 x86_64\n", FB_RED);
        return;
    }

    struct elf64_ehdr *h = (struct elf64_ehdr *)data;

    printk_color("elf: valid\n", FB_GREEN);
    printk("  entry:  "); printk_hex(h->e_entry); printk("\n");
    printk("  type:   "); printk_dec(h->e_type);  printk("\n");
    printk("  phoff:  "); printk_hex(h->e_phoff); printk("\n");
    printk("  phnum:  "); printk_dec(h->e_phnum); printk("\n");
    printk("  phentsize: "); printk_dec(h->e_phentsize); printk("\n");

    for (u16 i = 0; i < h->e_phnum; i++) {
        struct elf64_phdr *ph = (struct elf64_phdr *)
            (data + h->e_phoff + i * h->e_phentsize);

        if (ph->p_type != PT_LOAD) continue;

        printk_color("  seg ", FB_YELLOW);
        printk_dec(i);
        printk(": vaddr="); printk_hex(ph->p_vaddr);
        printk(" filesz=");  printk_hex(ph->p_filesz);
        printk(" memsz=");   printk_hex(ph->p_memsz);
        printk(" flags=");
        if (ph->p_flags & PF_R) printk("R");
        if (ph->p_flags & PF_W) printk("W");
        if (ph->p_flags & PF_X) printk("X");
        printk(" off=");     printk_hex(ph->p_offset);
        printk("\n");
    }
}
