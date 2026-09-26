#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <kernel/types.h>

// ELF64 header
struct elf64_ehdr {
    u8  e_ident[16];      // magic + class + data + version + os abi + pad
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} __attribute__((packed));

// program header (segment)
struct elf64_phdr {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} __attribute__((packed));

// p_type values
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

// p_flags
#define PF_X 1
#define PF_W 2
#define PF_R 4

// returns 0 if valid ELF64 x86_64, -1 otherwise
int elf_check(const u8 *data, u64 size);

// dump info to fb
void elf_dump(const u8 *data, u64 size);

// get entry point, or 0 if invalid
u64 elf_entry(const u8 *data, u64 size);

#endif
