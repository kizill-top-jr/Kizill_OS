#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/serial.h>
#include <kernel/task.h>

#define LIMINE_FRAMEBUFFER_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x9d5827dcd881dd75, 0xa3148604f6fab11b \
}
#define LIMINE_MEMMAP_REQUEST { \
    0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, \
    0x67cf3d9d378a806f, 0xe304acdfc50c3c62 \
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

// memmap entry types
#define MEMMAP_USABLE                0
#define MEMMAP_RESERVED              1
#define MEMMAP_ACPI_RECLAIMABLE      2
#define MEMMAP_ACPI_NVS              3
#define MEMMAP_BAD_MEMORY            4
#define MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define MEMMAP_KERNEL_AND_MODULES    6
#define MEMMAP_FRAMEBUFFER           7

__attribute__((section(".limine_requests_start"), used))
static volatile u64 limine_requests_start_marker[4] = LIMINE_REQUESTS_START_MARKER;

__attribute__((section(".limine_requests"), used))
static volatile u64 limine_base_revision[3] = LIMINE_BASE_REVISION(3);

__attribute__((section(".limine_requests"), used))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests"), used))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests_end"), used))
static volatile u64 limine_requests_end_marker[4] = LIMINE_REQUESTS_END_MARKER;

static void fb_print_at(u64 x, u64 y, char c, u32 color) {
    fb_draw_char(x * 8, y * 8, c, color, FB_BLACK);
}

// helper: print unsigned number at (x, y) in given color
static void fb_print_num_at(u64 x, u64 y, u64 v, u32 color) {
    char buf[24];
    int n = 0;
    if (v == 0) buf[n++] = '0';
    while (v > 0) { buf[n++] = '0' + (v % 10); v /= 10; }
    for (int i = n - 1; i >= 0; i--) {
        fb_print_at(x++, y, buf[i], color);
    }
}

static void task_a(void) {
    u64 counter = 0;
    while (1) {
        counter++;

        // print only every 100k iterations to not spam the fb
        if ((counter % 100000) == 0) {
            fb_print_at(20, 10, 'A', FB_RED);
            fb_print_at(22, 10, ':', FB_RED);
            fb_print_num_at(24, 10, counter, FB_RED);
        }
    }
}

static void task_b(void) {
    u64 counter = 0;
    while (1) {
        counter++;

        if ((counter % 100000) == 0) {
            fb_print_at(20, 12, 'B', FB_GREEN);
            fb_print_at(22, 12, ':', FB_GREEN);
            fb_print_num_at(24, 12, counter, FB_GREEN);
        }
    }
}

void kmain(void) {
    serial_init();
    serial_puts("=== Kizill_OS boot ===\n");

    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        serial_puts("FATAL: no framebuffer\n");
        for (;;) asm volatile("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_init(fb);
    fb_clear(FB_BLACK);

    printk_color("Kizill_OS v0.3 x86_64\n", FB_GREEN);
    printk("fb:  "); printk_dec(fb->width); printk("x");
    printk_dec(fb->height); printk(" "); printk_dec(fb->bpp); printk("bpp\n\n");

    // --- memory map dump ---
    if (memmap_request.response) {
        struct limine_memmap_response *mm = memmap_request.response;
        u64 total_usable = 0;

        printk_color("memmap:\n", FB_YELLOW);
        printk("entries: "); printk_dec(mm->entry_count); printk("\n");

        for (u64 i = 0; i < mm->entry_count; i++) {
            struct limine_memmap_entry *e = mm->entries[i];

            if (e->type == MEMMAP_USABLE) {
                total_usable += e->length;
            }

            // print: base, length, type
            printk("  base="); printk_hex(e->base);
            printk(" len=");  printk_hex(e->length);
            printk(" type="); printk_dec(e->type);
            printk("\n");
        }

        printk_color("total usable: ", FB_GREEN);
        printk_dec(total_usable / 1024 / 1024);
        printk(" MB\n\n");
    } else {
        printk_color("memmap: no response\n", FB_RED);
    }

    gdt_init();
    idt_init();

    scheduler_init();
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");

    printk_color("tasks: a, b, main\n", FB_YELLOW);
    printk("watching for A/B counters...\n");
    printk("unmask timer + sti\n");

    pic_unmask_timer();
    asm volatile("sti");

    for (;;) {
        asm volatile("hlt");
    }
}
