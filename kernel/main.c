#include <stdint.h>

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

static void put_pixel(struct limine_framebuffer *fb, uint64_t x, uint64_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;
    uint32_t *ptr = (uint32_t *)((uint8_t *)fb->address + y * fb->pitch + x * (fb->bpp / 8));
    *ptr = color;
}

static void fill_screen(struct limine_framebuffer *fb, uint32_t color) {
    for (uint64_t y = 0; y < fb->height; y++)
        for (uint64_t x = 0; x < fb->width; x++)
            put_pixel(fb, x, y, color);
}

void kmain(void) {
    if (framebuffer_request.response == 0 ||
        framebuffer_request.response->framebuffer_count < 1) {
        for (;;) asm volatile("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fill_screen(fb, 0x00800000);

    for (;;) asm volatile("hlt");
}
