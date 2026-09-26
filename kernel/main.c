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
#include <kernel/tar.h>
#include <kernel/elf.h>

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

__attribute__((section(".limine_requests"), used))
volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST, .revision = 0, .response = 0
};

__attribute__((section(".limine_requests_end"), used))
volatile u64 limine_requests_end_marker[4] = LIMINE_REQUESTS_END_MARKER;

// ---- helpers ----

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

// ---- test tasks (preemption proof) ----

static void task_a(void) {
    u64 counter = 0;
    while (1) {
        counter++;
        if ((counter % 100000) == 0) {
            fb_print_at(70, 22, 'A', FB_RED);
            fb_print_at(72, 22, ':', FB_RED);
            fb_print_num_at(74, 22, counter, FB_RED);
        }
    }
}

static void task_b(void) {
    u64 counter = 0;
    while (1) {
        counter++;
        if ((counter % 100000) == 0) {
            fb_print_at(70, 23, 'B', FB_GREEN);
            fb_print_at(72, 23, ':', FB_GREEN);
            fb_print_num_at(74, 23, counter, FB_GREEN);
        }
    }
}

// ---- kmain ----

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

    printk_color("Kizill_OS v0.6 x86_64\n", FB_GREEN);
    printk("fb:  "); printk_dec(fb->width); printk("x");
    printk_dec(fb->height); printk(" "); printk_dec(fb->bpp); printk("bpp\n\n");

    gdt_init();
    idt_init();
    pmm_init();
    heap_init();

    u64 hhdm = hhdm_request.response->offset;

    printk("hhdm: "); printk_hex(hhdm); printk("\n");
    printk("pmm:  total="); printk_dec(pmm_total_pages());
    printk(" used=");       printk_dec(pmm_used_pages());
    printk(" free=");       printk_dec(pmm_total_pages() - pmm_used_pages());
    printk("\n\n");

    scheduler_init();
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");

    // ---- initramfs: list + parse ----
    if (module_request.response && module_request.response->module_count > 0) {
        struct limine_file *mod = module_request.response->modules[0];
        printk_color("initramfs: ", FB_YELLOW);
        printk_dec(mod->size);
        printk(" bytes\n");

        const u8 *tar = (const u8 *)mod->address;
        u64 tar_size = mod->size;

        // list all files
        const tar_entry_t *e = 0;
        while ((e = tar_next(tar, tar_size, e))) {
            printk("  file: ");
            printk(e->name);
            printk("  size: ");
            printk_dec(e->size);
            printk("\n");
        }

        // parse hello.elf once, after listing
        const tar_entry_t *elf_entry = tar_find(tar, tar_size, "hello.elf");
        if (elf_entry) {
            printk_color("\nparsing hello.elf:\n", FB_YELLOW);
            elf_dump(elf_entry->data, elf_entry->size);

            // ---- load it and spawn as user task ----
            u64 entry = elf_load(elf_entry->data, elf_entry->size);
            if (entry) {
                printk_color("elf: loaded, entry=", FB_GREEN);
                printk_hex(entry);
                printk("\n");

                // allocate user stack: 2 pages at 0x7FF000..0x800000
                void *s1 = pmm_alloc();
                void *s2 = pmm_alloc();
                if (!s1 || !s2) {
                    printk_color("elf: no memory for stack\n", FB_RED);
                } else {
                    map_page(0x7FF000, (u64)s1 - hhdm,
                             PTE_PRESENT | PTE_WRITE | PTE_USER);
                    map_page(0x800000, (u64)s2 - hhdm,
                             PTE_PRESENT | PTE_WRITE | PTE_USER);

                    // spawn user task: entry from ELF, stack top 0x800000
                    task_create_user((void (*)(void))entry, 0x800000, "hello");
                    printk_color("user: hello task created\n", FB_YELLOW);
                }
            } else {
                printk_color("elf: load failed\n", FB_RED);
            }
        } else {
            printk_color("hello.elf not found in initramfs\n", FB_RED);
        }
    } else {
        printk_color("initramfs: no modules\n", FB_RED);
    }

    printk("\n");
    printk_color("unmask timer + sti\n", FB_YELLOW);

    pic_unmask_timer();
    asm volatile("sti");

    // main idles; scheduler + IRQ0 handle the rest
    for (;;) asm volatile("hlt");
}
