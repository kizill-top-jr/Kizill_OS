#include <kernel/elf.h>
#include <kernel/fb.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>

// HHDM offset -- Limine maps all physical memory at this base
#define HHDM_BASE 0xFFFF800000000000ULL

int elf_check(const u8 *data, u64 size) {
    if (size < sizeof(struct elf64_ehdr)) return -1;

    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F')
        return -1;
    if (data[4] != 2) return -1;  // 64-bit
    if (data[5] != 1) return -1;  // little-endian

    struct elf64_ehdr *h = (struct elf64_ehdr *)data;

    if (h->e_machine != 0x3E) return -1;  // x86_64
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
    printk("  entry:  ");     printk_hex(h->e_entry);    printk("\n");
    printk("  type:   ");     printk_dec(h->e_type);     printk("\n");
    printk("  phoff:  ");     printk_hex(h->e_phoff);    printk("\n");
    printk("  phnum:  ");     printk_dec(h->e_phnum);    printk("\n");
    printk("  phentsize: ");  printk_dec(h->e_phentsize);printk("\n");

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

static inline u64 round_up_page(u64 v) {
    return (v + 0xFFF) & ~0xFFFULL;
}

u64 elf_load(const u8 *data, u64 size) {
    if (elf_check(data, size) != 0) return 0;

    struct elf64_ehdr *h = (struct elf64_ehdr *)data;

    for (u16 i = 0; i < h->e_phnum; i++) {
        struct elf64_phdr *ph = (struct elf64_phdr *)
            (data + h->e_phoff + i * h->e_phentsize);

        if (ph->p_type != PT_LOAD) continue;
        if (ph->p_memsz == 0) continue;

        u64 vaddr_start = ph->p_vaddr & ~0xFFFULL;
        u64 vaddr_end   = round_up_page(ph->p_vaddr + ph->p_memsz);
        u64 npages = (vaddr_end - vaddr_start) / 0x1000;

        u64 flags = PTE_PRESENT | PTE_USER;
        if (ph->p_flags & PF_W) flags |= PTE_WRITE;

        // allocate and map each page, zero them
        for (u64 p = 0; p < npages; p++) {
            void *page = pmm_alloc();
            if (!page) return 0;

            u64 phys = (u64)page - HHDM_BASE;
            u64 va = vaddr_start + p * 0x1000;

            if (map_page(va, phys, flags) != 0) return 0;

            u8 *dst = (u8 *)page;
            for (u64 j = 0; j < 0x1000; j++) dst[j] = 0;
        }

        // copy file data into mapped pages via HHDM
        u64 src_off = ph->p_offset;
        u64 dst_addr = ph->p_vaddr;
        u64 remaining = ph->p_filesz;

        while (remaining > 0) {
            u64 page_off = dst_addr & 0xFFF;
            u64 chunk = 0x1000 - page_off;
            if (chunk > remaining) chunk = remaining;

            u64 phys = virt_to_phys(dst_addr);
            if (phys == 0) return 0;

            u8 *dst = (u8 *)(HHDM_BASE + phys);
            const u8 *src = data + src_off;

            for (u64 j = 0; j < chunk; j++) dst[j] = src[j];

            dst_addr  += chunk;
            src_off   += chunk;
            remaining -= chunk;
        }
        // BSS (memsz > filesz) already zeroed above
    }

    return h->e_entry;
}
