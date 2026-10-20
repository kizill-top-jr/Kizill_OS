#include <kernel/task.h>
#include <kernel/serial.h>
#include <kernel/types.h>
#include <kernel/fb.h>
#include <kernel/task.h>
#include <kernel/keyboard.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/elf.h>
#include <kernel/tar.h>
#include <kernel/limine.h>
#include <uapi/syscall.h>

extern volatile struct limine_module_request module_request;
extern volatile u64 g_ticks;

static void do_clear(void) {
    extern void fb_clear(u32 color);
    fb_clear(FB_BLACK);
    printk_color("Kizill_OS shell\n", FB_GREEN);
    printk("\n");
}

static void do_reboot(void) {
    // try warm reset first
    asm volatile("outb %0, %1" : : "a"((u8)0x06), "Nd"((u16)0xCF9));
    // try cold reset
    asm volatile("outb %0, %1" : : "a"((u8)0x0E), "Nd"((u16)0xCF9));

    // if that didn't work, force triple fault (load null IDT and trigger exception)
    struct { u16 limit; u64 base; } __attribute__((packed)) null_idt = {0, 0};
    asm volatile("lidt %0" : : "m"(null_idt));
    asm volatile("int $3");

    for (;;) asm volatile("hlt");
}

static int do_spawn(const char *name) {
    if (!module_request.response || module_request.response->module_count == 0)
        return -1;

    struct limine_file *mod = module_request.response->modules[0];
    const u8 *tar = (const u8 *)mod->address;
    u64 tar_size = mod->size;

    //validate the whole 64-byte name buffer once
    char fname[64];
    u64 cr3 = task_current_cr3();
    if (copy_from_user(cr3, fname, name, 64) != 0) return -1;
    fname[63] = 0;
    if (fname[0] == 0) return -1;

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
    if (!s1) {
        pml4_destroy(pml4_phys);
        return -1;
    }

    void *s2 = pmm_alloc();
    if (!s2) {
        pmm_free(s1);
        pml4_destroy(pml4_phys);
        return -1;
    }

    if (map_page_in(pml4_phys, 0x7FF000, (u64)s1 - hhdm_offset(),
                    PTE_PRESENT | PTE_WRITE | PTE_USER) != 0) {
        pmm_free(s2);
        pmm_free(s1);
        pml4_destroy(pml4_phys);
        return -1;
    }

    if (map_page_in(pml4_phys, 0x800000, (u64)s2 - hhdm_offset(),
                    PTE_PRESENT | PTE_WRITE | PTE_USER) != 0) {
        pmm_free(s2);
        pmm_free(s1);
        pml4_destroy(pml4_phys);
        return -1;
    }

    int idx = task_create_user((void (*)(void))entry, 0x800000, pml4_phys, fname);
    if (idx < 0) {
        pmm_free(s2);
        pmm_free(s1);
        pml4_destroy(pml4_phys);
        return -1;
    }

    return 0;
}

u64 syscall_dispatch(struct syscall_frame *f) {
    switch (f->rax) {
    case SYS_WRITE: {
        u64 len = f->rdx;
        if (len == 0) { f->rax = 0; return (u64)f; }
        if (len > 1024) len = 1024; // safety limit

        char kbuf[1024];
        if (copy_from_user(task_current_cr3(), kbuf, (const void *)f->rsi, len) != 0) {
            f->rax = (u64)-1;
            return (u64)f;
        }

        for (u64 i = 0; i < len; i++) {
            char c = kbuf[i];
            if (c == 0) break;
            if (c == '\n') printk("\n");
            else if (c == '\b') printk("\b");
            else { char s[2] = { c, 0 }; printk(s); }
        }
        f->rax = len;
        return (u64)f;
    }

    case SYS_READ: {
        u64 len = f->rdx;
        if (len == 0) { f->rax = 0; return (u64)f; }
        if (len > 1024) len = 1024;

        if (!keyboard_has_data()) { f->rax = (u64)-1; return (u64)f; }

        char kbuf[1024];
        u64 got = 0;
        while (got < len) {
            int c = keyboard_getchar();
            if (c < 0) break;
            kbuf[got++] = (char)c;
            if (c == '\n') break;
        }

        if (got == 0) { f->rax = (u64)-1; return (u64)f; }

        if (copy_to_user(task_current_cr3(), (void *)f->rsi, kbuf, got) != 0) {
            f->rax = (u64)-1;
            return (u64)f;
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
            // if we wait for a specific pid that doesn't exist, return ECHILD
            if (want > 0 && !task_exists(want)) {
                f->rax = (u64)-ECHILD;
            } else {
                f->rax = (u64)-EAGAIN;
            }
        }
        return (u64)f;
    }

    case SYS_CLEAR:
        do_clear();
        f->rax = 0;
        return (u64)f;

    case SYS_SPAWN: {
        char *name = (char *)f->rdi;
        int r = do_spawn(name);
        f->rax = (u64)r;
        return (u64)f;
    }

    case SYS_UPTIME:
        // g_ticks at 100 Hz -> centiseconds. return ticks for now.
        f->rax = g_ticks;
        return (u64)f;

    case SYS_TASKS: {
        int maxn = (int)f->rsi;
        if (maxn <= 0) { f->rax = 0; return (u64)f; }
        if (maxn > MAX_TASKS) maxn = MAX_TASKS;

        task_info_t kbuf[MAX_TASKS];
        int written = 0;
        for (int i = 0; i < MAX_TASKS && written < maxn; i++) {
            if (task_get_info(i, &kbuf[written]) == 0) {
                written++;
            }
        }

        u64 bytes = (u64)written * sizeof(task_info_t);
        if (bytes > 0) {
            if (copy_to_user(task_current_cr3(), (void *)f->rdi, kbuf, bytes) != 0) {
                f->rax = (u64)-1;
                return (u64)f;
            }
        }
        f->rax = (u64)written;
        return (u64)f;
    }

    case SYS_REBOOT:
        do_reboot();
        f->rax = 0;
        return (u64)f;

    case SYS_FORK: {
        u64 parent_cr3 = task_current_cr3();
        u64 parent_pid = (u64)task_current_pid();
        u64 child_pml4 = pml4_copy_for_fork(parent_cr3);

        serial_puts("fork: parent_cr3=");
        serial_hex(parent_cr3);
        serial_puts(" child_pml4=");
        serial_hex(child_pml4);
        serial_puts("\n");

        if (!child_pml4) {
            f->rax = (u64)-1;
            return (u64)f;
        }

        int child_pid = task_fork((u64)f, child_pml4, parent_pid);
        if (child_pid < 0) {
            pml4_destroy(child_pml4);
            f->rax = (u64)-1;
            return (u64)f;
        }

        f->rax = (u64)child_pid;   // parent sees child's pid
        return (u64)f;
    }

    case SYS_EXIT:
        return task_exit_current((int)f->rdi);

    default:
        f->rax = (u64)-1; // -ENOSYS
        return (u64)f;
    }
}
