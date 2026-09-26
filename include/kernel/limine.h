#ifndef KERNEL_LIMINE_H
#define KERNEL_LIMINE_H

#include <kernel/types.h>

#define LIMINE_FRAMEBUFFER_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x9d5827dcd881dd75, 0xa3148604f6fab11b \
}
#define LIMINE_MEMMAP_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x67cf3d9d378a806f, 0xe304acdfc50c3c62 \
}
#define LIMINE_HHDM_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x48dcf1cb8ad2b852, 0x63984e959a98244b \
}
#define LIMINE_BASE_REVISION(n) { \
    0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (n) \
}
#define LIMINE_REQUESTS_START_MARKER { \
    0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, \
    0x785c6ed5e089c9e0, 0x3a5c0e8d7d9b3b1a \
}
#define LIMINE_REQUESTS_END_MARKER { \
    0xa85c461c66038e13, 0x96a6f5b5dfb8f9a0, \
    0x7b8c7f6e5d4c3b2a, 0x1908a7b6c5d4e3f2 \
}

struct limine_framebuffer {
    void *address; u64 width; u64 height; u64 pitch; u16 bpp;
    u8 memory_model, red_mask_size, red_mask_shift;
    u8 green_mask_size, green_mask_shift;
    u8 blue_mask_size, blue_mask_shift;
    u8 unused[7]; u64 edid_size; void *edid;
};

struct limine_framebuffer_response {
    u64 revision;
    u64 framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

struct limine_framebuffer_request {
    u64 id[4];
    u64 revision;
    struct limine_framebuffer_response *response;
};

struct limine_memmap_entry {
    u64 base;
    u64 length;
    u64 type;
};

struct limine_memmap_response {
    u64 revision;
    u64 entry_count;
    struct limine_memmap_entry **entries;
};

struct limine_memmap_request {
    u64 id[4];
    u64 revision;
    struct limine_memmap_response *response;
};

struct limine_hhdm_response {
    u64 revision;
    u64 offset;
};

struct limine_hhdm_request {
    u64 id[4];
    u64 revision;
    struct limine_hhdm_response *response;
};

// ---- modules ----
struct limine_file {
    u64 revision;
    u64 address;
    u64 size;
    char *path;
    char *cmdline;
    u32 media_type;
    u32 unused;
    u32 tftp_ip;
    u32 tftp_port;
    u32 partition_index;
    u32 mbr_disk_id;
    u64 gpt_disk_uuid;
    u64 gpt_part_uuid;
    u64 part_uuid;
};

struct limine_module_response {
    u64 revision;
    u64 module_count;
    struct limine_file **modules;
};

struct limine_module_request {
    u64 id[4];
    u64 revision;
    struct limine_module_response *response;
};

#define LIMINE_MODULE_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x3e7e279702be32af, 0xca1c4f3bd1280cee \
}

// memmap entry types
#define MEMMAP_USABLE                0
#define MEMMAP_RESERVED              1
#define MEMMAP_ACPI_RECLAIMABLE      2
#define MEMMAP_ACPI_NVS              3
#define MEMMAP_BAD_MEMORY            4
#define MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define MEMMAP_KERNEL_AND_MODULES    6
#define MEMMAP_FRAMEBUFFER           7

#endif
