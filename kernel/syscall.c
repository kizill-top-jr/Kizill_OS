#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/task.h>
#include <kernel/keyboard.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/elf.h>
#include <kernel/tar.h>
#include <kernel/limine.h>

#define SYS_WRITE   1
#define SYS_READ    0
#define SYS_YIELD   24
#define SYS_GETPID  39
#define SYS_EXIT    60
#define SYS_WAIT    61
#define SYS_CLEAR   99
#define SYS_EXEC    100
#define SYS_UPTIME  101
#define SYS_TASKS   102
#define SYS_REBOOT  103

#define HHDM_BASE 0xFFFF800000000000ULL

extern volatile struct limine_module_request module_request;
extern volatile u64 g_ticks;

struct syscall_frame {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 vector, errcode;
    u64 rip, cs, rflags, rsp, ss;
};

static void do_clear(void) {
    extern void fb_clear(u32 color);
    fb_clear(FB_BLACK);
    printk_color("Kizill_OS shell\n", FB_GREEN);
    printk("\n");
}

static void do_reboot(void) {
    // 0xCF9 = reset control register
    // 0x06 = warm reset, 0x0E = full reset
    asm volatile("outb %0, %1" : : "a"((u8)0x06), "Nd"((u16)0xCF9));
    // if that didn't work, triple fault
    for (;;) asm volatile("hlt");
}

static int do_exec(const char *name) {
    if (!module_request.response || module_request.response->module_count == 0)
        return -1;

    struct limine_file *mod = module_request.response->modules[0];
    const u8 *tar = (const u8 *)mod->address;
    u64 tar_size = mod->size;

    char fname[64];
    int i = 0;
    while (name[i] && i < 63) { fname[i] = name[i]; i++; }
    fname[i] = 0;

    const tar_entry_t *e = tar_find(tar, tar_size, fname);
    if (!e) return -1;

    u64 pml4_phys = pml4_create();
    if (!pml4_phys) return -1;

    u64 entry = elf_load_in(pml4_phys, e->data, e->size);
    if (!entry) {
        pml4_destroy(pml4_phys);
        return -1;
    }

    void *s1 = pmm_alloc();
    void *s2 = pmm_alloc();
    if (!s1 || !s2) return -1;

    map_page_in(pml4_phys, 0x7FF000, (u64)s1 - HHDM_BASE,
                PTE_PRESENT | PTE_WRITE | PTE_USER);
    map_page_in(pml4_phys, 0x800000, (u64)s2 - HHDM_BASE,
                PTE_PRESENT | PTE_WRITE | PTE_USER);

    int idx = task_create_user((void (*)(void))entry, 0x800000, pml4_phys, fname);
    if (idx < 0) return -1;

    extern void task_set_parent(int idx, u64 parent_pid);
    task_set_parent(idx, (u64)task_current_pid());

    return 0;
}

u64 syscall_dispatch(struct syscall_frame *f) {
    switch (f->rax) {
    case SYS_WRITE: {
        char *buf = (char *)f->rsi;
        u64 len = f->rdx;
        for (u64 i = 0; i < len; i++) {
            char c = buf[i];
            if (c == 0) break;
            if (c == '\n') printk("\n");
            else if (c == '\b') printk("\b");
            else { char s[2] = { c, 0 }; printk(s); }
        }
        f->rax = len;
        return (u64)f;
    }

    case SYS_READ: {
        char *ubuf = (char *)f->rsi;
        u64 len = f->rdx;
        if (len == 0) { f->rax = 0; return (u64)f; }

        if (!keyboard_has_data()) { f->rax = 0; return (u64)f; }

        u64 got = 0;
        while (got < len) {
            int c = keyboard_getchar();
            if (c < 0) break;
            ubuf[got++] = (char)c;
            if (c == '\n') break;
        }
        f->rax = got;
        return (u64)f;
    }

    case SYS_YIELD:
        return task_yield((u64)f);

    case SYS_GETPID:
        f->rax = (u64)task_current_pid();
        return (u64)f;

    case SYS_WAIT: {
        int want = (int)f->rdi;
        u64 my_pid = (u64)task_current_pid();
        int code;
        if (task_try_reap(my_pid, want, &code)) {
            f->rax = (u64)code;
        } else {
            f->rax = (u64)-1;
        }
        return (u64)f;
    }

    case SYS_CLEAR:
        do_clear();
        f->rax = 0;
        return (u64)f;

    case SYS_EXEC: {
        char *name = (char *)f->rdi;
        int r = do_exec(name);
        f->rax = (u64)r;
        return (u64)f;
    }

    case SYS_UPTIME:
        // g_ticks at 100 Hz -> centiseconds. return ticks for now.
        f->rax = g_ticks;
        return (u64)f;

    case SYS_TASKS: {
        // rdi = buf, rsi = max_count
        task_info_t *ubuf = (task_info_t *)f->rdi;
        int maxn = (int)f->rsi;
        if (maxn <= 0) { f->rax = 0; return (u64)f; }

        int written = 0;
        for (int i = 0; i < 8 && written < maxn; i++) {
            task_info_t info;
            if (task_get_info(i, &info) == 0) {
                ubuf[written++] = info;
            }
        }
        f->rax = (u64)written;
        return (u64)f;
    }

    case SYS_REBOOT:
        do_reboot();
        f->rax = 0;
        return (u64)f;

    case SYS_EXIT:
        return task_exit_current((int)f->rdi);

    default:
        printk_color("[sys] unknown: ", FB_RED);
        printk_dec(f->rax);
        printk("\n");
        f->rax = (u64)-1;
        return (u64)f;
    }
}
