#include <kernel/fb.h>
#include <kernel/types.h>
#include <kernel/exception.h>
#include <kernel/task.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/limine.h>

extern u64   task_current_cr3(void);
extern u64  *pte_for_va(u64 pml4_phys, u64 va);
extern void *pmm_alloc(void);
extern void  pmm_ref_dec(void *page);
extern volatile struct limine_hhdm_request hhdm_request;

static const char *names[] = {
    "Divide error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode", "Device n/a",
    "Double fault", "Coprocessor", "Invalid TSS", "Segment not present",
    "Stack fault", "General protection", "Page fault", "Reserved",
    "x87 FPU", "Alignment", "Machine check", "SIMD FPU",
    "Virtualization", "Control protection",
};

// kernel-mode panic -- never returns
static void kernel_panic(struct exception_frame *f) {
    fb_clear(0x00C00000);

    printk_color("KERNEL PANIC: ", FB_WHITE);
    u64 vec = f->vector;
    if (vec < 22) printk(names[vec]);
    else          printk("Unknown");
    printk("  (vector "); printk_dec(vec); printk(")\n");

    printk("rip: "); printk_hex(f->rip);
    printk("  cs: ");  printk_hex(f->cs);
    printk("  err: "); printk_hex(f->errcode);
    printk("\n");

    if (vec == 14) {
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printk("fault addr: "); printk_hex(cr2); printk("\n");
    }

    for (;;) asm volatile("cli; hlt");
}

u64 exception_dispatch(struct exception_frame *f) {
    u64 vec = f->vector;

    // rpl bits of CS: 0 = kernel, 3 = user
    int from_user = (f->cs & 3) == 3;

    // #DF is always fatal -- can't recover
    if (vec == 8) {
        fb_clear(0x00C00000);
        printk_color("DOUBLE FAULT -- kernel bug or bad IDT\n", FB_WHITE);
        printk("rip: "); printk_hex(f->rip); printk("\n");
        printk("err: "); printk_hex(f->errcode); printk("\n");
        for (;;) asm volatile("cli; hlt");
    }

    // ---- CoW page fault handling ----
    if (vec == 14 && from_user) {
        int is_write   = (f->errcode & 2) != 0;
        int is_present = (f->errcode & 1) != 0;

        if (is_write && is_present) {
            u64 cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));

            u64 cur_pml4 = task_current_cr3();
            u64 *pte = pte_for_va(cur_pml4, cr2);

            if (pte && (*pte & PTE_COW)) {
                u64 old_phys = *pte & 0x000FFFFFFFFFF000ULL;
                void *new_page = pmm_alloc();

                if (new_page) {
                    u64 hhdm = hhdm_request.response->offset;
                    u8 *dst = (u8 *)new_page;
                    u8 *src = (u8 *)(old_phys + hhdm);
                    for (u64 i = 0; i < 4096; i++) dst[i] = src[i];

                    u64 new_phys = (u64)new_page - hhdm;
                    u64 nx = *pte & PTE_NX;

                    *pte = new_phys | PTE_PRESENT | PTE_WRITE | PTE_USER | nx;

                    pmm_ref_dec((void *)(old_phys + hhdm));
                    asm volatile("invlpg (%0)" : : "r"(cr2) : "memory");

                    return (u64)f;
                }
                // out of memory -> kill user below
            }
        }
    }

    if (from_user) {
        // log and kill the task, don't panic
        printk_color("[fault] killed user task: ", FB_YELLOW);
        if (vec < 22) printk(names[vec]); else printk("unknown");
        printk("  vec="); printk_dec(vec);
        printk("  rip="); printk_hex(f->rip);
        printk("\n");

        u64 rsp = task_exit_current(128 + (int)vec);
        return rsp;
    }

    // kernel-mode exception -> unrecoverable
    kernel_panic(f);
    return 0;  // unreachable
}
