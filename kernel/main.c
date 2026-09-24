#include <kernel/heap.h>
#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/pmm.h>
#include <kernel/limine.h>

// requests — non-static so pmm.c can use them via extern
__attribute__((section(".limine_requests_start"), used))
volatile u64 limine_requests_start_marker[4] = LIMINE_REQUESTS_START_MARKER;

__attribute__((section(".limine_requests"), used))
volatile u64 limine_base_revision[3] = LIMINE_BASE_REVISION(3);

__attribute__((section(".limine_requests"), used))
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests"), used))
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests"), used))
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests_end"), used))
volatile u64 limine_requests_end_marker[4] = LIMINE_REQUESTS_END_MARKER;

static void fb_print_at(u64 x, u64 y, char c, u32 color) {
    fb_draw_char(x * 8, y * 8, c, color, FB_BLACK);
}

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

    if (hhdm_request.response) {
        printk("hhdm: "); printk_hex(hhdm_request.response->offset); printk("\n");
    }

    if (memmap_request.response) {
        struct limine_memmap_response *mm = memmap_request.response;
        u64 total_usable = 0;
        for (u64 i = 0; i < mm->entry_count; i++) {
            struct limine_memmap_entry *e = mm->entries[i];
            if (e->type == MEMMAP_USABLE) total_usable += e->length;
        }
        extern u64 __kernel_end;
        printk("kernel end: "); printk_hex((u64)&__kernel_end); printk("\n");
        printk_color("total usable: ", FB_GREEN);
        printk_dec(total_usable / 1024 / 1024);
        printk(" MB\n\n");
    }

    gdt_init();
    idt_init();

    pmm_init();
    printk_color("pmm: init\n", FB_GREEN);
    printk("  total pages: "); printk_dec(pmm_total_pages()); printk("\n");
    printk("  used pages:  "); printk_dec(pmm_used_pages());  printk("\n");
    printk("  free pages:  ");
    printk_dec(pmm_total_pages() - pmm_used_pages()); printk("\n\n");

    // test: allocate 5 pages, print addresses, free them
    void *p1 = pmm_alloc();
    void *p2 = pmm_alloc();
    void *p3 = pmm_alloc();
    printk_color("test alloc:\n", FB_YELLOW);
    printk("  p1="); printk_hex((u64)p1); printk("\n");
    printk("  p2="); printk_hex((u64)p2); printk("\n");
    printk("  p3="); printk_hex((u64)p3); printk("\n");

    pmm_free(p2);
    printk_color("after free p2:\n", FB_YELLOW);
    void *p4 = pmm_alloc();
    printk("  p4="); printk_hex((u64)p4); printk("\n");
    printk("  used pages: "); printk_dec(pmm_used_pages()); printk("\n\n");

    heap_init();
    printk_color("heap: init\n", FB_GREEN);
    printk("  total: "); printk_dec(heap_total_bytes()); printk(" bytes\n");
    printk("  used:  "); printk_dec(heap_used_bytes());  printk(" bytes\n");
    printk("  free:  "); printk_dec(heap_free_bytes());  printk(" bytes\n\n");

    // test allocations
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    void *c = kmalloc(1024);
    printk_color("kmalloc test:\n", FB_YELLOW);
    printk("  a(64)   = "); printk_hex((u64)a); printk("\n");
    printk("  b(128)  = "); printk_hex((u64)b); printk("\n");
    printk("  c(1024) = "); printk_hex((u64)c); printk("\n");

    kfree(b);
    printk_color("after kfree(b):\n", FB_YELLOW);
    void *d = kmalloc(64);
    printk("  d(64)   = "); printk_hex((u64)d); printk("\n");
    printk("  used:   "); printk_dec(heap_used_bytes()); printk(" bytes\n\n");

    kfree(a);
    kfree(c);
    kfree(d);

    scheduler_init();
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");

    pic_unmask_timer();
    asm volatile("sti");

    for (;;) asm volatile("hlt");
}
