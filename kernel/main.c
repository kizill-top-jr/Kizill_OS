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
#include <kernel/keyboard.h>
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

static int spawn_shell(void) {
    if (!module_request.response || module_request.response->module_count == 0)
        return -1;

    struct limine_file *mod = module_request.response->modules[0];
    const u8 *tar = (const u8 *)mod->address;
    u64 tar_size = mod->size;

    const tar_entry_t *sh = tar_find(tar, tar_size, "sh.elf");
    if (!sh) return -1;

    u64 pml4_phys = pml4_create();
    if (!pml4_phys) return -1;

    u64 entry = elf_load_in(pml4_phys, sh->data, sh->size);
    if (!entry) {
        pml4_destroy(pml4_phys);
        return -1;
    }

    void *s1 = pmm_alloc();
    void *s2 = pmm_alloc();
    if (!s1 || !s2) {
        pml4_destroy(pml4_phys);
        return -1;
    }

    extern volatile struct limine_hhdm_request hhdm_request;
    u64 hhdm = hhdm_request.response->offset;

    map_page_in(pml4_phys, 0x7FF000, (u64)s1 - hhdm,
                PTE_PRESENT | PTE_WRITE | PTE_USER);
    map_page_in(pml4_phys, 0x800000, (u64)s2 - hhdm,
                PTE_PRESENT | PTE_WRITE | PTE_USER);

    int idx = task_create_user((void (*)(void))entry, 0x800000, pml4_phys, "sh");
    if (idx < 0) {
        pml4_destroy(pml4_phys);
        return -1;
    }

    return idx;
}

// ---- kmain ----

void kmain(void) {
    serial_init();

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
    keyboard_init();
    pmm_init();

    u64 hhdm = hhdm_request.response->offset;

    // ---- test pml4_create ----
    u64 test_pml4 = pml4_create();
    if (test_pml4) {
        printk_color("pml4: created at ", FB_GREEN);
        printk_hex(test_pml4);
        printk("\n");

        // verify kernel entries copied
        u64 *master = (u64 *)(hhdm + pml4_master());
        u64 *newp   = (u64 *)(hhdm + test_pml4);

        int kernel_ok = 1;
        for (int i = 256; i < 512; i++) {
            if (master[i] != newp[i]) { kernel_ok = 0; break; }
        }
        // user entries must be zero
        int user_ok = 1;
        for (int i = 0; i < 256; i++) {
            if (newp[i] != 0) { user_ok = 0; break; }
        }

        if (kernel_ok && user_ok) {
            printk_color("pml4: kernel copied, user empty -- OK\n", FB_GREEN);
        } else {
            printk_color("pml4: BAD copy\n", FB_RED);
        }

        pml4_destroy(test_pml4);
        printk("pml4: destroyed\n\n");
    } else {
        printk_color("pml4: create failed\n", FB_RED);
    }
    heap_init();

    printk("hhdm: "); printk_hex(hhdm); printk("\n");
    printk("pmm:  total="); printk_dec(pmm_total_pages());
    printk(" used=");       printk_dec(pmm_used_pages());
    printk(" free=");       printk_dec(pmm_total_pages() - pmm_used_pages());
    printk("\n\n");

    scheduler_init();
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");

   serial_puts("=== boot ok ===\n");
   spawn_shell();

   // ---- initramfs info (optional) ----
   if (module_request.response && module_request.response->module_count > 0) {
       struct limine_file *mod = module_request.response->modules[0];
       printk_color("initramfs: ", FB_YELLOW);
       printk_dec(mod->size);
       printk(" bytes\n");
   }

   printk("\n");
   printk_color("unmask timer + sti\n", FB_YELLOW);

   pic_unmask_timer();
   asm volatile("sti");

   int code;
   int shell_alive = 1;
   for (;;) {
       while (task_try_reap(0, -1, &code)) { }

       if (shell_alive && !task_is_alive_by_name("sh")) {
           shell_alive = 0;
           printk_color("[init] shell died, respawning...\n", FB_YELLOW);
           spawn_shell();
           shell_alive = 1;
       }

       asm volatile("hlt");
   }
}
