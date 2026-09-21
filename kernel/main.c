#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>

#define LIMINE_FRAMEBUFFER_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x9d5827dcd881dd75, 0xa3148604f6fab11b \
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
    void     *address;
    uint64_t  width;
    uint64_t  height;
    uint64_t  pitch;
    uint16_t  bpp;
    uint8_t   memory_model;
    uint8_t   red_mask_size;
    uint8_t   red_mask_shift;
    uint8_t   green_mask_size;
    uint8_t   green_mask_shift;
    uint8_t   blue_mask_size;
    uint8_t   blue_mask_shift;
    uint8_t   unused[7];
    uint64_t  edid_size;
    void     *edid;
};

struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

struct limine_framebuffer_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_framebuffer_response *response;
};

__attribute__((section(".limine_requests_start"), used))
static volatile uint64_t limine_requests_start_marker[4] = LIMINE_REQUESTS_START_MARKER;

__attribute__((section(".limine_requests"), used))
static volatile uint64_t limine_base_revision[3] = LIMINE_BASE_REVISION(0);

__attribute__((section(".limine_requests"), used))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = 0
};

__attribute__((section(".limine_requests_end"), used))
static volatile uint64_t limine_requests_end_marker[4] = LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
   // gdt_init();
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        for (;;) asm volatile("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    fb_init(fb);
    fb_clear(FB_BLACK);

    printk_color("Kizill_OS v0.2 x86_64\n", FB_GREEN);
    printk("boot: Limine, long mode\n");
    printk("fb: ");
    printk_dec(fb->width); printk("x");
    printk_dec(fb->height); printk(" ");
    printk_dec(fb->bpp); printk("bpp\n");
    printk("addr: "); printk_hex((u64)fb->address); printk("\n\n");

    printk_color("Hello, world.\n", FB_YELLOW);

    for (;;) asm volatile("hlt");
}
