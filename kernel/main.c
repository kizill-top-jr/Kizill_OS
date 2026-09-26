#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/pmm.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/limine.h>

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
    for (int i = n - 1; i >= 0; i--) fb_print_at(x++, y, buf[i], color);
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

// ---------------- user programs ----------------

// child: write("C: hello\n", 9); exit(7);
static const u8 child_code[] = {
    0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,   // mov rax, 1 (write)
    0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00,   // mov rdi, 1
    0x48, 0x8D, 0x35, 0x1B, 0x00, 0x00, 0x00,   // lea rsi, [rip+0x1B]
    0x48, 0xC7, 0xC2, 0x09, 0x00, 0x00, 0x00,   // mov rdx, 9
    0xCD, 0x80,                                 // int 0x80
    0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00,   // mov rax, 60 (exit)
    0x48, 0xC7, 0xC7, 0x07, 0x00, 0x00, 0x00,   // mov rdi, 7
    0xCD, 0x80,                                 // int 0x80
    0xEB, 0xFE,                                 // jmp $
    'C',':',' ','h','e','l','l','o','\n',
};

// parent: write("P: start\n", 9);
//         loop: r=wait(); if (r==-1) goto loop;
//         write("P: done\n", 8);
//         exit(r);
static const u8 parent_code[] = {
    0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,   // mov rax, 1
    0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00,   // mov rdi, 1
    0x48, 0x8D, 0x35, 0x47, 0x00, 0x00, 0x00,   // lea rsi, [rip+0x47]
    0x48, 0xC7, 0xC2, 0x09, 0x00, 0x00, 0x00,   // mov rdx, 9
    0xCD, 0x80,                                 // int 0x80
    // loop:
    0x48, 0xC7, 0xC0, 0x3D, 0x00, 0x00, 0x00,   // mov rax, 61 (wait)
    0xCD, 0x80,                                 // int 0x80
    0x48, 0x83, 0xF8, 0xFF,                     // cmp rax, -1
    0x74, 0xF1,                                 // je loop
    0x48, 0x89, 0xC3,                           // mov rbx, rax
    0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,   // mov rax, 1
    0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00,   // mov rdi, 1
    0x48, 0x8D, 0x35, 0x20, 0x00, 0x00, 0x00,   // lea rsi, [rip+0x20]
    0x48, 0xC7, 0xC2, 0x08, 0x00, 0x00, 0x00,   // mov rdx, 8
    0xCD, 0x80,                                 // int 0x80
    0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00,   // mov rax, 60
    0x48, 0x89, 0xDF,                           // mov rdi, rbx
    0xCD, 0x80,                                 // int 0x80
    0xEB, 0xFE,                                 // jmp $
    'P',':',' ','s','t','a','r','t','\n',
    'P',':',' ','d','o','n','e','\n',
};

void kmain(void) {
    serial_init();
    serial_puts("=== Kizill_OS boot ===\n");

    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        for (;;) asm volatile("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_init(fb);
    fb_clear(FB_BLACK);

    printk_color("Kizill_OS v0.5 x86_64\n", FB_GREEN);
    printk("fb:  "); printk_dec(fb->width); printk("x");
    printk_dec(fb->height); printk(" "); printk_dec(fb->bpp); printk("bpp\n\n");

    gdt_init();
    idt_init();
    pmm_init();
    heap_init();

    u64 hhdm = hhdm_request.response->offset;

    // child code + stack, parent code + stack
    void *c_code = pmm_alloc();
    void *c_stk  = pmm_alloc();
    void *p_code = pmm_alloc();
    void *p_stk  = pmm_alloc();

    map_page(0x400000, (u64)c_code - hhdm, PTE_PRESENT | PTE_WRITE | PTE_USER);
    map_page(0x500000, (u64)c_stk  - hhdm, PTE_PRESENT | PTE_WRITE | PTE_USER);
    map_page(0x600000, (u64)p_code - hhdm, PTE_PRESENT | PTE_WRITE | PTE_USER);
    map_page(0x700000, (u64)p_stk  - hhdm, PTE_PRESENT | PTE_WRITE | PTE_USER);

    u8 *d;
    d = (u8 *)c_code; for (u64 i = 0; i < sizeof(child_code); i++) d[i] = child_code[i];
    d = (u8 *)p_code; for (u64 i = 0; i < sizeof(parent_code); i++) d[i] = parent_code[i];

    scheduler_init();
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");

    int child_idx  = task_create_user((void (*)(void))0x400000, 0x501000, "child");
    int parent_idx = task_create_user((void (*)(void))0x600000, 0x701000, "parent");

    // parent is child's parent: pid = idx + 1
    task_set_parent(child_idx, (u64)(parent_idx + 1));

    printk_color("user: child + parent, wait-test\n", FB_YELLOW);

    pic_unmask_timer();
    asm volatile("sti");
    for (;;) asm volatile("hlt");
}
